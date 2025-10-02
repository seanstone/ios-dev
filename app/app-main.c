#include <stdio.h>
#include <dirent.h>   // opendir, readdir, closedir
#include <string.h>   // strerror
#include <errno.h>    // errno
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>   // stat()

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
        // Build full path for stat
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", bundle_dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1) {
            printf("  %-20s [stat error: %s]\n", entry->d_name, strerror(errno));
            continue;
        }

        // Print permissions and name
        print_permissions(st.st_mode);
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