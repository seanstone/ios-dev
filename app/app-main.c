#include <stdio.h>
#include <dirent.h>    // opendir, readdir, closedir
#include <string.h>    // strerror
#include <errno.h>     // errno
#include <fcntl.h>     // open
#include <unistd.h>    // read, write, close
#include <sys/stat.h>  // stat, fchmod
#include <dlfcn.h>     // dlopen, dlsym, dlerror, dlclose
#include <signal.h>

static void print_permissions(mode_t mode)
{
    char perms[11];

    // File type
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else perms[0] = '-';

    // Owner
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';

    // Group
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';

    // Others
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';

    perms[10] = '\0';

    printf("%s", perms);
}

static int copy_file(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY);
    if (in < 0) {
        fprintf(stderr, "open src failed %s: %s\n", src, strerror(errno));
        return -1;
    }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (out < 0) {
        fprintf(stderr, "open dst failed %s: %s\n", dst, strerror(errno));
        close(in);
        return -1;
    }

    char buf[4096];
    ssize_t r;
    while ((r = read(in, buf, sizeof(buf))) > 0) {
        ssize_t w = write(out, buf, r);
        if (w != r) {
            fprintf(stderr, "write failed: %s\n", strerror(errno));
            close(in);
            close(out);
            return -1;
        }
    }
    close(in);
    close(out);
    return (r < 0) ? -1 : 0;
}

static int ios_execv(const char *path, char *const argv[])
{
    // compute argc
    int argc = 0;
    if (argv) {
        while (argv[argc] != NULL) argc++;
    }

    // open image
    dlerror(); // clear any old error
    void *h = dlopen(path, RTLD_NOW);
    if (!h) {
        const char *err = dlerror();
        fprintf(stderr, "dlopen failed for '%s': %s\n", path, err ? err : "(unknown)");
        return -1;
    }

    // find main symbol
    dlerror();
    int (*main_entry)(int, char **) = (int(*)(int,char**)) dlsym(h, "main");
    const char *sym_err = dlerror();
    if (sym_err || !main_entry) {
        fprintf(stderr, "dlsym('main') failed: %s\n", sym_err ? sym_err : "(unknown)");
        dlclose(h);
        return -2;
    }

    // call entry
    int rc = main_entry(argc, argv);

    if (dlclose(h) != 0) {
        const char *cerr = dlerror();
        fprintf(stderr, "dlclose failed: %s\n", cerr ? cerr : "(unknown)");
    }

    return rc;
}

static int list_dir(const char *target_dir)
{
    DIR *dir = opendir(target_dir);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open dir: %s (error: %s)\n", target_dir, strerror(errno));
        return -1;
    }

    printf("Contents of %s:\n", target_dir);

    for (;;) {
        errno = 0; // so we can detect readdir errors
        struct dirent *entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0) {
                fprintf(stderr, "readdir error: %s\n", strerror(errno));
                closedir(dir);
                return -1;
            }
            break; // end of directory
        }

        // Build full path safely
        char fullpath[1024];
        int n = snprintf(fullpath, sizeof(fullpath), "%s/%s", target_dir, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(fullpath)) {
            fprintf(stderr, "  [name too long]: %s/%s\n", target_dir, entry->d_name);
            continue;
        }

        // Stat the entry
        struct stat st;
        if (stat(fullpath, &st) == -1) {
            fprintf(stderr, "  %-20s [stat error: %s]\n", entry->d_name, strerror(errno));
            continue;
        }

        // Print permissions and name
        print_permissions(st.st_mode);
        printf("  %s\n", entry->d_name);
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "closedir error: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int demo_main(const char* program_dir);

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir)
{
    (void)frameworks_dir;

    // Build source and destination paths
    char src[1024], dst[1024];
    const char *program = "sqlite3";
    if (snprintf(src, sizeof(src), "%s/bin/%s", bundle_dir, program) >= (int)sizeof(src) ||
        snprintf(dst, sizeof(dst), "%s/%s", cache_dir, program) >= (int)sizeof(dst)) {
        fprintf(stderr, "Path too long\n");
        return;
    }

    // list_dir(bundle_dir);

    // Copy program
    // if (copy_file(src, dst) != 0) {
    //     fprintf(stderr, "Failed to copy %s to cache\n", program);
    //     return;
    // }
    // printf("Copied %s to %s\n", src, dst);

    // list_dir(cache_dir);

    // argv[0] is program name
    // char *args[] = {
    //     "sqlite3",
    //     "--version",
    //     NULL
    // };

    // int rc = ios_execv(src, args);
    // if (rc < 0) {
    //     fprintf(stderr, "ios_execv failed with code %d for '%s'\n", rc, dst);
    // } else {
    //     printf("Program '%s' returned %d\n", program, rc);
    // }

    int res = demo_main(src);

    //pause();
}
