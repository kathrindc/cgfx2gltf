#include "cgfx/data.h"

uint8_t magic_eq(FILE *file, const char *magic, uint8_t optional) {
  char found[4];
  assert(fread(found, 1, 4, file) == 4);

  if (strncmp(found, magic, 4) != 0) {
    if (optional) {
      return 0;
    }

    fclose(file);
    fprintf(stderr, "Error: Invalid %s magic (%02x %02x %02x %02x)\n", magic,
            found[0], found[1], found[2], found[3]);
    exit(1);
  }

  return 1;
}

void read_rel_offset(FILE *file, uint32_t *result) {
  uint32_t pos = ftell(file);
  int32_t offset = 0;
  assert(fread(&offset, 4, 1, file) == 1);

  *result = pos + offset;
}

void read_string(FILE *file, uint32_t offset, uint32_t *length, char *text) {
  uint32_t previous_pos = ftell(file);
  char current_ch;

  if (length) {
    *length = 0;
  }

  fseek(file, offset, SEEK_SET);

  do {
    assert(fread(&current_ch, 1, 1, file) == 1);

    if (length) {
      *length += 1;
    }

    if (text) {
      *text++ = current_ch;
    }
  } while (current_ch);

  fseek(file, previous_pos, SEEK_SET);
}

char *read_string_alloc(FILE *file, uint32_t offset) {
  uint32_t length;
  read_string(file, offset, &length, NULL);

  char *str = malloc(length);
  read_string(file, offset, NULL, str);

  return str;
}

void read_vec3f(FILE *file, float *vec) {
  assert(fread(vec, 4, 1, file) == 1);
  assert(fread(vec + 1, 4, 1, file) == 1);
  assert(fread(vec + 2, 4, 1, file) == 1);
}

void read_mat4x3f(FILE *file, float *mat) {
  read_vec3f(file, mat);
  assert(fread(mat + 3, 4, 1, file) == 1);
  read_vec3f(file, mat + 4);
  assert(fread(mat + 7, 4, 1, file) == 1);
  read_vec3f(file, mat + 8);
  assert(fread(mat + 11, 4, 1, file) == 1);
}

void read_rgba(FILE *file, uint32_t *colour) {
  uint8_t *components = (uint8_t *)colour;

  assert(fread(components, 1, 1, file) == 1);
  assert(fread(components + 1, 1, 1, file) == 1);
  assert(fread(components + 2, 1, 1, file) == 1);
  assert(fread(components + 3, 1, 1, file) == 1);
}
