#ifndef _ESP32GFX_H
#define _ESP32GFX_H

#include "ttconfig.h"
#include "SimplePALOutput.h"
#include "GuillotineBinPack.h"

#define SC_DEFAULT 0
#define XRES 320
#define YRES 240
#define LAST_LINE 400

#define MAX_BG 4

#define MAX_SPRITES 32
#define MAX_SPRITE_W 32
#define MAX_SPRITE_H 32

#define MAX_PRIO	(MAX_BG-1)

struct esp32gfx_mode_t {
  int x;
  int y;
  int top;
  int left;
  int pclk;
};

class ESP32GFX {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return XRES;
  }
  inline uint16_t height() {
    return YRES;
  }

  int numModes() { return 1; }
  void setMode(uint8_t mode);
  void setColorSpace(uint8_t palette) {}
  inline int lastLine() { return LAST_LINE; }

  bool blockFinished() { return true; }

#ifdef USE_BG_ENGINE
  bool setBgSize(uint8_t bg, uint16_t width, uint16_t height);

  inline uint8_t bgWidth(uint8_t bg) {
    return m_bg[bg].w;
  }
  inline uint8_t bgHeight(uint8_t bg) {
    return m_bg[bg].h;
  }

  inline void setBgTileSize(uint8_t bg_idx, uint8_t tile_size_x, uint8_t tile_size_y) {
    struct bg_t *bg = &m_bg[bg_idx];
    bg->tile_size_x = tile_size_x;
    bg->tile_size_y = tile_size_y;
    m_bg_modified = true;
  }

  inline void setBgPattern(uint8_t bg_idx, uint16_t pat_x, uint16_t pat_y, uint16_t pat_w) {
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

  void resetBgs();
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
  
  void setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t);
  void setBgTiles(uint8_t bg_idx, uint16_t x, uint16_t y, const uint8_t *tiles, int count);
  void mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to);

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
  void updateBg();

  void resetSprites();
  void setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y);
  void resizeSprite(uint8_t num, uint8_t w, uint8_t h);
  void moveSprite(uint8_t num, int16_t x, int16_t y);
  void setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y = 0, bool flip_x = false, bool flip_y = false);
  void setSpriteKey(uint8_t num, int16_t key);
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
    return !m_sprite[num].p.opaque;
  }
  inline int16_t spriteX(uint8_t num) {
    return m_sprite[num].pos_x;
  }
  inline int16_t spriteY(uint8_t num) {
    return m_sprite[num].pos_y;
  }

  inline void setSpriteOpaque(uint8_t num, bool enable) {
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

  void spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t *tiles, uint8_t num_tiles);
  uint8_t spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile);
  uint8_t spriteCollision(uint8_t collidee, uint8_t collider);

  inline void forceRedraw() {
    m_bg_modified = true;
  }
  
  inline void setFrameskip(uint32_t v) {
    m_frameskip = v;
  }
  inline uint32_t getFrameskip() {
    return m_frameskip;
  }
#endif

  bool allocBacking(int w, int h, int &x, int &y);
  void freeBacking(int x, int y, int w, int h);

  void reset();

  void setBorder(uint8_t y, uint8_t uv) {}
  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w) {}
  inline uint16_t borderWidth() { return 42; }

  inline void setPixel(uint16_t x, uint16_t y, uint8_t c) {
    m_pixels[y][x] = c;
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
  inline uint8_t getPixel(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }

  void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);

  inline void setInterlace(bool interlace) {
    //m_interlace = interlace;
  }
  inline void setLowpass(bool lowpass) {
    //m_lowpass = lowpass;
  }
  inline void setLineAdjust(int8_t line_adjust) {
    //m_line_adjust = line_adjust;
  }

  void setSpiClock(uint32_t div) {
  }
  void setSpiClockRead() {
  }
  void setSpiClockWrite() {
  }
  void setSpiClockMax() {
  }

  inline void writeBytes(uint32_t address, uint8_t *data, uint32_t len) {
    memcpy((void *)address, data, len);
  }
  inline uint32_t pixelAddr(int x, int y) {	// XXX: uint32_t? ouch...
    return (uint32_t)&m_pixels[y][x];
  }
  inline uint32_t piclineByteAddress(int line) {
      return (uint32_t)&m_pixels[line][0];
  }

  inline uint32_t frame() { return m_frame; }

  void render();

private:
  struct esp32gfx_mode_t m_current_mode;
  GuillotineBinPack m_bin;

  uint32_t m_frame;
  uint8_t *m_pixels[LAST_LINE];

#ifdef USE_BG_ENGINE
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
    uint8_t key;
    bool flip_x:1, flip_y:1, opaque:1;
  };
    
  struct sprite_t {
    struct sprite_props p;
    int16_t pos_x, pos_y;
    uint8_t prio;
    bool enabled:1;
  } m_sprite[MAX_SPRITES];
  struct sprite_t *m_sprites_ordered[MAX_SPRITES];
  static int cmp_sprite_y(const void *one, const void *two);

  uint32_t m_frameskip;

  bool m_bg_modified;
#endif

  SimplePALOutput m_pal;
};

extern ESP32GFX vs23;

static inline bool blockFinished() { return true; }

#endif
