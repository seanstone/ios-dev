#ifndef IOS_SYSCALLS_H
#define IOS_SYSCALLS_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

int   ios_system(const char *cmd);
FILE* ios_popen (const char *cmd, const char *mode);
int   ios_pclose(FILE *stream);

#ifdef __cplusplus
}
#endif

#ifdef __APPLE__
  #undef system
  #define system ios_system

  #undef popen
  #define popen ios_popen
  
  #undef pclose
  #define pclose ios_pclose
#endif

#endif