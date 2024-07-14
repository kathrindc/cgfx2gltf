#pragma once

#include "cgfx/dict.h"

#define CULL_NEVER 0
#define CULL_FRONT_FACE 1
#define CULL_BACK_FACE 2

typedef struct {
  uint8_t has_skeleton;
  uint32_t revision;
  uint32_t offset_name;
  dict user_data;
  uint32_t unk_17;
  uint8_t is_branch_visible;
  uint32_t num_children;
  uint32_t unk_18;
  dict animation_groups;
  float transform_scale_vec[3];
  float transform_rotate_vec[3];
  float transform_translate_vec[3];
  float local_matrix[12];
  float world_matrix[12];
  dict objects;
  dict materials;
  dict shapes;
  dict nodes;
  uint8_t is_visible;
  uint8_t is_non_uniform_scalable;
  uint32_t layer_id;
  uint32_t cull_mode;
  uint32_t offset_skel_info;
} cmdl_header;
