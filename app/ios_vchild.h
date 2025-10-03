// ios_vchild.h
#pragma once
#include <pthread.h>

typedef struct {
    int stdin_fd;   // parent writes here
    int stdout_fd;  // parent reads here
    int stderr_fd;  // parent reads here
    int exit_code;  // valid after wait
    pthread_t thread;
    int running;    // 1 while the "child" thread runs
} ios_child_t;

int ios_spawn_vchild(const char *dylib_path, char *const argv[], ios_child_t *out);
int ios_wait_vchild(ios_child_t *child, int *exit_code); // joins thread
void ios_close_vchild(ios_child_t *child); // close any remaining fds
