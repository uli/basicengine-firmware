// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht
// Copyright (c) 2023 Ulrich Hecht

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>
#include "eb_config.h"
#include "eb_types.h"

#include "BString.h"

typedef struct {
  int8_t NTSC;         // NTSC設定 (0,1,2,3)
  int8_t line_adjust;  // number of lines to add to default
  int16_t KEYBOARD;    // キーボード設定 (0:JP, 1:US)
  uint8_t color_scheme[CONFIG_COLS][3];
  bool interlace;
  bool lowpass;
  uint8_t mode;
  uint8_t font;
  pixel_t cursor_color;
  uint8_t beep_volume;
  bool keyword_sep_optional;  // do not require separation of keywords
  uint8_t language;
#ifdef H3
  uint32_t phys_mode;
#endif
  bool record_at_boot;
  BString editor;
  BString audio_device;
} SystemConfig;

extern SystemConfig CONFIG;

#endif
