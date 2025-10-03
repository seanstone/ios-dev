#![allow(clippy::not_unsafe_ptr_arg_deref)]

use libc::{c_char, c_int, dup, dup2, fcntl, pipe, signal, write, FD_CLOEXEC, F_GETFD, F_SETFD, SIGPIPE};
use std::ffi::{CStr, CString};
use std::io::{self, Read, Write as _};
use std::os::fd::{FromRawFd, RawFd};
use std::ptr;
use std::sync::{
    atomic::{AtomicBool, Ordering},
    Arc, Condvar, Mutex,
};
use std::thread;

// ---------- tiny helpers ----------
unsafe fn set_cloexec(fd: c_int) {
    if fd >= 0 {
        let flags = fcntl(fd, F_GETFD);
        if flags >= 0 {
            let _ = fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
        }
    }
}

fn set_linebuf_stdio() {
    // Darwin (macOS/iOS) exposes __stdinp/__stdoutp/__stderrp.
    unsafe {
        unsafe extern "C" {
            static mut __stdinp: *mut libc::FILE;
            static mut __stdoutp: *mut libc::FILE;
            static mut __stderrp: *mut libc::FILE;
            fn setvbuf(
                stream: *mut libc::FILE,
                buf: *mut libc::c_char,
                mode: libc::c_int,
                size: libc::size_t,
            ) -> libc::c_int;
        }
        let _ = setvbuf(__stdinp, ptr::null_mut(), libc::_IONBF, 0);
        let _ = setvbuf(__stdoutp, ptr::null_mut(), libc::_IOLBF, 0);
        let _ = setvbuf(__stderrp, ptr::null_mut(), libc::_IONBF, 0);
    }
}

// ---------- pumps ----------
fn start_pump(fd: RawFd, mut on_chunk: impl FnMut(&[u8]) + Send + 'static) -> thread::JoinHandle<()> {
    thread::spawn(move || {
        let file = unsafe { std::fs::File::from_raw_fd(fd) }; // take ownership/close on drop
        let mut reader = io::BufReader::new(file);
        let mut buf = [0u8; 4096];
        loop {
            match reader.read(&mut buf) {
                Ok(0) => break, // EOF
                Ok(n) => on_chunk(&buf[..n]),
                Err(ref e) if e.kind() == io::ErrorKind::Interrupted => continue,
                Err(_) => break,
            }
        }
    })
}

// ---------- virtual child ----------
struct IosChild {
    // parent-facing fds
    stdin_fd: RawFd,  // write here
    stdout_fd: RawFd, // read here
    stderr_fd: RawFd, // read here

    // control
    running: Arc<AtomicBool>,
    exit_code: Arc<Mutex<i32>>,

    // internal joins
    main_thread: Option<thread::JoinHandle<()>>,
    pump_out: Option<thread::JoinHandle<()>>,
    pump_err: Option<thread::JoinHandle<()>>,
}

impl IosChild {
    unsafe fn spawn(dylib_path: &CStr, argv: &[&CStr]) -> io::Result<Self> {
        // Ignore SIGPIPE so writes after close return EPIPE instead of a signal
        libc::signal(SIGPIPE, libc::SIG_IGN);

        // Create pipes
        let mut in_pipe = [0; 2];
        let mut out_pipe = [0; 2];
        let mut err_pipe = [0; 2];
        if pipe(in_pipe.as_mut_ptr()) != 0
            || pipe(out_pipe.as_mut_ptr()) != 0
            || pipe(err_pipe.as_mut_ptr()) != 0
        {
            return Err(io::Error::last_os_error());
        }

        set_cloexec(in_pipe[1]);
        set_cloexec(out_pipe[0]);
        set_cloexec(err_pipe[0]);

        // Prepare data to move into the child thread:
        // - path and argv as owned CStrings (Send)
        let path_owned = CString::from(CString::from_vec_unchecked(dylib_path.to_bytes().to_vec()));
        let argv_owned: Vec<CString> = argv.iter().map(|s| CString::from(CString::from_vec_unchecked(s.to_bytes().to_vec()))).collect();

        // State shared with child thread
        let running = Arc::new(AtomicBool::new(true));
        let exit_code = Arc::new(Mutex::new(0i32));
        let ready_pair = Arc::new((Mutex::new(false), Condvar::new()));
        let ready_pair_thread = Arc::clone(&ready_pair);
        let running_thread = Arc::clone(&running);
        let exit_code_thread = Arc::clone(&exit_code);

        // FDs to move
        let in_r = in_pipe[0];
        let in_w = in_pipe[1];
        let out_r = out_pipe[0];
        let out_w = out_pipe[1];
        let err_r = err_pipe[0];
        let err_w = err_pipe[1];

        // Start child thread
        let child_thread = thread::spawn(move || {
            unsafe {
                // Open dylib inside the child thread (avoids sending lib handle across threads)
                let lib = match libloading::Library::new(path_owned.as_c_str().to_string_lossy().as_ref()) {
                    Ok(l) => l,
                    Err(e) => {
                        eprintln!("dlopen: {e}");
                        *exit_code_thread.lock().unwrap() = -1;
                        running_thread.store(false, Ordering::SeqCst);
                        return;
                    }
                };

                // Resolve main
                let main_sym: libloading::Symbol<
                    unsafe extern "C" fn(argc: c_int, argv: *mut *mut c_char) -> c_int,
                > = match lib.get(b"main\0") {
                    Ok(s) => s,
                    Err(e) => {
                        eprintln!("dlsym(main): {e}");
                        *exit_code_thread.lock().unwrap() = -1;
                        running_thread.store(false, Ordering::SeqCst);
                        return;
                    }
                };

                // Build argv **here** (local pointers valid while CStrings live)
                let mut argv_ptrs: Vec<*mut c_char> =
                    argv_owned.iter().map(|s| s.as_ptr() as *mut c_char).collect();
                argv_ptrs.push(ptr::null_mut());

                // Save original stdio
                let save0 = dup(0);
                let save1 = dup(1);
                let save2 = dup(2);
                if save0 < 0 || save1 < 0 || save2 < 0 {
                    *exit_code_thread.lock().unwrap() = -127;
                    running_thread.store(false, Ordering::SeqCst);
                    return;
                }

                // Remap stdio to pipes
                if dup2(in_r, 0) < 0 || dup2(out_w, 1) < 0 || dup2(err_w, 2) < 0 {
                    *exit_code_thread.lock().unwrap() = -126;
                    // restore & bail
                    let _ = dup2(save0, 0);
                    let _ = dup2(save1, 1);
                    let _ = dup2(save2, 2);
                    if save0 >= 0 { libc::close(save0); }
                    if save1 >= 0 { libc::close(save1); }
                    if save2 >= 0 { libc::close(save2); }
                    running_thread.store(false, Ordering::SeqCst);
                    return;
                }

                // Notify parent stdio is ready
                {
                    let (lock, cv) = &*ready_pair_thread;
                    let mut ready = lock.lock().unwrap();
                    *ready = true;
                    cv.notify_one();
                }

                // Close read ends we don't need in this thread
                libc::close(out_r);
                libc::close(err_r);

                set_linebuf_stdio();

                // Call main
                let code = (main_sym)(argv_owned.len() as c_int, argv_ptrs.as_mut_ptr());
                *exit_code_thread.lock().unwrap() = code;

                // Flush & restore stdio
                libc::fflush(ptr::null_mut());
                let _ = dup2(save0, 0);
                let _ = dup2(save1, 1);
                let _ = dup2(save2, 2);
                if save0 >= 0 { libc::close(save0); }
                if save1 >= 0 { libc::close(save1); }
                if save2 >= 0 { libc::close(save2); }

                // Close child ends
                libc::close(in_r);
                libc::close(out_w);
                libc::close(err_w);

                // Drop lib at end of thread
                drop(lib);

                running_thread.store(false, Ordering::SeqCst);
            }
        });

        // Wait until stdio is remapped before returning
        {
            let (lock, cv) = &*ready_pair;
            let mut ready = lock.lock().unwrap();
            while !*ready {
                ready = cv.wait(ready).unwrap();
            }
        }

        // Parent-visible fds
        let child = IosChild {
            stdin_fd: in_w,
            stdout_fd: out_r,
            stderr_fd: err_r,
            running,
            exit_code,
            main_thread: Some(child_thread),
            pump_out: None,
            pump_err: None,
        };

        Ok(child)
    }

    fn start_pumps(
        mut self,
        mut on_stdout: impl FnMut(&[u8]) + Send + 'static,
        mut on_stderr: impl FnMut(&[u8]) + Send + 'static,
    ) -> Self {
        let out_fd = self.stdout_fd;
        let err_fd = self.stderr_fd;

        self.pump_out = Some(start_pump(out_fd, move |chunk| on_stdout(chunk)));
        self.pump_err = Some(start_pump(err_fd, move |chunk| on_stderr(chunk)));

        self
    }

    fn write_all(&self, s: &str) -> io::Result<()> {
        let mut off = 0;
        let bytes = s.as_bytes();
        while off < bytes.len() {
            let k = unsafe { write(self.stdin_fd, bytes[off..].as_ptr().cast(), (bytes.len() - off) as _) };
            if k > 0 {
                off += k as usize;
            } else {
                let err = io::Error::last_os_error();
                if err.kind() == io::ErrorKind::Interrupted {
                    continue;
                } else {
                    return Err(err);
                }
            }
        }
        Ok(())
    }

    fn wait(mut self) -> i32 {
        if let Some(h) = self.pump_out.take() { let _ = h.join(); }
        if let Some(h) = self.pump_err.take() { let _ = h.join(); }
        if let Some(h) = self.main_thread.take() { let _ = h.join(); }
        *self.exit_code.lock().unwrap()
    }

    fn close(&mut self) {
        unsafe {
            if self.stdin_fd >= 0 { libc::close(self.stdin_fd); self.stdin_fd = -1; }
            if self.stdout_fd >= 0 { libc::close(self.stdout_fd); self.stdout_fd = -1; }
            if self.stderr_fd >= 0 { libc::close(self.stderr_fd); self.stderr_fd = -1; }
        }
    }
}

// ---------- demo_main equivalent ----------
fn demo_main(program_dylib: &CStr) -> i32 {
    // argv: sqlite3 -interactive :memory:
    let argv = [
        CStr::from_bytes_with_nul(b"bash\0").unwrap(),
        // CStr::from_bytes_with_nul(b"--version\0").unwrap(),
    ];

    let child = unsafe { IosChild::spawn(program_dylib, &argv) };
    let mut child = match child {
        Ok(c) => c,
        Err(e) => {
            eprintln!("spawn error: {e}");
            return -1;
        }
    };

    // stdout/stderr pumps -> print to our stdout/stderr
    child = child.start_pumps(
        |chunk| {
            let _ = std::io::stdout().write_all(chunk);
            let _ = std::io::stdout().flush();
        },
        |chunk| {
            let _ = std::io::stderr().write_all(chunk);
            let _ = std::io::stderr().flush();
        },
    );

    // Send the same script as your C demo
    // let _ = child.write_all(".print PIPE_OK\n");
    // let _ = child.write_all(".echo on\n.print OK\n.echo off\n");
    // let _ = child.write_all(".mode line\n");
    // let _ = child.write_all("select 2*3 as v;\n");
    // let _ = child.write_all("create table t(x); insert into t values(42);\n");
    // let _ = child.write_all("select * from t;\n");
    // let _ = child.write_all(".quit\n");

    // Close stdin so sqlite can exit cleanly
    unsafe {
        if child.stdin_fd >= 0 {
            libc::close(child.stdin_fd);
        }
    }

    let code = child.wait();
    eprintln!("(child exit={code})");
    code
}

use std::io::{stdin, stdout, Write};

fn pause() {
    let mut stdout = stdout();
    stdout.write(b"Press Enter to continue...").unwrap();
    stdout.flush().unwrap();
    stdin().read(&mut [0]).unwrap();
}

// ---------- C ABI entry point ----------
#[unsafe(no_mangle)]
pub extern "C" fn ios_main(
    _cache_dir: *const c_char,
    bundle_dir: *const c_char,
    _frameworks_dir: *const c_char,
) {
    // Safety: pointers come from ObjC side
    let bundle = unsafe {
        if bundle_dir.is_null() {
            eprintln!("bundle_dir is null");
            return;
        }
        CStr::from_ptr(bundle_dir)
    };

    // Build "<bundle>/bin/bash"
    let mut path = String::new();
    path.push_str(bundle.to_string_lossy().as_ref());
    path.push_str("/bin/bash");

    let c_path = match CString::new(path) {
        Ok(s) => s,
        Err(_) => {
            eprintln!("Path contained NUL");
            return;
        }
    };

    let _ = demo_main(&c_path);

    pause();
}
