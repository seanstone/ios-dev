#include "ios_syscalls.h"

int ios_system(const char *cmd) {
    if (!cmd) {
        /* POSIX: nonzero if a shell is available. On iOS, say "not available". */
        return 0;
    }
    errno = ENOSYS;
    return -1;
}

FILE* ios_popen(const char *cmd, const char *mode) {
    errno = ENOSYS;
    return NULL;
}

int ios_pclose(FILE *stream) {
    errno = ECHILD; /* reasonable error if there's nothing to wait for */
    return -1;
}