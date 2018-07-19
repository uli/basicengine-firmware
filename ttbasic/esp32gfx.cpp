#ifdef ESP32

#include <Arduino.h>
#include <soc/rtc.h>
#include "esp32gfx.h"
#include "colorspace.h"
#include "Psx.h"

ESP32GFX vs23;

static void pal_core(void *data)
{
  for (;;)
    vs23.render();
}

void ESP32GFX::render()
{
  if (!m_display_enabled)
    delay(16);
  else {
    switch (m_current_mode.pclk) {
      case 1:
        m_pal.sendFrame1ppc(&m_current_mode, m_pixels);
        break;
      case 2:
        m_pal.sendFrame(&m_current_mode, m_pixels);
        break;
      case 4:
        m_pal.sendFrame4ppc(&m_current_mode, m_pixels);
        break;
      default:
        break;
    }
  }
  m_frame++;
}

void ESP32GFX::begin(bool interlace, bool lowpass, uint8_t system)
{
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  m_pixels = NULL;
  setMode(SC_DEFAULT);

  m_bin.Init(0, 0);

  m_frame = 0;
  m_pal.init();
  reset();
  xTaskCreatePinnedToCore(pal_core, "c", 1024, NULL, 2, NULL, 0);
  m_display_enabled = true;
}

void ESP32GFX::reset()
{
#ifdef USE_BG_ENGINE
  m_bg_modified = true;
  resetSprites();
  resetBgs();
#endif
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x);
  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);
  setColorSpace(0);
}

void ESP32GFX::MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint16_t width, uint16_t height, uint8_t dir)
{
  if (dir) {
    x_src -= width - 1;
    x_dst -= width - 1;
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width);
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memcpy(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width);
      y_dst++;
      y_src++;
      height--;
    }
  }
}

// XXX: duplicate code in VS23S010
bool ESP32GFX::allocBacking(int w, int h, int &x, int &y)
{
  Rect r = m_bin.Insert(w, h, false, GuillotineBinPack::RectBestAreaFit, GuillotineBinPack::Split256);
  x = r.x; y = r.y + m_current_mode.y;
  return r.height != 0;
}

void ESP32GFX::freeBacking(int x, int y, int w, int h)
{
  Rect r;
  r.x = x; r.y = y - m_current_mode.y; r.width = w; r.height = h;
  m_bin.Free(r, true);
}

#include <esp_heap_alloc_caps.h>

void ESP32GFX::setMode(uint8_t mode)
{
  m_display_enabled = false;
  delay(16);

  for (int i = 0; i < m_last_line; ++i) {
    if (m_pixels[i])
      free(m_pixels[i]);
  }
  free(m_pixels);

  m_current_mode = modes_pal[mode];
  m_pal.setMode(m_current_mode);

  m_last_line = _max(131072 / m_current_mode.x, m_current_mode.y + 8);
  m_pixels = (uint8_t **)pvPortMallocCaps(sizeof(*m_pixels) * m_last_line, MALLOC_CAP_32BIT);
  for (int i = 0; i < m_last_line; ++i) {
    m_pixels[i] = (uint8_t *)calloc(1, m_current_mode.x);
  }
  m_display_enabled = true;
}

//#define PROFILE_BG

void IRAM_ATTR __attribute__((optimize("O3"))) ESP32GFX::updateBg()
{
  static uint32_t last_frame = 0;

  if (m_frame <= last_frame + m_frameskip || !m_bg_modified)
    return;
  m_bg_modified = false;
  last_frame = m_frame;

#ifdef PROFILE_BG
  uint32_t start = micros();
#endif
  for (int b = 0; b < MAX_BG; ++b) {
    bg_t *bg = &m_bg[b];
    if (!bg->enabled)
      continue;

    int tsx = bg->tile_size_x;
    int tsy = bg->tile_size_y;
    int sx = bg->scroll_x;
    int ex = m_current_mode.x + bg->scroll_x;
    int sy = bg->scroll_y;
    int ey = m_current_mode.y + bg->scroll_y;

    for (int y = sy; y < ey; ++y) {
      for (int x = sx; x < ex; ++x) {
        int off_x = x % tsx;
        int off_y = y % tsy;
        int tile_x = x / tsx;
        int tile_y = y / tsy;
next:
        uint8_t tile = bg->tiles[tile_x % bg->w + (tile_y % bg->h) * bg->w];
        int t_x = bg->pat_x + (tile % bg->pat_w) * tsx + off_x;
        int t_y = bg->pat_y + (tile / bg->pat_w) * tsy + off_y;
        if (!off_x && x < ex - tsx) {
          memcpy(&m_pixels[y-sy][x-sx], &m_pixels[t_y][t_x], tsx);
          x += tsx;
          tile_x++;
          goto next;
        } else {
          m_pixels[y-sy][x-sx] = m_pixels[t_y][t_x];
        }
      }
    }
  }
#ifdef PROFILE_BG
  uint32_t taken = micros() - start;
  Serial.printf("rend %d\r\n", taken);
#endif
  for (int si = 0; si < MAX_SPRITES; ++si) {
    sprite_t *s = &m_sprite[si];
    // skip if not visible
    if (!s->enabled)
      continue;
    if (s->pos_x + s->p.w < 0 ||
        s->pos_x >= width() ||
        s->pos_y + s->p.h < 0 ||
        s->pos_y >= height())
      continue;

    // consider flipped axes
    int dx, offx;
    if (s->p.flip_x) {
      dx = -1; offx = s->p.w - 1;
    } else {
      dx = 1; offx = 0;
    }
    int dy, offy;
    if (s->p.flip_y) {
      dy = -1; offy = s->p.h - 1;
    } else {
      dy = 1; offy = 0;
    }

    // sprite pattern start coordinates
    int px = s->p.pat_x + s->p.frame_x * s->p.w + offx;
    int py = s->p.pat_y + s->p.frame_y * s->p.h + offy;

    for (int y = 0; y != s->p.h; ++y) {
      int yy = y + s->pos_y;
      if (yy < 0 || yy > height())
        continue;
      for (int x = 0; x != s->p.w; ++x) {
        int xx = x + s->pos_x;
        if (xx < 0 || xx > width())
          continue;
        uint8_t p = m_pixels[py+y*dy][px+x*dx];
        // draw only non-keyed pixels
        if (p != s->p.key)
          m_pixels[yy][xx] = p;
      }
    }
  }
}

#ifdef USE_BG_ENGINE
bool ESP32GFX::setBgSize(uint8_t bg_idx, uint16_t width, uint16_t height)
{
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
  return false;
}

void ESP32GFX::enableBg(uint8_t bg)
{
  m_bg_modified = true;
  if (m_bg[bg].tiles) {
    m_bg[bg].enabled = true;
  }
}

void ESP32GFX::disableBg(uint8_t bg)
{
  m_bg_modified = true;
  m_bg[bg].enabled = false;
}

void ESP32GFX::freeBg(uint8_t bg_idx)
{
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
}

void ESP32GFX::setBgWin(uint8_t bg_idx, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  struct bg_t *bg = &m_bg[bg_idx];
  bg->win_x = x;
  bg->win_y = y;
  bg->win_w = w;
  bg->win_h = h;
  m_bg_modified = true;
}

void ESP32GFX::resetSprites()
{
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
  }
}

void ESP32GFX::resetBgs()
{
  m_bg_modified = true;
  for (int i=0; i < MAX_BG; ++i) {
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

void ESP32GFX::resizeSprite(uint8_t num, uint8_t w, uint8_t h)
{
  struct sprite_t *s = &m_sprite[num];
  if (w != s->p.w || h != s->p.h) {
    s->p.w = w;
    s->p.h = h;
    m_bg_modified = true;
  }
}

void GROUP(basic_esp32gfx) ESP32GFX::setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y, bool flip_x, bool flip_y)
{
  struct sprite_t *s = &m_sprite[num];
  if (frame_x != s->p.frame_x || frame_y != s->p.frame_y ||
      flip_x != s->p.flip_x || flip_y != s->p.flip_y) {
    s->p.frame_x = frame_x;
    s->p.frame_y = frame_y;
    s->p.flip_x = flip_x;
    s->p.flip_y = flip_y;
    // XXX: opaque?
    m_bg_modified = true;
  }
}

void ESP32GFX::setSpriteKey(uint8_t num, int16_t key)
{
  struct sprite_t *s = &m_sprite[num];
  if (s->p.key != key) {
    s->p.key = key;
    m_bg_modified = true;
  }
}

void GROUP(basic_esp32gfx) ESP32GFX::setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y)
{
  struct sprite_t *s = &m_sprite[num];
  if (s->p.pat_x != pat_x || s->p.pat_y != pat_y) {
    s->p.pat_x = pat_x;
    s->p.pat_y = pat_y;
    s->p.frame_x = s->p.frame_y = 0;

    m_bg_modified = true;
  }
}

void ESP32GFX::enableSprite(uint8_t num)
{
  struct sprite_t *s = &m_sprite[num];
  if (!s->enabled) {
    s->enabled = true;
    m_bg_modified = true;
  }
}

void ESP32GFX::disableSprite(uint8_t num)
{
  struct sprite_t *s = &m_sprite[num];
  if (s->enabled) {
    s->enabled = false;
    m_bg_modified = true;
  }
}

int GROUP(basic_esp32gfx) ESP32GFX::cmp_sprite_y(const void *one, const void *two)
{
  return (*(struct sprite_t**)one)->pos_y - (*(struct sprite_t **)two)->pos_y;
}

void GROUP(basic_esp32gfx) ESP32GFX::moveSprite(uint8_t num, int16_t x, int16_t y)
{
  sprite_t *s = &m_sprite[num];
  if (s->pos_x != x || s->pos_y != y) {
    s->pos_x = x;
    s->pos_y = y;
    qsort(m_sprites_ordered, MAX_SPRITES, sizeof(struct sprite_t *), cmp_sprite_y);
    m_bg_modified = true;
  }
}

void ESP32GFX::mapBgTile(uint8_t bg_idx, uint8_t from, uint8_t to)
{
  struct bg_t *bg = &m_bg[bg_idx];
  if (!bg->tile_map) {
    bg->tile_map = (uint8_t *)malloc(256);
    for (int i = 0; i < 256; ++i)
      bg->tile_map[i] = i;
  }
  bg->tile_map[from] = to;
}

void ESP32GFX::setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t)
{
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

void ESP32GFX::setBgTiles(uint8_t bg_idx, uint16_t x, uint16_t y, const uint8_t *tiles, int count)
{
  struct bg_t *bg = &m_bg[bg_idx];

  int off = (y % bg->h) * bg->w;
  for (int xx = x; xx < x+count; ++xx) {
    uint8_t t = *tiles++;
    if (bg->tile_map)
      t = bg->tile_map[t];
    bg->tiles[off + (xx % bg->w)] = t;
  }
  m_bg_modified = true;
}

void ESP32GFX::spriteTileCollision(uint8_t sprite, uint8_t bg_idx, uint8_t *tiles, uint8_t num_tiles)
{
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

  // Calculate offset, width and height in tiles.
  // round down top/left
  int bg_first_tile_off = bg_left_x / tsx + bg->w * (bg_top_y / tsy);
  // round up width/height
  int bg_tile_width = (spr->p.w + tsx - 1) / tsx;
  int bg_tile_height =  (spr->p.h + tsy - 1) / tsy;
  
  int bg_last_tile_off = bg_first_tile_off + bg_tile_width + bg_tile_height * bg->w;
  
  // Iterate over all tiles overlapping the sprite and record in what way they are
  // overlapping.
  // For every line
  for (int t = bg_first_tile_off, ty = bg_top_y; t < bg_last_tile_off; t += bg->w, ty += tsy) {
    // For every column
    for (int tt = t, tx = bg_left_x; tt < t + bg->w; ++tt, tx += tsx) {
      // For every tile code to be checked
      for (int m = 0; m < num_tiles; ++m) {
        if (tiles[m] == bg->tiles[tt % (bg->w*bg->h)]) {
          res[m] = 0x40;	// indicates collision in general
          if (tx < spr->pos_x)
            res[m] |= psxLeft;
          else if (tx + tsx > spr->pos_x + spr->p.w)
            res[m] |= psxRight;
          if (ty < spr->pos_y)
            res[m] |= psxUp;
          else if (ty + tsy > spr->pos_y + spr->p.h)
            res[m] |= psxDown;
        }
      }
    }
  }
  os_memcpy(tiles, res, num_tiles);
}

uint8_t ESP32GFX::spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile)
{
  spriteTileCollision(sprite, bg, &tile, 1);
  return tile;
}

uint8_t GROUP(basic_esp32gfx) ESP32GFX::spriteCollision(uint8_t collidee, uint8_t collider)
{
  uint8_t dir = 0x40;	// indicates collision

  sprite_t *us = &m_sprite[collidee];
  sprite_t *them = &m_sprite[collider];
  
  if (us->pos_x + us->p.w < them->pos_x)
    return 0;
  if (them->pos_x + them->p.w < us->pos_x)
    return 0;
  if (us->pos_y + us->p.h < them->pos_y)
    return 0;
  if (them->pos_y + them->p.h < us->pos_y)
    return 0;
  
  // sprite frame as bounding box; we may want something more flexible...
  if (them->pos_x < us->pos_x)
    dir |= psxLeft;
  else if (them->pos_x + them->p.w > us->pos_x + us->p.w)
    dir |= psxRight;

  sprite_t *upper = us, *lower = them;
  if (them->pos_y < us->pos_y) {
    dir |= psxUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them->p.h > us->pos_y + us->p.h)
    dir |= psxDown;

#if 0
  // Check for pixels in overlapping area.
  bool really = false;
  for (int y = lower->pos_y - upper->pos_y; y < upper->p.h; ++y) {
    // Check if both sprites have any pixels in the lines they overlap in.
    int lower_py = y - lower->pos_y + upper->pos_y;
    int upper_len, upper_off, lower_len, lower_off;
    if (upper->pat) {
      sprite_line *upper_line = &upper->pat->lines[y];
      upper_len = upper_line->len;
      upper_off = upper_line->off;
    } else {
      upper_len = upper->p.w;
      upper_off = 0;
    }
    if (lower->pat) {      
      sprite_line *lower_line = &lower->pat->lines[lower_py];
      lower_len = lower_line->len;
      lower_off = lower_line->off;
    } else {
      lower_len = lower->p.w;
      lower_off = 0;
    }
    if (upper_len && lower_len) {
      int dist_x = abs(upper->pos_x - lower->pos_x);
      int left_len, left_off, right_off;
      if (upper->pos_x < lower->pos_x) {
        left_len = upper_len;
        left_off = upper_off;
        right_off = lower_off;
      } else {
        left_len = lower_len;
        left_off = lower_off;
        right_off = upper_off;
      }
      if (left_off + left_len >= right_off + dist_x) {
        really = true;
        break;
      }
    }
  }
  if (!really)
    return 0;
#endif

  return dir;
}
#endif	// USE_BG_ENGINE

#endif	// ESP32
