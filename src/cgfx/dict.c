#include "cgfx/data.h"
#include "cgfx/dict.h"

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
