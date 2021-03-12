// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifdef H3

#include <Arduino.h>
#include "h3gfx.h"
#include "colorspace.h"
#include <joystick.h>
#include <mmu.h>

H3GFX vs23;

#define FILTER_OFF	(0 << 0)
#define FILTER_ON	(1 << 0)

#define ASPECT_4_3	(0 << 1)
#define ASPECT_16_9	(1 << 1)

const struct video_mode_t H3GFX::modes_pal[H3_SCREEN_MODES] = {
  { 460, 224, 0, 0, ASPECT_4_3 },
  { 436, 216, 0, 0, ASPECT_4_3 },
  { 320, 216, 0, 0, ASPECT_4_3 },   // VS23 NTSC demo
  { 320, 200, 0, 0, ASPECT_4_3 },   // (M)CGA, Commodore et al.
  { 256, 224, 0, 0, ASPECT_4_3 },  // SNES
  { 256, 192, 0, 0, ASPECT_4_3 },  // MSX, Spectrum, NDS
  { 160, 200, 0, 0, ASPECT_4_3 },   // Commodore/PCjr/CPC
                             // multi-color
  // "Overscan modes"
  { 352, 240, 0, 0, ASPECT_4_3 },  // PCE overscan (barely)
  { 282, 240, 0, 0, ASPECT_4_3 },  // PCE overscan (underscan on PAL)
  { 508, 240, 0, 0, ASPECT_4_3 },
  // ESP32GFX modes
  { 320, 256, 0, 0, ASPECT_4_3 },  // maximum PAL at 2 clocks per pixel
  { 320, 240, 0, 0, ASPECT_4_3 },  // DawnOfAV demo, Mode X
  { 640, 256, 0, 0, ASPECT_4_3 },
  // default H3 mode
  { 480, 270, 0, 0, ASPECT_4_3 },
  // default SDL mode
  { 640, 480, 0, 0, ASPECT_4_3 | FILTER_ON },
  { 800, 600, 0, 0, ASPECT_4_3 | FILTER_ON },
  { 1024, 768, 0, 0, ASPECT_4_3 | FILTER_ON },
  { 1280, 720, 0, 0, ASPECT_16_9 | FILTER_ON },
  { 1280, 1024, 0, 0, ASPECT_4_3 | FILTER_ON },
  { 1920, 1080, 0, 0, ASPECT_16_9 },
};

#include <usb.h>
#include <config.h>
void H3GFX::begin(bool interlace, bool lowpass, uint8_t system) {
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  m_pixels = NULL;
  setMode(CONFIG.mode - 1);

  m_bin.Init(0, 0);

  m_frame = 0;
  reset();

  m_display_enabled = true;
}

void H3GFX::reset() {
  BGEngine::reset();
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x * 4);
  setColorSpace(0);
}

void H3GFX::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                     uint16_t y_dst, uint16_t width, uint16_t height) {
  // optimize the common case of scrolling the whole screen
  if (x_src == 0 && x_dst == 0 &&		// starts at the end of the line
      y_src > y_dst &&				// goes upwards
      width == m_current_mode.x &&		// whole screen width
      y_src + height <= m_current_mode.y	// does not go beyond the real framebuffer
  ) {
    memcpy(m_pixels[y_dst] - m_current_mode.left,
           m_pixels[y_src] - m_current_mode.left,
           height * (m_current_mode.x + m_current_mode.left * 2) * sizeof(pixel_t));
    return;
  }

  if ((y_dst > y_src) ||
      (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  }

  mmu_flush_dcache();
}

static uint32_t *backbuffer = 0;

void H3GFX::resetLinePointers() {
  for (int i = 0; i < m_current_mode.y; ++i) {
    m_pixels[i] = (pixel_t *)display_active_buffer +
                  (m_current_mode.x + m_current_mode.left * 2) *
                          (i + m_current_mode.top) +
                  m_current_mode.left;
  }
}

bool H3GFX::setMode(uint8_t mode) {
  m_display_enabled = false;

  free(m_pixels);
  free(backbuffer);

  m_current_mode = modes_pal[mode];

  // Find a suitable border size. For unfiltered modes we want it to be scaled by
  // an integral factor in both directions, and keep the aspect ratio as
  // close as possible. For filtered modes, we want the aspect ratio to be
  // precise.
  switch (m_current_mode.vclkpp) {
  case ASPECT_4_3 | FILTER_OFF:
    if (m_force_filter ||
        (DISPLAY_HDMI_RES_X / m_current_mode.x >= 3 &&
         DISPLAY_HDMI_RES_Y / m_current_mode.y >= 3)) {
      m_current_mode.top = 0;
      m_current_mode.left = 0.1666667d * m_current_mode.x;	// pillar-boxing
    } else {
      // find an integral scale factor
      int yscale = DISPLAY_HDMI_RES_Y / m_current_mode.y;			// scale, rounded down
      m_current_mode.top = (DISPLAY_HDMI_RES_Y - yscale * m_current_mode.y)	// pixels to add to fill up the screen
                           / yscale					// correct for scaling by video driver
                           / 2;						// only one side, the other side is implicit
      int xscale = DISPLAY_HDMI_RES_X / m_current_mode.x / 1.333333d;	// scale corrected for aspect ratio, rounded down
      m_current_mode.left = (DISPLAY_HDMI_RES_X - xscale * m_current_mode.x)
                           / xscale
                           / 2;
    }
    display_enable_filter(m_force_filter);
    break;
  case ASPECT_4_3 | FILTER_ON:
    m_current_mode.top = 0;
    m_current_mode.left = 0.1666667d * m_current_mode.x;	// pillar-boxing
    display_enable_filter(true);
    break;
  case ASPECT_16_9 | FILTER_OFF:
    if (m_force_filter) {
      m_current_mode.top = 0;
      m_current_mode.left = 0;
    } else {
      int yscale = DISPLAY_HDMI_RES_Y / m_current_mode.y;		// scale, rounded down
      m_current_mode.top = (DISPLAY_HDMI_RES_Y - yscale * m_current_mode.y) / yscale / 2;
      int xscale = DISPLAY_HDMI_RES_X / m_current_mode.x;		// scale, rounded down
      m_current_mode.left = (DISPLAY_HDMI_RES_X - xscale * m_current_mode.x) / xscale / 2;
    }
    display_enable_filter(m_force_filter);
    break;
  case ASPECT_16_9 | FILTER_ON:
    m_current_mode.top = 0;
    m_current_mode.left = 0;
    display_enable_filter(true);
    break;
  default:
    break;
  }

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(524288 / m_current_mode.x,
                     m_current_mode.y + m_current_mode.y / MIN_FONT_SIZE_Y);

  m_pixels = (uint32_t **)malloc(sizeof(*m_pixels) * m_last_line);

  backbuffer = (uint32_t *)calloc(sizeof(pixel_t),
                                  m_current_mode.x * (m_last_line - m_current_mode.y));

  for (int i = m_current_mode.y; i < m_last_line; ++i) {
    m_pixels[i] = backbuffer + m_current_mode.x * (i - m_current_mode.y);
  }

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  display_set_mode(m_current_mode.x + m_current_mode.left * 2,
                   m_current_mode.y + m_current_mode.top * 2,
                   0, 0);
  display_single_buffer = true;
  display_swap_buffers();

  resetLinePointers();

  m_display_enabled = true;

  return true;
}

//#define PROFILE_BG

inline void H3GFX::blitBuffer(void *buf) {
  memcpy((void *)display_active_buffer, buf,
         (m_current_mode.x + m_current_mode.left * 2) *
         (m_current_mode.y + m_current_mode.top * 2) *
         sizeof(pixel_t));
}

void H3GFX::updateStatus() {
  bool enabled = false;
  for (int i = 0; i < MAX_BG; ++i) {
    if (bgEnabled(i))
      enabled = true;
  }
  if (enabled != m_engine_enabled) {
    if (m_engine_enabled) {
      // We're running multi-buffered and are switching to single-buffered mode.
      // We need to copy what is currently visible to the single buffer if they
      // are not the same.
      void *latest_content = (void *)display_visible_buffer;
      display_single_buffer = true;
      display_swap_buffers();
      if (display_active_buffer != latest_content) {
        blitBuffer(latest_content);
      }
    } else {
      // We're running single-buffered and are switching to multi-buffered mode.
      // We need to copy the single buffer to all buffers to avoid flickering.
      void *latest_content = (void *)display_visible_buffer;
      display_single_buffer = false;
      display_swap_buffers();
      blitBuffer(latest_content);
    }
    resetLinePointers();
  }
  m_engine_enabled = enabled;
}

void H3GFX::updateBg() {
  static uint32_t last_frame = 0;
  static int post_render_count = 0;

  if (frame() <= last_frame + m_frameskip)
    return;

  last_frame = frame();

  if (!m_bg_modified) {
    if (display_single_buffer) {
      mmu_flush_dcache();
      return;
    }
    // need to render a few extra frames to make sure we can actually see
    // the current state
    if (post_render_count > 2)
      return;
    post_render_count++;
  }

  if (m_dupe_active) {
    if (!display_single_buffer) {
      void *active = (void *)display_active_buffer;
      display_swap_buffers();
      blitBuffer(active);
      display_swap_buffers();
      m_dupe_active = false;
    }
    m_dupe_active = false;
  }

  m_bg_modified = false;
  post_render_count = 0;

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
          memcpy(&m_pixels[y + owy][x + owx], &m_pixels[t_y][t_x], tsx * 4);
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

  display_swap_buffers();
  resetLinePointers();
}

#ifdef USE_BG_ENGINE
uint8_t H3GFX::spriteCollision(uint8_t collidee, uint8_t collider) {
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
      pixel_t leftpixel = m_pixels[leftpy][leftpx];
      pixel_t rightpixel = m_pixels[rightpy][rightpx];

      if (leftpixel != left->p.key && rightpixel != right->p.key)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}
#endif	// USE_BG_ENGINE

#endif	// H3
