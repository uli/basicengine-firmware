// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <Arduino.h>
#include "sdlgfx.h"
#include "colorspace.h"
#include <joystick.h>

#include <SDL/SDL_gfxPrimitives.h>

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

extern const SDL_VideoInfo *sdl_info;
extern int sdl_flags;
extern bool sdl_keep_res;

Uint32 SDLGFX::timerCallback(Uint32 t) {
  SDL_Event ev;
  ev.type = SDL_USEREVENT;
  SDL_PushEvent(&ev);
  vs23.m_frame++;
  return t;
}

#include <config.h>

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

  m_display_enabled = true;

  SDL_SetTimer(16, timerCallback);
}

void SDLGFX::reset() {
  BGEngine::reset();
  setColorSpace(0);
}

void SDLGFX::blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst,
                      uint16_t y_dst, uint16_t width, uint16_t height) {
#ifdef PROFILE_BLIT
  uint32_t s = micros();
#endif
  if (y_dst == y_src && x_dst > x_src) {
    while (height) {
      memmove(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
              width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  } else if ((y_dst > y_src) || (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memcpy(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
             width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memcpy(&pixelText(x_dst, y_dst), &pixelText(x_src, y_src),
             width * sizeof(pixel_t));
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
}

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

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(524288 / modes_pal[mode].x,
                     modes_pal[mode].y + modes_pal[mode].y / MIN_FONT_SIZE_Y);

  m_last_line++;
  printf("newmode %d %d %d %d\n", modes_pal[mode].x + modes_pal[mode].left * 2,
         modes_pal[mode].y + modes_pal[mode].top * 2,
         modes_pal[mode].x + modes_pal[mode].left * 2,
         m_last_line);

  m_current_mode = modes_pal[mode];

  SDL_Rect **modes;
  modes = SDL_ListModes(sdl_info->vfmt, sdl_flags);

  int final_w = 0, final_h = 0;

  if (sdl_keep_res) {
    final_w = sdl_info->current_w;
    final_h = sdl_info->current_h;
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

  setColorSpace(0);

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  m_display_enabled = true;
  m_dirty = true;

  setBorder(0, 0, 0, m_screen->w);

  return true;
}

void SDLGFX::toggleFullscreen() {
  if (m_screen) {
    SDL_WM_ToggleFullScreen(m_screen);
    sdl_flags ^= SDL_FULLSCREEN;
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

  for (int y = 0; y < m_screen->h; ++y) {
    for (int xx = x; xx < w + x; ++xx) {
      *screenPixel(xx, y) = color;
    }
  }
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

void SDLGFX::updateBg() {
  static uint32_t last_frame = 0;

  if (frame() <= last_frame + m_frameskip)
    return;

  last_frame = frame();

  if (!m_bg_modified && !m_dirty)
    return;

  if (m_dirty) {
    if (SDL_BPP == 8)
      scale_integral_8(m_composite_surface, m_screen, m_current_mode.y);
    else if (SDL_BPP == 16)
      scale_integral_16(m_composite_surface, m_screen, m_current_mode.y);
    else
      scale_integral_32(m_composite_surface, m_screen, m_current_mode.y);

    SDL_Flip(m_screen);
    m_dirty = false;
  }

  memcpy(m_composite_surface->pixels, m_text_surface->pixels,
         m_text_surface->pitch * m_current_mode.y);

  m_bg_modified = false;
  m_dirty = true;

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

#ifdef PROFILE_BG
    uint32_t taken = micros() - start;
    Serial.printf("rend %d\r\n", taken);
#endif

    for (int si = 0; si < MAX_SPRITES; ++si) {
      sprite_t *s = &m_sprite[si];

      // skip if not visible
      if (!s->enabled || s->prio != prio)
        continue;
      if (s->pos_x + s->p.w < 0 || s->pos_x >= width() ||
          s->pos_y + s->p.h < 0 || s->pos_y >= height())
        continue;

      // sprite pattern start coordinates
      int px = s->p.pat_x + s->p.frame_x * s->p.w;
      int py = s->p.pat_y + s->p.frame_y * s->p.h;

      pixel_t skey = s->p.key;

      // refresh surface as necessary
      if (s->must_reload || !s->surf) {
        // XXX: do this asynchronously
        if (s->surf)
          delete s->surf;

        // XXX: shouldn't this happen on the rotozoom surface?
        pixel_t alpha = s->alpha << 24;
        if (s->p.key != 0) {
          for (int y = 0; y < s->p.h; ++y) {
            for (int x = 0; x < s->p.w; ++x) {
              if ((pixelText(px + x, py + y) & 0xffffff) == (s->p.key & 0xffffff)) {
                pixelText(px + x, py + y) = pixelText(px + x, py + y) & 0xffffff;
              } else {
                pixelText(px + x, py + y) =
                        (pixelText(px + x, py + y) & 0xffffff) | alpha;
              }
            }
          }
        }

        rz_surface_t in(s->p.w, s->p.h, (uint32_t *)(&pixelText(px, py)),
                        m_text_surface->pitch, 0);

        rz_surface_t *out = rotozoomSurfaceXY(
                &in, s->angle, s->p.flip_x ? -s->scale_x : s->scale_x,
                s->p.flip_y ? -s->scale_y : s->scale_y, 0);
        s->surf = out;
        s->must_reload = false;
      }

      int dst_x = s->pos_x;
      int dst_y = s->pos_y;

      int blit_width = s->surf->w;
      int blit_height = s->surf->h;

      int src_x = 0;
      int src_y = 0;

      if (dst_x < 0) {
        blit_width += dst_x;
        src_x -= dst_x;
        dst_x = 0;
      } else if (dst_x + blit_width >= m_current_mode.x)
        blit_width = m_current_mode.x - dst_x;

      if (dst_y < 0) {
        blit_height += dst_y;
        src_y -= dst_y;
        dst_y = 0;
      } else if (dst_y + blit_height >= m_current_mode.y)
        blit_height = m_current_mode.y - dst_y;

      overlay_alpha_stride_div255_round_approx(
              (uint8_t *)&pixelComp(dst_x, dst_y),
              (uint8_t *)&s->surf->pixels[src_y * s->surf->w + src_x],
              (uint8_t *)&pixelComp(dst_x, dst_y),
              m_composite_surface->pitch / sizeof(pixel_t), blit_height,
              blit_width, s->surf->w);
    }
  }
}

#ifdef USE_BG_ENGINE

// implementation based on RZ surface
uint8_t SDLGFX::spriteCollision(uint8_t collidee, uint8_t collider) {
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

      if (alphaFromColor(leftpixel) >= 0x80 && alphaFromColor(rightpixel) >= 0x80)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}

#endif  // USE_BG_ENGINE
