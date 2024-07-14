#pragma once

#include "common.h"

void pica200_texture_decode(uint8_t *data, uint32_t width, uint32_t height,
                            uint32_t format, uint8_t **out_pixels);
