// demo.c
#include "ios_vchild.h"
#include "ios_vchild_pty.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>

static void write_all(int fd, const char *s) {
    size_t n = strlen(s), off = 0;
    while (off < n) {
        ssize_t k = write(fd, s + off, n - off);
        if (k > 0) off += (size_t)k;
        else if (k < 0 && (errno == EINTR)) continue;
        else break;
    }
}

int demo_main(const char* program_dir)
{
    // Avoid crashing if the child closes the PTY
    signal(SIGPIPE, SIG_IGN);

    ios_child_pty_t ch;
    char *args[] = { "sqlite3", ":memory:", NULL };

    if (ios_spawn_vchild_pty(program_dir, args, &ch) != 0) {
        perror("ios_spawn_vchild_pty");
        return 1;
    }

    // Nonblocking read is optional; using poll() is enough
    struct pollfd p = { .fd = ch.pty_master, .events = POLLIN };

    // Send a few interactive commands
    write_all(ch.pty_master, ".mode line\n");
    write_all(ch.pty_master, "select 2*3 as v;\n");
    write_all(ch.pty_master, "create table t(x); insert into t values(42);\n");
    write_all(ch.pty_master, "select * from t;\n");
    write_all(ch.pty_master, ".quit\n");

    // Read until EOF
    char buf[4096];
    for (;;) {
        int pr = poll(&p, 1, 5000);
        if (pr < 0 && errno == EINTR) continue;
        if (pr <= 0) break; // timeout or error -> stop

        if (p.revents & POLLIN) {
            ssize_t r = read(ch.pty_master, buf, sizeof(buf));
            if (r > 0) fwrite(buf, 1, (size_t)r, stdout);
            else if (r == 0) break;         // EOF
            else if (errno != EINTR) break; // error
        }
    }

    int code = 0;
    ios_wait_vchild_pty(&ch, &code);
    ios_close_vchild_pty(&ch);

    fprintf(stderr, "\n(child exit code=%d)\n", code);
    return 0;
}
