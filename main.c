#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <arpa/inet.h>
#include <assert.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

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
  char* name;
  uint32_t height;
  uint32_t width;
  uint32_t format;
  uint32_t size;
  uint8_t *data;
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

    output_dir[dnl] = '/';

    for (int i = 0; i < bnl; ++i) {
      output_dir[i + dnl + 1] = bn[i];
    }

    output_dir[dnl + 1 + bnl] = 0;

    free(dar);
    free(dbn);

    if (mkdir(output_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 &&
        errno != EEXIST) {
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

  uint16_t num_textures = 0;
  texture* textures = NULL;

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

    if (textures == NULL) {
      textures = malloc(sizeof(texture));
    }
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
    fprintf(stderr, "invalid %s magic (%02x %02x %02x %02x)\n", magic, found[0],
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
