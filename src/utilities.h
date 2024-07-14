#pragma once

#include "common.h"

char *make_file_path(char *dir, char *file, char *ext);

#ifdef WIN32
char *basename(char *path);
char *dirname(char *path);
#endif
