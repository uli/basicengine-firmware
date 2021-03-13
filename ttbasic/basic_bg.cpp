// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "basic.h"
#include "c_bg.h"

// Saved background and text window dimensions.
// These values are set when a program is interrupted and the text window is
// forced to be visible at the bottom of the screen. That way, the original
// geometries can be restored when the program is CONTinued.
#ifdef USE_BG_ENGINE
static int original_bg_height[MAX_BG];
static bool turn_bg_back_on[MAX_BG];
bool restore_bgs = false;
static uint16_t original_text_pos[2];
bool restore_text_window = false;
#endif

uint8_t event_sprite_proc_idx;

#ifdef USE_BG_ENGINE
void BASIC_INT Basic::event_handle_sprite() {
  uint8_t dir;
  for (int i = 0; i < MAX_SPRITES; ++i) {
    if (!c_sprite_enabled(i))
      continue;
    for (int j = i + 1; j < MAX_SPRITES; ++j) {
      if (!c_sprite_enabled(j))
        continue;
      if ((dir = c_sprite_collision(i, j))) {
        init_stack_frame();
        push_num_arg(i);
        push_num_arg(j);
        push_num_arg(dir);
        do_call(event_sprite_proc_idx);
        event_sprite_proc_idx = NO_PROC;  // prevent interrupt storms
        return;
      }
    }
  }
}
#endif

/***bc bg BG
Defines a tiled background's properties.
\desc
Using `BG`, you can define a tiled background's map and tile size, tile set,
window size and position, and priority, as well as turn it on or off.

`BG OFF` turns off all backgrounds.
\usage
BG bg [TILES w, h] [PATTERN px, py, pw] [SIZE tx, ty] [WINDOW wx, wy, ww, wh]
      [PRIO priority] [<ON|OFF>]
BG OFF
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@w	background map width, in tiles
@h	background map height, in tiles
@px	tile set X coordinate, pixels [`0` to `PSIZE(0)-1`]
@py	tile set Y coordinate, pixels [`0` to `PSIZE(2)-1`]
@tx	tile width, pixels [`8` (default) to `32`]
@ty	tile height, pixels [`8` (default) to `32`]
@wx	window X coordinate, pixels [`0` (default) to `PSIZE(0)-1`]
@wy	window Y coordinate, pixels [`0` (default) to `PSIZE(1)-1`]
@ww	window width, pixels [`9` to `PSIZE(0)-wx` (default)]
@wh	window height, pixels [`0` to `PSIZE(1)-wy` (default)]
@priority Background priority [`0` to `{MAX_BG_m1}`, default: `bg`]
\note
* The `BG` command's attributes can be specified in any order, but it is
  usually a good idea to place the `ON` attribute at the end if used.
* A background cannot be turned on unless at least the `TILES` attribute
  is set. In order to get sensible output, it will probably be necessary
  to specify the `PATTERN` attribute as well.
\bugs
If a background cannot be turned on, no error is generated.
\ref LOAD_BG LOAD_PCX MOVE_BG SAVE_BG
***/
void Basic::ibg() {
#ifdef USE_BG_ENGINE
  int32_t m;
  int32_t w, h, px, py, pw, tx, ty, wx, wy, ww, wh, prio;

  if (*cip == I_OFF) {
    ++cip;
    c_bg_off();
    return;
  }

  if (getParam(m, I_NONE))
    return;

  // Background modified, do not restore the saved values when CONTing.
  original_bg_height[m] = -1;
  turn_bg_back_on[m] = false;

  for (;;) switch (*cip++) {
  case I_TILES:
    if (getParam(w, I_COMMA)) return;
    if (getParam(h, I_NONE)) return;
    if (c_bg_set_size(m, w, h))
      return;
    break;
  case I_PATTERN:
    if (getParam(px, I_COMMA)) return;
    if (getParam(py, I_COMMA)) return;
    if (getParam(pw, I_NONE)) return;
    c_bg_set_pattern(m, px, py, pw);
    break;
  case I_SIZE:
    if (getParam(tx, I_COMMA)) return;
    if (getParam(ty, I_NONE)) return;
    c_bg_set_tile_size(m, tx, ty);
    break;
  case I_WINDOW:
    if (getParam(wx, I_COMMA)) return;
    if (getParam(wy, I_COMMA)) return;
    if (getParam(ww, I_COMMA)) return;
    if (getParam(wh, I_NONE)) return;
    c_bg_set_window(m, wx, wy, ww, wh);
    break;
  case I_ON:
    // XXX: do sanity check before enabling
    c_bg_enable(m);
    break;
  case I_OFF:
    c_bg_disable(m);
    break;
  case I_PRIO:
    if (getParam(prio, I_NONE)) return;
    c_bg_set_priority(m, prio);
    break;
  default:
    cip--;
    if (!end_of_statement())
      SYNTAX_T("exp BG parameter");
    return;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg LOAD BG
Loads a background map from storage.
\usage LOAD BG bg, file$
\args
@bg	background number. [`0` to `{MAX_BG_m1}`]
@file$	name of background map file
\ref SAVE_BG
***/
void Basic::iloadbg() {
#ifdef USE_BG_ENGINE
  int32_t bg;
  BString filename;

  cip++;

  if (getParam(bg, I_COMMA))
    return;
  if (!(filename = getParamFname()))
    return;

  c_bg_load(bg, filename.c_str());
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg SAVE BG
Saves a background map to storage.
\usage SAVE BG bg TO file$
\args
@bg	background number. [`0` to `{MAX_BG_m1}`]
@file$	name of background map file
\bugs
Does not check if the specified background is properly defined.
\ref LOAD_BG
***/
void Basic::isavebg() {
#ifdef USE_BG_ENGINE
  int32_t bg;
  BString filename;

  cip++;

  if (getParam(bg, I_TO))
    return;
  if (!(filename = getParamFname()))
    return;

  c_bg_save(bg, filename.c_str());
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg MOVE BG
Scrolls a tiled background.
\usage MOVE BG bg TO pos_x, pos_y
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@pos_x	top/left corner offset horizontal, pixels
@pos_y	top/left corner offset vertical, pixels
\note
There are no restrictions on the coordinates that can be used; backgrounds
maps wrap around if the display window extends beyond the map boundaries.
\ref BG
***/
void BASIC_FP Basic::imovebg() {
#ifdef USE_BG_ENGINE
  int32_t bg, x, y;
  if (getParam(bg, I_TO))
    return;
  if (getParam(x, I_COMMA))
    return;
  if (getParam(y, I_NONE))
    return;

  c_bg_move(bg, x, y);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg SPRITE
Defines a sprite's properties.
\desc
Using `SPRITE`, you can define a sprite's appearance, size, animation frame, color key,
prority, special effects, and turn it on and off.

`SPRITE OFF` turns all sprites off.
\usage
SPRITE num [PATTERN pat_x, pat_y][SIZE w, h][FRAME frame_x, frame_y]
           [FLAGS flags][KEY key][PRIO priority][<ON|OFF>]

SPRITE OFF
\args
@pat_x X 	coordinate of the top/left sprite pattern, pixels
@pat_y Y	coordinate of the top/left sprite pattern, pixels
@w		sprite width, pixels [`0` to `{MAX_SPRITE_W_m1}`]
@h		sprite height, pixels [`0` to `{MAX_SPRITE_H_m1}`]
@frame_x	X coordinate of the pattern section to be used,
                in multiples of sprite width
@frame_y	Y coordinate of the pattern section to be used,
                in multiples of sprite height
@flags		special effect flags [`0` to `7`]
@key		key color value to be used for transparency masking +
                [`0` to `255`]
@priority	sprite priority in relation to background layers +
                [`0` to `{MAX_BG_m1}`]

\sec FLAGS
The `FLAGS` attribute is the sum of any of the following bit values:
\table header
| Bit value | Effect
| `1` | Sprite opacity (opaque if set)
| `2` | Horizontal flip
| `4` | Vertical flip
\endtable
\note
The `SPRITE` command's attributes can be specified in any order, but it is
usually a good idea to place the `ON` attribute at the end if used.
\ref LOAD_PCX MOVE_SPRITE
***/
void BASIC_INT Basic::isprite() {
#ifdef USE_BG_ENGINE
  int32_t num, pat_x, pat_y, w, h, frame_x, frame_y, flags, prio;
  uint32_t key;
  bool set_frame = false, set_opacity = false;

  if (*cip == I_OFF) {
    ++cip;
    c_sprite_off();
    return;
  }

  if (getParam(num, 0, MAX_SPRITES, I_NONE))
    return;

  frame_x = c_sprite_frame_x(num);
  frame_y = c_sprite_frame_y(num);
  flags = (c_sprite_opaque(num) << 0) |
          (c_sprite_flip_x(num) << 1) |
          (c_sprite_flip_y(num) << 2);

  for (;;) switch (*cip++) {
  case I_PATTERN:
    if (getParam(pat_x, I_COMMA)) return;
    if (getParam(pat_y, I_NONE)) return;
    c_sprite_set_pattern(num, pat_x, pat_y);
    break;
  case I_SIZE:
    if (getParam(w, I_COMMA)) return;
    if (getParam(h, I_NONE)) return;
    c_sprite_set_size(num, w, h);
    break;
  case I_FRAME:
    if (getParam(frame_x, I_NONE)) return;
    if (*cip == I_COMMA) {
      ++cip;
      if (getParam(frame_y, I_NONE)) return;
    } else
      frame_y = 0;
    set_frame = true;
    break;
  case I_ON:
    c_sprite_enable(num);
    break;
  case I_OFF:
    c_sprite_disable(num);
    break;
  case I_FLAGS: {
      int32_t new_flags;
      if (getParam(new_flags, 0, 7, I_NONE))
        return;
      if (new_flags != flags) {
        if ((new_flags & 1) != (flags & 1))
          set_opacity = true;
        if ((new_flags & 6) != (flags & 6))
          set_frame = true;
        flags = new_flags;
      }
    }
    break;
  case I_KEY:
    if (getParam(key, I_NONE)) return;
    c_sprite_set_key(num, key);
    break;
  case I_PRIO:
    if (getParam(prio, I_NONE)) return;
    c_sprite_set_priority(num, prio);
    break;
  default:
    // XXX: throw an error if nothing has been done
    cip--;
    if (!end_of_statement())
      SYNTAX_T("exp sprite parameter");
    if (set_frame)
      c_sprite_set_frame(num, frame_x, frame_y, flags & 2, flags & 4);
    if (set_opacity || (set_frame && (flags & 1)))
      c_sprite_set_opacity(num, flags & 1);
    if (c_sprite_reload(num)) {
      if (!err)
        err = ERR_OOM;
    }
    return;
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg MOVE SPRITE
Moves a sprite.
\usage MOVE SPRITE num TO pos_x, pos_y
\args
@num Sprite number [`0` to `{MAX_SPRITES_m1}`]
@pos_x Sprite position X coordinate, pixels
@pos_y Sprite position X coordinate, pixels
\note
There are no restrictions on the coordinates that can be used; sprites
that are placed completely outside the dimensions of the current screen
mode will not be drawn.
\ref SPRITE
***/
void BASIC_FP Basic::imovesprite() {
#ifdef USE_BG_ENGINE
  int32_t num, pos_x, pos_y;
  if (getParam(num, I_TO))
    return;
  if (getParam(pos_x, I_COMMA))
    return;
  if (getParam(pos_y, I_NONE))
    return;
  c_sprite_move(num, pos_x, pos_y);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void BASIC_FP Basic::imove() {
  if (*cip == I_SPRITE) {
    ++cip;
    imovesprite();
  } else if (*cip == I_BG) {
    ++cip;
    imovebg();
  } else
    SYNTAX_T("exp BG or SPRITE");
}

/***bc bg PLOT
Sets the value of one or more background tiles.
\usage PLOT bg, x, y, <tile|tile$>
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@x	tile X coordinate [`0` to background width in tiles]
@y	tile Y coordinate [`0` to background height in tiles]
@tile	tile identifier [`0` to `255`]
@tile$	tile identifiers
\note
If the numeric `tile` argument is given, a single tile will be set.
If a string of tiles (`tile$`) is passed, a tile will be set for each of the
elements of the string, starting from the specified tile coordinates and
proceeding horizontally.
\ref BG PLOT_MAP
***/
/***bc bg PLOT MAP
Creates a mapping from one tile to another.
\desc
Tile numbers depend on the locations of graphics data in memory and do not
usually represent printable characters, often making it difficult to handle
them in program text. By remapping a printable character to a tile as
represented by the graphics data, it is possible to use that human-readable
character in your program, making it easier to write and to understand.
\usage PLOT bg MAP from TO to
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
@from	tile number to remap [`0` to `255`]
@to	tile number to map to [`0` to `255`]
\example
====
----
PLOT 0 MAP ASC("X") TO 1
PLOT 0 MAP ASC("O") TO 3
...
PLOT 0,5,10,"XOOOXXOO"
----
====
\ref PLOT
***/
void Basic::iplot() {
#ifdef USE_BG_ENGINE
  int32_t bg, x, y, t;
  if (getParam(bg, I_NONE))
    return;
  if (!c_bg_get_tiles(bg)) {
    // BG not defined
    err = ERR_RANGE;
    return;
  }
  if (*cip == I_MAP) {
    ++cip;
    if (getParam(x, I_TO))
      return;
    if (getParam(y, I_NONE))
      return;
    c_bg_map_tile(bg, x, y);
  } else if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
  } else {
    if (getParam(x, I_COMMA))
      return;
    if (getParam(y, I_COMMA))
      return;
    if (is_strexp()) {
      BString dat = istrexp();
      c_bg_set_tiles(bg, x, y, (const uint8_t *)dat.c_str(), dat.length());
    } else {
      if (getParam(t, I_NONE))
        return;
      c_bg_set_tile(bg, x, y, t);
    }
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc bg FRAMESKIP
Sets the number of frames that should be skipped before a new frame is rendered
by the BG/sprite engine.

This serves to increase the available CPU power to BASIC by reducing the load
imposed by the graphics subsystem.
\usage FRAMESKIP frm
\args
@frm	number of frames to be skipped [`0` (default) to `60`]
\note
It is not possible to mitigate flickering caused by overloading the graphics
engine using `FRAMESKIP` because each frame that is actually rendered must
be so within a single TV frame.
***/
void Basic::iframeskip() {
#ifdef USE_BG_ENGINE
  int32_t skip;
  if (getParam(skip, I_NONE))
    return;
  c_frameskip(skip);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bf bg TILECOLL
Checks if a sprite has collided with a background tile.
\usage res = TILECOLL(spr, bg, tile)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
@bg	background number [`0` to `{MAX_BG_m1}`]
@tile	tile number [`0` to `255`]
\ret
If the return value is `0`, no collision has taken place. Otherwise
the return value is the sum of `64` and any of the following
bit values:
\table header
| Bit value | Meaning
| `1` (aka `<<LEFT>>`) | The tile is left of the sprite.
| `2` (aka `<<DOWN>>`) | The tile is below the sprite.
| `4` (aka `<<RIGHT>>`) | The tile is right of the sprite.
| `8` (aka `<<UP>>`) | The tile is above the sprite.
\endtable

Note that it is possible that none of these bits are set even if
a collision has taken place (i.e., the return value is `64`) if
the sprite's size is larger than the tile size of the given
background layer, and the sprite covers the tile completely.
\bugs
WARNING: This function has never been tested.

Unlike `SPRCOLL()`, tile collision detection is not pixel-accurate.
\ref SPRCOLL()
***/
num_t BASIC_FP Basic::ntilecoll() {
#ifdef USE_BG_ENGINE
  int32_t a, b, c;
  if (checkOpen())
    return 0;
  if (getParam(a, 0, MAX_SPRITES, I_COMMA))
    return 0;
  if (getParam(b, 0, MAX_BG, I_COMMA))
    return 0;
  if (getParam(c, 0, 255, I_NONE))
    return 0;
  if (checkClose())
    return 0;
  return c_sprite_tile_collision(a, b, c);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf bg SPRCOLL
Checks if a sprite has collided with another sprite.
\usage res = SPRCOLL(spr1, spr2)
\args
@spr1	sprite number 1 [`0` to `{MAX_SPRITES_m1}`]
@spr2	sprite number 2 [`0` to `{MAX_SPRITES_m1}`]
\ret
If the return value is `0`, no collision has taken place. Otherwise
the return value is the sum of `64` and any of the following
bit values:
\table header
| Bit value | Meaning
| `1` (aka `<<LEFT>>`) | Sprite 2 is left of sprite 1.
| `2` (aka `<<DOWN>>`) | Sprite 2 is below sprite 1.
| `4` (aka `<<RIGHT>>`) | Sprite 2 is right of sprite 1.
| `8` (aka `<<UP>>`) | Sprite 2 is above sprite 1.
\endtable

It is possible that none of these bits are set even if
a collision has taken place (i.e., the return value is `64`) if
sprite 1 is larger than sprite 2 and covers it completely.
\note
* Collisions are only detected if both sprites are enabled.
* Sprite/sprite collision detection is pixel-accurate, i.e. a collision is
only detected if visible pixels of two sprites overlap.
\bugs
Does not return an intuitive direction indication if sprite 2 is larger than
sprite 1 and covers it completely. (It will indicate that sprite 2 is
up/left of sprite 1.)
\ref TILECOLL()
***/
num_t BASIC_FP Basic::nsprcoll() {
#ifdef USE_BG_ENGINE
  int32_t a, b;
  if (checkOpen())
    return 0;
  if (getParam(a, I_COMMA))
    return 0;
  if (getParam(b, I_NONE))
    return 0;
  if (checkClose())
    return 0;
  if (c_sprite_enabled(a) && c_sprite_enabled(b))
    return c_sprite_collision(a, b);
  else
    return 0;
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf bg SPRX
Returns the horizontal position of a given sprite.
\usage p = SPRX(spr)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
\ret
X coordinate of sprite `spr`.
\ref MOVE_SPRITE SPRY()
***/
num_t BASIC_FP Basic::nsprx() {
#ifdef USE_BG_ENGINE
  int32_t spr;

  if (checkOpen() || getParam(spr, I_CLOSE))
    return 0;

  return c_sprite_x(spr);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}
/***bf bg SPRY
Returns the vertical position of a given sprite.
\usage p = SPRY(spr)
\args
@spr	sprite number [`0` to `{MAX_SPRITES_m1}`]
\ret
Y coordinate of sprite `spr`.
\ref MOVE_SPRITE SPRX()
***/
num_t BASIC_FP Basic::nspry() {
#ifdef USE_BG_ENGINE
  int32_t spr;

  if (checkOpen() || getParam(spr, I_CLOSE))
    return 0;

  return c_sprite_y(spr);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

/***bf bg BSCRX
Returns the horizontal scrolling offset of a given background layer.
\usage p = BSCRX(bg)
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
\ret
X scrolling offset of background `bg`.
\ref BG BSCRY()
***/
num_t BASIC_FP Basic::nbscrx() {
#ifdef USE_BG_ENGINE
  int32_t bg;

  if (checkOpen() || getParam(bg, I_CLOSE))
    return 0;

  return c_bg_x(bg);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}
/***bf bg BSCRY
Returns the vertical scrolling offset of a given background layer.
\usage p = BSCRY(bg)
\args
@bg	background number [`0` to `{MAX_BG_m1}`]
\ret
Y scrolling offset of background `bg`.
\ref BG BSCRX()
***/
num_t BASIC_FP Basic::nbscry() {
#ifdef USE_BG_ENGINE
  int32_t bg;

  if (checkOpen() || getParam(bg, I_CLOSE))
    return 0;

  return c_bg_y(bg);
#else
  err = ERR_NOT_SUPPORTED;
  return 0;
#endif
}

void SMALL resize_windows() {
#ifdef USE_BG_ENGINE
  int x, y, w, h;
  sc0.getWindow(x, y, w, h);
  restore_bgs = false;
  restore_text_window = false;
  // Default text window?
  if (x == 0 && w == sc0.getScreenWidth() &&
      y == 0 && h == sc0.getScreenHeight()) {
    // If there are backgrounds enabled, they partially or fully obscure
    // the text window. We adjust it to cover the bottom 5 lines only,
    // and clip or disable any backgrounds that overlap the new text
    // window.
    bool obscured = false;
    int top_y = (y + (sc0.getScreenHeight() - 5)) * sc0.getFontHeight();
    for (int i = 0; i < MAX_BG; ++i) {
      if (!c_bg_enabled(i))
        continue;
      if (c_bg_win_y(i) >= top_y) {
        c_bg_disable(i);
        turn_bg_back_on[i] = true;
        restore_bgs = true;
      } else if (c_bg_win_y(i) + c_bg_win_height(i) >= top_y) {
        original_bg_height[i] = c_bg_win_height(i);
        c_bg_set_win(i, c_bg_win_x(i), c_bg_win_y(i), c_bg_win_width(i),
                      c_bg_win_height(i) - c_bg_win_y(i) -
                              c_bg_win_height(i) + top_y);
        obscured = true;
        turn_bg_back_on[i] = false;
        restore_bgs = true;
      } else {
        original_bg_height[i] = -1;
        turn_bg_back_on[i] = false;
      }
    }
    if (obscured) {
      original_text_pos[0] = sc0.c_x();
      original_text_pos[1] = sc0.c_y();
      sc0.setWindow(x, y + sc0.getScreenHeight() - 5, w, 5);
      sc0.setScroll(true);
      sc0.locate(0, 0);
      sc0.cls();
      restore_text_window = true;
    }
  }
#endif
}

void SMALL restore_windows() {
#ifdef USE_BG_ENGINE
  if (restore_text_window) {
    restore_text_window = false;
    sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
    sc0.setScroll(true);
    sc0.locate(original_text_pos[0], original_text_pos[1]);
  }
  if (restore_bgs) {
    restore_bgs = false;
    for (int i = 0; i < MAX_BG; ++i) {
      if (turn_bg_back_on[i]) {
        c_bg_enable(i);
      } else if (original_bg_height[i] >= 0) {
        c_bg_set_win(i, c_bg_win_x(i), c_bg_win_y(i), c_bg_win_width(i),
                      original_bg_height[i]);
      }
    }
  }
#endif
}
