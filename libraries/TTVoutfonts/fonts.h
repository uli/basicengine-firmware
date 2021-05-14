#include <stdint.h>

#define NUM_FONTS 5

struct builtin_font_t {
  const uint8_t *data;
  uint8_t w, h;
};

extern struct builtin_font_t builtin_fonts[NUM_FONTS];

#define MIN_FONT_SIZE_X 6
#define MIN_FONT_SIZE_Y 8

#include "ati_6x8.h"
#include "amstrad_8x8.h"
#include "cbm_ascii_8x8.h"
#include "font6x8tt.h"
#include "hp100lx_8x8.h"

