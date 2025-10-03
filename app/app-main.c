#include <stdio.h>
#include <dirent.h>    // opendir, readdir, closedir
#include <string.h>    // strerror
#include <errno.h>     // errno
#include <fcntl.h>     // open
#include <unistd.h>    // read, write, close
#include <sys/stat.h>  // stat, fchmod
#include <dlfcn.h>     // dlopen, dlsym, dlerror, dlclose
#include <signal.h>
#include "ios_vchild.h"

int demo_main(const char* program_dir);

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir)
{
    (void)frameworks_dir;

    char src[1024];
    const char *program = "sqlite3";
    if (snprintf(src, sizeof(src), "%s/bin/%s", bundle_dir, program) >= (int)sizeof(src)) {
        fprintf(stderr, "Path too long\n");
        return;
    }

    int res = demo_main(src);
}

// simple writer
static void write_all(int fd, const char *s) {
    size_t n = strlen(s), off = 0;
    while (off < n) {
        ssize_t k = write(fd, s + off, n - off);
        if (k > 0) off += (size_t)k;
        else if (k < 0 && errno == EINTR) continue;
        else break; // EPIPE etc.
    }
}

// stdout/stderr callbacks
static void on_out(const char *data, size_t len, void *user) {
    (void)user;
    fwrite(data, 1, len, stdout);
}
static void on_err(const char *data, size_t len, void *user) {
    (void)user;
    fwrite(data, 1, len, stderr);
}

int demo_main(const char* program_dir) {
    // Prevent crashes if the child closes STDERR/STDOUT early.
    signal(SIGPIPE, SIG_IGN);

    ios_child_t ch;
    // Tip: for REPL-ish tools, add "-interactive" when using pipes.
    char *args[] = { "sqlite3", "-interactive", ":memory:", NULL };
    ios_spawn_vchild(program_dir, args, &ch);
    ios_vchild_start_pumps(&ch, on_out, on_err, NULL);

    write_all(ch.stdin_fd, ".print PIPE_OK\n");
    write_all(ch.stdin_fd, ".echo on\n.print OK\n.echo off\n");
    write_all(ch.stdin_fd, ".mode line\n");
    write_all(ch.stdin_fd, "select 2*3 as v;\n");
    write_all(ch.stdin_fd, "create table t(x); insert into t values(42);\n");
    write_all(ch.stdin_fd, "select * from t;\n");
    write_all(ch.stdin_fd, ".quit\n");

    close(ch.stdin_fd); ch.stdin_fd = -1;
    int code = 0;
    ios_wait_vchild(&ch, &code);   // joins pumps inside
    ios_close_vchild(&ch);

    fprintf(stderr, "(child exit=%d)\n", code);

    // pause();
    return 0;
}