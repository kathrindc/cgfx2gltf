#pragma once

#include "cgfx/dict.h"

typedef struct {
  uint32_t offset;
  uint32_t size;
  dict dicts[16];
} data_section;

uint8_t magic_eq(FILE *file, const char *magic, uint8_t optional);
void read_rel_offset(FILE *file, uint32_t *offset);
void read_string(FILE *file, uint32_t offset, uint32_t *length, char *text);
char *read_string_alloc(FILE *file, uint32_t offset);
void read_vec3f(FILE *file, float *vec);
void read_mat4x3f(FILE *file, float *mat);
void read_rgba(FILE *file, uint32_t *colour);
