// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifdef H3

#include <Arduino.h>
#include "h3gfx.h"
#include "colorspace.h"
#include <joystick.h>
#include <mmu.h>
#include <smp.h>

H3GFX vs23;

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
  { 640, 480, 0, 0, ASPECT_4_3 },
  { 800, 600, 0, 0, ASPECT_4_3 },
  { 1024, 768, 0, 0, ASPECT_4_3 },
  { 1280, 720, 0, 0, ASPECT_16_9 },
  { 1280, 1024, 0, 0, ASPECT_4_3 },
  { 1920, 1080, 0, 0, ASPECT_16_9 },
};

const struct display_phys_mode_t phys_modes[] = {
  { 148500000, 1920,  88,  44, 148, 0, 1080,  4, 5, 36, 0, 1 },
  { 108000000, 1280,  48, 112, 248, 0, 1024,  1, 3, 38, 0, 0 },
  {  74250000, 1280, 110,  40, 220, 0,  720,  5, 5, 20, 0, 1 },
  {  65000000, 1024,  24, 136, 160, 0,  768,  3, 6, 29, 0, 0 },
  {  40000000,  800,  40, 128,  88, 0,  600,  1, 4, 23, 0, 0 },
  {  25175000,  640,  16,  96,  48, 1,  480, 10, 2, 33, 1, 0 },
  { 193160000, 1920, 128, 208, 336, 0, 1200,  1, 3, 38, 0, 1 },
};

#define DISPLAY_CORE	1
#define DISPLAY_CORE_STACK_SIZE	0x1000
static void *display_core_stack;

extern "C" void task_updatebg(void) {
  for (;;) {
    smp_wait_for_event();
    vs23.updateBgTask();
  }
}

void hook_display_vblank(void) {
  smp_send_event();
}

#include <usb.h>
#include <config.h>
void H3GFX::begin(bool interlace, bool lowpass, uint8_t system) {
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  m_pixels = NULL;
  m_bgpixels = NULL;
  m_textmode_buffer = NULL;
  m_offscreenbuffer = NULL;

  const struct display_phys_mode_t *phys_mode;
  if (CONFIG.phys_mode < sizeof(phys_modes) / sizeof(*phys_modes))
    phys_mode = &phys_modes[CONFIG.phys_mode];
  else
    phys_mode = &phys_modes[0];

  if (!display_is_digital)
    tve_init(CONFIG.NTSC == 0 ? TVE_NORM_NTSC : TVE_NORM_PAL);

  display_init(phys_mode);

  setMode(CONFIG.mode - 1);

  m_bin.Init(0, 0);

  m_frame = 0;
  reset();

  display_core_stack = malloc(DISPLAY_CORE_STACK_SIZE);
  if (!display_core_stack) {
    // Hopeless.
    printf("OOM in display initialization, giving up\n");
    for (;;);
  }

  m_buffer_lock = false;
  m_display_enabled = true;
  m_engine_enabled = false;
  m_bg_modified = false;
  m_textmode_buffer_modified = false;

  smp_start_secondary_core(DISPLAY_CORE, task_updatebg,
                           display_core_stack, DISPLAY_CORE_STACK_SIZE);
}

void H3GFX::reset() {
  BGEngine::reset();
  // XXX: does this make sense?
  for (int i = 0; i < m_last_line; ++i)
    memset(m_pixels[i], 0, m_current_mode.x * sizeof(pixel_t));
  setColorSpace(0);
}

//#define PROFILE_BLIT
void H3GFX::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                     uint16_t y_dst, uint16_t width, uint16_t height) {
#ifdef PROFILE_BLIT
  uint32_t s = micros();
#endif
  if (y_dst == y_src && x_dst > x_src) {
    while (height) {
      memmove(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  } else if ((y_dst > y_src) ||
      (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memcpy(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memcpy(m_pixels[y_dst] + x_dst, m_pixels[y_src] + x_src, width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  }
#ifdef PROFILE_BLIT
  uint32_t e = micros();
  if (e - s > 1000)
    printf("blit %d us\n", e - s);
#endif
  mmu_flush_dcache();
}

void H3GFX::resetLinePointers(pixel_t **pixels, pixel_t *buffer) {
  for (int i = 0; i < m_current_mode.y; ++i) {
    pixels[i] = buffer +
                  (m_current_mode.x + m_current_mode.left * 2) *
                          (i + m_current_mode.top) +
                  m_current_mode.left;
  }
}

bool H3GFX::setMode(uint8_t mode) {
  m_display_enabled = false;

  free(m_pixels);
  free(m_textmode_buffer);
  free(m_offscreenbuffer);

  m_current_mode = modes_pal[mode];

  // Find a suitable border size. For unfiltered modes we want it to be scaled by
  // an integral factor in both directions, and keep the aspect ratio as
  // close as possible. For filtered modes, we want the aspect ratio to be
  // precise.
  if ((double)DISPLAY_PHYS_RES_X / (double)DISPLAY_PHYS_RES_Y > 1.55) {
    // widescreen
    switch (m_current_mode.vclkpp) {
    case ASPECT_4_3:
      if (m_force_filter ||
          (DISPLAY_PHYS_RES_X / m_current_mode.x >= 3 &&
           DISPLAY_PHYS_RES_Y / m_current_mode.y >= 3)) {
        m_current_mode.top = 0;
        m_current_mode.left = 0.1666667d * m_current_mode.x;	// pillar-boxing
      } else {
        // find an integral scale factor
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;			// scale, rounded down
        m_current_mode.top = (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y)	// pixels to add to fill up the screen
                             / yscale					// correct for scaling by video driver
                             / 2;						// only one side, the other side is implicit
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x / 1.333333d;	// scale corrected for aspect ratio, rounded down
        m_current_mode.left = (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x)
                             / xscale
                             / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    case ASPECT_16_9:
      if (m_force_filter) {
        m_current_mode.top = 0;
        m_current_mode.left = 0;
      } else {
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;		// scale, rounded down
        m_current_mode.top = (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y) / yscale / 2;
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x;		// scale, rounded down
        m_current_mode.left = (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x) / xscale / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    default:
      break;
    }
  } else {
    // not so widescreen
    switch (m_current_mode.vclkpp) {
    case ASPECT_16_9:
      if (m_force_filter ||
          (DISPLAY_PHYS_RES_X / m_current_mode.x >= 3 &&
           DISPLAY_PHYS_RES_Y / m_current_mode.y >= 3)) {
        m_current_mode.top = 0.1666667d * m_current_mode.y;	// letter-boxing
        m_current_mode.left = 0;
      } else {
        // find an integral scale factor
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;			// scale, rounded down
        m_current_mode.top = (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y)	// pixels to add to fill up the screen
                             / yscale					// correct for scaling by video driver
                             / 2;						// only one side, the other side is implicit
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x / 1.777777d;	// scale corrected for aspect ratio, rounded down
        m_current_mode.left = (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x)
                             / xscale
                             / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    case ASPECT_4_3:
      if (m_force_filter) {
        m_current_mode.top = 0;
        m_current_mode.left = 0;
      } else {
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;		// scale, rounded down
        m_current_mode.top = (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y) / yscale / 2;
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x;		// scale, rounded down
        m_current_mode.left = (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x) / xscale / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    default:
      break;
    }
  }

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(524288 / m_current_mode.x,
                     m_current_mode.y + m_current_mode.y / MIN_FONT_SIZE_Y);

  m_pixels = (uint32_t **)malloc(sizeof(*m_pixels) * m_last_line);
  m_bgpixels = (uint32_t **)malloc(sizeof(*m_bgpixels) * m_last_line);

  m_textmode_buffer = (uint32_t *)calloc(sizeof(pixel_t),
                                  (m_current_mode.x + m_current_mode.left * 2) * (m_last_line + m_current_mode.top * 2));
  m_offscreenbuffer = (uint32_t *)calloc(sizeof(pixel_t), m_current_mode.x * (m_last_line - m_current_mode.y));

  for (int i = m_current_mode.y; i < m_last_line; ++i) {
    m_pixels[i] = m_offscreenbuffer + m_current_mode.x * (i - m_current_mode.y);
    m_bgpixels[i] = m_offscreenbuffer + m_current_mode.x * (i - m_current_mode.y);
  }

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  display_set_mode(m_current_mode.x + m_current_mode.left * 2,
                   m_current_mode.y + m_current_mode.top * 2,
                   0, 0);
  display_single_buffer = true;
  display_swap_buffers();

  resetLinePointers(m_pixels, (pixel_t *)display_active_buffer);
  resetLinePointers(m_bgpixels, (pixel_t *)display_active_buffer);

  m_display_enabled = true;

  return true;
}

//#define PROFILE_BG

inline void H3GFX::blitBuffer(pixel_t *dst, pixel_t *buf) {
  memcpy((void *)dst, buf,
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
  if (!enabled) {
    for (int i = 0; i < MAX_SPRITES; ++i) {
      if (spriteEnabled(i)) {
        enabled = true;
        break;
      }
    }
  }

  if (enabled != m_engine_enabled) {
    spin_lock(&m_buffer_lock);
    if (m_engine_enabled) {
      // We're running multi-buffered and are switching to single-buffered mode.
      // We need to copy what is in the backbuffer to the visible buffer.
      display_single_buffer = true;
      display_swap_buffers();
      resetLinePointers(m_pixels, (pixel_t *)display_visible_buffer);
      blitBuffer((pixel_t *)display_visible_buffer, m_textmode_buffer);
    } else {
      // We're running single-buffered and are switching to multi-buffered mode.
      pixel_t *latest_content = (pixel_t *)display_visible_buffer;
      display_single_buffer = false;
      display_swap_buffers();
      resetLinePointers(m_pixels, m_textmode_buffer);
      resetLinePointers(m_bgpixels, (pixel_t *)display_active_buffer);
      blitBuffer(m_textmode_buffer, latest_content);
    }
    mmu_flush_dcache();
    m_engine_enabled = enabled;
    spin_unlock(&m_buffer_lock);
  }
}

void H3GFX::updateBgTask() {
  static uint32_t last_frame = 0;

  if (tick_counter <= last_frame + m_frameskip)
    return;

  last_frame = tick_counter;

  if (!m_bg_modified && (!m_textmode_buffer_modified || display_single_buffer)) {
    m_frame++;
    smp_send_event();
    return;
  }

  spin_lock(&m_buffer_lock);
  // While we were waiting for the lock, the BG engine might have been
  // turned off, and we are not allowed to draw to the framebuffer any
  // longer.
  if (!m_engine_enabled) {
    m_frame++;
    spin_unlock(&m_buffer_lock);
    smp_send_event();
    return;
  }

  // Text screen layer goes at the bottom.
  m_textmode_buffer_modified = false;
  blitBuffer((pixel_t *)display_active_buffer, m_textmode_buffer);

  m_bg_modified = false;

#ifdef PROFILE_BG
  uint32_t start = micros();
#endif
  for (int prio = 0; prio <= MAX_PRIO; ++prio) {
    for (int b = 0; b < MAX_BG; ++b) {
      bg_t *bg = &m_bg[b];
      if (!bg->enabled || bg->prio != prio)
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
            memcpy(&m_bgpixels[y + owy][x + owx], &m_pixels[t_y][t_x], tsx * sizeof(pixel_t));
            x += tsx;
            tile_x++;
            goto next;
          } else {
            m_bgpixels[y + owy][x + owx] = m_pixels[t_y][t_x];
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
      if (!s->enabled || s->prio != prio)
        continue;
      if (s->pos_x + s->p.w < 0 ||
          s->pos_x >= width() ||
          s->pos_y + s->p.h < 0 ||
          s->pos_y >= height())
        continue;

      // sprite pattern start coordinates
      int px = s->p.pat_x + s->p.frame_x * s->p.w;
      int py = s->p.pat_y + s->p.frame_y * s->p.h;

      if (s->must_reload || !s->surf) {
        // XXX: do this asynchronously
        if (s->surf)
          delete s->surf;

        int pitch = m_current_mode.x;
        if (py < m_current_mode.y)
          pitch += m_current_mode.left * 2;

        rz_surface_t in(s->p.w, s->p.h, (uint32_t*)(&m_pixels[py][px]),
                        pitch * sizeof(pixel_t), s->p.key);

        rz_surface_t *out = rotozoomSurfaceXY(&in, s->angle,
                                              s->p.flip_x ? -s->scale_x : s->scale_x,
                                              s->p.flip_y ? -s->scale_y : s->scale_y, 0);
        s->surf = out;
        s->must_reload = false;
      }

      for (int y = 0; y != s->surf->h; ++y) {
        int yy = y + s->pos_y;
        if (yy < 0 || yy >= height())
          continue;
        for (int x = 0; x != s->surf->w; ++x) {
          int xx = x + s->pos_x;
          if (xx < 0 || xx >= width())
            continue;
          pixel_t p = s->surf->getPixel(x, y);
          // draw only non-keyed pixels
          if (p != s->p.key)
            m_bgpixels[yy][xx] = p;
        }
      }
    }
  }

  // Not doing this produces a nice distortion effect...
  mmu_flush_dcache();

  display_swap_buffers();
  resetLinePointers(m_bgpixels, (pixel_t *)display_active_buffer);

  spin_unlock(&m_buffer_lock);

  m_frame++;
  smp_send_event();
}

void H3GFX::updateBg() {
  if (!m_engine_enabled)
    mmu_flush_dcache();	// commit single-buffer renderings to DRAM
}

#ifdef USE_BG_ENGINE
uint8_t H3GFX::spriteCollision(uint8_t collidee, uint8_t collider) {
  uint8_t dir = 0x40;  // indicates collision

  const sprite_t *us = &m_sprite[collidee];
  const rz_surface_t *us_surf = m_sprite[collidee].surf;
  const sprite_t *them = &m_sprite[collider];
  const rz_surface_t *them_surf = m_sprite[collider].surf;

  if (!us_surf || !them_surf)
    return 0;

  if (us->pos_x + us_surf->w < them->pos_x)
    return 0;
  if (them->pos_x + them_surf->w < us->pos_x)
    return 0;
  if (us->pos_y + us_surf->h < them->pos_y)
    return 0;
  if (them->pos_y + them_surf->h < us->pos_y)
    return 0;

  // sprite frame as bounding box; we may want something more flexible...
  const sprite_t *left = us, *right = them;
  rz_surface_t *left_surf, *right_surf;
  if (them->pos_x < us->pos_x) {
    dir |= joyLeft;
    left = them;
    right = us;
  } else if (them->pos_x + them_surf->w > us->pos_x + us_surf->w)
    dir |= joyRight;
  left_surf = left->surf;
  right_surf = right->surf;

  const sprite_t *upper = us, *lower = them;
  const rz_surface_t *upper_surf, *lower_surf;
  if (them->pos_y < us->pos_y) {
    dir |= joyUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them_surf->h > us->pos_y + us_surf->h)
    dir |= joyDown;
  upper_surf = upper->surf;
  lower_surf = lower->surf;

  // Check for pixels in overlapping area.
  pixel_t lkey, rkey;
  lkey = left->p.key;
  rkey = right->p.key;

  for (int y = lower->pos_y;
       y < _min(lower->pos_y + lower_surf->h, upper->pos_y + upper_surf->h);
       y++) {
    int leftpy = y - left->pos_y;
    int rightpy = y - right->pos_y;

    for (int x = right->pos_x;
         x < _min(right->pos_x + right_surf->w, left->pos_x + left_surf->w);
         x++) {
      int leftpx = x - left->pos_x;
      int rightpx = x - right->pos_x;
      pixel_t leftpixel = left_surf->getPixel(leftpx, leftpy);
      pixel_t rightpixel = right_surf->getPixel(rightpx, rightpy);

      if (leftpixel != lkey && rightpixel != rkey)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}
#endif	// USE_BG_ENGINE

#endif	// H3
