// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifdef ESP32

#include <Arduino.h>
#include <soc/rtc.h>
#include "esp32gfx.h"
#include "colorspace.h"
#include "joystick.h"

ESP32GFX vs23;

static void pal_core(void *data) {
  for (;;)
    vs23.render();
}

void ESP32GFX::render() {
  if (!m_display_enabled)
    delay(16);
  else {
    switch (m_current_mode.vclkpp) {
    case 1:  m_pal.sendFrame1ppc( &m_current_mode, m_pixels); break;
    case 2:  m_pal.sendFrame(     &m_current_mode, m_pixels); break;
    case 4:  m_pal.sendFrame4ppc( &m_current_mode, m_pixels); break;
    case 25: m_pal.sendFrame2pp5c(&m_current_mode, m_pixels); break;
    default: break;
    }
  }
  m_frame++;
}

#include "config.h"

void ESP32GFX::begin(bool interlace, bool lowpass, uint8_t system) {
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  m_pixels = NULL;
  setMode(CONFIG.mode - 1);

  m_bin.Init(0, 0);

  m_frame = 0;
  m_pal.init();
  reset();
  xTaskCreatePinnedToCore(pal_core, "c", 1024, NULL, 2, NULL, 0);
  // Ugly hack: We have disabled heap allocator locking in the SDK because
  // it causes temporary display corruption by disabling interrupts during
  // allocation.  This is not normally a problem because the PRO core is
  // busy creating the video signal and thus does not pose a threat for the
  // heap, but (I guess) the OS allocates memory when starting that task.
  // We therefore do this in a feeble attempt to avoid interfering with it:
  delay(16);

  m_display_enabled = true;
}

void ESP32GFX::reset() {
  BGEngine::reset();
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x);
  setColorSpace(0);
}

void ESP32GFX::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                        uint16_t y_dst, uint16_t width, uint16_t height) {
  if ((y_dst > y_src) ||
      (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
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
bool ESP32GFX::setMode(uint8_t mode) {
  m_display_enabled = false;
  delay(16);

  for (int i = 0; i < m_last_line; ++i) {
    if (m_pixels[i])
      free(m_pixels[i]);
  }
  free(m_pixels);

  m_current_mode = modes_pal[mode];
  m_pal.setMode(m_current_mode);

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(131072 / m_current_mode.x,
                     m_current_mode.y + m_current_mode.y / MIN_FONT_SIZE_Y);

  m_pixels = (pixel_t **)pvPortMallocCaps(sizeof(*m_pixels) * m_last_line,
                                          MALLOC_CAP_32BIT);
  for (int i = 0; i < m_last_line; ++i) {
    m_pixels[i] = (pixel_t *)calloc(sizeof(pixel_t), m_current_mode.x);
  }

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  m_display_enabled = true;

  return true;
}

//#define PROFILE_BG

void IRAM_ATTR __attribute__((optimize("O3"))) ESP32GFX::updateBg() {
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

    // start/end coordinates of the visible BG window, relative to the
    // BG's origin, in pixels
    int sx = bg->scroll_x;
    int ex = bg->win_w + bg->scroll_x;
    int sy = bg->scroll_y;
    int ey = bg->win_h + bg->scroll_y;

    // offset to add to BG-relative coordinates to get screen coordinates
    int owx = -sx + bg->win_x;
    int owy = -sy + bg->win_y;

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
          // can draw a whole tile line
          memcpy(&m_pixels[y + owy][x + owx], &m_pixels[t_y][t_x], tsx);
          x += tsx;
          tile_x++;
          goto next;
        } else {
          m_pixels[y + owy][x + owx] = m_pixels[t_y][t_x];
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
      dx = -1;
      offx = s->p.w - 1;
    } else {
      dx = 1;
      offx = 0;
    }
    int dy, offy;
    if (s->p.flip_y) {
      dy = -1;
      offy = s->p.h - 1;
    } else {
      dy = 1;
      offy = 0;
    }

    // sprite pattern start coordinates
    int px = s->p.pat_x + s->p.frame_x * s->p.w + offx;
    int py = s->p.pat_y + s->p.frame_y * s->p.h + offy;

    for (int y = 0; y != s->p.h; ++y) {
      int yy = y + s->pos_y;
      if (yy < 0 || yy >= height())
        continue;
      for (int x = 0; x != s->p.w; ++x) {
        int xx = x + s->pos_x;
        if (xx < 0 || xx >= width())
          continue;
        pixel_t p = m_pixels[py + y * dy][px + x * dx];
        // draw only non-keyed pixels
        if (p != s->p.key)
          m_pixels[yy][xx] = p;
      }
    }
  }
}

#ifdef USE_BG_ENGINE
uint8_t GROUP(basic_esp32gfx) ESP32GFX::spriteCollision(uint8_t collidee,
                                                        uint8_t collider) {
  uint8_t dir = 0x40;  // indicates collision

  const sprite_t *us = &m_sprite[collidee];
  const sprite_t *them = &m_sprite[collider];

  if (us->pos_x + us->p.w < them->pos_x)
    return 0;
  if (them->pos_x + them->p.w < us->pos_x)
    return 0;
  if (us->pos_y + us->p.h < them->pos_y)
    return 0;
  if (them->pos_y + them->p.h < us->pos_y)
    return 0;

  // sprite frame as bounding box; we may want something more flexible...
  const sprite_t *left = us, *right = them;
  if (them->pos_x < us->pos_x) {
    dir |= joyLeft;
    left = them;
    right = us;
  } else if (them->pos_x + them->p.w > us->pos_x + us->p.w)
    dir |= joyRight;

  const sprite_t *upper = us, *lower = them;
  if (them->pos_y < us->pos_y) {
    dir |= joyUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them->p.h > us->pos_y + us->p.h)
    dir |= joyDown;

  // Check for pixels in overlapping area.
  const int leftpatx = left->p.pat_x + left->p.frame_x * left->p.w;
  const int leftpaty = left->p.pat_y + left->p.frame_y * left->p.h;
  const int rightpatx = right->p.pat_x + right->p.frame_x * right->p.w;
  const int rightpaty = right->p.pat_y + right->p.frame_y * right->p.h;

  for (int y = lower->pos_y;
       y < _min(lower->pos_y + lower->p.h, upper->pos_y + upper->p.h);
       y++) {
    int leftpy = leftpaty + y - left->pos_y;
    int rightpy = rightpaty + y - right->pos_y;

    for (int x = right->pos_x;
         x < _min(right->pos_x + right->p.w, left->pos_x + left->p.w);
         x++) {
      int leftpx = leftpatx + x - left->pos_x;
      int rightpx = rightpatx + x - right->pos_x;
      int leftpixel = m_pixels[leftpy][leftpx];
      int rightpixel = m_pixels[rightpy][rightpx];

      if (leftpixel != left->p.key && rightpixel != right->p.key)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}
#endif  // USE_BG_ENGINE

#endif  // ESP32
