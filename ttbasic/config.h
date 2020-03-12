// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <stdint.h>

#define COL_BG      0
#define COL_FG      1
#define COL_KEYWORD 2
#define COL_LINENUM 3
#define COL_NUM     4
#define COL_VAR     5
#define COL_LVAR    6
#define COL_OP      7
#define COL_STR     8
#define COL_PROC    9
#define COL_COMMENT 10
#define COL_BORDER  11

#define CONFIG_COLS 12

typedef struct {
  int8_t NTSC;         // NTS Settings (0,1,2,3)
  int8_t line_adjust;  // number of lines to add to default
  int16_t KEYBOARD;    // NTSC setting Keyboard setting (0: JP, 1: US)
  uint8_t color_scheme[CONFIG_COLS][3];
  bool interlace;
  bool lowpass;
  uint8_t mode;
  uint8_t font;
  pixel_t cursor_color;
  uint8_t beep_volume;
  bool keyword_sep_optional;  // do not require separation of keywords
} SystemConfig;

extern SystemConfig CONFIG;
