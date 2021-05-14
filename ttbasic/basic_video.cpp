// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2021 Ulrich Hecht

#include "basic.h"
#include "eb_conio.h"
#include "eb_video.h"

#include "tTVscreen.h"
tTVscreen sc0;

#include <fonts.h>

bool screen_putch_disable_escape_codes = false;
int screen_putch_paging_counter = -1;

static inline bool is_hex(utf8_int32_t c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static void put_hex_digit(uint32_t hex_value, uint8_t hex_type, bool lazy)
{
  if (hex_type == 0)
    sc0.setColor(sc0.getFgColor(), csp.fromIndexed(hex_value));
  else if (hex_type == 1)
    sc0.setColor(csp.fromIndexed(hex_value), sc0.getBgColor());
  else
    sc0.putch(hex_value, lazy);
}

// XXX: 168 byte jump table
void BASIC_INT NOJUMP screen_putch(utf8_int32_t c, bool lazy) {
  static bool escape = false;
  static int predef_color = -1;
  static uint8_t hex_digit = 0, hex_type, hex_max_len;
  static ipixel_t hex_value;
  static bool reverse = false;

  if (kb.state(PS2KEY_L_Alt)) {
    sc0.peekKey();
    delay(3);
  }

  if (screen_putch_disable_escape_codes) {
    sc0.putch(c, lazy);
    return;
  }

  if (c == '\\') {
    if (!escape) {
      escape = true;
      if (hex_digit) {
        put_hex_digit(hex_value, hex_type, lazy);
        hex_digit = 0;
      }
      return;
    }
    sc0.putch('\\');
  } else {
    if (predef_color != -1) {
      int color = 0;

      switch (c) {
      case 'b':	color = COL(BG); break;
      case 'f':	color = COL(FG); break;
      case 'k':	color = COL(KEYWORD); break;
      case 'l':	color = COL(LINENUM); break;
      case 'n':	color = COL(NUM); break;
      case 'v':	color = COL(VAR); break;
      case 'L':	color = COL(LVAR); break;
      case 'o':	color = COL(OP); break;
      case 's':	color = COL(STR); break;
      case 'p':	color = COL(PROC); break;
      case 'c':	color = COL(COMMENT); break;
      case 'B':	color = COL(BORDER); break;
      default: break;
      }

      if (predef_color == 'F')
        sc0.setColor(color, sc0.getBgColor());
      else
        sc0.setColor(sc0.getFgColor(), color);

      predef_color = -1;
      goto out;
    } else if (hex_digit > 0) {
      if (hex_digit == 1)
        hex_value = 0;
      if (is_hex(c) && hex_digit <= hex_max_len) {
        hex_value = (hex_value << 4) | hex2value(c);
        hex_digit++;
        goto out;
      }
      if (!is_hex(c) || hex_digit > hex_max_len) {
        put_hex_digit(hex_value, hex_type, lazy);
        hex_digit = 0;
        if (is_hex(c) && hex_digit > hex_max_len)
          goto out;
      }
    }

    if (escape) {
      switch (c) {
      case 'R':
        if (!reverse) {
          reverse = true;
          sc0.flipColors();
        }
        break;
      case 'N':
        if (reverse) {
          reverse = false;
          sc0.flipColors();
        }
        break;
      case 'l':
        if (sc0.c_x())
          sc0.locate(sc0.c_x() - 1, sc0.c_y());
        else if (sc0.c_y() > 0)
          sc0.locate(sc0.getWidth() - 1, sc0.c_y() - 1);
        break;
      case 'r':
        if (sc0.c_x() < sc0.getWidth() - 1)
          sc0.locate(sc0.c_x() + 1, sc0.c_y());
        else if (sc0.c_y() < sc0.getHeight() - 1)
          sc0.locate(0, sc0.c_y() + 1);
        break;
      case 'u':
        if (sc0.c_y() > 0)
          sc0.locate(sc0.c_x(), sc0.c_y() - 1);
        break;
      case 'd':
        if (sc0.c_y() < sc0.getHeight() - 1)
          sc0.locate(sc0.c_x(), sc0.c_y() + 1);
        break;
      case 'c': sc0.cls();  // fallthrough
      case 'h': sc0.locate(0, 0); break;
      case 'f':
        hex_digit = 1;
        hex_type = 1;
        hex_max_len = csp.getColorSpace() < 2 ? 2 : sizeof(ipixel_t) * 2;
        break;
      case 'F':
      case 'B':
        predef_color = c;
        break;
      case 'b':
        hex_digit = 1;
        hex_type = 0;
        hex_max_len = csp.getColorSpace() < 2 ? 2 : sizeof(ipixel_t) * 2;
        break;
      case 'x':
        hex_digit = 1;
        hex_type = 2;
        hex_max_len = 6;
        break;
      default:
        sc0.putch(c, lazy);
        break;
      }
    } else {
      switch (c) {
      case '\n': {
          if (screen_putch_paging_counter != -1) {
            // XXX: This fails to take wraparound into account. (Not a problem
            // for the (currently) only user (HELP), which does word-wrapping
            // manually.)
            if (screen_putch_paging_counter++ >= sc0.getHeight() - 2) {
              newline();
              c_puts("\\RPress any key to continue\\N");
              while (!c_getch()) {
                yield();
              }
              c_puts("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
              screen_putch_paging_counter = 0;
            }
          }
          newline();
        }
        break;
      case '\r': sc0.locate(0, sc0.c_y()); break;
      case '\b':
        if (sc0.c_x() > 0) {
          sc0.locate(sc0.c_x() - 1);
          sc0.putch(' ');
          sc0.locate(sc0.c_x() - 1);
        }
        break;
      case '\t': {
        int skip = 8 - sc0.c_x() % 8;
        if (skip == 0)
          skip = 8;
        for (int i = 0; i < skip; ++i)
          sc0.putch(' ');
        }
        break;
      default: sc0.putch(c, lazy); break;
      }
    }
  }
out:
  escape = false;
}

/***bc scr WINDOW
Sets the text screen window.

`WINDOW OFF` resets the text window to cover the entire screen.
\usage
WINDOW x, y, w, h

WINDOW OFF
\args
@x	left boundary of the text window, in characters +
        [`0` to `CSIZE(0)-1`]
@y	top boundary of the text window, in characters +
        [`0` to `CSIZE(1)-1`]
@w	width of the text window, in characters +
        [`1` to `CSIZE(0)-x`]
@h	height of the text window, in characters +
        [`1` to `CSIZE(1)-y`]
\ref CSIZE()
***/
void BASIC_INT Basic::iwindow() {
  int32_t x, y, w, h;

#ifdef USE_BG_ENGINE
  // Discard the dimensions saved for CONTing.
  restore_text_window = false;
#endif

  if (*cip == I_OFF) {
    ++cip;
    eb_window_off();
    return;
  }

  if (getParam(x, I_COMMA))
    return;
  if (getParam(y, I_COMMA))
    return;
  if (getParam(w, I_COMMA))
    return;
  if (getParam(h, I_NONE))
    return;

  eb_window(x, y, w, h);
}

/***bc scr FONT
Sets the current text font.
\usage FONT font_num
\args
@font_num	font number [`0` to `{NUM_FONTS_m1}`]
\sec FONTS
The following fonts are available:
\table
| 0 | ATI console font, 6x8 pixels (default)
| 1 | CPC font, 8x8 pixels
| 2 | PETSCII font, 8x8 pixels
| 3 | Japanese font, 6x8 pixels
\endtable
\note
The font set at power-on can be set using the `CONFIG` command.
\ref CONFIG
***/
void Basic::ifont() {
  int32_t idx;
  if (getParam(idx, 0, eb_font_count() - 1, I_NONE))
    return;

  eb_font(idx);
}

/***bc scr SCREEN
Change the screen resolution.
\usage SCREEN mode
\args
@mode screen mode [`1` to `10`]
// XXX: No convenient way to auto-update the number of modes.
\sec MODES
The following modes are available:
\table header
| Mode | Resolution | Comment
| 1 | 460x224 | Maximum usable resolution in NTSC mode.
| 2 | 436x216 | Slightly smaller than mode 1; helpful if
                the display does not show the entirety of mode 1.
| 3 | 320x216 |
| 4 | 320x200 | Common resolution on many home computer systems.
| 5 | 256x224 | Compatible with the SNES system.
| 6 | 256x192 | Common resolution used by MSX, ZX Spectrum and others.
| 7 | 160x200 | Common low-resolution mode on several home computer systems.
| 8 | 352x240 | PC Engine-compatible overscan mode
| 9 | 282x240 | PC Engine-compatible overscan mode
| 10 | 508x240 | Maximum usable resolution in PAL mode. (Overscan on NTSC
                 systems.)
\endtable
\note
* While the resolutions are the same for NTSC and PAL configurations, the
  actual sizes and aspect ratios vary depending on the TV system.

* Resolutions have been chosen to match those used in other systems to
  facilitate porting of existing programs.

* Apart from changing the mode, `SCREEN` clears the screen, resets the font
  and color space (palette) to their defaults, and disables sprites and
  tiled backgrounds.
\ref BG CLS FONT PALETTE SPRITE
***/
void SMALL Basic::iscreen() {
  int32_t m;

  if (getParam(m, I_NONE))
    return;

  eb_screen(m);
}

/***bc scr PALETTE
Sets the color space ("palette") for the current screen mode, as well as
coefficients for the color conversion function `RGB()`.
\usage PALETTE pal[, hw, sw, vw, f]
\args
@pal	colorspace number [`0` or `1`]
@hw	hue weight for color conversion [`0` to `7`]
@sw	saturation weight for color conversion [`0` to `7`]
@vw	value weight for color conversion [`0` to `7`]
@f	conversion fix-ups enabled [`0` or `1`]
\note
The default component weights depend on the color space. They are:
\table header
|Color space | H | S | V
| 0 | 7 | 3 | 6
| 1 | 7 | 4 | 7
\endtable

Conversion fix-ups are enabled by default.
\ref RGB() SCREEN
***/
void Basic::ipalette() {
  int32_t p, hw = -1, sw = -1, vw = -1, f = -1;
  if (getParam(p, 0, CSP_NUM_COLORSPACES - 1, I_NONE))
    return;

  if (*cip == I_COMMA) {
    cip++;
    if (getParam(hw, I_COMMA))
      return;
    if (getParam(sw, I_COMMA))
      return;
    if (getParam(vw, I_COMMA))
      return;
    if (getParam(f, 0, 1, I_NONE))
      return;
  }

  eb_palette(p, hw, sw, vw, !!f);
}

/***bc scr BORDER
Sets the color of the screen border.

The color can be set individually for each screen column.
\usage BORDER uv, y[, x, w]
\args
@uv	UV component of the border color [`0` to `255`]
@y	Y component of the border color [`0` to `255`]
@x	first column to set [default: `0`]
@w	number of columns to set [default: all]
\note
There is no direct correspondence between border and screen color values.
\bugs
There is no way to find the allowed range of values for `x` and `w` without
trial and error.
***/
void Basic::iborder() {
  int32_t y, uv, x = -1, w = -1;
  if (getParam(uv, I_COMMA))
    return;
  if (getParam(y, I_NONE))
    return;
  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(x, I_COMMA))
      return;
    if (getParam(w, I_NONE))
      return;
  }

  eb_border(y, uv, x, w);
}

/***bc scr VSYNC
Synchronize with video output.
\desc
Waits until the video frame counter has reached or exceeded a given
value, or until the next frame if none is specified.
\usage VSYNC [frame]
\args
@frame	frame number to be waited for
\note
Specifying a frame number helps avoid slowdowns by continuing immediately
if the program is "late", and not wait for another frame.
====
----
f = FRAME()
DO
  ' stuff that may take longer than expected
  VSYNC f+1	// <1>
  f = f + 1
LOOP
----
<1> This command will not wait if frame `f+1` has already passed.
====
\ref FRAME()
***/
void Basic::ivsync() {
  uint32_t tm;
  if (end_of_statement())
    tm = 0;
  else if (getParam(tm, 0, INT32_MAX, I_NONE))
    return;

  eb_vsync(tm);
}

static const uint8_t vs23_write_regs[] PROGMEM = {
  0x01, 0x82, 0xb8, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x34, 0x35, 0x36
};

/***bc scr VREG
Writes to a video controller register.

WARNING: Incorrect use of this command can configure the video controller to
produce invalid output, with may cause damage to older CRT displays.
\usage VREG reg[, val[, val ...]]
\args
@reg	video controller register number
@val	value [`0` to `65535`]
\note
Valid registers numbers are: `$01`, `$28` to `$30`, `$34` to `$36`, `$82` and `$B8`

The number of values required depends on the register accessed:
\table
| `$34`, `$35` | 3 values
| `$36` | no value
| `$30` | 2 values
| all others | 1 value
\endtable
\ref VREG()
***/
void BASIC_INT Basic::ivreg() {
#ifdef USE_VS23
  int32_t opcode;
  int vals;

  if (getParam(opcode, 1, 255, I_NONE))
    return;

  for (uint i = 0; i <= sizeof(vs23_write_regs); i++) {
    if (i == sizeof(vs23_write_regs)) {  // out of data
      err = ERR_VALUE;
      return;
    }
    if (pgm_read_byte(&vs23_write_regs[i]) == opcode)
      break;
  }

  if (opcode != BLOCKMV_S && *cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }

  switch (opcode) {
  case BLOCKMVC1:
  case BLOCKMVC2:   vals = 3; break;
  case BLOCKMV_S:   vals = 0; break;
  case PROGRAM:     vals = 2; break;
  default:          vals = 1; break;
  }

  int32_t values[3];    // max size
  for (int i = 0; i < vals; ++i) {
    if (getParam(values[i], 0, 65535, i == vals - 1 ? I_NONE : I_COMMA))
      return;
  }

  switch (opcode) {
  case BLOCKMVC1:       SpiRamWriteBMCtrl(BLOCKMVC1, values[0], values[1], values[2]); break;
  case BLOCKMVC2:       SpiRamWriteBM2Ctrl(values[0], values[1], values[2]); break;
  case BLOCKMV_S:       vs23.startBlockMove(); break;
  case PROGRAM:         SpiRamWriteProgram(PROGRAM, values[0], values[1]); break;
  case WRITE_GPIO_CTRL:
  case WRITE_MULTIIC:
  case WRITE_STATUS:    SpiRamWriteByteRegister(opcode, values[0]); break;
  default:              SpiRamWriteRegister(opcode, values[0]); break;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bf scr FRAME
Returns the number of video frames since power-on.
\usage fr = FRAME()
\ret Frame count.
\ref VSYNC
***/
num_t BASIC_FP Basic::nframe() {
  if (checkOpen() || checkClose())
    return 0;
  return eb_frame();
}

/***bf scr VREG
Read a video controller register.
\usage v = VREG(reg)
\args
@reg	video controller register number
\ret Register contents.
\note
Valid register numbers are: `$05`, `$53`, `$84`, `$86`, `$9F` and `$B7`.
\ref VREG
***/
num_t BASIC_INT Basic::nvreg() {
#ifdef USE_VS23
  uint32_t opcode = getparam();
  switch (opcode) {
  case 0x05:
  case 0x84:
  case 0x86:
  case 0xB7: return SpiRamReadRegister8(opcode);
  case 0x53:
  case 0x9F: return SpiRamReadRegister(opcode);
  default:   err = ERR_VALUE; return 0;
  }
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf scr VPEEK
Read a location in video memory.
\usage v = VPEEK(video_addr)
\args
@video_addr	Byte address in video memory. [`0` to `131071`]
\ret Memory content.
\ref VPOKE VREG()
***/
num_t BASIC_FP Basic::nvpeek() {
#ifdef USE_VS23
  num_t addr = getparam();
  if (addr < 0 || addr > 131071) {
    E_VALUE(0, 131071);
    return 0;
  }
  return SpiRamReadByte(addr);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bc scr VPOKE
Write data to video memory.

WARNING: Incorrect use of this command can configure the video controller to
produce invalid output, with may cause damage to older CRT displays.
\usage VPOKE addr, val
\args
@addr	video memory address [`0` to `131071`]
@val	numeric value [`0` to `255`] or string expression
\ref VPEEK()
***/
void BASIC_INT Basic::ivpoke() {
#ifdef USE_VS23
  int32_t addr, value;
  if (getParam(addr, 0, 131071, I_COMMA))
    return;
  if (is_strexp()) {
    BString str = istrexp();
    if ((value = str.length()))
      SpiRamWriteBytes(addr, (uint8_t *)str.c_str(), value);
    return;
  }
  if (getParam(value, 0, 255, I_NONE))
    return;
  SpiRamWriteByte(addr, value);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc scr LOCATE
Moves the text cursor to the specified position.
\usage LOCATE x_pos, y_pos
\args
@x_pos	cursor X coordinate +
        [`0` to `CSIZE(0)-1`]
@y_pos	cursor Y coordinate +
        [`0` to `CSIZE(1)-1`]
\ref CSIZE()
***/
void Basic::ilocate() {
  int32_t x, y;
  if (getParam(x, I_COMMA))
    return;
  if (getParam(y, I_NONE))
    return;

  eb_locate(x, y);
}

/***bf scr RGB
Converts RGB value to hardware color value.
\desc
All Engine BASIC commands that require a color value expect that value
to be in the video controller's format, which depends on the <<PALETTE>>
setting.

RGB() can be used to calculate hardware color values from the more easily
understood RGB format, and can also help with programs that need to work
with different color palettes.
\usage color = RGB(red, green, blue)
\args
@red	red component [0 to 255]
@green	green component [0 to 255]
@blue	blue component [0 to 255]
\ret YUV color value
\note
The color conversion method used is optimized for use with pixel art.
Its results can be tweaked by setting conversion coefficients with
the `PALETTE` command.

Component values that are out of range will be silently clamped.
\ref PALETTE COLOR
***/
num_t BASIC_FP Basic::nrgb() {
  int32_t r, g, b;
  if (checkOpen() ||
      getParam(r, I_COMMA) ||
      getParam(g, I_COMMA) ||
      getParam(b, I_CLOSE)) {
    return 0;
  }
  return eb_rgb_indexed(r, g, b);
}

/***bc scr COLOR
Changes the foreground and background color for text output.
\usage COLOR fg_color[, bg_color [, cursor_color]]
\args
@fg_color	foreground color [range depends on color space]
@bg_color	background color [range depends on color space, default: `0`]
@cursor_color	cursor color [range depends on color space]
\ref RGB()
***/
void Basic::icolor() {
  uint32_t fc, bgc = 0;
  // XXX: allow 32-bit colors
  if (getParam(fc, 0, IPIXEL_MAX, I_NONE))
    return;
  if (*cip == I_COMMA) {
    cip++;
    if (getParam(bgc, 0, IPIXEL_MAX, I_NONE))
      return;
    if (*cip == I_COMMA) {
      ++cip;
      int32_t cc;
      if (getParam(cc, 0, IPIXEL_MAX, I_NONE))
        return;
      eb_cursor_color(csp.fromIndexed((ipixel_t)cc));
    }
  }
  eb_color(csp.fromIndexed((ipixel_t)fc), csp.fromIndexed((ipixel_t)bgc));
}

/***bf scr CSIZE
Returns the dimensions of the current screen mode in text characters.
\usage size = CSIZE(dimension)
\args
@dimension	requested screen dimension: +
                `0`: text screen width +
                `1`: text screen height
\ret Size in characters.
\ref FONT SCREEN
***/
num_t BASIC_FP Basic::ncsize() {
  // 画面サイズ定数の参照
  int32_t a = getparam();
  switch (a) {
  case 0:	return eb_csize_width();
  case 1:	return eb_csize_height();
  default:	E_VALUE(0, 1); return 0;
  }
}

/***bf scr PSIZE
Returns the dimensions of the current screen mode in pixels.
\usage size = PSIZE(dimension)
\args
@dimension	requested screen dimension: +
                `0`: screen width +
                `1`: visible screen height +
                `2`: total screen height
\ret Size in pixels.
\ref SCREEN
***/
num_t BASIC_FP Basic::npsize() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return eb_psize_width();
  case 1:	return eb_psize_height();
  case 2:	return eb_psize_lastline();
  default:	E_VALUE(0, 2); return 0;
  }
}

/***bf scr POS
Returns the text cursor position.
\usage position = POS(axis)
\args
@axis	requested axis: +
        `0`: horizontal +
        `1`: vertical
\ret Position in characters.
***/
num_t BASIC_FP Basic::npos() {
  int32_t a = getparam();
  switch (a) {
  case 0:	return sc0.c_x();
  case 1:	return sc0.c_y();
  default:	E_VALUE(0, 1); return 0;
  }
}

/***bf scr CHAR
Returns the text character on screen at a given position.
\usage c = CHAR(x, y)
\args
@x	X coordinate, characters
@y	Y coordinate, characters
\ret Character code.
\note
* If the coordinates exceed the dimensions of the text screen, `0` will be returned.
* The value returned represents the current character in text screen memory. It
  may not represent what is actually visible on screen if it has been manipulated
  using pixel graphics functions, such as `GPRINT`.
\ref CHAR GPRINT
***/
int32_t BASIC_INT Basic::ncharfun() {
  int32_t value; // 値
  int32_t x, y;  // 座標

  if (checkOpen())
    return 0;
  if (getParam(x, I_COMMA))
    return 0;
  if (getParam(y, I_NONE))
    return 0;
  if (checkClose())
    return 0;

  return eb_char_get(x, y);
}

/***bc scr CHAR
Prints a single character to the screen.
\usage CHAR x, y, c
\args
@x	X coordinate, characters [`0` to `CSIZE(0)`]
@y	Y coordinate, characters [`0` to `CSIZE(1)`]
@c	character [`0` to `255`]
\note
This command does nothing if the coordinates are outside the screen limits.
***/
void BASIC_FP Basic::ichar() {
  int32_t x, y, c;
  if (getParam(x, I_COMMA))
    return;
  if (getParam(y, I_COMMA))
    return;
  if (getParam(c, I_NONE))
    return;
  eb_char_set(x, y, c);
}

/***bc scr CSCROLL
Scrolls the text screen contents in the given direction.
\usage CSCROLL x1, y1, x2, y2, direction
\args
@x1	start of the screen region, X coordinate, characters [`0` to `CSIZE(0)-1`]
@y1	start of the screen region, Y coordinate, characters [`0` to `CSIZE(1)-1`]
@x2	end of the screen region, X coordinate, characters [`x1+1` to `CSIZE(0)-1`]
@y2	end of the screen region, Y coordinate, characters [`y1+1` to `CSIZE(1)-1`]
@direction	direction
\sec DIRECTION
Valid values for `direction` are:
\table
| `0` | up
| `1` | down
| `2` | left
| `3` | right
\endtable
\bugs
Colors are lost when scrolling.
\ref BLIT GSCROLL CSIZE()
***/
void Basic::icscroll() {
  int32_t x1, y1, x2, y2, d;
  if (getParam(x1, I_COMMA) || getParam(y1, I_COMMA) ||
      getParam(x2, I_COMMA) || getParam(y2, I_COMMA) ||
      getParam(d, I_NONE))
    return;
  eb_cscroll(x1, y1, x2, y2, d);
}

/***bc pix GSCROLL
Scroll a portion screen contents in the given direction.
\usage GSCROLL x1, y1, x2, y2, direction
\args
@x1	start of the screen region, X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y1	start of the screen region, Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@x2	end of the screen region, X coordinate, pixels [`x1+1` to `PSIZE(0)-1`]
@y2	end of the screen region, Y coordinate, pixels [`y1+1` to `PSIZE(2)-1`]
@direction	direction
\sec DIRECTION
Valid values for `direction` are:
\table
| `0` | up
| `1` | down
| `2` | left
| `3` | right
\endtable
\ref BLIT CSCROLL PSIZE()
***/
void Basic::igscroll() {
  int32_t x1, y1, x2, y2, d;
  if (getParam(x1, I_COMMA) || getParam(y1, I_COMMA) ||
      getParam(x2, I_COMMA) || getParam(y2, I_COMMA) ||
      getParam(d, I_NONE))
    return;
  eb_gscroll(x1, y1, x2, y2, d);
}

/***bf pix POINT
Returns the color of the pixel at the given coordinates.
\usage px = POINT(x, y)
\args
@x	X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y	Y coordinate, pixels [`0` to `PSIZE(2)-1`]
\ret Pixel color [`0` to `255`]
\note
If the coordinates exceed the dimensions of the pixel memory area, `0` will be returned.
***/
num_t BASIC_INT Basic::npoint() {
  int32_t x, y;  // 座標
  if (checkOpen())
    return 0;
  if (getParam(x, I_COMMA))
    return 0;
  if (getParam(y, I_NONE))
    return 0;
  if (checkClose())
    return 0;
  return eb_point(x, y);
}

/***bc pix GPRINT
Prints a string of characters at the specified pixel position.
\desc
`GPRINT` allows printing characters outside the fixed text grid, including
off-screen pixel memory.
\usage GPRINT x, y, *expressions*
\args
@x	start point, X coordinate, pixels [`0` to `PSIZE(0)-1`]
@y	start point, Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@expressions	`PRINT`-format list of expressions specifying what to draw
\note
* The expression list functions as it does in the `PRINT` command.
* The "new line" character that is generally appended at the end of a `PRINT`
  command is drawn literally by `GPRINT`, showing up as a funny character
  at the end of the drawn text. This can be avoided by appending a semicolon
  (`;`) to the print parameters.
\ref PRINT
***/
void Basic::igprint() {
  int32_t x, y;
  if (getParam(x, 0, sc0.getGWidth() - 1, I_COMMA))
    return;
  if (getParam(y, 0, vs23.lastLine() - 1, I_COMMA))
    return;
  sc0.set_gcursor(x, y);
  iprint(2);
}

/***bc pix PSET
Draws a pixel.
\usage PSET x_coord, y_coord, color
\args
@x_coord X coordinate of the pixel
@y_coord Y coordinate of the pixel
@color	 color of the pixel [range depends on color space]
\ref RGB()
***/
void GROUP(basic_video) Basic::ipset() {
  int32_t x, y;
  ipixel_t c;
  if (getParam(x, I_COMMA) || getParam(y, I_COMMA) || getParam(c, I_NONE))
    return;

  c = csp.fromIndexed(c);
  eb_pset(x, y, c);
}

/***bc pix LINE
Draws a line.
\usage
LINE x1_coord, y1_coord, x2_coord, y2_coord[, color]
\args
@x1_coord X coordinate of the line's starting point +
          [`0` to `PSIZE(0)-1`]
@y1_coord Y coordinate of the line's starting point +
          [`0` to `PSIZE(2)-1`]
@x2_coord X coordinate of the line's end point +
          [`0` to `PSIZE(0)-1`]
@y2_coord Y coordinate of the line's end point +
          [`0` to `PSIZE(2)-1`]
@color	  color of the line [default: text foreground color]
\bugs
Coordinates that exceed the valid pixel memory area will be clamped.
This can change the slope of the line and is not the behavior one would
expect.
\ref PSIZE() RGB()
***/
void GROUP(basic_video) Basic::iline() {
  int32_t x1, x2, y1, y2;
  ipixel_t c;

  if (getParam(x1, I_COMMA) || getParam(y1, I_COMMA) ||
      getParam(x2, I_COMMA) || getParam(y2, I_NONE))
    return;

  if (*cip == I_COMMA) {
    ++cip;
    getParam(c, I_NONE);
    c = csp.fromIndexed(c);
  } else
    c = -1;
  eb_line(x1, y1, x2, y2, c);
}

/***bc pix CIRCLE
Draws a circle.

\usage CIRCLE x_coord, y_coord, radius, color, fill_color

\args
@x_coord	X coordinate of the circle's center +
                [`0` to `PSIZE(0)-1`]
@y_coord	Y coordinate of the circle's center +
                [`0` to `PSIZE(2)-1`]
@radius		circle's radius
@color		color of the circle's outline [range depends on color space]
@fill_color	color of the circle's body +
                [range depends on color space, `-1` for an unfilled circle]
\bugs
* `fill_color` cannot be omitted.
\ref PSIZE() RGB()
***/
void GROUP(basic_video) Basic::icircle() {
  int32_t x, y, r;
  ipixel_t c, f;
  if (getParam(x, I_COMMA) || getParam(y, I_COMMA) || getParam(r, I_COMMA) ||
      getParam(c, I_COMMA) || getParam(f, I_NONE))
    return;

  c = csp.fromIndexed(c);
  if (f != (ipixel_t)-1)
    f = csp.fromIndexed(f);

  eb_circle(x, y, r, c, f);
}

/***bc pix RECT
Draws a rectangle.
\usage
RECT x1_coord, y1_coord, x2_coord, y2_coord, color, fill_color
\args
@x1_coord X coordinate of the rectangle's top/left corner +
          [`0` to `PSIZE(0)-1`]
@y1_coord Y coordinate of the rectangle's top/left corner +
          [`0` to `PSIZE(2)-1`]
@x2_coord X coordinate of the rectangle's bottom/right corner +
          [`0` to `PSIZE(0)-1`]
@y2_coord Y coordinate of the rectangle's bottom/right corner +
          [`0` to `PSIZE(2)-1`]
@color	  color of the rectangle's outline
@fill_color color of the rectangle's body +
            [range depends on color space, `-1` for an unfilled rectangle]
\bugs
* `fill_color` cannot be omitted.
\ref PSIZE() RGB()
***/
void GROUP(basic_video) Basic::irect() {
  int32_t x1, y1, x2, y2;
  ipixel_t c, f;
  if (getParam(x1, I_COMMA) || getParam(y1, I_COMMA) ||
      getParam(x2, I_COMMA) || getParam(y2, I_COMMA) ||
      getParam(c, I_COMMA) || getParam(f, I_NONE))
    return;

  c = csp.fromIndexed(c);
  if (f != (ipixel_t)-1)
    f = csp.fromIndexed(f);

  eb_rect(x1, y1, x2, y2, c, f);
}

/***bc pix BLIT
Copies a rectangular area of pixel memory to another area.
\usage BLIT x, y TO dest_x, dest_y SIZE width, height [ALPHA]
\args
@x	source area, X coordinate [`0` to `PSIZE(0)-1`]
@y	source area, Y coordinate [`0` to `PSIZE(2)-1`]
@dest_x	destination area, X coordinate [`0` to `PSIZE(0)-1`]
@dest_y	destination area, Y coordinate [`0` to `PSIZE(2)-1`]
@width	area width [`0` to `PSIZE(0)-x`]
@height	area height [`0` to `PSIZE(2)-y`]
\note
By default an opaque blit is performed, ignoring the alpha channel of the
source area. To enable alpha-blending, `ALPHA` must be appended at the end.
\ref GSCROLL
***/
void GROUP(basic_video) Basic::iblit() {
  int32_t x, y, w, h, dx, dy;

  if (getParam(x, I_COMMA) ||
      getParam(y, I_TO) ||
      getParam(dx, I_COMMA) ||
      getParam(dy, I_SIZE) ||
      getParam(w, I_COMMA) ||
      getParam(h, I_NONE))
    return;

  if (*cip == I_ALPHA) {
    ++cip;
    eb_blit_alpha(x, y, dx, dy, w, h);
  } else
    eb_blit(x, y, dx, dy, w, h);
}
