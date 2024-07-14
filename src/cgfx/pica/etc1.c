#include "etc1.h"

static int32_t etc1_lut[8][4] = {{2, 8, -2, -8},       {5, 17, -5, -17},
                                 {9, 29, -9, -29},     {13, 42, -13, -42},
                                 {18, 60, -18, -60},   {24, 80, -24, -80},
                                 {33, 106, -33, -106}, {47, 183, -47, -183}};

static uint8_t etc1_saturate(int32_t value) {
  if (value > 0xFF) {
    return 0xFF;
  }

  if (value < 0) {
    return 0;
  }

  return (uint8_t)(value & 0xFF);
}

static uint32_t etc1_pixel(uint32_t r, uint32_t g, uint32_t b, uint32_t x, uint32_t y,
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

          memcpy(decoded + decoded_offset, decoded_color_block + block_offset,
                 3);

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
