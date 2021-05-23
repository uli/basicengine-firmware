// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "eb_bg.h"
#include "eb_api.h"

#ifdef USE_BG_ENGINE

void eb_bg_off(void) {
  vs23.resetBgs();
}

int eb_bg_set_size(int bg, int tiles_w, int tiles_h) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      // XXX: valid range depends on tile size
      check_param(tiles_w, 0, sc0.getGWidth()) ||
      check_param(tiles_h, 0, sc0.getGHeight()))
    return -1;

  bool ret = vs23.setBgSize(bg, tiles_w, tiles_h);

  if (ret) {
      err = ERR_OOM;
      return -1;
  }
  return 0;
}

int eb_bg_set_pattern(int bg, int pattern_x, int pattern_y, int pattern_w) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(pattern_x, 0, sc0.getGWidth()) ||
      check_param(pattern_y, 0, vs23.lastLine()) ||
      // XXX: valid range depends on tile size
      check_param(pattern_w, 0, sc0.getGWidth()))
    return -1;

  vs23.setBgPattern(bg, pattern_x, pattern_y, pattern_w);
  return 0;
}

int eb_bg_set_tile_size(int bg, int tile_size_x, int tile_size_y) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(tile_size_x, 8, 32) ||
      check_param(tile_size_y, 8, 32))
    return -1;

  vs23.setBgTileSize(bg, tile_size_x, tile_size_y);
  return 0;
}

int eb_bg_set_window(int bg, int win_x, int win_y, int win_w, int win_h) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(win_x, 0, sc0.getGWidth() - 1) ||
      check_param(win_y, 0, sc0.getGHeight() - 1) ||
      check_param(win_w, 9, sc0.getGWidth() - win_x) ||
      check_param(win_h, 0, sc0.getGHeight() - win_y))
    return -1;

  vs23.setBgWin(bg, win_x, win_y, win_w, win_h);
  return 0;
}

int eb_bg_enable(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
      return -1;

  vs23.enableBg(bg);
  return 0;
}

int eb_bg_disable(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
      return -1;

  vs23.disableBg(bg);
  return 0;
}

int eb_bg_set_priority(int bg, int priority) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(priority, 0, MAX_PRIO))
    return -1;

  vs23.setBgPriority(bg, priority);
  return 0;
}

int eb_bg_load(int bg, const char *file) {
  if (check_param(bg, 0, MAX_BG - 1))
      return -1;

  uint8_t w, h, tsx, tsy;
  FILE *f = fopen(file, "r");
  if (!f) {
    err = ERR_FILE_OPEN;
    return -1;
  }

  w = getc(f);
  h = getc(f);
  tsx = getc(f);
  tsy = getc(f);

  if (!w || !h || !tsx || !tsy) {
    err = ERR_FORMAT;
    goto out;
  }

  vs23.setBgTileSize(bg, tsx, tsy);

  if (vs23.setBgSize(bg, w, h)) {
    err = ERR_OOM;
    goto out;
  }

  if (fread((char *)vs23.bgTiles(bg), 1, w * h, f) != w * h) {
    err = ERR_FILE_READ;
    goto out;
  }

out:
  fclose(f);
  if (err)
      return -1;
  return 0;
}

int eb_bg_save(int bg, const char *file) {
  if (check_param(bg, 0, MAX_BG - 1))
      return -1;

  uint8_t w, h;
  FILE *f = fopen(file, "w");
  if (!f) {
    err = ERR_FILE_OPEN;
    return -1;
  }

  w = vs23.bgWidth(bg);
  h = vs23.bgHeight(bg);

  putc(w, f);
  putc(h, f);
  putc(vs23.bgTileSizeX(bg), f);
  putc(vs23.bgTileSizeY(bg), f);

  fwrite((char *)vs23.bgTiles(bg), 1, w * h, f);

  fclose(f);
  return 0;
}

int eb_bg_move(int bg, int x, int y) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;
  // XXX: arbitrary limitation?

  vs23.scroll(bg, x, y);
  return 0;
}

void eb_sprite_off(void) {
  vs23.resetSprites();
}

int eb_sprite_set_pattern(int s, int pat_x, int pat_y) {
  if (check_param(s, 0, MAX_SPRITES) ||
      check_param(pat_x, 0, sc0.getGWidth()) ||
      check_param(pat_y, 0, 1023))
    return -1;

  vs23.setSpritePattern(s, pat_x, pat_y);
  return 0;
}

int eb_sprite_set_size(int s, int w, int h) {
  if (check_param(s, 0, MAX_SPRITES) ||
      check_param(w, 1, MAX_SPRITE_W) ||
      check_param(h, 1, MAX_SPRITE_H))
    return -1;

  vs23.resizeSprite(s, w, h);
  return 0;
}

int eb_sprite_enable(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.enableSprite(s);
  return 0;
}

int eb_sprite_disable(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.disableSprite(s);
  return 0;
}

int eb_sprite_set_key(int s, ipixel_t key) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteKey(s, key);
  return 0;
}

int eb_sprite_set_priority(int s, int prio) {
  if (check_param(s, 0, MAX_SPRITES) ||
      check_param(prio, 0, MAX_PRIO))
    return -1;

  vs23.setSpritePriority(s, prio);
  return 0;
}

int eb_sprite_set_frame(int s, int frame_x, int frame_y, int flip_x, int flip_y) {
  if (check_param(s, 0, MAX_SPRITES) ||
      check_param(frame_x, 0, 255) ||
      check_param(frame_y, 0, 255))
    return -1;

  vs23.setSpriteFrame(s, frame_x, frame_y, flip_x, flip_y);
  return 0;
}

int eb_sprite_set_opacity(int s, int onoff) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteOpaque(s, onoff);
  return 0;
}

#ifdef USE_ROTOZOOM
int eb_sprite_set_angle(int s, double angle) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteAngle(s, angle);
  return 0;
}

int eb_sprite_set_scale_x(int s, double scale_x) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteScaleX(s, scale_x);
  return 0;
}

int eb_sprite_set_scale_y(int s, double scale_y) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteScaleY(s, scale_y);
  return 0;
}
#endif

#ifdef TRUE_COLOR
int eb_sprite_set_alpha(int s, uint8_t a) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.setSpriteAlpha(s, a);
  return 0;
}
#endif

int eb_sprite_reload(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return !vs23.spriteReload(s);
}

int eb_sprite_move(int s, int x, int y) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  vs23.moveSprite(s, x, y);
  return 0;
}

uint8_t *eb_bg_get_tiles(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return NULL;

  return vs23.bgTiles(bg);
}

int eb_bg_map_tile(int bg, int from, int to) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(from, 0, 255) ||
      check_param(to, 0, 255))
    return -1;

  vs23.mapBgTile(bg, from, to);
  return 0;
}

int eb_bg_set_tiles(int bg, int x, int y, const uint8_t *tiles, int count) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(x, 0, INT_MAX) ||	// XXX: is that really a reasonable limit?
      check_param(y, 0, INT_MAX) ||
      tiles == NULL)
    return -1;

  vs23.setBgTiles(bg, x, y, tiles, count);
  return 0;
}

int eb_bg_set_tile(int bg, int x, int y, int t) {
  if (check_param(bg, 0, MAX_BG - 1) ||
      check_param(x, 0, INT_MAX) ||	// XXX: is that really a reasonable limit?
      check_param(y, 0, INT_MAX) ||
      check_param(t, 0, 255))
    return -1;

  vs23.setBgTile(bg, x, y, t);
  return 0;
}

int eb_frameskip(int skip) {
  if (check_param(skip, 0, 60))
    return -1;

  vs23.setFrameskip(skip);
  return 0;
}

int eb_sprite_tile_collision(int s, int bg, int tile) {
  if (check_param(s, 0, MAX_SPRITES) ||
      check_param(bg, 0, MAX_BG - 1) ||
      check_param(tile, 0, 255))
    return -1;

  return vs23.spriteTileCollision(s, bg, tile);
}

int eb_sprite_collision(int s1, int s2) {
  if (check_param(s1, 0, MAX_SPRITES) ||
      check_param(s2, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteCollision(s1, s2);
}

int eb_sprite_enabled(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteEnabled(s);
}

int eb_sprite_x(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteX(s);
}

int eb_sprite_y(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteY(s);
}

int eb_sprite_w(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteWidth(s);
}

int eb_sprite_h(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteHeight(s);
}

int eb_bg_x(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgScrollX(bg);
}

int eb_bg_y(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgScrollY(bg);
}

int eb_bg_win_x(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgWinX(bg);
}

int eb_bg_win_y(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgWinY(bg);
}

int eb_bg_win_width(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgWinWidth(bg);
}

int eb_bg_win_height(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgWinHeight(bg);
}

int eb_bg_enabled(int bg) {
  if (check_param(bg, 0, MAX_BG - 1))
    return -1;

  return vs23.bgEnabled(bg);
}

int eb_sprite_frame_x(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteFrameX(s);
}

int eb_sprite_frame_y(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteFrameY(s);
}

int eb_sprite_flip_x(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteFlipX(s);
}

int eb_sprite_flip_y(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteFlipY(s);
}

int eb_sprite_opaque(int s) {
  if (check_param(s, 0, MAX_SPRITES))
    return -1;

  return vs23.spriteOpaque(s);
}

int eb_add_bg_layer(eb_layer_painter_t painter, int prio, void *userdata) {
  return vs23.addBgLayer(painter, prio, userdata);
}

void eb_remove_bg_layer(int id) {
  vs23.removeBgLayer(id);
}

#endif
