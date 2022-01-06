// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <Arduino.h>
#include "sdlgfx.h"
#include "colorspace.h"
#include <joystick.h>

SDLGFX vs23;

const struct video_mode_t SDLGFX::modes_pal[SDL_SCREEN_MODES] = {
  { 460, 224, 16, 16, 1 },
  { 436, 216, 16, 16, 1 },
  { 320, 216, 16, 16, 2 },   // VS23 NTSC demo
  { 320, 200, 0, 0, 2 },     // (M)CGA, Commodore et al.
  { 256, 224, 16, 16, 25 },  // SNES
  { 256, 192, (200 - 192) / 2, (320 - 256) / 2, 25 },  // MSX, Spectrum, NDS
  { 160, 200, 16, 16, 4 },   // Commodore/PCjr/CPC multi-color
  // "Overscan modes"
  { 352, 240, 16, 16, 2 },  // PCE overscan (barely)
  { 282, 240, 16, 16, 2 },  // PCE overscan (underscan on PAL)
  { 508, 240, 16, 16, 1 },
  // ESP32GFX modes
  { 320, 256, 16, 16, 2 },  // maximum PAL at 2 clocks per pixel
  { 320, 240, 0, 0, 2 },    // DawnOfAV demo, Mode X
  { 640, 256, (480 - 256) / 2, 0, 1 },
  // default H3 mode
  { 480, 270, 16, 16, 1 },
  // default SDL mode
  { 640, 480, 0, 0, 0 },
  { 800, 600, 0, 0, 0 },
  { 1024, 768, 0, 0, 0 },
  { 1280, 720, 0, 0, 0 },
  { 1280, 1024, 0, 0, 0 },
  { 1920, 1080, 0, 0, 0 },
};

int SDLGFX::modeFromSize(int &w, int &h) {
  int diff = INT_MAX;
  int best_mode = 0;
  for (int i = 0; i < sizeof(modes_pal) / sizeof(modes_pal[0]); ++i) {
    if (modes_pal[i].x < w || modes_pal[i].y < h)
      continue;

    int d = modes_pal[i].x - w + modes_pal[i].y - h;
    if (d < diff) {
      diff = d;
      best_mode = i;
    }
  }
  w = modes_pal[best_mode].x;
  h = modes_pal[best_mode].y;
  return best_mode;
}

void SDLGFX::modeSize(int m, int &w, int &h) {
  w = modes_pal[m].x;
  h = modes_pal[m].y;
}

extern const SDL_VideoInfo *sdl_info;
extern int sdl_flags;
extern bool sdl_keep_res;
extern int sdl_user_w, sdl_user_h;

#include <config.h>

extern "C" int gfx_thread(void *data);

void SDLGFX::begin(bool interlace, bool lowpass, uint8_t system) {
  m_display_enabled = false;
  m_last_line = 0;
  printf("set mode\n");
  m_current_mode = modes_pal[CONFIG.mode - 1];

  SDL_ShowCursor(SDL_DISABLE);
  setMode(CONFIG.mode - 1);

  m_bin.Init(0, 0);

  m_frame = 0;
  reset();

  m_scalecond = SDL_CreateCond();
  m_scalemut = SDL_CreateMutex();
  SDL_CreateThread(gfx_thread, "gfx_thread", this);

  m_lowpass = lowpass;

  m_display_enabled = true;
}

void SDLGFX::reset() {
  BGEngine::reset();
  setColorSpace(DEFAULT_COLORSPACE);
}

#define GFXCLASS SDLGFX
#include "../gfx/blit.h"

void SDLGFX::fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                      pixel_t color) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"	// I refuse to "fix" this.
  SDL_Rect dst = { (Sint16)x1, (Sint16)y1, x2 - x1, y2 - y1 };
#pragma GCC diagnostic pop
  SDL_FillRect(m_text_surface, &dst, color);
}

bool SDLGFX::setMode(uint8_t mode) {
  m_display_enabled = false;

  m_last_line = modes_pal[mode].y * 2;

  printf("newmode %d %d %d %d\n", modes_pal[mode].x + modes_pal[mode].left * 2,
         modes_pal[mode].y + modes_pal[mode].top * 2,
         modes_pal[mode].x + modes_pal[mode].left * 2,
         m_last_line);

  m_current_mode = modes_pal[mode];

  SDL_Rect **modes;
  modes = SDL_ListModes(sdl_info->vfmt, sdl_flags);

  int final_w = 0, final_h = 0;

  if (sdl_keep_res) {
    final_w = sdl_user_w;
    final_h = sdl_user_h;
  } else if (modes != NULL && modes != (SDL_Rect **)-1) {
    // First choice: Modes that are 1x or 2x the desired resolution.
    for (int i = 0; modes[i]; ++i) {
      int w = modes[i]->w, h = modes[i]->h;
      if ((w == m_current_mode.x && h == m_current_mode.y) ||
          (w == m_current_mode.x * 2 && h == m_current_mode.y * 2)) {
        final_w = w;
        final_h = h;
        break;
      }
    }

    // Second choice: Mode that is large enough and has the smallest
    // difference in size to the desired resolution.
    int min_diff = 10000;
    int max_w = 0, max_h = 0;
    if (final_w == 0) {
      for (int i = 0; modes[i]; ++i) {
        int w = modes[i]->w, h = modes[i]->h;
        if (w < m_current_mode.x || h < m_current_mode.y)
          continue;
        int diff = (w - m_current_mode.x) + (h - m_current_mode.y);
        if (diff < min_diff) {
          final_w = w;
          final_h = h;
          min_diff = diff;
        }
        if (max_w < w) {
          max_w = w;
          max_h = h;
        }
      }
    }

    // Still nothing -> we don't have a large enough mode. Try the widest.
    if (final_w == 0) {
      final_w = max_w;
      final_h = max_h;
    }
  } else {
    // No mode list -> everything goes.
    final_w = m_current_mode.x;
    final_h = m_current_mode.y;
  }

  m_screen = SDL_SetVideoMode(final_w, final_h, SDL_BPP, sdl_flags);

  if (m_text_surface)
    SDL_FreeSurface(m_text_surface);
  if (m_composite_surface)
    SDL_FreeSurface(m_composite_surface);

  SDL_PixelFormat *fmt = m_screen->format;

  m_text_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
    m_current_mode.x,
    m_last_line,
    32,
    0x000000ffUL,
    0x0000ff00UL,
    0x00ff0000UL,
    0xff000000UL
  );
  m_composite_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
    m_current_mode.x,
    m_last_line,
    32,
    0x000000ffUL,
    0x0000ff00UL,
    0x00ff0000UL,
    0
  );

  // XXX: handle fail

  //printf("last_line %d x %d y %d fs %d smp %d\n", m_last_line, m_current_mode.x, m_current_mode.y, MIN_FONT_SIZE_Y, sizeof(*m_pixels));

  setColorSpace(DEFAULT_COLORSPACE);

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  m_display_enabled = true;
  m_dirty = true;

  setBorder(0, 0, 0, m_current_mode.x);

  return true;
}

void SDLGFX::toggleFullscreen() {
  int flags = SDL_GetWindowFlags(sdl_window);
  if (flags & SDL_WINDOW_FULLSCREEN) {
    SDL_SetWindowFullscreen(sdl_window, 0);
  } else {
    if (sdl_flags & SDL_WINDOW_FULLSCREEN)
      SDL_SetWindowFullscreen(sdl_window, sdl_flags);
    else
      SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
}

#include <border_pal.h>

void SDLGFX::setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w) {
  // couldn't find a working transformation, using a palette
  // generated by VS23Palette06.exe
  y /= 4;  // palette uses 6-bit luminance
  int v = 15 - (uv >> 4);
  int u = 15 - (uv & 0xf);

  pixel_t color = border_pal[((u & 0xf) << 6) | ((v & 0xf) << 10) | y];
#if SDL_BPP != 32
  color = SDL_MapRGB(m_text_surface->format, (color >> 16) & 0xff,
                     (color >> 8) & 0xff, color & 0xff);
#endif

  // XXX: draw border
}

void SDLGFX::setColorSpace(uint8_t palette) {
  Video::setColorSpace(palette);
  if (palette > 1)
    return;

  uint8_t *pal = csp.paletteData(palette);
  for (int i = 0; i < 256; ++i) {
#if SDL_BPP == 8
    SDL_Color c = { pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2] };
    SDL_SetColors(m_screen, &c, i, 1);
#endif
    m_current_palette[i] = SDL_MapRGB(m_text_surface->format, pal[i * 3],
                                      pal[i * 3 + 1], pal[i * 3 + 2]);
  }
}

//#define PROFILE_BG

#include "scalers.h"
#include "rotozoom.h"

#include <alpha-lib/overlay_alpha.h>

#define GFXCLASS SDLGFX
#include <drawbg.h>

extern "C" int gfx_thread(void *data) {
  SDLGFX *gfx = (SDLGFX *)data;
  for (;;) {
    SDL_mutexP(gfx->m_scalemut);
    SDL_CondWait(gfx->m_scalecond, gfx->m_scalemut);
    gfx->updateBgScale();
    SDL_mutexV(gfx->m_scalemut);
  }
}

void SDLGFX::updateBg() {
  if (m_ready) {
    m_ready = false;
    SDL_Flip(m_screen);
  }
  SDL_CondSignal(m_scalecond);
}

void SDLGFX::updateBgScale() {
  static uint32_t last_frame = 0;

  if (frame() <= last_frame + m_frameskip)
    return;

  last_frame = frame();

  if (!m_bg_modified && !m_dirty && m_external_layers.size() == 0) {
    return;
  }

  memcpy(m_composite_surface->pixels, m_text_surface->pixels,
         m_text_surface->pitch * m_current_mode.y);

  m_bg_modified = false;
  m_dirty = false;

#ifdef PROFILE_BG
  uint32_t start = micros();
#endif
  for (int prio = 0; prio <= MAX_PRIO; ++prio) {
    for (int b = 0; b < MAX_BG; ++b) {
      bg_t *bg = &m_bg[b];
      if (!bg->enabled || bg->prio != prio)
        continue;
      drawBg(bg);
    }
    for (auto l : m_external_layers) {
      if (l.prio == prio)
        l.painter(&pixelComp(0, 0), m_current_mode.x, m_current_mode.y,
                  compositePitch(), l.userdata);
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

      drawSprite(s);
    }
  }

  if (SDL_BPP == 8)
    scale_integral_8(m_composite_surface, m_screen, m_current_mode.y);
  else if (SDL_BPP == 16)
    scale_integral_16(m_composite_surface, m_screen, m_current_mode.y);
  else
    scale_integral_32(m_composite_surface, m_screen, m_current_mode.y);
  m_ready = true;
}

#ifdef USE_BG_ENGINE
#include "../gfx/spritecoll.h"
#endif  // USE_BG_ENGINE
