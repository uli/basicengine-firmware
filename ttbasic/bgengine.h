// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#ifndef _BGENGINE_H
#define _BGENGINE_H

#include "ttconfig.h"
#include <joystick.h>
#include "video_driver.h"
#include <stdint.h>
#include "rotozoom.h"
#include "eb_bg.h"
#include <vector>

#define MAX_BG	4

#define MAX_SPRITES  32
#define MAX_SPRITE_W 32
#define MAX_SPRITE_H 32
#define MAX_PRIO     (MAX_BG - 1)

class BGEngine : public Video {
#ifdef USE_BG_ENGINE
public:
  void reset() override;

  inline uint32_t bgWidth(uint8_t bg) {
    return m_bg[bg].w;
  }
  inline uint32_t bgHeight(uint8_t bg) {
    return m_bg[bg].h;
  }

  void setBgTile(uint8_t bg_idx, uint32_t x, uint32_t y, uint8_t t);
  void setBgTiles(uint8_t bg_idx, uint32_t x, uint32_t y, const uint8_t *tiles,
                  int count);
  void mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to);

  inline void setBgTileSize(uint8_t bg_idx, uint32_t tile_size_x,
                            uint32_t tile_size_y) {
    struct bg_t *bg = &m_bg[bg_idx];
    bg->tile_size_x = tile_size_x;
    bg->tile_size_y = tile_size_y;
    m_bg_modified = true;
  }

  inline void setBgPattern(uint8_t bg_idx, uint32_t pat_x, uint32_t pat_y,
                           uint32_t pat_w) {
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

  inline uint32_t bgTileSizeX(uint8_t bg) {
    return m_bg[bg].tile_size_x;
  }
  inline uint32_t bgTileSizeY(uint8_t bg) {
    return m_bg[bg].tile_size_y;
  }

  inline void getBgTileSize(uint8_t bg_idx, uint32_t &tsx, uint32_t &tsy) {
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

  void setBgWin(uint8_t bg, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
  inline uint32_t bgWinX(uint8_t bg) {
    return m_bg[bg].win_x;
  }
  inline uint32_t bgWinY(uint8_t bg) {
    return m_bg[bg].win_y;
  }
  inline uint32_t bgWinWidth(uint8_t bg) {
    return m_bg[bg].win_w;
  }
  inline uint32_t bgWinHeight(uint8_t bg) {
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

  bool setBgSize(uint8_t bg, uint32_t width, uint32_t height);
  void resetBgs();

  void setSpritePattern(uint32_t num, uint32_t pat_x, uint32_t pat_y);
  void setSpriteFrame(uint32_t num, uint32_t frame_x, uint32_t frame_y = 0,
                      bool flip_x = false, bool flip_y = false);
  void setSpriteKey(uint32_t num, ipixel_t key);
  void moveSprite(uint32_t num, int32_t x, int32_t y);
  void enableSprite(uint32_t num);
  void disableSprite(uint32_t num);

  inline uint32_t spriteFrameX(uint32_t num) {
    return m_sprite[num].p.frame_x;
  }
  inline uint32_t spriteFrameY(uint32_t num) {
    return m_sprite[num].p.frame_y;
  }
  inline bool spriteFlipX(uint32_t num) {
    return m_sprite[num].p.flip_x;
  }
  inline bool spriteFlipY(uint32_t num) {
    return m_sprite[num].p.flip_y;
  }
  inline bool spriteOpaque(uint32_t num) {
    return m_sprite[num].p.opaque;
  }
  inline int32_t spriteX(uint32_t num) {
    return m_sprite[num].pos_x;
  }
  inline int32_t spriteY(uint32_t num) {
    return m_sprite[num].pos_y;
  }
  inline int32_t spriteWidth(uint32_t num) {
#ifdef USE_ROTOZOOM
    if (m_sprite[num].surf)
      return m_sprite[num].surf->w;
    else
#endif
      return m_sprite[num].p.w;
  }
  inline int32_t spriteHeight(uint32_t num) {
#ifdef USE_ROTOZOOM
    if (m_sprite[num].surf)
      return m_sprite[num].surf->h;
    else
#endif
      return m_sprite[num].p.h;
  }

  inline bool spriteEnabled(uint32_t num) {
    return m_sprite[num].enabled;
  }

  inline void setSpritePriority(uint32_t num, uint8_t prio) {
    m_sprite[num].prio = prio;
    m_bg_modified = true;
  }

#ifdef USE_ROTOZOOM
  inline void setSpriteAngle(uint32_t num, double angle) {
    m_sprite[num].angle = angle;
    spriteReload(num);
  }

  inline void setSpriteScaleX(uint32_t num, double scale_x) {
    m_sprite[num].scale_x = scale_x;
    spriteReload(num);
  }

  inline void setSpriteScaleY(uint32_t num, double scale_y) {
    m_sprite[num].scale_y = scale_y;
    spriteReload(num);
  }
#endif

#ifdef TRUE_COLOR
  inline void setSpriteAlpha(uint32_t num, uint8_t alpha) {
    m_sprite[num].alpha = alpha;
    spriteReload(num);
  }
#endif

  inline bool spriteReload(uint32_t num) {
    m_sprite[num].must_reload = true;
    if (m_sprite[num].enabled)
      m_bg_modified = true;

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

  void spriteTileCollision(uint32_t sprite, uint8_t bg, uint8_t *tiles,
                           uint8_t num_tiles);
  uint8_t spriteTileCollision(uint32_t sprite, uint8_t bg, uint8_t tile);

  void resizeSprite(uint32_t num, uint32_t w, uint32_t h);
  virtual void resetSprites();

  virtual void lockSprites();
  virtual void unlockSprites();

  int addBgLayer(eb_layer_painter_t painter, int prio, void *userdata);
  void removeBgLayer(int id);

protected:
  uint32_t m_frameskip;
  virtual void updateStatus();

  bool m_bg_modified;

  struct bg_t {
    uint8_t *tiles;
    uint32_t pat_x, pat_y, pat_w;
    uint32_t w, h;
    uint32_t tile_size_x, tile_size_y;
    int scroll_x, scroll_y;
    uint32_t win_x, win_y, win_w, win_h;
    bool enabled;
    uint8_t prio;
    uint8_t *tile_map;
  } m_bg[MAX_BG];

  struct sprite_props {
    uint32_t pat_x, pat_y;
    uint32_t w, h;
    uint32_t frame_x, frame_y;
    pixel_t key;
    bool flip_x:1, flip_y:1, opaque:1;
  };

  struct sprite_line {
    pixel_t *pixels;
    uint32_t off;
    uint32_t len;
    uint32_t type;
  };

  struct sprite_pattern {
    struct sprite_props p;
    uint32_t last;
    struct sprite_line lines[0];
  };

  struct sprite_t {
    struct sprite_props p;
    int32_t pos_x, pos_y;
    struct sprite_pattern *pat;
#ifdef USE_ROTOZOOM
    double angle;
    double scale_x, scale_y;
    rz_surface_t *surf;
#endif
#ifdef TRUE_COLOR
    uint8_t alpha;
#endif
    uint8_t prio;
    bool enabled:1, must_reload:1;
  };

  struct sprite_t m_sprite[MAX_SPRITES];

  struct sprite_t *m_sprites_ordered[MAX_SPRITES];
  static int cmp_sprite_y(const void *one, const void *two);

  struct external_layer_t {
      eb_layer_painter_t painter;
      void *userdata;
      int prio;
  };

  std::vector<external_layer_t> m_external_layers;
#endif
};

#endif  // _BGENGINE_H
