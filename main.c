#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define KGFLAGS_IMPLEMENTATION
#include "kgflags.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
#include <errno.h>
#include <pathcch.h>
#include <windows.h>
#else
#include <libgen.h>
#include <sys/errno.h>
#include <unistd.h>
#endif

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

#ifdef __APPLE__
#define _STRCAT(dst, src, n) strlcat(dst, src, n)
#define _STRCPY(dst, src, n) strlcpy(dst, src, n)
#else
#define _STRCAT(dst, src, n) strncat(dst, src, n)
#define _STRCPY(dst, src, n) strncpy(dst, src, n)
#endif

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

typedef struct {
  uint32_t offset;
  uint32_t size;
  dict dicts[16];
} data_section;

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

typedef struct {
  char *name;
  uint32_t height;
  uint32_t width;
  uint32_t format;
  uint32_t size;
  uint32_t bpp;
  dict user_data;
} texture;

typedef struct {
  uint8_t has_skeleton;
  uint32_t revision;
  uint32_t offset_name;
  uint32_t num_user_data;
  uint32_t offset_user_data;
  uint32_t unk_17;
  uint8_t is_branch_visible;
  uint32_t num_children;
  uint32_t unk_18;
  uint32_t num_anim_types;
  uint32_t offset_anim_types;
  float transform_scale_vec[3];
  float transform_rotate_vec[3];
  float transform_translate_vec[3];
  float local_matrix[12];
  float world_matrix[12];
  uint32_t num_objs;
  uint32_t offset_objs;
  uint32_t num_mtob_entries;
  uint32_t offset_mtob_dict;
  uint32_t num_shapes;
  uint32_t offset_shapes;
  uint32_t num_other_entries;
  uint32_t offset_other_dict;
  uint8_t is_visible;
  uint8_t is_non_uniform_scalable;
  uint32_t layer_id;
  int32_t offset_skel_info;
} cmdl_header;

typedef struct {
  uint8_t is_model;
  uint8_t is_skeleton;
  uint32_t unk_11;
  uint32_t unk_12_off;
  uint8_t unk_13[12];
  uint32_t unk_14_off;
  float mesh_pos_offsets[3];
  uint32_t num_face_grps;
  uint32_t offset_face_grps_arr;
  uint32_t unk_15;
  uint32_t num_vert_grps;
  uint32_t offset_vert_grps_arr;
  uint32_t unk_16_off;
} sobj_header;

typedef struct {
  uint32_t num_bone_grps;
} face_group;

uint8_t magic_eq(FILE *file, const char *magic, uint8_t optional);
void read_rel_offset(FILE *file, uint32_t *offset);
void read_dict(FILE *file, dict *dict);
void read_dict_indirect(FILE *file, dict *dict);
void read_string(FILE *file, uint32_t offset, uint32_t *length, char *text);
char *read_string_alloc(FILE *file, uint32_t offset);
void pica200_decode(uint8_t *data, uint32_t width, uint32_t height,
                    uint32_t format, uint8_t **pixels);
char *make_file_path(char *dir, char *file, char *ext);

#ifdef WIN32
char *basename(char *path);
char *dirname(char *path);
#endif

int main(int argc, char **argv) {
  if (argc == 2 && strncmp(argv[1], "-h", 2) == 0) {
    printf("* * * * cgfx2gltf by kathrindc * * * *\n\n");
    printf("Usage:\n    ./cgfx2gltf [-v|-vv|-vvv] CGFX_FILE\n\n");
    printf("Options:\n    -h  displays this help page\n"
           "    -v  enable more verbose output\n\n");
    exit(0);
  }

  if (argc != 2 && argc != 3) {
    fprintf(stderr, "usage: cgfx2gltf [-v|-vv|-vvv] CGFX_FILE\n");
    exit(1);
  }

  stbi_write_tga_with_rle = 0;

  int verbose = 0;
  int cgfx_index = 1;

  if (argc == 3) {
    cgfx_index = 2;

    if (strcmp(argv[1], "-vvv") == 0) {
      verbose = 3;
    } else if (strcmp(argv[1], "-vv") == 0) {
      verbose = 2;
    } else if (strcmp(argv[1], "-v") == 0) {
      verbose = 1;
    } else {
      fprintf(stderr, "usage: cgfx2gltf [-v|-vv|-vvv] CGFX_FILE\n");
      exit(1);
    }
  }

  FILE *cgfx_file = fopen(argv[cgfx_index], "rb");
  if (!cgfx_file) {
    fprintf(stderr, "unable to open cgfx file (%d)\n", errno);
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  printf("Converting %s ...%c", basename(argv[cgfx_index]),
         verbose ? '\n' : ' ');

  char *output_dir;

  {
    char *dar = strdup(argv[cgfx_index]);
    char *dbn = strdup(argv[cgfx_index]);
    char *dn = dirname(dar);
    char *bn = basename(dbn);
    int dnl = strlen(dn);
    int bnl = strlen(bn);

    for (int i = bnl - 1; i > 0; --i) {
      if (bn[i] == '.') {
        bn[i] = 0;
        break;
      }
    }

    bnl = strlen(bn);
    output_dir = malloc(dnl + 1 + bnl + 1);

    for (int i = 0; i < dnl; ++i) {
      output_dir[i] = dn[i];
    }

#ifdef WIN32
    output_dir[dnl] = '\\';
#else
    output_dir[dnl] = '/';
#endif

    for (int i = 0; i < bnl; ++i) {
      output_dir[i + dnl + 1] = bn[i];
    }

    output_dir[dnl + 1 + bnl] = 0;

    free(dar);
    free(dbn);

#ifdef WIN32
    // TODO: Use Win32 API to retrieve working directory

    int mkdir_result = _mkdir(output_dir);
#else
    if (verbose > 1) {
      char cwd_buf[1024];
      memset(cwd_buf, 0, 1024);
      getcwd(cwd_buf, 1024);
      printf("working directory = %s\n", cwd_buf);
    }

    int mkdir_result = mkdir(output_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    if (mkdir_result != 0 && errno != EEXIST) {
      fclose(cgfx_file);
      fprintf(stderr, "Unable to create output directory (%d)\n", errno);
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
  }

  if (verbose) {
    printf("output directory = %s\n", output_dir);
  }

  magic_eq(cgfx_file, "CGFX", 0);

  {
    uint8_t endianess[2];
    assert(fread(endianess, 1, 2, cgfx_file) == 2);

    char *desc;
    if (endianess[0] == 0xFF && endianess[1] == 0xFE) {
      desc = "little endian";
    } else if (endianess[0] == 0xFE && endianess[1] == 0xFF) {
      desc = "big endian, uh-oh";
      fclose(cgfx_file);
      fprintf(stderr, "big-endian files are not supported yet\n");
      exit(1);
    } else {
      fclose(cgfx_file);
      fprintf(stderr, "invalid endianess field\n");
      exit(1);
    }

    if (verbose > 2) {
      printf("endianess = %s (0x%02x%02x)\n", desc, endianess[0], endianess[1]);
    }
  }

  {
    uint16_t header_size;
    assert(fread(&header_size, 2, 1, cgfx_file) == 1);

    if (verbose > 2) {
      printf("header size = 0x%04x\n", header_size);
    }
  }

  {
    uint32_t revision;
    assert(fread(&revision, 4, 1, cgfx_file) == 1);

    if (verbose > 2) {
      printf("revision = %u\n", revision);
    }
  }

  {
    uint32_t file_size;
    assert(fread(&file_size, 4, 1, cgfx_file) == 1);

    if (verbose > 2) {
      printf("file size = %u\n", file_size);
    }
  }

  uint32_t num_entries;
  assert(fread(&num_entries, 4, 1, cgfx_file) == 1);

  if (verbose > 2) {
    printf("num_entries = %u\n", num_entries);
  }

  data_section data;
  memset(&data, 0, sizeof(data_section));
  data.offset = ftell(cgfx_file);

  magic_eq(cgfx_file, "DATA", 0);
  assert(fread(&data.size, 4, 1, cgfx_file) == 1);

  for (int i = 0; i < TYPE__COUNT; ++i) {
    read_dict_indirect(cgfx_file, &data.dicts[i]);
  }

  if (verbose > 2) {
    printf(
        "DATA contains:\n"
        "  - %d models\n"
        "  - %d textures\n"
        "  - %d LUTs\n"
        "  - %d materials\n"
        "  - %d shaders\n"
        "  - %d cameras\n"
        "  - %d lights\n"
        "  - %d fogs\n"
        "  - %d scenes\n"
        "  - %d skeletal animations\n"
        "  - %d texture animations\n"
        "  - %d visibility animations\n"
        "  - %d camera animations\n"
        "  - %d light animations\n"
        "  - %d emitters\n"
        "  - %d particles\n",
        data.dicts[TYPE_MODEL].num_entries,
        data.dicts[TYPE_TEXTURE].num_entries, data.dicts[TYPE_LUTS].num_entries,
        data.dicts[TYPE_MATERIAL].num_entries,
        data.dicts[TYPE_SHADER].num_entries,
        data.dicts[TYPE_CAMERA].num_entries, data.dicts[TYPE_LIGHT].num_entries,
        data.dicts[TYPE_FOG].num_entries, data.dicts[TYPE_SCENE].num_entries,
        data.dicts[TYPE_ANIM_SKEL].num_entries,
        data.dicts[TYPE_ANIM_TEX].num_entries,
        data.dicts[TYPE_ANIM_VIS].num_entries,
        data.dicts[TYPE_ANIM_CAM].num_entries,
        data.dicts[TYPE_ANIM_LIGHT].num_entries,
        data.dicts[TYPE_EMITTER].num_entries,
        data.dicts[TYPE_PARTICLE].num_entries);
  } else if (verbose > 1) {
    printf("DATA contains:\n");

#define COND_PRINT_DICT(t, n)                                                  \
  if (data.dicts[t].num_entries) {                                             \
    printf("  - %d " n "\n", data.dicts[t].num_entries);                       \
  }

    COND_PRINT_DICT(TYPE_MODEL, "models");
    COND_PRINT_DICT(TYPE_TEXTURE, "textures");
    COND_PRINT_DICT(TYPE_LUTS, "LUTs");
    COND_PRINT_DICT(TYPE_MATERIAL, "materials");
    COND_PRINT_DICT(TYPE_SHADER, "shaders");
    COND_PRINT_DICT(TYPE_CAMERA, "cameras");
    COND_PRINT_DICT(TYPE_LIGHT, "lights");
    COND_PRINT_DICT(TYPE_FOG, "fogs");
    COND_PRINT_DICT(TYPE_SCENE, "scenes");
    COND_PRINT_DICT(TYPE_ANIM_SKEL, "skeletal animations");
    COND_PRINT_DICT(TYPE_ANIM_TEX, "texture animations");
    COND_PRINT_DICT(TYPE_ANIM_VIS, "visibility animations");
    COND_PRINT_DICT(TYPE_ANIM_CAM, "camera animations");
    COND_PRINT_DICT(TYPE_ANIM_LIGHT, "light animations");
    COND_PRINT_DICT(TYPE_EMITTER, "emitters");
    COND_PRINT_DICT(TYPE_PARTICLE, "particles");

#undef COND_PRINT_DICT
  }

  for (int i = 0; i < data.dicts[TYPE_TEXTURE].num_entries; ++i) {
    dict_entry *entry = data.dicts[TYPE_TEXTURE].entries + i;
    char *name = read_string_alloc(cgfx_file, entry->offset_name);

    if (verbose > 1) {
      printf("Extracting texture \"%s\"\n", name);
    }

    fseek(cgfx_file, entry->offset_data, SEEK_SET);

    txob_header txob;
    assert(fread(&txob.type, 4, 1, cgfx_file) == 1);
    magic_eq(cgfx_file, "TXOB", 0);
    assert(fread(&txob.revision, 4, 1, cgfx_file) == 1);
    read_rel_offset(cgfx_file, &txob.offset_name);
    read_dict_indirect(cgfx_file, &txob.user_data);
    assert(fread(&txob.height, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.width, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.gl_format, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.gl_type, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.mipmap_levels, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.tex_obj, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.location_flags, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.format, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.unk_19, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.unk_20, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.unk_21, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.data_length, 4, 1, cgfx_file) == 1);
    read_rel_offset(cgfx_file, &txob.data_offset);
    assert(fread(&txob.dyn_alloc, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.bpp, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.location_address, 4, 1, cgfx_file) == 1);
    assert(fread(&txob.memory_address, 4, 1, cgfx_file) == 1);

    uint8_t *data = malloc(txob.data_length);
    uint8_t *pixels;

    fseek(cgfx_file, txob.data_offset, SEEK_SET);
    assert(fread(data, 1, txob.data_length, cgfx_file) == txob.data_length);

    pica200_decode(data, txob.width, txob.height, txob.format, &pixels);
    if (pixels == NULL) {
      fprintf(stderr, "Error: Texture \"%s\" has unsupported format 0x%08x\n",
              name, txob.format);
      free(data);
      continue;
    }

    char *tga_path = make_file_path(output_dir, name, ".tga");

    int write_ok = stbi_write_tga(tga_path, txob.width, txob.height, 4, pixels);
    if (!write_ok) {
      fprintf(stderr, "Error: Failed to write \"%s\" to disk\n", name);
      free(data);
      continue;
    }

    if (verbose) {
      printf("Texture \"%s\" saved as TGA image\n", name);
    }

    free(data);
  }

  for (int i = 0; i < data.dicts[TYPE_MODEL].num_entries; ++i) {
  }

  fclose(cgfx_file);

  printf("OK\n");

  return 0;
}

uint8_t magic_eq(FILE *file, const char *magic, uint8_t optional) {
  char found[4];
  assert(fread(found, 1, 4, file) == 4);

  if (strncmp(found, magic, 4) != 0) {
    if (optional) {
      return 0;
    }

    fclose(file);
    fprintf(stderr, "Error: Invalid %s magic (%02x %02x %02x %02x)\n", magic, found[0],
            found[1], found[2], found[3]);
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

void read_dict(FILE *file, dict *dict) {
  magic_eq(file, "DICT", 1);
  assert(fread(&dict->size, 4, 1, file) == 1);
  assert(fread(&dict->num_entries, 4, 1, file) == 1);
  assert(fread(&dict->root_ref, 4, 1, file) == 1);
  assert(fread(&dict->left_node, 2, 1, file) == 1);
  assert(fread(&dict->right_node, 2, 1, file) == 1);
  read_rel_offset(file, &dict->offset_name);
  read_rel_offset(file, &dict->offset_data);

  uint32_t entries_size = sizeof(dict_entry) * dict->num_entries;
  dict->entries = malloc(entries_size);
  memset(dict->entries, 0, entries_size);

  for (int i = 0; i < dict->num_entries; ++i) {
    assert(fread(&dict->entries[i].ref_bit, 4, 1, file) == 1);
    assert(fread(&dict->entries[i].left_node, 2, 1, file) == 1);
    assert(fread(&dict->entries[i].right_node, 2, 1, file) == 1);
    read_rel_offset(file, &dict->entries[i].offset_name);
    read_rel_offset(file, &dict->entries[i].offset_data);
  }
}

void read_dict_indirect(FILE *file, dict *dict) {
  assert(fread(&dict->num_entries, 4, 1, file) == 1);
  read_rel_offset(file, &dict->offset);

  if (dict->num_entries) {
    uint32_t previous_pos = ftell(file);

    fseek(file, dict->offset, SEEK_SET);
    read_dict(file, dict);

    fseek(file, previous_pos, SEEK_SET);
  } else {
    dict->entries = NULL;
  }
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

int32_t pica200_tile_order[64] = {
    0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
    4,  5,  12, 13, 6,  7,  14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
    32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
    36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63};

int32_t etc1_lut[8][4] = {{2, 8, -2, -8},       {5, 17, -5, -17},
                          {9, 29, -9, -29},     {13, 42, -13, -42},
                          {18, 60, -18, -60},   {24, 80, -24, -80},
                          {33, 106, -33, -106}, {47, 183, -47, -183}};

uint8_t etc1_saturate(int32_t value) {
  if (value > 0xFF) {
    return 0xFF;
  }

  if (value < 0) {
    return 0;
  }

  return (uint8_t)(value & 0xFF);
}

uint32_t etc1_pixel(uint32_t r, uint32_t g, uint32_t b, uint32_t x, uint32_t y,
                    uint32_t block, uint32_t table) {
  uint32_t index = x * 4 + y;
  uint32_t msb = block << 1;
  uint32_t pixel = index < 8 ? etc1_lut[table][((block >> (index + 24)) & 1) +
                                               ((msb >> (index + 8)) & 2)]
                             : etc1_lut[table][((block >> (index + 8)) & 1) +
                                               ((msb >> (index - 8)) & 2)];

  r = (uint32_t)etc1_saturate((int32_t)(r + pixel));
  g = (uint32_t)etc1_saturate((int32_t)(g + pixel));
  b = (uint32_t)etc1_saturate((int32_t)(b + pixel));

  return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void etc1_decode_block(uint8_t *block, uint8_t *result) {
  uint32_t top_half = *((uint32_t *)block);
  uint32_t bottom_half = *(((uint32_t *)block) + 1);
  uint8_t flip = (top_half & 0x1000000) > 0;
  uint8_t diff = (top_half & 0x2000000) > 0;
  uint32_t r1, g1, b1, r2, g2, b2;

  if (diff) {
    r1 = top_half & 0xF8;
    g1 = (top_half & 0xF800) >> 8;
    b1 = (top_half & 0xF80000) >> 16;
    r2 = (uint32_t)((int8_t)(r1 >> 3) + ((int8_t)((top_half & 7) << 5) >> 5));
    g2 = (uint32_t)((int8_t)(g1 >> 3) +
                    ((int8_t)(((top_half) & 0x700) >> 3) >> 5));
    b2 = (uint32_t)((int8_t)(b1 >> 3) +
                    ((int8_t)(((top_half) & 0x70000) >> 11) >> 5));

    r1 |= r1 >> 5;
    g1 |= g1 >> 5;
    b1 |= b1 >> 5;
    r2 = (r2 << 3) | (r2 >> 2);
    g2 = (g2 << 3) | (g2 >> 2);
    b2 = (b2 << 3) | (b2 >> 2);
  } else {
    r1 = top_half & 0xF0;
    g1 = (top_half & 0xF000) >> 8;
    b1 = (top_half & 0xF00000) >> 16;
    r2 = (top_half & 0xF) << 4;
    g2 = (top_half & 0xF00) >> 4;
    b2 = (top_half & 0xF0000) >> 12;

    r1 |= r1 >> 4;
    g1 |= g1 >> 4;
    b1 |= b1 >> 4;
    r2 |= r2 >> 4;
    g2 |= g2 >> 4;
    b2 |= b2 >> 4;
  }

  uint32_t table1 = (top_half >> 29) & 7;
  uint32_t table2 = (top_half >> 26) & 7;

  if (flip) {
    for (int32_t y = 0; y < 2; ++y) {
      for (int32_t x = 0; x < 4; ++x) {
        uint32_t color1 = etc1_pixel(r1, g1, b1, x, y, bottom_half, table1);
        uint32_t color2 = etc1_pixel(r2, g2, b2, x, y + 2, bottom_half, table2);

        int32_t offset1 = (y * 4 + x) * 4;
        result[offset1] = (color1 >> 8) & 0xFF;
        result[offset1 + 1] = (color1 >> 16) & 0xFF;
        result[offset1 + 2] = (color1 >> 24) & 0xFF;

        int offset2 = ((y + 2) * 4 + x) * 4;
        result[offset2] = (color2 >> 8) & 0xFF;
        result[offset2 + 1] = (color2 >> 16) & 0xFF;
        result[offset2 + 2] = (color2 >> 24) & 0xFF;
      }
    }
  } else {
    for (int32_t y = 0; y < 4; ++y) {
      for (int32_t x = 0; x < 2; ++x) {
        uint32_t color1 = etc1_pixel(r1, g1, b1, x, y, bottom_half, table1);
        uint32_t color2 = etc1_pixel(r2, g2, b2, x + 2, y, bottom_half, table2);

        int32_t offset1 = (y * 4 + x) * 4;
        result[offset1] = (color1 >> 8) & 0xFF;
        result[offset1 + 1] = (color1 >> 16) & 0xFF;
        result[offset1 + 2] = (color1 >> 24) & 0xFF;

        int offset2 = (y * 4 + x + 2) * 4;
        result[offset2] = (color2 >> 8) & 0xFF;
        result[offset2 + 1] = (color2 >> 16) & 0xFF;
        result[offset2 + 2] = (color2 >> 24) & 0xFF;
      }
    }
  }
}

void etc1_decode(uint8_t *encoded, uint32_t width, uint32_t height,
                 uint8_t alpha, uint8_t **output) {
  uint8_t *decoded = malloc(width * height * 4);
  uint32_t offset = 0;

  for (uint32_t y = 0; y < height / 4; ++y) {
    for (uint32_t x = 0; x < width / 4; ++x) {
      uint8_t color_block[8] = {0, 0, 0, 0, 0, 0, 0, 0};
      uint8_t alpha_block[8] = {0, 0, 0, 0, 0, 0, 0, 0};

      if (alpha) {
        for (uint32_t i = 0; i < 8; ++i) {
          color_block[7 - i] = encoded[offset + 8 + i];
          alpha_block[i] = encoded[offset + i];
        }

        offset += 16;
      } else {
        for (uint32_t i = 0; i < 8; ++i) {
          color_block[7 - i] = encoded[offset + i];
          alpha_block[i] = 0xFF;
        }

        offset += 8;
      }

      uint8_t decoded_color_block[64];
      uint32_t alpha_offset = 0;
      uint8_t alpha_half = 0;

      etc1_decode_block(color_block, decoded_color_block);

      for (int32_t inner_x = 0; inner_x < 4; ++inner_x) {
        for (int32_t inner_y = 0; inner_y < 4; ++inner_y) {
          uint32_t decoded_offset =
              (x * 4 + inner_x + ((y * 4 + inner_y) * width)) * 4;
          uint32_t block_offset = (inner_x + inner_y * 4) * 4;
          uint8_t a = alpha_half ? ((alpha_block[alpha_offset++] & 0xF0) >> 4)
                                 : (alpha_block[alpha_offset] & 0xF);

          memcpy(decoded + decoded_offset, decoded_color_block + block_offset, 3);

          decoded[decoded_offset + 3] = a | (a << 4);
          alpha_half = !alpha_half;
        }
      }
    }
  }

  *output = decoded;
}

void etc1_scramble(uint32_t width, uint32_t height, int32_t **result) {
  int32_t scramble_len = (width / 4) * (height / 4);
  int32_t *scramble = malloc(scramble_len);
  int32_t base_accum = 0, base_num = 0;
  int32_t row_accum = 0, row_num = 0;

  for (int32_t tile = 0; tile < scramble_len; ++tile) {
    if ((tile % (width / 4)) == 0 && tile > 0) {
      if (row_accum < 1) {
        row_accum += 1;
        row_num += 2;
        base_num = row_num;
      } else {
        row_accum = 0;
        base_num -= 2;
        row_num = base_num;
      }
    }

    scramble[tile] = base_num;

    if (base_accum < 1) {
      base_accum += 1;
      base_num += 1;
    } else {
      base_accum = 0;
      base_num += 3;
    }
  }

  *result = scramble;
}

void pica200_decode(uint8_t *data, uint32_t width, uint32_t height,
                    uint32_t format, uint8_t **out_pixels) {
  uint8_t *pixels = malloc(width * height * 4);
  uint32_t data_offset = 0;
  uint8_t half_byte = 0;

#define LOOP_PIXELS_COMMON1(block)                                             \
  for (uint32_t y = 0; y < height / 8; ++y) {                                  \
    for (uint32_t x = 0; x < width / 8; ++x) {                                 \
      for (uint32_t p = 0; p < 64; ++p) {                                      \
        uint32_t ox = pica200_tile_order[p] % 8;                               \
        uint32_t oy = (pica200_tile_order[p] - ox) / 8;                        \
        uint32_t pixels_offset = ((x * 8) + ox + ((y * 8 + oy) * width)) * 4;  \
        block                                                                  \
      }                                                                        \
    }                                                                          \
  }

  switch (format) {
  case TEX_FORMAT_RGBA8:
    LOOP_PIXELS_COMMON1({
      memcpy(pixels + pixels_offset, data + data_offset + 1, 3);
      pixels[pixels_offset + 3] = data[data_offset];
      data_offset += 4;
    })
    break;

  case TEX_FORMAT_RGB8:
    LOOP_PIXELS_COMMON1({
      memcpy(pixels + pixels_offset, data + data_offset, 3);
      pixels[pixels_offset + 3] = 0xFF;
      data_offset += 3;
    })
    break;

  case TEX_FORMAT_RGBA5551:
    LOOP_PIXELS_COMMON1({
      uint16_t pixel_data = ((uint16_t)data[data_offset]) |
                            (((uint16_t)data[data_offset + 1]) << 8);
      uint8_t r = (uint8_t)(((pixel_data >> 1) & 0x1F) << 3);
      uint8_t g = (uint8_t)(((pixel_data >> 6) & 0x1F) << 3);
      uint8_t b = (uint8_t)(((pixel_data >> 11) & 0x1F) << 3);

      pixels[pixels_offset] = r | (r >> 5);
      pixels[pixels_offset + 1] = g | (g >> 5);
      pixels[pixels_offset + 2] = b | (b >> 5);
      pixels[pixels_offset + 3] = (uint8_t)((pixel_data & 1) * 0xFF);
      data_offset += 2;
    })
    break;

  case TEX_FORMAT_RGB565:
    LOOP_PIXELS_COMMON1({
      uint16_t pixel_data = ((uint16_t)data[data_offset]) |
                            (((uint16_t)data[data_offset + 1]) << 8);
      uint8_t r = (uint8_t)((pixel_data & 0x1F) << 3);
      uint8_t g = (uint8_t)(((pixel_data >> 5) & 0x3F) << 2);
      uint8_t b = (uint8_t)(((pixel_data >> 11) & 0x1F) << 3);

      pixels[pixels_offset] = r | (r >> 5);
      pixels[pixels_offset + 1] = g | (g >> 6);
      pixels[pixels_offset + 2] = b | (b >> 5);
      pixels[pixels_offset + 3] = 0xFF;
      data_offset += 2;
    })
    break;

  case TEX_FORMAT_RGBA4:
    LOOP_PIXELS_COMMON1({
      uint16_t pixel_data = ((uint16_t)data[data_offset]) |
                            (((uint16_t)data[data_offset + 1]) << 8);
      uint8_t r = (uint8_t)((pixel_data >> 4) & 0xF);
      uint8_t g = (uint8_t)((pixel_data >> 8) & 0xF);
      uint8_t b = (uint8_t)((pixel_data >> 12) & 0xF);
      uint8_t a = (uint8_t)(pixel_data & 0xF);

      pixels[pixels_offset] = r | (r << 4);
      pixels[pixels_offset + 1] = g | (g << 4);
      pixels[pixels_offset + 2] = b | (b << 4);
      pixels[pixels_offset + 3] = a | (a << 4);
      data_offset += 2;
    })
    break;

  case TEX_FORMAT_LA8:
  case TEX_FORMAT_HILO8:
    LOOP_PIXELS_COMMON1({
      pixels[pixels_offset] = data[data_offset];
      pixels[pixels_offset + 1] = data[data_offset];
      pixels[pixels_offset + 2] = data[data_offset];
      pixels[pixels_offset + 3] = data[data_offset + 1];
      data_offset += 2;
    })
    break;

  case TEX_FORMAT_L8:
    LOOP_PIXELS_COMMON1({
      pixels[pixels_offset] = data[data_offset];
      pixels[pixels_offset + 1] = data[data_offset];
      pixels[pixels_offset + 2] = data[data_offset];
      pixels[pixels_offset + 3] = 0xFF;
      data_offset += 1;
    })
    break;

  case TEX_FORMAT_A8:
    LOOP_PIXELS_COMMON1({
      pixels[pixels_offset] = 0xFF;
      pixels[pixels_offset + 1] = 0xFF;
      pixels[pixels_offset + 2] = 0xFF;
      pixels[pixels_offset + 3] = data[data_offset];
      data_offset += 1;
    })
    break;

  case TEX_FORMAT_LA4:
    LOOP_PIXELS_COMMON1({
      uint8_t l = (data[data_offset] >> 4) & 0xF;
      pixels[pixels_offset] = l;
      pixels[pixels_offset + 1] = l;
      pixels[pixels_offset + 2] = l;
      pixels[pixels_offset + 3] = data[data_offset] & 0xF;
      data_offset += 1;
    })
    break;

  case TEX_FORMAT_L4:
    LOOP_PIXELS_COMMON1({
      uint8_t l = half_byte ? ((data[data_offset++] & 0xF0) >> 4)
                            : (data[data_offset] & 0xF);
      half_byte = !half_byte;
      l |= (l << 4);
      pixels[pixels_offset] = l;
      pixels[pixels_offset + 1] = l;
      pixels[pixels_offset + 2] = l;
      pixels[pixels_offset + 3] = 0xFF;
    })
    break;

  case TEX_FORMAT_A4:
    LOOP_PIXELS_COMMON1({
      uint8_t a = half_byte ? ((data[data_offset++] & 0xF0) >> 4)
                            : (data[data_offset] & 0xF);
      half_byte = !half_byte;
      pixels[pixels_offset] = 0xFF;
      pixels[pixels_offset + 1] = 0xFF;
      pixels[pixels_offset + 2] = 0xFF;
      pixels[pixels_offset + 3] = a | (a << 4);
    })
    break;

  case TEX_FORMAT_ETC1:
  case TEX_FORMAT_ETC1A4: {
    uint8_t *etc1_decoded = NULL;
    int32_t *etc1_order = NULL;
    uint8_t alpha = (format == TEX_FORMAT_ETC1A4);
    int32_t i = 0;

    etc1_decode(data, width, height, alpha, &etc1_decoded);
    etc1_scramble(width, height, &etc1_order);

    for (int32_t tile_y = 0; tile_y < (height / 4); ++tile_y) {
      for (int32_t tile_x = 0; tile_x < (width / 4); ++tile_x) {
        int32_t order_x = etc1_order[i] % (width / 4);
        int32_t order_y = (etc1_order[i] - order_x) / (width / 4);
        i += 1;

        for (int32_t y = 0; y < 4; ++y) {
          for (int32_t x = 0; x < 4; ++x) {
            uint32_t pixels_offset =
                ((tile_x * 4) + x + (((tile_y * 4) + y) * width)) * 4;
            data_offset =
                ((order_x * 4) + x + (((order_y * 4) + y) * width)) * 4;

            memcpy(pixels + pixels_offset, etc1_decoded + data_offset, 4);
          }
        }
      }
    }

  } break;

  default:
    free(pixels);
    *out_pixels = NULL;
    return;
  }

#undef LOOP_PIXELS_COMMON1

  *out_pixels = pixels;
}

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
