#pragma once

#include "common.h"

#define COMBINE_SOURCE_PRIMARY_COLOUR 0
#define COMBINE_SOURCE_FRAG_PRIMARY_COLOUR 1
#define COMBINE_SOURCE_FRAG_SECONDARY_COLOUR 2
#define COMBINE_SOURCE_TEXTURE_0 3
#define COMBINE_SOURCE_TEXTURE_1 4
#define COMBINE_SOURCE_TEXTURE_2 5
#define COMBINE_SOURCE_TEXTURE_3 6
#define COMBINE_SOURCE_PREVIOUS_BUFFER 13
#define COMBINE_SOURCE_CONSTANT 14
#define COMBINE_SOURCE_PREVIOUS 15

#define COMBINE_OPERAND_RGB_COLOUR 0
#define COMBINE_OPERAND_RGB_INVERSE_COLOUR 1
#define COMBINE_OPERAND_RGB_ALPHA 2
#define COMBINE_OPERAND_RGB_INVERSE_ALPHA 3
#define COMBINE_OPERAND_RGB_RED 4
#define COMBINE_OPERAND_RGB_INVERSE_RED 5
#define COMBINE_OPERAND_RGB_GREEN 8
#define COMBINE_OEPRAND_RGB_INVERSE_GREEN 9
#define COMBINE_OPERAND_RGB_BLUE 12
#define COMBINE_OPERAND_RGB_INVERSE_BLUE 13

#define COMBINE_OPERAND_ALPHA_ALPHA 0
#define COMBINE_OPERAND_ALPHA_INVERSE_ALPHA 1
#define COMBINE_OPERAND_ALPHA_RED 2
#define COMBINE_OPERAND_ALPHA_INVERSE_RED 3
#define COMBINE_OPERAND_ALPHA_GREEN 4
#define COMBINE_OPERAND_ALPHA_INVERSE_GREEN 5
#define COMBINE_OPERAND_ALPHA_BLUE 6
#define COMBINE_OPERAND_ALPHA_INVERSE_BLUE 7

#define FRESNEL_NONE 0
#define FRESNEL_PRIMARY 1
#define FRESNEL_SECONDARY 2
#define FRESNEL_BOTH 3

#define FRAG_SAMPLE_INPUT_HALF_NORMAL_COS 0
#define FRAG_SAMPLE_INPUT_HALF_VIEW_COS 1
#define FRAG_SAMPLE_INPUT_VIEW_NORMAL_COS 2
#define FRAG_SAMPLE_INPUT_NORMAL_LIGHT_COS 3
#define FRAG_SAMPLE_INPUT_SPOT_LIGHT_COS 4
#define FRAG_SAMPLE_INPUT_PHI_COS 5

#define FRAG_SAMPLE_SCALE_ONE 0
#define FRAG_SAMPLE_SCALE_TWO 1
#define FRAG_SAMPLE_SCALE_FOUR 2
#define FRAG_SAMPLE_SCALE_EIGHT 3
#define FRAG_SAMPLE_SCALE_QUARTER 6
#define FRAG_SAMPLE_SCALE_HALF 7

#define TEST_FUNC_NEVER 0
#define TEST_FUNC_ALWAYS 1
#define TEST_FUNC_EQUAL 2
#define TEST_FUNC_NOT_EQUAL 3
#define TEST_FUNC_LESS 4
#define TEST_FUNC_LESS_EQUAL 5
#define TEST_FUNC_GREATER 6
#define TEST_FUNC_GREATER_EQUAL 7

#define BLEND_MODE_LOGICAL 0
#define BLEND_MODE_NOT_USED 2
#define BLEND_MODE_BLEND 3

typedef struct {
  uint16_t rgb_scale;
  uint16_t alpha_scale;
  uint32_t source_constant;
  uint32_t rgb_operator;
  uint32_t alpha_operator;
  uint32_t rgb_source[3];
  uint32_t rgb_operand[3];
  uint32_t alpha_source[3];
  uint32_t alpha_operand[3];
} texture_combiner;

typedef struct {
  uint8_t is_absolute;
  uint32_t input;
  uint32_t scale;
  char *name;
  char *lut_name;
} fragment_sampler;

typedef struct {
  uint8_t is_depth_test_enabled;
  uint8_t is_stencil_test_enabled;
  uint32_t depth_test_function;
  uint32_t is_depth_mask_enabled;
  uint32_t blend_mode;
  uint32_t logic_operation;
} fragment_operation;

typedef struct {
  uint32_t layer_config;
  uint32_t buffer_colour;
  uint32_t bump_texture;
  uint32_t bump_mode;
  uint8_t bump_renormalise;
  uint32_t fresnel_config;
  uint8_t is_clamp_highlight;
  uint8_t is_dist0_enabled;
  uint8_t is_dist1_enabled;
  uint8_t is_geomfac0_enabled;
  uint8_t is_geomfac1_enabled;
  uint8_t is_reflect_enabled;
  fragment_sampler reflect_red;
  fragment_sampler reflect_green;
  fragment_sampler reflect_blue;
  fragment_sampler dist0;
  fragment_sampler dist1;
  fragment_sampler fresnel;
  texture_combiner combiners[6];
  uint8_t is_alpha_test_enabled;
  uint32_t alpha_test_function;
  uint32_t alpha_test_reference;
} fragment_shader;
