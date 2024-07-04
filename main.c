#include "cgltf.h"
#include "cgltf_write.h"
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
#define TYPE_ENV 8
#define TYPE_ANIM_SKEL 9
#define TYPE_ANIM_TEX 10
#define TYPE_ANIM_VIS 11
#define TYPE_ANIM_CAM 12
#define TYPE_ANIM_LIGHT 13
#define TYPE_EMITTER 14
#define TYPE_UNKNOWN 15
#define TYPE__COUNT (TYPE_UNKNOWN + 1)

typedef struct {
  uint32_t offset;
  uint32_t size;
  uint32_t num_entries;
  int32_t root_ref;
  uint16_t left_node;
  uint16_t right_node;
  uint32_t offset_name;
  uint32_t offset_data;
} dict_ref;

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
  dict_ref dicts[16];
} data_section;

typedef struct {
  uint32_t type;
  uint32_t revision;
  uint32_t offset_name;
  uint32_t num_data_entries;
  uint32_t offset_data_entries;
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

void magic_eq(FILE *file, const char *magic);
void read_dict_head(FILE* file, dict_ref *dict);
void read_dict_entry(FILE* file, dict_entry *entry);
void read_rel_offset(FILE* file, uint32_t* offset);

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
    printf("Output files in dir %s\n", output_dir);
  }

  magic_eq(cgfx_file, "CGFX");

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

  magic_eq(cgfx_file, "DATA");
  assert(fread(&data.size, 4, 1, cgfx_file) == 1);

  for (int i = 0; i < TYPE__COUNT; ++i) {
    assert(fread(&data.dicts[i].num_entries, 4, 1, cgfx_file) == 1);
    read_rel_offset(cgfx_file, &data.dicts[i].offset);
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
        "  - %d unknown entries\n",
        data.dicts[TYPE_MODEL].num_entries,
        data.dicts[TYPE_TEXTURE].num_entries, data.dicts[TYPE_LUTS].num_entries,
        data.dicts[TYPE_MATERIAL].num_entries,
        data.dicts[TYPE_SHADER].num_entries,
        data.dicts[TYPE_CAMERA].num_entries, data.dicts[TYPE_LIGHT].num_entries,
        data.dicts[TYPE_FOG].num_entries, data.dicts[TYPE_ENV].num_entries,
        data.dicts[TYPE_ANIM_SKEL].num_entries,
        data.dicts[TYPE_ANIM_TEX].num_entries,
        data.dicts[TYPE_ANIM_VIS].num_entries,
        data.dicts[TYPE_ANIM_CAM].num_entries,
        data.dicts[TYPE_ANIM_LIGHT].num_entries,
        data.dicts[TYPE_EMITTER].num_entries,
        data.dicts[TYPE_UNKNOWN].num_entries);
  } else if (verbose > 1) {
    printf("DATA contains:\n");

    #define COND_PRINT_DICT(t, n) \
      if (data.dicts[t].num_entries) { \
        printf("  - %d "n"\n", data.dicts[t].num_entries); \
      }

    COND_PRINT_DICT(TYPE_MODEL, "models");
    COND_PRINT_DICT(TYPE_TEXTURE, "textures");
    COND_PRINT_DICT(TYPE_LUTS, "LUTs");
    COND_PRINT_DICT(TYPE_MATERIAL, "materials");
    COND_PRINT_DICT(TYPE_SHADER, "shaders");
    COND_PRINT_DICT(TYPE_CAMERA, "cameras");
    COND_PRINT_DICT(TYPE_LIGHT, "lights");
    COND_PRINT_DICT(TYPE_FOG, "fogs");
    COND_PRINT_DICT(TYPE_ENV, "scenes");
    COND_PRINT_DICT(TYPE_ANIM_SKEL, "skeletal animations");
    COND_PRINT_DICT(TYPE_ANIM_TEX, "texture animations");
    COND_PRINT_DICT(TYPE_ANIM_VIS, "visibility animations");
    COND_PRINT_DICT(TYPE_ANIM_CAM, "camera animations");
    COND_PRINT_DICT(TYPE_ANIM_LIGHT, "light animations");
    COND_PRINT_DICT(TYPE_EMITTER, "emitters");
    COND_PRINT_DICT(TYPE_UNKNOWN, "unknown entries");

    #undef COND_PRINT_DICT
  }

  read_dict_head(cgfx_file, &data.dicts[TYPE_MODEL]);
  for (int i = 0; i < data.dicts[TYPE_MODEL].num_entries; ++i) {
    dict_entry entry;
    read_dict_entry(cgfx_file, &entry);
  }

  fclose(cgfx_file);

  printf("OK\n");

  return 0;
}

void magic_eq(FILE *file, const char *magic) {
  char found[4];
  assert(fread(found, 1, 4, file) == 4);

  if (strncmp(found, magic, 4) != 0) {
    fclose(file);
    fprintf(stderr, "invalid %s magic (%02x %02x %02x %02x)\n",
            magic, found[0], found[1], found[2], found[3]);
    exit(1);
  }
}

void read_dict_head(FILE* file, dict_ref *dict) {
  magic_eq(file, "DICT");
  assert(fread(&dict->size, 4, 1, file) == 1);
  assert(fread(&dict->num_entries, 4, 1, file) == 1);
  assert(fread(&dict->root_ref, 4, 1, file) == 1);
  assert(fread(&dict->left_node, 2, 1, file) == 1);
  assert(fread(&dict->right_node, 2, 1, file) == 1);
  assert(fread(&dict->offset_name, 4, 1, file) == 1);
  assert(fread(&dict->offset_data, 4, 1, file) == 1);
}

void read_dict_entry(FILE* file, dict_entry *entry) {
  assert(fread(&entry->ref_bit, 4, 1, file) == 1);
  assert(fread(&entry->left_node, 2, 1, file) == 1);
  assert(fread(&entry->right_node, 2, 1, file) == 1);
  assert(fread(&entry->offset_name, 4, 1, file) == 1);
  assert(fread(&entry->offset_data, 4, 1, file) == 1);
}

void read_rel_offset(FILE* file, uint32_t* offset) {
  uint32_t pos = ftell(file);
  assert(fread(offset, 4, 1, file) == 1);

  *offset += pos;
}
