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
    switch (m_current_mode.vclkpp) {
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
  BGEngine::reset();
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x);
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

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

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
