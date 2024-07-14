#include "utilities.h"

char *make_file_path(char *dir, char *file, char *ext) {
  uint32_t file_path_length = strlen(dir) + strlen(file) + strlen(ext) + 2;
  char *file_path = malloc(file_path_length);
  memset(file_path, 0, file_path_length);
  _STRCPY(file_path, dir, file_path_length);
  _STRCAT(file_path, "/", file_path_length);
  _STRCAT(file_path, file, file_path_length);
  _STRCAT(file_path, ext, file_path_length);

  return file_path;
}

#ifdef WIN32
char *basename(char *path) {
  uint32_t start = 0;
  uint32_t len = strlen(path);

  for (uint32_t i = 0; path[i] != 0; ++i) {
    if ((path[i] == '/' || path[i] == '\\') && (i < (len - 1))) {
      start = i + 1;
    }
  }

  return path + start;
}

char *dirname(char *path) {
  assert(PathCchRemoveFileSpec(path, strlen(path)) == S_OK);
  return path;
}
#endif
