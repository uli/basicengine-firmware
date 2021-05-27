// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "basic.h"
#include "eb_conio.h"
#include <fonts.h>

SystemConfig CONFIG;

static const uint8_t default_color_scheme[CONFIG_COLS][3] PROGMEM = {
  {   0,   0,   0 },	// BG
  { 192, 192, 192 },	// FG
  { 255, 255, 255 },	// KEYWORD
  { 128, 128, 128 },	// LINENUM
  {  10, 120, 160 },	// NUM (teal)
  { 140, 140, 140 },	// VAR (light gray)
  { 244, 233, 123 },	// LVAR (beige)
  { 214,  91, 189 },	// OP (pink)
  {  50,  50, 255 },	// STR (blue)
  { 238, 137,  17 },	// PROC (orange)
  {  84, 255,   0 },	// COMMENT (green)
  {   0,   0,   0 },	// BORDER
};

/***bc sys CONFIG COLOR
Changes the color scheme.
\desc
The color scheme is a set of colors that are used to print system messages
and BASIC program listings. It also contains the default foreground and
background colors.
\usage CONFIG COLOR col, red, green, blue
\args
@col	color code [`0` to `{CONFIG_COLS_m1}`]
@red	red component [0 to 255]
@green	green component [0 to 255]
@blue	blue component [0 to 255]
\sec COLOR CODES
\table
| 0 | Default background
| 1 | Default foreground
| 2 | Syntax: BASIC keywords
| 3 | Syntax: line numbers
| 4 | Syntax: numbers
| 5 | Syntax: global variables
| 6 | Syntax: local variables and arguments
| 7 | Syntax: operands
| 8 | Syntax: string constants
| 9 | Syntax: procedure names
| 10 | Syntax: comments
| 11 | Default border color
\endtable
\note
* Unlike the `COLOR` command, `CONFIG COLOR` requires the colors to be given
  in RGB format; this is to ensure that the color scheme works with all YUV
  colorspaces.
* To set the current color scheme as the default, use `SAVE CONFIG`.
\bugs
The default border color is not used.
\ref COLOR CONFIG SAVE_CONFIG
***/
void SMALL Basic::config_color() {
  int32_t idx, r, g, b;
  if (getParam(idx, 0, CONFIG_COLS - 1, I_COMMA))
    return;
  if (getParam(r, 0, 255, I_COMMA))
    return;
  if (getParam(g, 0, 255, I_COMMA))
    return;
  if (getParam(b, 0, 255, I_NONE))
    return;
  CONFIG.color_scheme[idx][0] = r;
  CONFIG.color_scheme[idx][1] = g;
  CONFIG.color_scheme[idx][2] = b;
}

/***bc sys CONFIG
Changes configuration options.
\desc
The options will be reset to their defaults on system startup unless they
have been saved using <<SAVE CONFIG>>. Changing power-on default options does not
affect the system until the options are saved and the system is restarted.
\usage CONFIG option, value
\args
@option	configuration option to be set [`0` to `8`]
@value	option value
\sec OPTIONS
The following options exist:

* `0`: TV norm +
  Sets the TV norm to NTSC (`0`), PAL (`1`) or PAL60 (`2`).
+
WARNING: This configuration option does not make much sense; the available
TV norm depends on the installed color burst crystal and is automatically
detected; PAL60 mode is not currently implemented. +
The semantics of this option are therefore likely to change in the future.

* `1`: Keyboard layout +
  Four different keyboard layouts are supported: +
  `0` (Japanese), `1` (US English, default), `2` (German), `3` (French)
  and `4` (Spanish).

* `2`: Interlacing +
  Sets the video output to progressive (`0`) or interlaced (`1`). A change
  in this option will become effective on the next screen mode change.
+
WARNING: The intention of this option is to provide an interlaced signal
to TVs that do not handle a progressive signal well. So far, no displays
have turned up that require it, so it may be removed and/or replaced
with a different option in future releases.

* `3`: Luminance low-pass filter +
  This option enables (`1`) or disables (`0`, default) the video luminance
  low-pass filter. The recommended setting depends on the properties of the
  display device used.
+
Many recent TVs are able to handle high resolutions well; with such
devices, it is recommended to turn the low-pass filter off to achieve
a more crisp display.
+
On other (usually older) TVs, high-resolution images may cause excessive
color artifacting (a "rainbow" effect) or flicker; with such devices,
it is recommended to turn the low-pass filter on to reduce these effects,
at the expense of sharpness.

* `4`: Power-on screen mode [`1` (default) to `10`] +
  Sets the screen mode the system defaults to at power-on.

* `5`: Power-on screen font [`0` (default) to `{NUM_FONTS_m1}`] +
  Sets the screen font the system defaults to at power-on.

* `6`: Power-on cursor color [`0` to `255`] +
  Sets the cursor color the system defaults to at power-on.

* `7`: Beeper sound volume [`0` to `15` (default)] +
  Sets the volume of the "beeper" sound engine, which applies, among
  other things, to the start-up jingle.

* `8`: Screen line adjustment [`-128` to `128`, default: `0`]
  Adjusts the number of screen lines. A positive value adds lines, a negative
  value subtracts them. +
  This option may be useful to mitigate issues with color artifacting and
  flicker on some devices.
+
WARNING: It is not clear if this option is useful in practice, and it may be
removed in future releases.

* `9`: Keyword separation optional [`0` or `1` (default)] +
  Controls if BASIC keywords need to be separated from other language
  elements containing alphanumeric characters with a space (`1`), or if they
  can be adjacent to those like in traditional BASIC dialects (`0`).

* `10`: Physical screen mode (H3 platform only) [`0` (default) to `6`] +
  Sets the physical screen resolution: +
  `0` (1920x1080 HDMI), `1` (1280x1024 DVI), `2` (1280x720 HDMI), `3` +
  (1024x768 DVI), `4` (800x600 DVI), `5` (640x480 DVI) or `6` +
  (1920x1200 HDMI). +
  It is necessary to save the configuration and reboot the system for
  this option to take effect.

* `11`: Language +
  The following languages are supported for system messages: +
  `0` (US English, default), `1` (German), `2` (French), `3` (Spanish).

\note
To restore the default configuration, run the command `REMOVE
"/sd/config.ini"` and restart the system.
\bugs
* Changing the low-pass filter option (`3`) only takes effect at the time
  of the next block move, which happens for instance when scrolling the
  screen.
* There is no way to override a saved configuration, which may make it
  difficult or even impossible to fix an system configured to an unusable
  state.
\ref BEEP FONT SAVE_CONFIG SCREEN
***/
void SMALL Basic::iconfig() {
  int32_t itemNo;
  num_t value;

  if (*cip == I_COLOR) {
    ++cip;
    config_color();
    return;
  }

  if (getParam(itemNo, I_COMMA))
    return;
  if (getParam(value, I_NONE))
    return;
  switch (itemNo) {
  case 0:  // NTSC, PAL, PAL60 (XXX: unimplemented)
    if (value < 0 || value > 2) {
      E_VALUE(0, 2);
    } else {
      CONFIG.NTSC = value;
    }
    break;
  case 1:  // キーボード補正
    if (value < 0 || value > 4) {
      E_VALUE(0, 4);
    } else {
#if !defined(HOSTED) && !defined(__DJGPP__) && !defined(SDL)
      kb.setLayout(value);
#endif
      CONFIG.KEYBOARD = value;
    }
    break;
  case 2:
    CONFIG.interlace = value != 0;
    vs23.setInterlace(CONFIG.interlace);
    break;
  case 3:
    CONFIG.lowpass = value != 0;
    vs23.setLowpass(CONFIG.lowpass);
    break;
  case 4:
    if (value < 1 || value > vs23.numModes())
      E_VALUE(1, vs23.numModes());
    else
      CONFIG.mode = value;
    break;
  case 5:
    if (value < 0 || value >= NUM_FONTS)
      E_VALUE(0, NUM_FONTS - 1);
    else
      CONFIG.font = value;
    break;
  case 6:
    if (value < 0 || value >= (1ULL << sizeof(pixel_t) * 8))
      E_VALUE(0, (1ULL << sizeof(pixel_t) * 8) - 1);
    else
      CONFIG.cursor_color = csp.fromIndexed((ipixel_t)value);
    break;
  case 7:
    if (value < 0 || value > 15)
      E_VALUE(0, 15);
    else
      CONFIG.beep_volume = value;
    break;
  case 8:
    if (value < -128 || value > 127)
      E_VALUE(-128, 127);
    else {
      CONFIG.line_adjust = value;
      vs23.setLineAdjust(CONFIG.line_adjust);
    }
    break;
  case 9:
    CONFIG.keyword_sep_optional = value != 0;
    break;
#ifdef H3
  case 10:
    if (value < 0 || value > 6)
      E_VALUE(0, 6);
    else
      CONFIG.phys_mode = value;
    break;
#endif
  case 11:
    if (value < 0 || value > 3)
      E_VALUE(0, 3);
    else
      CONFIG.language = value;
    if (eb_load_lang_resources(CONFIG.language) < 0) {
      err = ERR_FILE_OPEN;
      err_expected = "could not load font files";
      CONFIG.language = 0;
    }
    break;
  default:
    E_VALUE(0, 10);
    break;
  }
}

void iloadconfig() {
  loadConfig();
}

#ifdef SDL
#define CONFIG_FILE (BString(getenv("ENGINEBASIC_ROOT")) + BString("/config.ini"))
#elif defined(H3)
#define CONFIG_FILE BString("/sd/config.ini")
#else
#define CONFIG_FILE BString(F("/flash/config.ini"))
#endif

// システム環境設定のロード
void loadConfig() {
  CONFIG.NTSC = 0;
  CONFIG.line_adjust = 0;
  CONFIG.KEYBOARD = 1;
  memcpy_P(CONFIG.color_scheme, default_color_scheme,
           sizeof(CONFIG.color_scheme));
  CONFIG.mode = SC_DEFAULT + 1;
  CONFIG.font = 0;
  CONFIG.keyword_sep_optional = false;
  CONFIG.language = 0;
#ifdef H3
  CONFIG.phys_mode = 0;
#endif

  // XXX: colorspace is not initialized yet, cannot use conversion methods
  if (sizeof(pixel_t) == 1)
    CONFIG.cursor_color = (pixel_t)0x92;
  else
    CONFIG.cursor_color = (pixel_t)0xff009500UL;

  CONFIG.beep_volume = 15;

  FILE *f = fopen(CONFIG_FILE.c_str(), "r");
  if (!f)
    return;

  char *line = NULL;
  size_t len = 0;
#ifdef H3
  while (__getline(&line, &len, f) != -1) {
#else
  while (getline(&line, &len, f) != -1) {
#endif
    char *v = strrchr(line, '=');
    if (!v)
      continue;
    *v++ = 0;
    if (!strncasecmp(line, "color", 5)) {
      uint32_t idx = atoi(line + 5);
      if (idx >= CONFIG_COLS)
        continue;
      char *comp = strtok(v, ",");
      if (comp) CONFIG.color_scheme[idx][0] = atoi(comp);
      comp = strtok(NULL, ",");
      if (comp) CONFIG.color_scheme[idx][1] = atoi(comp);
      comp = strtok(NULL, ",");
      if (comp) CONFIG.color_scheme[idx][2] = atoi(comp);
    } else {
      if (!strcasecmp(line, "keyboard")) CONFIG.KEYBOARD = atoi(v);
      if (!strcasecmp(line, "mode")) CONFIG.mode = atoi(v);
      if (!strcasecmp(line, "font")) CONFIG.font = atoi(v);
      if (!strcasecmp(line, "keyword_sep_optional")) CONFIG.keyword_sep_optional = !!atoi(v);
      if (!strcasecmp(line, "language")) CONFIG.language = atoi(v);
      if (!strcasecmp(line, "filter")) CONFIG.lowpass = !!atoi(v);
      if (!strcasecmp(line, "cursor_color")) CONFIG.cursor_color = strtoul(v, NULL, 0);
      if (!strcasecmp(line, "beep_volume")) CONFIG.beep_volume = atoi(v);
#ifdef H3
      if (!strcasecmp(line, "phys_mode")) CONFIG.phys_mode = atoi(v);
      if (!strcasecmp(line, "tv_norm")) CONFIG.NTSC = atoi(v);
#endif
    }
  }
  fclose(f);
}

/***bc sys SAVE CONFIG
Saves the current set of configuration options as default.
\usage SAVE CONFIG
\note
The configuration will be saved as a file under the name `/flash/.config`.
\ref CONFIG
***/
void isaveconfig() {
  FILE *f = fopen(CONFIG_FILE.c_str(), "w");
  if (!f) {
    err = ERR_FILE_OPEN;
  }

  fprintf(f, "keyboard=%d\n", CONFIG.KEYBOARD);
  fprintf(f, "mode=%d\n", CONFIG.mode);
  fprintf(f, "font=%d\n", CONFIG.font);
  fprintf(f, "keyword_sep_optional=%d\n", CONFIG.keyword_sep_optional);
  fprintf(f, "language=%d\n", CONFIG.language);
  fprintf(f, "filter=%d\n", CONFIG.lowpass);
  fprintf(f, "cursor_color=0x%x\n", (unsigned int)CONFIG.cursor_color);
  fprintf(f, "beep_volume=%d\n", CONFIG.beep_volume);
#ifdef H3
  fprintf(f, "phys_mode=%u\n", CONFIG.phys_mode);
  fprintf(f, "tv_norm=%u\n", CONFIG.NTSC);
#endif
  for (int i = 0; i < CONFIG_COLS; ++i)
    fprintf(f, "color%d=%d,%d,%d\n", i,
      CONFIG.color_scheme[i][0],
      CONFIG.color_scheme[i][1],
      CONFIG.color_scheme[i][2]);

  fclose(f);
}
