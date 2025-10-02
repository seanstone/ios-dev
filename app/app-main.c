#include <stdio.h>
#include <dirent.h>    // opendir, readdir, closedir
#include <string.h>    // strerror
#include <errno.h>     // errno
#include <unistd.h>
#include <sys/stat.h>  // stat()
#include <dlfcn.h>     // dlopen, dlsym, dlerror, dlclose

int ios_execv(const char *path, char *const argv[])
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

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir)
{
    (void)cache_dir;
    (void)frameworks_dir;

    DIR *dir = opendir(bundle_dir);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open bundle_dir %s: %s\n", bundle_dir, strerror(errno));
        return;
    }

    printf("Contents of %s:\n", bundle_dir);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("  %s\n", entry->d_name);
    }
    closedir(dir);

    // Build path safely with snprintf
    char path[1024];
    const char *program = "hello";
    int n = snprintf(path, sizeof(path), "%s/%s", bundle_dir, program);
    if (n < 0 || (size_t)n >= sizeof(path)) {
        fprintf(stderr, "Path too long when building executable path\n");
        return;
    }

    // argv[0] is program name
    char *args[] = { (char*)program, NULL };

    int rc = ios_execv(path, args);
    if (rc < 0) {
        fprintf(stderr, "ios_execv failed with code %d for '%s'\n", rc, path);
    } else {
        printf("Program '%s' returned %d\n", program, rc);
    }
}
