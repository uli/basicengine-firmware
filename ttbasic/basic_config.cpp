// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "basic.h"

SystemConfig CONFIG;

static const uint8_t default_color_scheme[CONFIG_COLS][3] PROGMEM = {
  {   0,   0, 170 },	// C=64 BG
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
  {   0, 136, 255 },	// C=64 BORDER
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
@option	configuration option to be set [`0` to `11`]
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
  `0` (Japanese), `1` (US English, default), `2` (German) and `3` (French).

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
* '9': do not require separation of keywords to allow more permissive variable
  declaration formats.  Especailly useful when loading another dialect's
  program listing

\note
To restore the default configuration, run the command `REMOVE
"/flash/.config"` and restart the system.
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
  int32_t value;

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
  case 1:  // Keyboard correction
    if (value < 0 || value > 3) {
      E_VALUE(0, 3);
    } else {
#if !defined(HOSTED) && !defined(H3) && !defined(__DJGPP__) && !defined(SDL)
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
    if (value < 0 || value >= (1 << sizeof(pixel_t) * 8))
      E_VALUE(0, (1 << sizeof(pixel_t) * 8) - 1);
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
  default:
    E_VALUE(0, 9);
    break;
  }
}

void iloadconfig() {
  loadConfig();
}

#ifdef SDL
#define CONFIG_FILE (BString(getenv("ENGINEBASIC_ROOT")) + BString("/.config"))
#else
#define CONFIG_FILE BString(F("/flash/.config"))
#endif

// Loading system preferences
void loadConfig() {
  CONFIG.NTSC = 0;
  CONFIG.line_adjust = 0;
  CONFIG.KEYBOARD = 1;
  memcpy_P(CONFIG.color_scheme, default_color_scheme,
           sizeof(CONFIG.color_scheme));
  CONFIG.mode = SC_DEFAULT + 1;
  CONFIG.font = 0;
  CONFIG.keyword_sep_optional = false;

  // XXX: colorspace is not initialized yet, cannot use conversion methods
  if (sizeof(pixel_t) == 1)
    CONFIG.cursor_color = (pixel_t)0x92;
  else
    CONFIG.cursor_color = (pixel_t)0x00009500UL;

  CONFIG.beep_volume = 15;


  FILE *f = fopen(CONFIG_FILE.c_str(), "r");
  if (!f)
    return;
  fread((char *)&CONFIG, 1, sizeof(CONFIG), f);
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
  fwrite((char *)&CONFIG, 1, sizeof(CONFIG), f);
  fclose(f);
}
