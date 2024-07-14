#include "cgfx/pica/shader.h"
#include "common.h"
#include <string.h>

#define READ_UINT32_(var) READ_UINT32(file, var)

static float u32_to_f32(uint32_t value) {
  uint32_t ordered = ((value & 0xFF) << 24) | (((value >> 8) & 0xFF) << 16) |
                     (((value >> 16) & 0xFF) << 8) | ((value >> 24) & 0xFF);

  return *((float *)&ordered);
}

void pica_cmdr_new(FILE *file, uint32_t num_words, uint8_t ignore_alignment,
                   pica_command_reader *reader) {
  reader->num_f32_uniforms = 0;
  reader->lut_index = 0;
  reader->f32_uniform_index = 0;
  reader->consumed_words = 0;
  reader->num_uniforms = 0;
  reader->uniforms = NULL;

  reader->command_buffer = malloc(sizeof(uint32_t) * PICA_CMDR_BUFFER_SIZE);
  memset(reader->f32_uniforms, 0, sizeof(float *) * PICA_CMDR_F32_UNIFORMS);
  memset(reader->f32_uniform_size, 0,
         sizeof(uint32_t) * PICA_CMDR_F32_UNIFORMS);

  reader->is_valid = 1;

#define READ_UNIFORM_DATA                                                      \
  uint32_t index = reader->num_uniforms++;                                     \
                                                                               \
  if (reader->uniforms == NULL || reader->num_uniforms < 1) {                  \
    reader->uniforms = malloc(sizeof(float) * reader->num_uniforms);           \
  } else {                                                                     \
    reader->uniforms =                                                         \
        realloc(reader->uniforms, sizeof(float) * reader->num_uniforms);       \
  }                                                                            \
                                                                               \
  reader->uniforms[index] = u32_to_f32(reader->command_buffer[command_id]);

  while (reader->consumed_words < num_words) {
    uint32_t parameters;
    uint32_t header;

    READ_UINT32_(&parameters);
    READ_UINT32_(&header);

    reader->consumed_words += 2;

    uint16_t command_id = (uint16_t)(header & 0xFFFF);
    uint32_t mask = (header >> 16) & 0xF;
    uint32_t num_extra_params = (header >> 20) & 0x7FF;
    uint8_t consecutive_write = (header & 0x80000000) > 0;

    reader->command_buffer[command_id] =
        (reader->command_buffer[command_id] & (~mask & 0xF)) |
        (parameters & (0xFFFFFFF0 | mask));

    if (command_id == PICA_CMD_BLOCK_END) {
      break;
    } else if (command_id == PICA_CMD_VERTEX_SHADER_FLOAT_UNIFORM_CONFIG) {
      reader->f32_uniform_index = parameters & 0x7FFFFFFF;
    } else if (command_id == PICA_CMD_VERTEX_SHADER_FLOAT_UNIFORM_DATA) {
      READ_UNIFORM_DATA
    } else if (command_id == PICA_CMD_FRAGMENT_SHADER_LOOK_UP_TABLE_DATA) {
      reader->lut[reader->lut_index++] = reader->command_buffer[command_id];
    }

    for (uint32_t param_index; param_index < num_extra_params; ++param_index) {
      if (consecutive_write) {
        command_id += 1;
      }

      READ_UINT32_(&parameters);

      reader->consumed_words += 1;
      reader->command_buffer[command_id] =
          (reader->command_buffer[command_id] & (~mask & 0xF)) |
          (parameters & (0xFFFFFFF0 | mask));

      if (command_id > PICA_CMD_VERTEX_SHADER_FLOAT_UNIFORM_CONFIG &&
          command_id < (PICA_CMD_VERTEX_SHADER_FLOAT_UNIFORM_DATA + 8)) {
        READ_UNIFORM_DATA
      } else if (command_id == PICA_CMD_FRAGMENT_SHADER_LOOK_UP_TABLE_DATA) {
        reader->lut[reader->lut_index++] = reader->command_buffer[command_id];
      }
    }

    if (reader->num_uniforms) {
      if (reader->f32_uniforms[reader->f32_uniform_index] == NULL) {
        reader->f32_uniforms[reader->f32_uniform_index] =
            malloc(sizeof(float) * reader->num_uniforms);
      } else {
        reader->f32_uniforms[reader->f32_uniform_index] = realloc(
            reader->f32_uniforms[reader->f32_uniform_index],
            sizeof(float) *
                    reader->f32_uniform_size[reader->f32_uniform_index] +
                reader->num_uniforms);
      }

      memcpy(reader->f32_uniforms[reader->f32_uniform_index] +
                 reader->f32_uniform_size[reader->f32_uniform_index],
             reader->uniforms, reader->num_uniforms);

      reader->f32_uniform_size[reader->f32_uniform_index] +=
          reader->num_uniforms;

      free(reader->uniforms);
      reader->num_uniforms = 0;
    }

    reader->lut_index = 0;

    if (!ignore_alignment) {
      while ((ftell(file) & 7) != 0) {
        fseek(file, 1, SEEK_CUR);
      }
    }
  }
}

void pica_cmdr_destroy(pica_command_reader *reader) {
  reader->is_valid = 0;

  for (uint32_t uniform_index = 0; uniform_index < PICA_CMDR_F32_UNIFORMS;
       ++uniform_index) {
    if (reader->f32_uniforms[uniform_index] != NULL) {
      free(reader->f32_uniforms[uniform_index]);
    }
  }

  free(reader->command_buffer);
}

depth_operation pica_cmdr_get_depth_test(pica_command_reader *reader) {
  depth_operation result;

  assert(reader->is_valid);

  uint32_t command = reader->command_buffer[PICA_CMD_DEPTH_TEST_CONFIG];
  result.is_test_enabled = (command & 1) > 0;
  result.is_mask_enabled = (command & 0x1000) > 0;
  result.test_func = (command >> 4) & 0xF;

  return result;
}

blend_operation pica_cmdr_get_blend_operation(pica_command_reader *reader) {
  blend_operation result;

  assert(reader->is_valid);

  uint32_t command = reader->command_buffer[PICA_CMD_BLEND_CONFIG];
  result.rgb_func_source = (command >> 16) & 0xF;
  result.rgb_func_dest = (command >> 20) & 0xF;
  result.rgb_blend_eq = command & 0xFF;
  result.alpha_func_source = (command >> 24) & 0xF;
  result.alpha_func_dest = (command >> 28) & 0xF;
  result.alpha_blend_eq = (command >> 8) & 0xFF;

  return result;
}

stencil_operation pica_cmdr_get_stencil_test(pica_command_reader *reader) {
  stencil_operation result;

  assert(reader->is_valid);

  uint32_t test = reader->command_buffer[PICA_CMD_STENCIL_TEST_CONFIG];
  result.is_test_enabled = (test & 1) > 0;
  result.test_func = (test >> 4) & 0xF;
  result.test_ref = (test >> 16) & 0xFF;
  result.test_mask = (test >> 24) & 0xFF;

  uint32_t operation =
      reader->command_buffer[PICA_CMD_STENCIL_OPERATION_CONFIG];
  result.fail_op = operation & 0xF;
  result.zfail_op = (operation >> 4) & 0xF;
  result.pass_op = (operation >> 8) & 0xF;

  return result;
}
