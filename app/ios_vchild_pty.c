// ios_vchild_pty.c
#define _DARWIN_C_SOURCE
#include "ios_vchild_pty.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#if __has_include(<util.h>)
#  include <util.h>   // openpty on iOS/macOS
#else
#  include <pty.h>
#endif

typedef struct {
    void *handle;
    int (*main_entry)(int,char**);
    int argc;
    char **argv;

    int mfd, sfd;  // PTY master/slave

    pthread_mutex_t mtx;
    pthread_cond_t  cv;
    int ready;     // stdio remapped
    ios_child_pty_t *child;
} vctx_t;

static void set_cloexec(int fd) {
    if (fd >= 0) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}

static void *child_thread(void *arg) {
    vctx_t *vc = (vctx_t*)arg;

    // Save originals
    int save0 = dup(0), save1 = dup(1), save2 = dup(2);
    if (save0 < 0 || save1 < 0 || save2 < 0) {
        perror("dup(save stdio)");
        vc->child->exit_code = -127;
        goto done;
    }

    // Map PTY slave to 0/1/2
    if (dup2(vc->sfd, 0) < 0 || dup2(vc->sfd, 1) < 0 || dup2(vc->sfd, 2) < 0) {
        perror("dup2(pty slave -> stdio)");
        vc->child->exit_code = -126;
        goto restore_stdio;
    }

    // Signal readiness (stdio mapped) before closing sfd
    pthread_mutex_lock(&vc->mtx);
    vc->ready = 1;
    pthread_cond_signal(&vc->cv);
    pthread_mutex_unlock(&vc->mtx);

    // Child keeps only 0/1/2; close the slave fd handle
    close(vc->sfd); vc->sfd = -1;

    // Make I/O interactive-friendly
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Call main
    vc->child->exit_code = vc->main_entry(vc->argc, vc->argv);

restore_stdio:
    fflush(NULL);
    if (save0 >= 0) { dup2(save0,0); close(save0); }
    if (save1 >= 0) { dup2(save1,1); close(save1); }
    if (save2 >= 0) { dup2(save2,2); close(save2); }

done:
    // Cleanup
    if (vc->sfd >= 0) close(vc->sfd);
    if (vc->handle) dlclose(vc->handle);
    if (vc->argv) free(vc->argv);

    vc->child->running = 0;

    pthread_mutex_destroy(&vc->mtx);
    pthread_cond_destroy(&vc->cv);
    free(vc);
    return NULL;
}

int ios_spawn_vchild_pty(const char *dylib_path, char *const argv[], ios_child_pty_t *out) {
    if (!dylib_path || !out) { errno = EINVAL; return -1; }
    memset(out, 0, sizeof(*out));
    out->pty_master = -1;

    // Duplicate argv (shallow copy of pointers is fine)
    int argc = 0; if (argv) while (argv[argc]) argc++;
    char **argv_dup = (char**)calloc((size_t)argc + 1, sizeof(char*));
    if (!argv_dup) return -1;
    for (int i=0;i<argc;i++) argv_dup[i] = argv[i];
    argv_dup[argc] = NULL;

    // Load dylib and main
    dlerror();
    void *h = dlopen(dylib_path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        fprintf(stderr, "dlopen '%s': %s\n", dylib_path, dlerror());
        free(argv_dup);
        return -1;
    }
    dlerror();
    int (*main_entry)(int,char**) = (int(*)(int,char**))dlsym(h,"main");
    const char *se = dlerror();
    if (se || !main_entry) {
        fprintf(stderr, "dlsym('main'): %s\n", se ? se : "(unknown)");
        dlclose(h);
        free(argv_dup);
        return -1;
    }

    // Create PTY
    int mfd=-1, sfd=-1;
    struct winsize wsz = { .ws_row = 24, .ws_col = 80 };
    if (openpty(&mfd, &sfd, NULL, NULL, &wsz) < 0) {
        perror("openpty");
        dlclose(h);
        free(argv_dup);
        return -1;
    }
    set_cloexec(mfd); set_cloexec(sfd);

    // Build context
    vctx_t *vc = (vctx_t*)calloc(1, sizeof(*vc));
    if (!vc) {
        perror("calloc");
        close(mfd); close(sfd);
        dlclose(h); free(argv_dup);
        return -1;
    }
    vc->handle = h;
    vc->main_entry = main_entry;
    vc->argc = argc;
    vc->argv = argv_dup;
    vc->mfd = mfd;
    vc->sfd = sfd;
    vc->child = out;
    pthread_mutex_init(&vc->mtx, NULL);
    pthread_cond_init(&vc->cv, NULL);
    vc->ready = 0;

    // Publish parent-visible master
    out->pty_master = mfd;

    // Start thread
    out->running = 1;
    int perr = pthread_create(&out->thread, NULL, child_thread, vc);
    if (perr) {
        errno = perr; perror("pthread_create");
        out->running = 0;
        close(mfd); close(sfd);
        dlclose(h); free(argv_dup); free(vc);
        out->pty_master = -1;
        return -1;
    }

    // Wait until stdio is remapped
    pthread_mutex_lock(&vc->mtx);
    while (!vc->ready) pthread_cond_wait(&vc->cv, &vc->mtx);
    pthread_mutex_unlock(&vc->mtx);

    return 0;
}

int ios_wait_vchild_pty(ios_child_pty_t *child, int *exit_code) {
    if (!child) { errno = EINVAL; return -1; }
    if (child->running) {
        void *ret = NULL;
        int perr = pthread_join(child->thread, &ret);
        if (perr) { errno = perr; return -1; }
        child->running = 0;
    }
    if (exit_code) *exit_code = child->exit_code;
    return 0;
}

void ios_close_vchild_pty(ios_child_pty_t *child) {
    if (!child) return;
    if (child->pty_master >= 0) { close(child->pty_master); child->pty_master = -1; }
}
