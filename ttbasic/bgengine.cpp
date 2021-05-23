// SPDX-License-Identifier: MIT
// Copyright (c) 2017, 2018 Ulrich Hecht

#include "ttconfig.h"
#ifdef USE_BG_ENGINE

#include "bgengine.h"
#include "colorspace.h"
#include <Arduino.h>

void BGEngine::enableBg(uint8_t bg) {
  m_bg_modified = true;
  if (m_bg[bg].tiles) {
    m_bg[bg].enabled = true;
  }
  updateStatus();
}

void BGEngine::disableBg(uint8_t bg) {
  m_bg_modified = true;
  m_bg[bg].enabled = false;
  updateStatus();
}

void BGEngine::freeBg(uint8_t bg_idx) {
  struct bg_t *bg = &m_bg[bg_idx];
  m_bg_modified = true;
  bg->enabled = false;
  if (bg->tiles) {
    free(bg->tiles);
    bg->tiles = NULL;
  }
  if (bg->tile_map) {
    free(bg->tile_map);
    bg->tile_map = NULL;
  }
  updateStatus();
}

void BGEngine::setBgWin(uint8_t bg_idx, uint16_t x, uint16_t y, uint16_t w,
                        uint16_t h) {
  struct bg_t *bg = &m_bg[bg_idx];
  bg->win_x = x;
  bg->win_y = y;
  bg->win_w = w;
  bg->win_h = h;
  m_bg_modified = true;
}

void BGEngine::mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to) {
  struct bg_t *bg = &m_bg[bg_idx];
  if (!bg->tile_map) {
    bg->tile_map = (uint8_t *)malloc(256);
    for (int i = 0; i < 256; ++i)
      bg->tile_map[i] = i;
  }
  bg->tile_map[from] = to;
}

void BGEngine::setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t) {
  struct bg_t *bg = &m_bg[bg_idx];

  if (bg->tile_map)
    t = bg->tile_map[t];

  int toff = (y % bg->h) * bg->w + (x % bg->w);
  if (bg->tiles[toff] == t)
    return;
  else
    bg->tiles[toff] = t;

  m_bg_modified = true;
}

void BGEngine::setBgTiles(uint8_t bg_idx, uint16_t x, uint16_t y,
                          const uint8_t *tiles, int count) {
  struct bg_t *bg = &m_bg[bg_idx];

  int off = (y % bg->h) * bg->w;
  for (int xx = x; xx < x + count; ++xx) {
    uint8_t t = *tiles++;
    if (bg->tile_map)
      t = bg->tile_map[t];
    bg->tiles[off + (xx % bg->w)] = t;
  }
  m_bg_modified = true;
}

int GROUP(basic_video) BGEngine::cmp_sprite_y(const void *one,
                                              const void *two) {
  return (*(struct sprite_t **)one)->pos_y - (*(struct sprite_t **)two)->pos_y;
}

void BGEngine::spriteTileCollision(uint8_t sprite, uint8_t bg_idx,
                                   uint8_t *tiles, uint8_t num_tiles) {
  sprite_t *spr = &m_sprite[sprite];
  bg_t *bg = &m_bg[bg_idx];
  int tsx = bg->tile_size_x;
  int tsy = bg->tile_size_y;
  uint8_t res[num_tiles];

  // Intialize results with "no collision".
  memset(res, 0, num_tiles);

  // Check if sprite is overlapping with background at all.
  if (spr->pos_x + spr->p.w <= bg->win_x ||
      spr->pos_x >= bg->win_x + bg->win_w ||
      spr->pos_y + spr->p.h <= bg->win_y ||
      spr->pos_y >= bg->win_y + bg->win_h)
    return;

  // Calculate pixel coordinates of top/left sprite corner in relation to
  // the background's origin.
  int bg_left_x = bg->scroll_x - bg->win_x + spr->pos_x;
  int bg_top_y = bg->scroll_y - bg->win_y + spr->pos_y;

  int bg_right_x = bg_left_x + spr->p.w;
  int bg_bottom_y = bg_top_y + spr->p.h;

  // Calculate offset, width and height in tiles.
  int bg_first_tile_off_x = bg_left_x / tsx;
  int bg_first_tile_off_y = bg_top_y / tsy;

  int bg_last_tile_off_x = (bg_left_x + spr->p.w) / tsx;
  int bg_last_tile_off_y = (bg_top_y + spr->p.h) / tsy;

  // Iterate over all tiles overlapping the sprite and record in what way they are
  // overlapping.
  // For every line
  for (int ty = bg_first_tile_off_y; ty <= bg_last_tile_off_y; ++ty) {
    // For every column
    for (int tx = bg_first_tile_off_x; tx <= bg_last_tile_off_x; ++tx) {
      // For every tile code to be checked
      for (int m = 0; m < num_tiles; ++m) {
        if (tiles[m] == bg->tiles[(tx + ty * bg->w) % (bg->w * bg->h)]) {
          res[m] = 0x40;  // indicates collision in general
          if (tx * tsx < spr->pos_x)
            res[m] |= EB_JOY_LEFT;
          else if ((tx + 1) * tsx > spr->pos_x + spr->p.w)
            res[m] |= EB_JOY_RIGHT;
          if (ty * tsy < spr->pos_y)
            res[m] |= EB_JOY_UP;
          else if ((ty + 1) * tsy > spr->pos_y + spr->p.h)
            res[m] |= EB_JOY_DOWN;
        }
      }
    }
  }
  os_memcpy(tiles, res, num_tiles);
}

uint8_t BGEngine::spriteTileCollision(uint8_t sprite, uint8_t bg,
                                      uint8_t tile) {
  spriteTileCollision(sprite, bg, &tile, 1);
  return tile;
}

void GROUP(basic_video) BGEngine::setSpriteFrame(uint8_t num, uint8_t frame_x,
                                                 uint8_t frame_y, bool flip_x,
                                                 bool flip_y) {
  struct sprite_t *s = &m_sprite[num];
  if (frame_x != s->p.frame_x || frame_y != s->p.frame_y ||
      flip_x != s->p.flip_x || flip_y != s->p.flip_y) {
    s->p.frame_x = frame_x;
    s->p.frame_y = frame_y;
    s->p.flip_x = flip_x;
    s->p.flip_y = flip_y;
    // XXX: opaque?
    s->must_reload = true;
    m_bg_modified = true;
  }
}

void BGEngine::setSpriteKey(uint8_t num, ipixel_t key) {
  struct sprite_t *s = &m_sprite[num];
  pixel_t pkey = csp.fromIndexed(key);

  if (s->p.key != pkey) {
    s->p.key = pkey;
    s->must_reload = true;
    m_bg_modified = true;
  }
}

void GROUP(basic_video) BGEngine::setSpritePattern(uint8_t num, uint16_t pat_x,
                                                   uint16_t pat_y) {
  struct sprite_t *s = &m_sprite[num];
  if (s->p.pat_x != pat_x || s->p.pat_y != pat_y) {
    s->p.pat_x = pat_x;
    s->p.pat_y = pat_y;
    s->p.frame_x = s->p.frame_y = 0;

    s->must_reload = true;
    m_bg_modified = true;
  }
}

void BGEngine::enableSprite(uint8_t num) {
  struct sprite_t *s = &m_sprite[num];
  if (!s->enabled) {
    s->must_reload = true;
    s->enabled = true;
    m_bg_modified = true;
  }
  updateStatus();
}

void BGEngine::disableSprite(uint8_t num) {
  struct sprite_t *s = &m_sprite[num];
  if (s->enabled) {
    s->enabled = false;
    m_bg_modified = true;
  }
  updateStatus();
}

void GROUP(basic_video) BGEngine::moveSprite(uint8_t num, int16_t x,
                                             int16_t y) {
  sprite_t *s = &m_sprite[num];
  if (s->pos_x != x || s->pos_y != y) {
    s->pos_x = x;
    s->pos_y = y;
    qsort(m_sprites_ordered, MAX_SPRITES, sizeof(struct sprite_t *),
          cmp_sprite_y);
    m_bg_modified = true;
  }
}

void BGEngine::resizeSprite(uint8_t num, uint8_t w, uint8_t h) {
  struct sprite_t *s = &m_sprite[num];
  if (w != s->p.w || h != s->p.h) {
    s->p.w = w;
    s->p.h = h;
    s->must_reload = true;
    m_bg_modified = true;
  }
}

bool BGEngine::setBgSize(uint8_t bg_idx, uint16_t width, uint16_t height) {
  struct bg_t *bg = &m_bg[bg_idx];

  m_bg_modified = true;
  bg->enabled = false;

  if (bg->tiles)
    free(bg->tiles);
  bg->tiles = (uint8_t *)calloc(width * height, 1);
  if (!bg->tiles)
    return true;

  bg->w = width;
  bg->h = height;
  bg->scroll_x = bg->scroll_y = 0;
  bg->win_x = bg->win_y = 0;
  bg->win_w = m_current_mode.x;
  bg->win_h = m_current_mode.y;

  updateStatus();
  return false;
}

void BGEngine::resetSprites() {
  m_bg_modified = true;
  for (int i = 0; i < MAX_SPRITES; ++i) {
    struct sprite_t *s = &m_sprite[i];
    m_sprites_ordered[i] = s;
    s->enabled = false;
    s->pos_x = s->pos_y = 0;
    s->p.frame_x = s->p.frame_y = 0;
    s->p.w = s->p.h = 8;
    s->p.key = 0;
    s->prio = MAX_PRIO;
    s->p.flip_x = s->p.flip_y = false;
    s->p.opaque = false;
#ifdef USE_ROTOZOOM
    s->angle = 0.0d;
    s->scale_x = 1.0d;
    s->scale_y = 1.0d;
    if (s->surf) {
      delete s->surf;
      s->surf = NULL;
    }
#endif
#ifdef TRUE_COLOR
    s->alpha = 0xff;
#endif
  }
  updateStatus();
}

void BGEngine::resetBgs() {
  m_bg_modified = true;
  for (int i = 0; i < MAX_BG; ++i) {
    struct bg_t *bg = &m_bg[i];
    freeBg(i);
    bg->tile_size_x = 8;
    bg->tile_size_y = 8;
    bg->pat_x = 0;
    bg->pat_y = m_current_mode.y + 8;
    bg->pat_w = m_current_mode.x / bg->tile_size_x;
    bg->prio = i;
  }
}

void BGEngine::reset() {
  m_bg_modified = true;
  resetSprites();
  resetBgs();

  Video::reset();
}

void BGEngine::updateStatus() {
}

#endif  // USE_BG_ENGINE
