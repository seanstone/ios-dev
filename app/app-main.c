#include <stdio.h>
#include <dirent.h>   // opendir, readdir, closedir
#include <string.h>   // strerror
#include <errno.h>    // errno
#include <unistd.h>
#include <string.h>

void ios_main(const char* cache_dir, const char* bundle_dir, const char* frameworks_dir)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(bundle_dir);
    if (dir == NULL) {
        printf("Failed to open bundle_dir: %s (error: %s)\n", bundle_dir, strerror(errno));
        return;
    }

    printf("Contents of %s:\n", bundle_dir);
    while ((entry = readdir(dir)) != NULL) {
        printf("  %s\n", entry->d_name);
    }

    closedir(dir);

    char path[1024] = "";
    strcat(path, bundle_dir);

    // Program to execute
    char *program = "hello";
    strcat(path, "/");
    strcat(path, program);

    // Arguments (argv[0] is conventionally the program name)
    char *args[] = { program, NULL };

    printf("Before execv...\n");

    // Replace current process with /bin/ls
    if (execv(path, args) == -1) {
        perror("execv failed");
    }

    // This line is only reached if execv fails
    printf("After execv (should not be printed if execv succeeds)\n");
}