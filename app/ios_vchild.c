#define _DARWIN_C_SOURCE
#include "ios_vchild.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// ---------- small helpers ----------
static void set_cloexec(int fd) { if (fd >= 0) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC); }
static void set_linebuf(void) {
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

// ---------- thread context ----------
typedef struct {
    void *handle;
    int (*main_entry)(int,char**);
    int argc;
    char **argv;

    // pipes
    int in_r,  in_w;
    int out_r, out_w;
    int err_r, err_w;

    // readiness sync
    pthread_mutex_t mtx;
    pthread_cond_t  cv;
    int ready;

    ios_child_t *child;

    // pumps
    ios_pump_cb on_stdout;
    ios_pump_cb on_stderr;
    void *user;
} vctx_t;

static void *pump_thread(void *arg) {
    struct { int fd; ios_pump_cb cb; void *user; } *p = arg;
    char buf[4096];
    for (;;) {
        ssize_t r = read(p->fd, buf, sizeof(buf));
        if (r > 0) { if (p->cb) p->cb(buf, (size_t)r, p->user); }
        else if (r == 0) break;                 // EOF
        else if (errno == EINTR) continue;      // retry
        else break;                             
    }
    return NULL;
}

static void *child_thread_main(void *arg) {
    vctx_t *vc = (vctx_t*)arg;

    // Save originals
    int save0 = dup(0), save1 = dup(1), save2 = dup(2);
    if (save0 < 0 || save1 < 0 || save2 < 0) {
        perror("dup(save stdio)");
        vc->child->exit_code = -127;
        goto done;
    }

    // Remap stdio to pipes (child ends)
    if (dup2(vc->in_r,  0) < 0 ||
        dup2(vc->out_w, 1) < 0 ||
        dup2(vc->err_w, 2) < 0) {
        perror("dup2(child stdio)");
        vc->child->exit_code = -126;
        goto restore_stdio;
    }

    // Signal readiness before closing any fds
    pthread_mutex_lock(&vc->mtx);
    vc->ready = 1;
    pthread_cond_signal(&vc->cv);
    pthread_mutex_unlock(&vc->mtx);

    // Close ends the child thread doesn't need
    close(vc->in_w);
    close(vc->out_r);
    close(vc->err_r);

    set_linebuf();

    // Call main
    vc->child->exit_code = vc->main_entry(vc->argc, vc->argv);

restore_stdio:
    fflush(NULL);
    if (save0 >= 0) { dup2(save0,0); close(save0); }
    if (save1 >= 0) { dup2(save1,1); close(save1); }
    if (save2 >= 0) { dup2(save2,2); close(save2); }

    // Close child's ends
    close(vc->in_r);
    close(vc->out_w);
    close(vc->err_w);

done:
    vc->child->running = 0;

    // Unload + free
    if (vc->handle) dlclose(vc->handle);
    free(vc->argv);
    pthread_mutex_destroy(&vc->mtx);
    pthread_cond_destroy(&vc->cv);
    free(vc);
    return NULL;
}

int ios_spawn_vchild(const char *dylib_path, char *const argv[], ios_child_t *out) {
    if (!dylib_path || !out) { errno = EINVAL; return -1; }
    memset(out, 0, sizeof(*out));
    out->stdin_fd = out->stdout_fd = out->stderr_fd = -1;
    out->pumps_running = 0;

    // Ignore SIGPIPE for safety (write after close -> EPIPE instead of crash)
    signal(SIGPIPE, SIG_IGN);

    // argv duplication (shallow pointers is fine)
    int argc = 0; if (argv) while (argv[argc]) argc++;
    char **argv_dup = (char**)calloc((size_t)argc + 1, sizeof(char*));
    if (!argv_dup) return -1;
    for (int i=0;i<argc;i++) argv_dup[i] = argv[i];
    argv_dup[argc] = NULL;

    // Load dylib & main
    dlerror();
    void *h = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        fprintf(stderr, "dlopen '%s': %s\n", dylib_path, dlerror());
        free(argv_dup);
        return -1;
    }
    dlerror();
    int (*main_entry)(int,char**) = (int(*)(int,char**))dlsym(h, "main");
    const char *se = dlerror();
    if (se || !main_entry) {
        fprintf(stderr, "dlsym('main'): %s\n", se ? se : "(unknown)");
        dlclose(h);
        free(argv_dup);
        return -1;
    }

    // Create pipes
    int in_pipe[2], out_pipe[2], err_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe) || pipe(err_pipe)) {
        perror("pipe");
        dlclose(h); free(argv_dup);
        return -1;
    }

    // Build context
    vctx_t *vc = (vctx_t*)calloc(1, sizeof(*vc));
    if (!vc) { perror("calloc"); dlclose(h); free(argv_dup);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return -1; }
    vc->handle = h; vc->main_entry = main_entry;
    vc->argc = argc; vc->argv = argv_dup; vc->child = out;
    vc->in_r = in_pipe[0];  vc->in_w = in_pipe[1];
    vc->out_r = out_pipe[0]; vc->out_w = out_pipe[1];
    vc->err_r = err_pipe[0]; vc->err_w = err_pipe[1];
    pthread_mutex_init(&vc->mtx, NULL);
    pthread_cond_init(&vc->cv, NULL);
    vc->ready = 0;

    // Parent visible ends
    out->stdin_fd  = vc->in_w;   set_cloexec(out->stdin_fd);
    out->stdout_fd = vc->out_r;  set_cloexec(out->stdout_fd);
    out->stderr_fd = vc->err_r;  set_cloexec(out->stderr_fd);

    // Start child thread
    out->running = 1;
    int perr = pthread_create(&out->thread, NULL, child_thread_main, vc);
    if (perr) {
        errno = perr; perror("pthread_create");
        out->running = 0;
        dlclose(h); free(argv_dup); free(vc);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return -1;
    }

    // Wait until stdio is mapped
    pthread_mutex_lock(&vc->mtx);
    while (!vc->ready) pthread_cond_wait(&vc->cv, &vc->mtx);
    pthread_mutex_unlock(&vc->mtx);

    return 0;
}

int ios_wait_vchild(ios_child_t *child, int *exit_code) {
    if (!child) { errno = EINVAL; return -1; }
    if (child->pumps_running) ios_vchild_stop_pumps(child);
    if (child->running) {
        void *ret = NULL;
        int perr = pthread_join(child->thread, &ret);
        if (perr) { errno = perr; return -1; }
        child->running = 0;
    }
    if (exit_code) *exit_code = child->exit_code;
    return 0;
}

void ios_close_vchild(ios_child_t *child) {
    if (!child) return;
    if (child->stdin_fd  >= 0) { close(child->stdin_fd);  child->stdin_fd  = -1; }
    if (child->stdout_fd >= 0) { close(child->stdout_fd); child->stdout_fd = -1; }
    if (child->stderr_fd >= 0) { close(child->stderr_fd); child->stderr_fd = -1; }
}

// ---- pumps ----
typedef struct {
    int fd;
    ios_pump_cb cb;
    void *user;
} pump_arg_t;

static void *pump_trampoline(void *arg) {
    pump_arg_t *pa = (pump_arg_t*)arg;
    pump_thread(pa);
    free(pa);
    return NULL;
}

int ios_vchild_start_pumps(ios_child_t *child,
                           ios_pump_cb on_stdout,
                           ios_pump_cb on_stderr,
                           void *user) {
    if (!child) { errno = EINVAL; return -1; }
    if (child->pumps_running) return 0;

    // stdout pump
    pump_arg_t *ao = malloc(sizeof(*ao));
    if (!ao) return -1;
    ao->fd = child->stdout_fd; ao->cb = on_stdout; ao->user = user;

    int perr = pthread_create(&child->pump_out_th, NULL, pump_trampoline, ao);
    if (perr) { free(ao); errno = perr; return -1; }

    // stderr pump
    pump_arg_t *ae = malloc(sizeof(*ae));
    if (!ae) { /* cancel previous */ pthread_cancel(child->pump_out_th); return -1; }
    ae->fd = child->stderr_fd; ae->cb = on_stderr; ae->user = user;

    perr = pthread_create(&child->pump_err_th, NULL, pump_trampoline, ae);
    if (perr) { free(ae); pthread_cancel(child->pump_out_th); errno = perr; return -1; }

    child->pumps_running = 1;
    return 0;
}

int ios_vchild_stop_pumps(ios_child_t *child) {
    if (!child || !child->pumps_running) return 0;
    // Joining; pumps exit on EOF when child closes its ends or when parent closes fds.
    pthread_join(child->pump_out_th, NULL);
    pthread_join(child->pump_err_th, NULL);
    child->pumps_running = 0;
    return 0;
}
