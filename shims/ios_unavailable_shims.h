#ifndef IOS_UNAVAILABLE_SHIMS_H
#define IOS_UNAVAILABLE_SHIMS_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

int   ios_shim_system(const char *cmd);
FILE* ios_shim_popen (const char *cmd, const char *mode);
int   ios_shim_pclose(FILE *stream);

#ifdef __cplusplus
}
#endif

#ifdef __APPLE__
  #undef system
  #define system ios_shim_system

  #undef popen
  #define popen ios_shim_popen
  
  #undef pclose
  #define pclose ios_shim_pclose
#endif

#endif