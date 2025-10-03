#pragma once
#include <pthread.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int stdin_fd;    // parent writes here
    int stdout_fd;   // parent reads here
    int stderr_fd;   // parent reads here
    int exit_code;   // valid after wait
    pthread_t thread;
    int running;     // 1 while the "child" thread runs

    // internal: pump threads
    pthread_t pump_out_th;
    pthread_t pump_err_th;
    int pumps_running;
} ios_child_t;

// Spawn a “virtual child” whose 0/1/2 are redirected to pipes.
// Loads `dylib_path`, finds `main`, and runs it on a dedicated thread.
// Returns 0 on success.
int ios_spawn_vchild(const char *dylib_path, char *const argv[], ios_child_t *out);

// Wait for the child thread to finish. Returns 0 on success.
int ios_wait_vchild(ios_child_t *child, int *exit_code);

// Close any remaining fds (safe to call multiple times).
void ios_close_vchild(ios_child_t *child);

// ---- Pumps (reader threads) ----

typedef void (*ios_pump_cb)(const char *data, size_t len, void *user);

// Start background threads that read stdout/stderr and call your callbacks.
// Either callback may be NULL (that stream will be discarded).
int ios_vchild_start_pumps(ios_child_t *child,
                           ios_pump_cb on_stdout,
                           ios_pump_cb on_stderr,
                           void *user);

// Stop pumps (joins threads). Safe to call even if not started.
int ios_vchild_stop_pumps(ios_child_t *child);

#ifdef __cplusplus
}
#endif
