#include "cgfx.h"
#include "cgfx/pica/shader.h"
#include "kgflags.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "utilities.h"

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

int main(int argc, char **argv) {
  const char *cgfx_path;
  kgflags_string("i", NULL, "The input file that should be processed", true,
                 &cgfx_path);

  char *output_dir;
  kgflags_string("o", NULL,
                 "Directory path which exported files should be stored in",
                 false, (const char **)&output_dir);

  bool verbose;
  kgflags_bool("v", false, "Write more detailed output to console", false,
               &verbose);

  bool list_contents;
  kgflags_bool("l", false, "Only list file contents then stop", false,
               &list_contents);

#ifdef WIN32
  kgflags_set_prefix("/");
#else
  kgflags_set_prefix("-");
#endif

  kgflags_set_custom_description("* * * * cgfx2gltf by kathrindc * * * *");

  if (!kgflags_parse(argc, argv)) {
    kgflags_print_errors();
    printf("\n");
    kgflags_print_usage();
    exit(1);
  }

  stbi_write_tga_with_rle = 0;

  FILE *cgfx_file = fopen(cgfx_path, "rb");
  if (!cgfx_file) {
    fprintf(stderr, "unable to open cgfx file (%d)\n", errno);
    fprintf(stderr, "%s\n", strerror(errno));
    exit(1);
  }

  if (!list_contents) {
    if (output_dir == NULL) {
      char *dar = strdup(cgfx_path);
      char *dbn = strdup(cgfx_path);
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
    }

#ifdef WIN32
    int mkdir_result = _mkdir(output_dir);
#else
    int mkdir_result = mkdir(output_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

    if (mkdir_result != 0 && errno != EEXIST) {
      fclose(cgfx_file);
      fprintf(stderr, "Unable to create output directory (%d)\n", errno);
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }

    if (verbose) {
      printf("output directory = %s\n", output_dir);
    }
  }

  magic_eq(cgfx_file, "CGFX", 0);

  if (verbose) {
    printf("header info:\n");
  }

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

    if (verbose) {
      printf("  endianess: %s (0x%02x%02x)\n", desc, endianess[0],
             endianess[1]);
    }
  }

  {
    uint16_t header_size;
    assert(fread(&header_size, 2, 1, cgfx_file) == 1);

    if (verbose) {
      printf("header size: %uh bytes\n", header_size);
    }
  }

#define READ_UINT32_(var) READ_UINT32(cgfx_file, var)

  {
    uint32_t revision;
    READ_UINT32_(&revision);

    if (verbose) {
      printf("revision: %u\n", revision);
    }
  }

  {
    uint32_t file_size;
    READ_UINT32_(&file_size);

    if (verbose) {
      printf("file size: %u bytes\n", file_size);
    }
  }

  // NOTE: This field is basically useless to us, but we just still read it
  //       so the file cursor advances. Maybe we'll use it later.
  uint32_t num_entries;
  READ_UINT32_(&num_entries);

  data_section data;
  memset(&data, 0, sizeof(data_section));
  data.offset = ftell(cgfx_file);

  magic_eq(cgfx_file, "DATA", 0);
  READ_UINT32_(&data.size);

  for (int i = 0; i < TYPE__COUNT; ++i) {
    read_dict_indirect(cgfx_file, &data.dicts[i]);
  }

  if (list_contents) {
    printf("%s contains:\n", basename((char *)cgfx_path));

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

    fclose(cgfx_file);
    exit(0);
  }

  {
    char *cgfx_name = strdup(cgfx_path);
    printf("Converting %s ...%c", basename(cgfx_name), verbose ? '\n' : ' ');
    free(cgfx_name);
  }

  for (int i = 0; i < data.dicts[TYPE_TEXTURE].num_entries; ++i) {
    dict_entry *entry = data.dicts[TYPE_TEXTURE].entries + i;
    char *name = read_string_alloc(cgfx_file, entry->offset_name);

    if (verbose) {
      printf("extracting texture \"%s\"\n", name);
    }

    fseek(cgfx_file, entry->offset_data, SEEK_SET);

    txob_header txob;
    READ_UINT32_(&txob.type);
    magic_eq(cgfx_file, "TXOB", 0);
    READ_UINT32_(&txob.revision);
    read_rel_offset(cgfx_file, &txob.offset_name);
    read_dict_indirect(cgfx_file, &txob.user_data);
    READ_UINT32_(&txob.height);
    READ_UINT32_(&txob.width);
    READ_UINT32_(&txob.gl_format);
    READ_UINT32_(&txob.gl_type);
    READ_UINT32_(&txob.mipmap_levels);
    READ_UINT32_(&txob.tex_obj);
    READ_UINT32_(&txob.location_flags);
    READ_UINT32_(&txob.format);
    READ_UINT32_(&txob.unk_19);
    READ_UINT32_(&txob.unk_20);
    READ_UINT32_(&txob.unk_21);
    READ_UINT32_(&txob.data_length);
    read_rel_offset(cgfx_file, &txob.data_offset);
    READ_UINT32_(&txob.dyn_alloc);
    READ_UINT32_(&txob.bpp);
    READ_UINT32_(&txob.location_address);
    READ_UINT32_(&txob.memory_address);

    uint8_t *data = malloc(txob.data_length);
    uint8_t *pixels;

    fseek(cgfx_file, txob.data_offset, SEEK_SET);
    assert(fread(data, 1, txob.data_length, cgfx_file) == txob.data_length);

    pica200_texture_decode(data, txob.width, txob.height, txob.format, &pixels);
    free(data);

    if (pixels == NULL) {
      fprintf(stderr, "Error: Texture \"%s\" has unsupported format 0x%08x\n",
              name, txob.format);
      continue;
    }

    char *tga_path = make_file_path(output_dir, name, ".tga");
    int write_ok = stbi_write_tga(tga_path, txob.width, txob.height, 4, pixels);

    free(pixels);
    free(tga_path);

    if (!write_ok) {
      fprintf(stderr, "Error: Failed to write \"%s\" to disk\n", name);
      continue;
    }

    if (verbose) {
      printf("Texture \"%s\" saved as TGA image\n", name);
    }
  }

  for (int model_index = 0; model_index < data.dicts[TYPE_MODEL].num_entries;
       ++model_index) {
    dict_entry *model_entry = data.dicts[TYPE_MODEL].entries + model_index;
    uint32_t flags;
    cmdl_header cmdl;

    fseek(cgfx_file, model_entry->offset_data, SEEK_SET);

    READ_UINT32_(&flags);
    cmdl.has_skeleton = (flags & 0x80) > 0;

    magic_eq(cgfx_file, "CMDL", 0);
    READ_UINT32_(&cmdl.revision);
    read_rel_offset(cgfx_file, &cmdl.offset_name);
    read_dict_indirect(cgfx_file, &cmdl.user_data);
    READ_UINT32_(&cmdl.unk_17);

    READ_UINT32_(&flags);
    cmdl.is_branch_visible = (flags & 1) > 0;

    READ_UINT32_(&cmdl.num_children);
    READ_UINT32_(&cmdl.unk_18);
    read_dict_indirect(cgfx_file, &cmdl.animation_groups);
    read_vec3f(cgfx_file, cmdl.transform_scale_vec);
    read_vec3f(cgfx_file, cmdl.transform_rotate_vec);
    read_vec3f(cgfx_file, cmdl.transform_translate_vec);
    read_mat4x3f(cgfx_file, cmdl.local_matrix);
    read_mat4x3f(cgfx_file, cmdl.world_matrix);
    read_dict_indirect(cgfx_file, &cmdl.objects);
    read_dict_indirect(cgfx_file, &cmdl.materials);
    read_dict_indirect(cgfx_file, &cmdl.shapes);
    read_dict_indirect(cgfx_file, &cmdl.nodes);

    READ_UINT32_(&flags);
    cmdl.is_visible = (flags & 1) > 0;
    cmdl.is_non_uniform_scalable = (flags & 0x100) > 0;

    READ_UINT32_(&cmdl.cull_mode);
    READ_UINT32_(&cmdl.layer_id);

    if (cmdl.has_skeleton) {
      read_rel_offset(cgfx_file, &cmdl.offset_skel_info);
    }

    for (int material_index = 0; material_index < cmdl.materials.num_entries;
         ++material_index) {
      mtob_header mtob;

      READ_UINT32_(&flags);
      magic_eq(cgfx_file, "MTOB", 0);
      READ_UINT32_(&mtob.revision);
      read_rel_offset(cgfx_file, &mtob.offset_name);
      read_dict_indirect(cgfx_file, &mtob.user_data);

      READ_UINT32_(&flags);
      mtob.is_fragment_light_enabled = (flags & 1) > 0;
      mtob.is_vertex_light_enabled = (flags & 2) > 0;
      mtob.is_hemisphere_light_enabled = (flags & 4) > 0;
      mtob.is_hemisphere_occlusion_enabled = (flags & 8) > 0;
      mtob.is_fog_enabled = (flags & 0x16) > 0;

      READ_UINT32_(&mtob.texture_coords_config);
      READ_UINT32_(&mtob.translucency_kind);

      // Skip colours stored as floats
      fseek(cgfx_file, 44, SEEK_CUR);

      read_rgba(cgfx_file, &mtob.colour.emission);
      read_rgba(cgfx_file, &mtob.colour.ambient);
      read_rgba(cgfx_file, &mtob.colour.diffuse);
      read_rgba(cgfx_file, &mtob.colour.specular_0);
      read_rgba(cgfx_file, &mtob.colour.specular_1);
      read_rgba(cgfx_file, &mtob.colour.constant_0);
      read_rgba(cgfx_file, &mtob.colour.constant_1);
      read_rgba(cgfx_file, &mtob.colour.constant_2);
      read_rgba(cgfx_file, &mtob.colour.constant_3);
      read_rgba(cgfx_file, &mtob.colour.constant_4);
      read_rgba(cgfx_file, &mtob.colour.constant_5);

      READ_UINT32_(&flags);
      mtob.rasterisation.is_polygon_offset_enabled = (flags & 1) > 0;

      READ_UINT32_(&mtob.rasterisation.cull_mode);
      READ_UINT32_(&mtob.rasterisation.polygon_offset_unit);

      fseek(cgfx_file, 12, SEEK_CUR);

      // Read depth fragment test
      {
        pica_command_reader depth_cmdr;

        READ_UINT32_(&flags);
        pica_cmdr_new(cgfx_file, 4, 1, &depth_cmdr);
        mtob.fragment_operation.depth = pica_cmdr_get_depth_test(&depth_cmdr);
        mtob.fragment_operation.depth.is_test_enabled = (flags & 1) > 0;
        mtob.fragment_operation.depth.is_mask_enabled = (flags & 2) > 0;
        pica_cmdr_destroy(&depth_cmdr);
      }

      // Read blend fragment operation
      {
        pica_command_reader blend_cmdr;
        uint32_t blend_mode;
        uint32_t blend_colour;

        READ_UINT32_(&blend_mode);
        switch (blend_mode) {
        case 1:
        case 2:
          blend_mode = BLEND_MODE_BLEND;
          break;

        case 3:
          blend_mode = BLEND_MODE_LOGICAL;
          break;

        default:
          blend_mode = BLEND_MODE_NOT_USED;
          break;
        }

        read_float_rgba(cgfx_file, &blend_colour);
        pica_cmdr_new(cgfx_file, 5, 1, &blend_cmdr);
        mtob.fragment_operation.blend =
            pica_cmdr_get_blend_operation(&blend_cmdr);
        mtob.fragment_operation.blend.mode = blend_mode;
        mtob.fragment_operation.blend.colour = blend_colour;
        pica_cmdr_destroy(&blend_cmdr);
      }

      // Read stencil fragment test
      {
        pica_command_reader stencil_cmdr;

        READ_UINT32_(&flags);
        pica_cmdr_new(cgfx_file, 4, 1, &stencil_cmdr);
        mtob.fragment_operation.stencil =
            pica_cmdr_get_stencil_test(&stencil_cmdr);
        pica_cmdr_destroy(&stencil_cmdr);
      }
    }
  }

  fclose(cgfx_file);
  free(output_dir);

  printf("OK\n");

  return 0;
}
