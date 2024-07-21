#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
#include <errno.h>
#include <pathcch.h>
#include <windows.h>
#else
#include <libgen.h>
#include <sys/errno.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#define _STRCAT(dst, src, n) strlcat(dst, src, n)
#define _STRCPY(dst, src, n) strlcpy(dst, src, n)
#else
#define _STRCAT(dst, src, n) strncat(dst, src, n)
#define _STRCPY(dst, src, n) strncpy(dst, src, n)
#endif

#define READ_UINT32(file, var) assert(fread(var, 4, 1, file) == 1)

typedef struct {
  uint32_t x;
  uint32_t y;
} size2d;

