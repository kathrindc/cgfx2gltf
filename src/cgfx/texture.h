#pragma once

#include "common.h"
#include "cgfx/dict.h"

#define TEX_FORMAT_RGBA8 0
#define TEX_FORMAT_RGB8 1
#define TEX_FORMAT_RGBA5551 2
#define TEX_FORMAT_RGB565 3
#define TEX_FORMAT_RGBA4 4
#define TEX_FORMAT_LA8 5
#define TEX_FORMAT_HILO8 6
#define TEX_FORMAT_L8 7
#define TEX_FORMAT_A8 8
#define TEX_FORMAT_LA4 9
#define TEX_FORMAT_L4 10
#define TEX_FORMAT_A4 11
#define TEX_FORMAT_ETC1 12
#define TEX_FORMAT_ETC1A4 13
#define TEX_FORMAT_OTHER 14

typedef struct {
  uint32_t type;
  uint32_t revision;
  uint32_t offset_name;
  dict user_data;
  uint32_t height;
  uint32_t width;
  uint32_t gl_format;
  uint32_t gl_type;
  uint32_t mipmap_levels;
  uint32_t tex_obj;
  uint32_t location_flags;
  uint32_t format;
  uint32_t unk_19;
  uint32_t unk_20;
  uint32_t unk_21;
  uint32_t data_length;
  uint32_t data_offset;
  uint32_t dyn_alloc;
  uint32_t bpp;
  uint32_t location_address;
  uint32_t memory_address;
} txob_header;

