#pragma once

#include "common.h"

void etc1_decode(uint8_t *encoded, uint32_t width, uint32_t height,
                 uint8_t alpha, uint8_t **output);
void etc1_scramble(uint32_t width, uint32_t height, int32_t **result);
