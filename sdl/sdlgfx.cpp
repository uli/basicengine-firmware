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
      memmove(&PIXELT(x_dst, y_dst), &PIXELT(x_src, y_src),
              width * sizeof(pixel_t));
      y_dst++;
      y_src++;
      height--;
    }
  } else if ((y_dst > y_src) || (y_dst == y_src && x_dst > x_src)) {
    y_dst += height - 1;
    y_src += height - 1;
    while (height) {
      memcpy(&PIXELT(x_dst, y_dst), &PIXELT(x_src, y_src),
             width * sizeof(pixel_t));
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      memcpy(&PIXELT(x_dst, y_dst), &PIXELT(x_src, y_src),
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
    fmt->BitsPerPixel,
    fmt->Rmask,
    fmt->Gmask,
    fmt->Bmask,
    fmt->Amask
  );
  m_composite_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
    m_current_mode.x,
    m_last_line,
    fmt->BitsPerPixel,
    fmt->Rmask,
    fmt->Gmask,
    fmt->Bmask,
    fmt->Amask
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
#if SDL_BPP > 8
  if (palette > 1)
    return;
#endif
  uint8_t *pal = csp.paletteData(palette);
  for (int i = 0; i < 256; ++i) {
#if SDL_BPP == 8
    SDL_Color c = { pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2] };
    SDL_SetColors(m_screen,  &c, i, 1);
    SDL_SetColors(m_text_surface, &c, i, 1);
    SDL_SetColors(m_composite_surface, &c, i, 1);
#else
    m_current_palette[i] = SDL_MapRGB(m_text_surface->format, pal[i * 3],
                                      pal[i * 3 + 1], pal[i * 3 + 2]);
#endif
  }
}

//#define PROFILE_BG

#include "scalers.h"
#ifdef USE_ROTOZOOM
#include "rotozoom.h"
#endif

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

  memcpy(m_composite_surface->pixels, m_text_surface->pixels, m_text_surface->pitch * m_current_mode.y);

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

      int tsx = bg->tile_size_x;
      int tsy = bg->tile_size_y;
      int ypoff = bg->scroll_y % tsy;

      // start/end coordinates of the visible BG window, relative to the
      // BG's origin, in pixels
      int sx = bg->scroll_x;
      int sy = bg->scroll_y;

      // offset to add to BG-relative coordinates to get screen coordinates
      int owx = -sx + bg->win_x;
      int owy = -sy + bg->win_y;

      int tile_start_x, tile_start_y;
      int tile_end_x, tile_end_y;

      tile_start_y = bg->scroll_y / tsy;
      tile_end_y = tile_start_y + (bg->win_h + ypoff) / tsy + 1;
      tile_start_x = bg->scroll_x / bg->tile_size_x;
      tile_end_x = tile_start_x + (bg->win_w + tsx - 1) / tsx + 1;

      SDL_Rect clip = {
        (Sint16)bg->win_x, (Sint16)bg->win_y,
        bg->win_w,         bg->win_h
      };
      SDL_SetClipRect(m_composite_surface, &clip);

      for (int y = tile_start_y; y < tile_end_y; ++y) {
        for (int x = tile_start_x; x < tile_end_x; ++x) {
          uint8_t tile = bg->tiles[x % bg->w + (y % bg->h) * bg->w];

          int t_x = bg->pat_x + (tile % bg->pat_w) * tsx;
          int t_y = bg->pat_y + (tile / bg->pat_w) * tsy;

          SDL_Rect dst;
          dst.x = x * tsx + owx;
          dst.y = y * tsy + owy;
          dst.w = tsx;
          dst.h = tsy;

          SDL_Rect src;
          src.x = t_x;
          src.y = t_y;
          src.w = tsx;
          src.h = tsy;

          SDL_BlitSurface(m_text_surface, &src, m_composite_surface, &dst);
        }
      }

      SDL_SetClipRect(m_composite_surface, NULL);
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

#ifndef USE_ROTOZOOM
      // consider flipped axes
      // rotozoom bakes this into the surface
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
#endif

      // sprite pattern start coordinates
#ifdef USE_ROTOZOOM
      int px = s->p.pat_x + s->p.frame_x * s->p.w;
      int py = s->p.pat_y + s->p.frame_y * s->p.h;
#else
      int px = s->p.pat_x + s->p.frame_x * s->p.w + offx;
      int py = s->p.pat_y + s->p.frame_y * s->p.h + offy;
#endif

      pixel_t skey = s->p.key;

#ifdef USE_ROTOZOOM
      // refresh surface as necessary
      if (s->must_reload || !s->surf) {
        // XXX: do this asynchronously
        if (s->surf)
          delete s->surf;

        rz_surface_t in(s->p.w, s->p.h,
                        &((uint32_t*)m_text_surface->pixels)[py * m_text_surface->pitch/4 + px],
                        m_text_surface->pitch, s->p.key);

        rz_surface_t *out = rotozoomSurfaceXY(&in, s->angle,
                                              s->p.flip_x ? -s->scale_x : s->scale_x,
                                              s->p.flip_y ? -s->scale_y : s->scale_y, 0);
        s->surf = out;
        s->must_reload = false;
      }
#endif

#ifdef USE_ROTOZOOM
      int sprite_w = s->surf->w;
      int sprite_h = s->surf->h;
#else
      int sprite_w = s->p.w;
      int sprite_h = s->p.h;
#endif

      for (int y = 0; y != sprite_h; ++y) {
        int yy = y + s->pos_y;
        if (yy < 0 || yy >= height())
          continue;
        for (int x = 0; x != sprite_w; ++x) {
          int xx = x + s->pos_x;
          if (xx < 0 || xx >= width())
            continue;
#ifdef USE_ROTOZOOM
          pixel_t p = s->surf->getPixel(x, y);
#else
          pixel_t p = getPixel(px + x * dx, py + y * dy);
#endif
          // draw only non-keyed pixels
          if (p != skey)
            PIXELC(xx, yy) = p;
        }
      }
    }
  }
}

#ifdef USE_BG_ENGINE

#ifdef USE_ROTOZOOM

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

#else // USE_ROTOZOOM

// implementation based on pattern in video memory
uint8_t SDLGFX::spriteCollision(uint8_t collidee, uint8_t collider) {
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

  pixel_t lkey, rkey;
  lkey = left->p.key;
  rkey = right->p.key;

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
      pixel_t leftpixel = getPixel(leftpx, leftpy);
      pixel_t rightpixel = getPixel(rightpx, rightpy);

      if (leftpixel != lkey && rightpixel != rkey)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}

#endif	// USE_ROTOZOOM

#endif  // USE_BG_ENGINE
