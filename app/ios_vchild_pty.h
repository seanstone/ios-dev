// ios_vchild_pty.h
#pragma once
#include <pthread.h>

typedef struct {
    int pty_master;     // parent reads/writes here
    int exit_code;      // set after wait
    pthread_t thread;
    int running;        // 1 while child thread runs
} ios_child_pty_t;

// Start "child": dlopen(dylib), dlsym("main"), map PTY slave to 0/1/2, call main
int ios_spawn_vchild_pty(const char *dylib_path, char *const argv[], ios_child_pty_t *out);

// Join the thread; returns 0 on success; *exit_code set if non-NULL
int ios_wait_vchild_pty(ios_child_pty_t *child, int *exit_code);

// Close master and zero fields (safe to call multiple times)
void ios_close_vchild_pty(ios_child_pty_t *child);
