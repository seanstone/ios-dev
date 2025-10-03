// ios_vchild.c
#define _DARWIN_C_SOURCE
#include "ios_vchild.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    void *handle;
    int (*main_entry)(int,char**);

    // pipes
    int in_r,  in_w;    // stdin pipe:  parent writes -> child reads (dup to 0)
    int out_r, out_w;   // stdout pipe: child writes (dup to 1) -> parent reads
    int err_r, err_w;   // stderr pipe: child writes (dup to 2) -> parent reads

    int argc;
    char **argv;
    ios_child_t *child;

    // readiness sync
    pthread_mutex_t mtx;
    pthread_cond_t  cv;
    int ready;  // 0 until stdio dup2 done
} vctx_t;

static void set_cloexec(int fd) { fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC); }
static void set_nonblock(int fd) { fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK); }

static void *child_thread_main(void *arg) {
    vctx_t *vc = (vctx_t*)arg;

    int save_stdin  = dup(STDIN_FILENO);
    int save_stdout = dup(STDOUT_FILENO);
    int save_stderr = dup(STDERR_FILENO);
    if (save_stdin < 0 || save_stdout < 0 || save_stderr < 0) {
        perror("dup(save stdio)");
        vc->child->exit_code = -127;
        goto done_no_restore;
    }

    // Map child's stdio
    if (dup2(vc->in_r,  STDIN_FILENO)  < 0 ||
        dup2(vc->out_w, STDOUT_FILENO) < 0 ||
        dup2(vc->err_w, STDERR_FILENO) < 0) {
        perror("dup2(child stdio)");
        vc->child->exit_code = -126;
        goto restore_stdio;
    }

    // Let parent know stdio is ready BEFORE any closes
    pthread_mutex_lock(&vc->mtx);
    vc->ready = 1;
    pthread_cond_signal(&vc->cv);
    pthread_mutex_unlock(&vc->mtx);

    // Now the child closes its own unused ends
    close(vc->in_w);   // child's not using parent's write end
    close(vc->out_r);  // child's not using parent's read end
    close(vc->err_r);

    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    vc->child->exit_code = vc->main_entry(vc->argc, vc->argv);

restore_stdio:
    fflush(NULL);
    if (save_stdin  >= 0) { dup2(save_stdin,  STDIN_FILENO);  close(save_stdin); }
    if (save_stdout >= 0) { dup2(save_stdout, STDOUT_FILENO); close(save_stdout); }
    if (save_stderr >= 0) { dup2(save_stderr, STDERR_FILENO); close(save_stderr); }

    // Close child's ends
    close(vc->in_r);
    close(vc->out_w);
    close(vc->err_w);

done_no_restore:
    vc->child->running = 0;
    // Unload
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

    // argv dup
    int argc = 0; if (argv) while (argv[argc]) argc++;
    char **argv_dup = calloc((size_t)argc + 1, sizeof(char*));
    if (!argv_dup) return -1;
    for (int i=0;i<argc;i++) argv_dup[i] = argv[i];
    argv_dup[argc] = NULL;

    // load
    dlerror();
    void *h = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
    if (!h) { fprintf(stderr,"dlopen '%s': %s\n", dylib_path, dlerror()); free(argv_dup); return -1; }
    dlerror();
    int (*main_entry)(int,char**) = (int(*)(int,char**))dlsym(h,"main");
    const char *se = dlerror();
    if (se || !main_entry) {
        fprintf(stderr, "dlsym(main): %s\n", se?se:"(unknown)");
        dlclose(h); free(argv_dup); return -1;
    }

    // pipes
    int in_pipe[2], out_pipe[2], err_pipe[2];
    if (pipe(in_pipe) || pipe(out_pipe) || pipe(err_pipe)) {
        perror("pipe"); dlclose(h); free(argv_dup); return -1;
    }

    // context
    vctx_t *vc = calloc(1, sizeof(*vc));
    if (!vc) { perror("calloc"); dlclose(h); free(argv_dup); close(in_pipe[0]);close(in_pipe[1]); close(out_pipe[0]);close(out_pipe[1]); close(err_pipe[0]);close(err_pipe[1]); return -1; }
    vc->handle = h; vc->main_entry = main_entry; vc->argc = argc; vc->argv = argv_dup; vc->child = out;
    vc->in_r  = in_pipe[0]; vc->in_w  = in_pipe[1];
    vc->out_r = out_pipe[0]; vc->out_w = out_pipe[1];
    vc->err_r = err_pipe[0]; vc->err_w = err_pipe[1];
    pthread_mutex_init(&vc->mtx, NULL);
    pthread_cond_init(&vc->cv, NULL);
    vc->ready = 0;

    // parent-visible ends
    out->stdin_fd  = vc->in_w;   // parent writes
    out->stdout_fd = vc->out_r;  // parent reads
    out->stderr_fd = vc->err_r;  // parent reads

    // Start thread
    out->running = 1;
    int perr = pthread_create(&out->thread, NULL, child_thread_main, vc);
    if (perr) {
        errno = perr; perror("pthread_create");
        out->running = 0;
        // cleanup
        dlclose(h); free(argv_dup); free(vc);
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return -1;
    }

    // Wait for child stdio to be ready before caller writes
    pthread_mutex_lock(&vc->mtx);
    while (!vc->ready) pthread_cond_wait(&vc->cv, &vc->mtx);
    pthread_mutex_unlock(&vc->mtx);

    return 0;
}

int ios_wait_vchild(ios_child_t *child, int *exit_code) {
    if (!child) { errno = EINVAL; return -1; }
    if (!child->running) { if (exit_code) *exit_code = child->exit_code; return 0; }
    void *ret = NULL;
    int perr = pthread_join(child->thread, &ret);
    if (perr != 0) { errno = perr; return -1; }
    child->running = 0;
    if (exit_code) *exit_code = child->exit_code;
    return 0;
}

void ios_close_vchild(ios_child_t *child) {
    if (!child) return;
    if (child->stdin_fd  >= 0) { close(child->stdin_fd);  child->stdin_fd  = -1; }
    if (child->stdout_fd >= 0) { close(child->stdout_fd); child->stdout_fd = -1; }
    if (child->stderr_fd >= 0) { close(child->stderr_fd); child->stderr_fd = -1; }
}
