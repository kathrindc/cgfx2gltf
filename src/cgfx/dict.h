#pragma once

#include "common.h"

#define TYPE_MODEL 0
#define TYPE_TEXTURE 1
#define TYPE_LUTS 2
#define TYPE_MATERIAL 3
#define TYPE_SHADER 4
#define TYPE_CAMERA 5
#define TYPE_LIGHT 6
#define TYPE_FOG 7
#define TYPE_SCENE 8
#define TYPE_ANIM_SKEL 9
#define TYPE_ANIM_TEX 10
#define TYPE_ANIM_VIS 11
#define TYPE_ANIM_CAM 12
#define TYPE_ANIM_LIGHT 13
#define TYPE_EMITTER 14
#define TYPE_PARTICLE 15
#define TYPE__COUNT (TYPE_PARTICLE + 1)

typedef struct {
  int32_t ref_bit;
  uint16_t left_node;
  uint16_t right_node;
  uint32_t offset_name;
  uint32_t offset_data;
} dict_entry;

typedef struct {
  uint32_t offset;
  uint32_t size;
  uint32_t num_entries;
  dict_entry *entries;
  int32_t root_ref;
  uint16_t left_node;
  uint16_t right_node;
  uint32_t offset_name;
  uint32_t offset_data;
} dict;

void read_dict(FILE *file, dict *dict);
void read_dict_indirect(FILE *file, dict *dict);

