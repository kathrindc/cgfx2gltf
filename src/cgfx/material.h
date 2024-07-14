#pragma once

#include "cgfx/dict.h"
#include "cgfx/shader.h"

#define MAT_TRANSLUCENCY_OPAQUE 0
#define MAT_TRANSLUCENCY_TRANSLUCENT 1
#define MAT_TRANSLUCENCY_SUBTRACTIVE 2
#define MAT_TRANSLUCENCY_ADDITIVE 3

#define COLOUR_CONST_CONSTANT_0 0
#define COLOUR_CONST_CONSTANT_1 1
#define COLOUR_CONST_CONSTANT_2 2
#define COLOUR_CONST_CONSTANT_3 3
#define COLOUR_CONST_CONSTANT_4 4
#define COLOUR_CONST_CONSTANT_5 5
#define COLOUR_CONST_EMISSION 6
#define COLOUR_CONST_AMBIENT 7
#define COLOUR_CONST_DIFFUSE 8
#define COLOUR_CONST_SPECULAR_0 9
#define COLOUR_CONST_SPECULAR_1 10

#define TEX_PROJ_UV_MAP 0
#define TEX_PROJ_CAM_CUBE_MAP 1
#define TEX_PROJ_CAM_SPHERE_MAP 2
#define TEX_PROJ_PROJECTION_MAP 3
#define TEX_PROJ_SHADOW_MAP 4
#define TEX_PROJ_SHADOW_CUBE_MAP 5

#define TEX_WRAP_CLAMP_EDGE 0
#define TEX_WRAP_CLAMP_BORDER 1
#define TEX_WRAP_REPEAT 2
#define TEX_WRAP_MIRROR_REPEAT 3

typedef struct {
  uint32_t projection;
  uint32_t reference_cam;
  float scale_u;
  float scale_v;
  float rotate;
  float translate_u;
  float translate_v;
} texture_transform;

typedef struct {
  uint32_t min_filter;
  uint32_t mag_filter;
  uint32_t wrap_u;
  uint32_t wrap_v;
  uint32_t min_lod;
  float lod_bias;
  uint32_t border_colour;
} texture_mapping;

typedef struct {
  uint32_t emission;
  uint32_t ambient;
  uint32_t diffuse;
  uint32_t specular_0;
  uint32_t specular_1;
  uint32_t constant_0;
  uint32_t constant_1;
  uint32_t constant_2;
  uint32_t constant_3;
  uint32_t constant_4;
  uint32_t constant_5;
  float scale;
} material_colour;

typedef struct {
  uint32_t cull_mode;
  uint8_t is_polygon_offset_enabled;
  float polygon_offset_unit;
} rasterisation_params;

typedef struct {
  uint32_t revision;
  uint32_t offset_name;
  dict user_data;
  material_colour colour;
  rasterisation_params rasterisation;
  texture_transform texture_transforms[3];
  texture_mapping texture_mappings[3];
  fragment_shader fragment_shader;
  fragment_operation fragment_operation;
  uint32_t light_set_index;
  uint32_t fog_index;
  uint8_t is_fragment_light_enabled;
  uint8_t is_vertex_light_enabled;
  uint8_t is_hemisphere_light_enabled;
  uint8_t is_hemisphere_occlusion_enabled;
  uint8_t is_fog_enabled;
  uint32_t texture_coords_config;
  uint32_t translucency_kind;
} mtob_header;
