#include "cgfx/texture.h"
#include "cgfx/pica/etc1.h"
#include "cgfx/pica/texture.h"

static int32_t pica200_tile_order[64] = {
    0,  1,  8,  9,  2,  3,  10, 11, 16, 17, 24, 25, 18, 19, 26, 27,
    4,  5,  12, 13, 6,  7,  14, 15, 20, 21, 28, 29, 22, 23, 30, 31,
    32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59,
    36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63};


void pica200_texture_decode(uint8_t *data, uint32_t width, uint32_t height,
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

    free(etc1_order);
  } break;

  default:
    free(pixels);
    *out_pixels = NULL;
    return;
  }

#undef LOOP_PIXELS_COMMON1

  *out_pixels = pixels;
}
