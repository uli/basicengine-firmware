// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifdef H3

#include <Arduino.h>
#include "h3gfx.h"
#include "colorspace.h"
#include <joystick.h>
#include <smp.h>

H3GFX vs23;

#define ASPECT_4_3  (0 << 1)
#define ASPECT_16_9 (1 << 1)

const struct video_mode_t H3GFX::modes_pal[H3_SCREEN_MODES] = {
  { 460, 224, 0, 0, ASPECT_4_3 },
  { 436, 216, 0, 0, ASPECT_4_3 },
  { 320, 216, 0, 0, ASPECT_4_3 },  // VS23 NTSC demo
  { 320, 200, 0, 0, ASPECT_4_3 },  // (M)CGA, Commodore et al.
  { 256, 224, 0, 0, ASPECT_4_3 },  // SNES
  { 256, 192, 0, 0, ASPECT_4_3 },  // MSX, Spectrum, NDS
  { 160, 200, 0, 0, ASPECT_4_3 },  // Commodore/PCjr/CPC
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
  { 480, 270, 0, 0, ASPECT_16_9 },
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

#define DISPLAY_CORE            1
#define DISPLAY_CORE_STACK_SIZE 0x1000
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
    for (;;)
      ;
  }

  m_buffer_lock = false;
  m_display_enabled = true;
  m_engine_enabled = false;
  m_bg_modified = false;
  m_textmode_buffer_modified = false;

  smp_start_secondary_core(DISPLAY_CORE, task_updatebg, display_core_stack,
                           DISPLAY_CORE_STACK_SIZE);
}

void H3GFX::reset() {
  BGEngine::reset();
  // XXX: does this make sense?
  for (int i = 0; i < m_last_line; ++i)
    memset(&pixelText(0, i), 0, m_current_mode.x * sizeof(pixel_t));
  setColorSpace(DEFAULT_COLORSPACE);
}

#define GFXCLASS H3GFX
#include "../gfx/blit.h"

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
      if (m_force_filter || (DISPLAY_PHYS_RES_X / m_current_mode.x >= 3 &&
                             DISPLAY_PHYS_RES_Y / m_current_mode.y >= 3)) {
        m_current_mode.top = 0;
        m_current_mode.left = 0.1666667d * m_current_mode.x;  // pillar-boxing
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
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;  // scale, rounded down
        m_current_mode.top =
                (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y) / yscale / 2;
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x;  // scale, rounded down
        m_current_mode.left =
                (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x) / xscale / 2;
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
      if (m_force_filter || (DISPLAY_PHYS_RES_X / m_current_mode.x >= 3 &&
                             DISPLAY_PHYS_RES_Y / m_current_mode.y >= 3)) {
        m_current_mode.top = 0.1666667d * m_current_mode.y;  // letter-boxing
        m_current_mode.left = 0;
      } else {
        // find an integral scale factor
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;			// scale, rounded down
        m_current_mode.top = (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y)	// pixels to add to fill up the screen
                             / yscale					// correct for scaling by video driver
                             / 2;						// only one side, the other side is implicit
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x / 1.777777d;	// scale corrected for aspect ratio, rounded down
        m_current_mode.left =
                (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x) / xscale / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    case ASPECT_4_3:
      if (m_force_filter) {
        m_current_mode.top = 0;
        m_current_mode.left = 0;
      } else {
        int yscale = DISPLAY_PHYS_RES_Y / m_current_mode.y;  // scale, rounded down
        m_current_mode.top =
                (DISPLAY_PHYS_RES_Y - yscale * m_current_mode.y) / yscale / 2;
        int xscale = DISPLAY_PHYS_RES_X / m_current_mode.x;  // scale, rounded down
        m_current_mode.left =
                (DISPLAY_PHYS_RES_X - xscale * m_current_mode.x) / xscale / 2;
      }
      display_enable_filter(m_force_filter);
      break;
    default:
      break;
    }
  }

  m_last_line = m_current_mode.y * 2;

  m_pixels = (uint32_t **)malloc(sizeof(*m_pixels) * m_last_line);
  m_bgpixels = (uint32_t **)malloc(sizeof(*m_bgpixels) * m_last_line);

  m_textmode_buffer =
          (uint32_t *)calloc(sizeof(pixel_t),
                             (m_current_mode.x + m_current_mode.left * 2) *
                                     (m_last_line + m_current_mode.top * 2));
  m_offscreenbuffer = (uint32_t *)calloc(
          sizeof(pixel_t), m_current_mode.x * (m_last_line - m_current_mode.y));

  for (int i = m_current_mode.y; i < m_last_line; ++i) {
    m_pixels[i] = m_offscreenbuffer + m_current_mode.x * (i - m_current_mode.y);
    m_bgpixels[i] = m_offscreenbuffer + m_current_mode.x * (i - m_current_mode.y);
  }

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  display_set_mode(m_current_mode.x + m_current_mode.left * 2,
                   m_current_mode.y + m_current_mode.top * 2, 0, 0);
  display_single_buffer = true;
  display_swap_buffers();

  resetLinePointers(m_pixels, (pixel_t *)display_active_buffer);
  resetLinePointers(m_bgpixels, (pixel_t *)display_active_buffer);

  m_display_enabled = true;

  return true;
}

// Blit a full screen's worth (including borders) from buf to dst.
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
    for (auto l : m_external_layers) {
      if (l.prio != -1)
        enabled = true;
    }
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
    cleanCache();
    m_engine_enabled = enabled;
    spin_unlock(&m_buffer_lock);
  }
}

//#define PROFILE_BG

#include <alpha-lib/overlay_alpha.h>

#define GFXCLASS H3GFX
#include <drawbg.h>

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

#ifdef PROFILE_BG
  uint32_t start = micros();
#endif

  // Text screen layer goes at the bottom.
  m_textmode_buffer_modified = false;
  blitBuffer((pixel_t *)display_active_buffer, m_textmode_buffer);

  m_bg_modified = false;

#ifdef PROFILE_BG
  uint32_t textblit = micros() - start;
#endif
  for (int prio = 0; prio <= MAX_PRIO; ++prio) {
    for (int b = 0; b < MAX_BG; ++b) {
      bg_t *bg = &m_bg[b];
      if (!bg->enabled || bg->prio != prio)
        continue;
      drawBg(bg);
    }
    for (auto l : m_external_layers) {
      if (l.prio == prio) {
        l.painter(&pixelComp(0, 0), m_current_mode.x, m_current_mode.y,
                  compositePitch(), l.userdata);
        m_bg_modified = true;
      }
    }

    for (int si = 0; si < MAX_SPRITES; ++si) {
      sprite_t *s = &m_sprite[si];
      // skip if not visible
      if (!s->enabled || s->prio != prio)
        continue;
      drawSprite(s);
    }
  }

#ifdef PROFILE_BG
  uint32_t background = micros() - start - textblit;
#endif

  // Not doing this produces a nice distortion effect...
  cleanCache();

  display_swap_buffers();
  resetLinePointers(m_bgpixels, (pixel_t *)display_active_buffer);

  spin_unlock(&m_buffer_lock);

#ifdef PROFILE_BG
  uint32_t finish = micros() - start - textblit - background;
  printf("textblit %lu background %lu finish %lu total %lu\n", textblit,
         background, finish, textblit + background + finish);
#endif

  m_frame++;
  smp_send_event();
}

void H3GFX::updateBg() {
  if (!m_engine_enabled)
    cleanCache();  // commit single-buffer renderings to DRAM
}

#ifdef USE_BG_ENGINE
#include "../gfx/spritecoll.h"
#endif  // USE_BG_ENGINE

#endif  // H3
