// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#ifndef _BGENGINE_H
#define _BGENGINE_H

#include "ttconfig.h"
#include <joystick.h>
#include "video_driver.h"
#include <stdint.h>

#define MAX_BG	4

#define MAX_SPRITES  32
#define MAX_SPRITE_W 32
#define MAX_SPRITE_H 32
#define MAX_PRIO     (MAX_BG - 1)

class BGEngine : public Video {
#ifdef USE_BG_ENGINE
public:
  void reset() override;

  inline uint8_t bgWidth(uint8_t bg) {
    return m_bg[bg].w;
  }
  inline uint8_t bgHeight(uint8_t bg) {
    return m_bg[bg].h;
  }

  void setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t);
  void setBgTiles(uint8_t bg_idx, uint16_t x, uint16_t y, const uint8_t *tiles,
                  int count);
  void mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to);

  inline void setBgTileSize(uint8_t bg_idx, uint8_t tile_size_x,
                            uint8_t tile_size_y) {
    struct bg_t *bg = &m_bg[bg_idx];
    bg->tile_size_x = tile_size_x;
    bg->tile_size_y = tile_size_y;
    m_bg_modified = true;
  }

  inline void setBgPattern(uint8_t bg_idx, uint16_t pat_x, uint16_t pat_y,
                           uint16_t pat_w) {
    struct bg_t *bg = &m_bg[bg_idx];
    bg->pat_x = pat_x;
    bg->pat_y = pat_y;
    bg->pat_w = pat_w;
    m_bg_modified = true;
  }

  inline void setBgPriority(uint8_t bg_idx, uint8_t prio) {
    m_bg[bg_idx].prio = prio;
    m_bg_modified = true;
  }

  inline uint8_t bgTileSizeX(uint8_t bg) {
    return m_bg[bg].tile_size_x;
  }
  inline uint8_t bgTileSizeY(uint8_t bg) {
    return m_bg[bg].tile_size_y;
  }

  inline void getBgTileSize(uint8_t bg_idx, uint8_t &tsx, uint8_t &tsy) {
    struct bg_t *bg = &m_bg[bg_idx];
    tsx = bg->tile_size_x;
    tsy = bg->tile_size_y;
  }

  inline uint8_t *bgTiles(uint8_t bg) {
    return m_bg[bg].tiles;
  }

  void enableBg(uint8_t bg);
  void disableBg(uint8_t bg);
  void freeBg(uint8_t bg);

  void setBgWin(uint8_t bg, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  inline uint16_t bgWinX(uint8_t bg) {
    return m_bg[bg].win_x;
  }
  inline uint16_t bgWinY(uint8_t bg) {
    return m_bg[bg].win_y;
  }
  inline uint16_t bgWinWidth(uint8_t bg) {
    return m_bg[bg].win_w;
  }
  inline uint16_t bgWinHeight(uint8_t bg) {
    return m_bg[bg].win_h;
  }
  inline bool bgEnabled(uint8_t bg) {
    return m_bg[bg].enabled;
  }
  inline int bgScrollX(uint8_t bg) {
    return m_bg[bg].scroll_x;
  }
  inline int bgScrollY(uint8_t bg) {
    return m_bg[bg].scroll_y;
  }

  inline void scroll(uint8_t bg_idx, int x, int y) {
    struct bg_t *bg = &m_bg[bg_idx];
    while (x < 0)
      x += bg->tile_size_x * bg->w;
    while (y < 0)
      y += bg->tile_size_y * bg->h;
    if (bg->scroll_x != x || bg->scroll_y != y) {
      bg->scroll_x = x;
      bg->scroll_y = y;
      m_bg_modified = true;
    }
  }

  bool setBgSize(uint8_t bg, uint16_t width, uint16_t height);
  void resetBgs();

  void setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y);
  void setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y = 0,
                      bool flip_x = false, bool flip_y = false);
  void setSpriteKey(uint8_t num, int16_t key);
  void moveSprite(uint8_t num, int16_t x, int16_t y);
  void enableSprite(uint8_t num);
  void disableSprite(uint8_t num);

  inline uint8_t spriteFrameX(uint8_t num) {
    return m_sprite[num].p.frame_x;
  }
  inline uint8_t spriteFrameY(uint8_t num) {
    return m_sprite[num].p.frame_y;
  }
  inline bool spriteFlipX(uint8_t num) {
    return m_sprite[num].p.flip_x;
  }
  inline bool spriteFlipY(uint8_t num) {
    return m_sprite[num].p.flip_y;
  }
  inline bool spriteOpaque(uint8_t num) {
    return m_sprite[num].p.opaque;
  }
  inline int16_t spriteX(uint8_t num) {
    return m_sprite[num].pos_x;
  }
  inline int16_t spriteY(uint8_t num) {
    return m_sprite[num].pos_y;
  }

  inline bool spriteEnabled(uint8_t num) {
    return m_sprite[num].enabled;
  }

  inline void setSpritePriority(uint8_t num, uint8_t prio) {
    m_sprite[num].prio = prio;
    m_bg_modified = true;
  }

  inline bool spriteReload(uint8_t num) {
    return true;
  }

  inline void forceRedraw() {
    m_bg_modified = true;
  }

  inline void setFrameskip(uint32_t v) {
    m_frameskip = v;
  }
  inline uint32_t getFrameskip() {
    return m_frameskip;
  }

  void spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t *tiles,
                           uint8_t num_tiles);
  uint8_t spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile);

  void resizeSprite(uint8_t num, uint8_t w, uint8_t h);
  virtual void resetSprites();

protected:
  uint32_t m_frameskip;

  bool m_bg_modified;

  struct bg_t {
    uint8_t *tiles;
    uint16_t pat_x, pat_y, pat_w;
    uint8_t w, h;
    uint8_t tile_size_x, tile_size_y;
    int scroll_x, scroll_y;
    uint16_t win_x, win_y, win_w, win_h;
    bool enabled;
    uint8_t prio;
    uint8_t *tile_map;
  } m_bg[MAX_BG];

  struct sprite_props {
    uint16_t pat_x, pat_y;
    uint8_t w, h;
    uint8_t frame_x, frame_y;
    pixel_t key;
    bool flip_x:1, flip_y:1, opaque:1;
  };

  struct sprite_line {
    pixel_t *pixels;
    uint8_t off;
    uint8_t len;
    uint8_t type;
  };

  struct sprite_pattern {
    struct sprite_props p;
    uint32_t last;
    uint8_t ref;
    struct sprite_line lines[0];
  };

  struct sprite_t {
    struct sprite_props p;
    int16_t pos_x, pos_y;
    struct sprite_pattern *pat;
    uint8_t prio;
    bool enabled:1, must_reload:1;
  };

  struct sprite_t m_sprite[MAX_SPRITES];

  struct sprite_t *m_sprites_ordered[MAX_SPRITES];
  static int cmp_sprite_y(const void *one, const void *two);

#endif
};

#endif  // _BGENGINE_H
