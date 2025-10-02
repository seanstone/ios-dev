#include <stdio.h>
#include <dirent.h>   // opendir, readdir, closedir
#include <string.h>   // strerror
#include <errno.h>    // errno

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
}