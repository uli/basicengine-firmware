#include "fonts.h"

struct builtin_font_t builtin_fonts[NUM_FONTS] = {
  { console_font_6x8, "hp100lx", 6, 8 },
  { console_font_8x8, "cpc", 8, 8 },
  { cbm_ascii_font_8x8, "bescii", 8, 8 },
  { font6x8tt, "tt", 8, 8 },
  { hp100lx_8x8, "hp100lx", 8, 8 },
};
