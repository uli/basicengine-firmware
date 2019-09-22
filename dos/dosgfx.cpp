#ifdef __DJGPP__

#include <Arduino.h>
#include "dosgfx.h"
#include "colorspace.h"
#include "Psx.h"

extern "C" {
#include "vesa.h"
}

DOSGFX vs23;

VBESURFACE *vbesurface_ptr;

const struct video_mode_t DOSGFX::modes_pal[DOS_SCREEN_MODES] = {
	{460, 224, 16, 16, 1},
	{436, 216, 16, 16, 1},
	{320, 216, 16, 16, 2},	// VS23 NTSC demo
	{320, 200, 0, 0, 2},	// (M)CGA, Commodore et al.
	{256, 224, 16, 16, 25},	// SNES
	{256, 192, (200-192)/2, (320-256)/2, 25},	// MSX, Spectrum, NDS
	{160, 200, 16, 16, 4},	// Commodore/PCjr/CPC
						// multi-color
	// "Overscan modes"
	{352, 240, 16, 16, 2},	// PCE overscan (barely)
	{282, 240, 16, 16, 2},	// PCE overscan (underscan on PAL)
	{508, 240, 16, 16, 1},
	// ESP32GFX modes
	{320, 256, 16, 16, 2},	// maximum PAL at 2 clocks per pixel
	{320, 240, 0, 0, 2},	// DawnOfAV demo, Mode X
	{640, 256, (480-256)/2, 0, 1},
	// default H3 mode
	{480, 270, 16, 16, 1},
	// default DOS mode
	{640, 480, 0, 0, 0},
};

void DOSGFX::begin(bool interlace, bool lowpass, uint8_t system)
{
  m_display_enabled = false;
  delay(16);
  m_last_line = 0;
  printf("set mode\n");
  m_current_mode = modes_pal[SC_DEFAULT];
  if (!setMode(SC_DEFAULT)) {
    if (!setMode(SC_DEFAULT_SECONDARY)) {
      printf("failed to enter graphics mode\n");
      delay(500);
      exit(1);
    }
  }

  m_bin.Init(0, 0);

  m_frame = 0;
  reset();

  m_display_enabled = true;
}

void DOSGFX::reset()
{
  BGEngine::reset();
//  clear_to_color(screen, 0);
  setColorSpace(0);
}

void DOSGFX::MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint16_t width, uint16_t height, uint8_t dir)
{
  uint8_t *m_pixels = (uint8_t*)vbesurface_ptr->offscreen_ptr;
  int sw = m_current_mode.x;
  int sh = m_current_mode.y;
  if (dir) {
    x_src -= width - 1;
    x_dst -= width - 1;
    while (height) {
      _movedatal(vbesurface_ptr->lfb_selector, y_src * sw + x_src,
                 vbesurface_ptr->lfb_selector, y_dst * sw + x_dst,
                 width / 4);
      y_dst--;
      y_src--;
      height--;
    }
  } else {
    while (height) {
      _movedatal(vbesurface_ptr->lfb_selector, y_src * sw + x_src,
                 vbesurface_ptr->lfb_selector, y_dst * sw + x_dst,
                 width / 4);
      y_dst++;
      y_src++;
      height--;
    }
  }
}

bool DOSGFX::setMode(uint8_t mode)
{
  m_display_enabled = false;

  // Try to allocate no more than 128k, but make sure it's enough to hold
  // the specified resolution plus color memory.
  m_last_line = _max(524288 / modes_pal[mode].x,
                     modes_pal[mode].y + modes_pal[mode].y / MIN_FONT_SIZE_Y);

  m_last_line++;
  printf("newmode %d %d %d %d\n", modes_pal[mode].x + modes_pal[mode].left * 2, modes_pal[mode].y + modes_pal[mode].top * 2, modes_pal[mode].x + modes_pal[mode].left * 2, m_last_line);
  delay(1000);
  vbesurface_ptr = VBEinfoInit(modes_pal[mode].x + modes_pal[mode].left * 2, modes_pal[mode].y + modes_pal[mode].top * 2, 8, 2000);
  if (!vbesurface_ptr) {
    printf("bad!!\n");
    delay(2000);
    vbesurface_ptr = VBEinit(m_current_mode.x + m_current_mode.left * 2, m_current_mode.y + m_current_mode.top * 2, 8);
    setColorSpace(0);
    return false;
  }

  m_current_mode = modes_pal[mode];

  setColorSpace(0);

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  m_display_enabled = true;
  
  return true;
}

void DOSGFX::setColorSpace(uint8_t palette)
{
  Video::setColorSpace(palette);
  uint8_t *pal = csp.paletteData(palette);
  for (int i = 0; i < 256; ++i) {
//    RGB c = { pal[i*3] >> 2, pal[i*3+1] >> 2, pal[i*3+2] >> 2 };
//    set_color(i, &c);
  }
}

//#define PROFILE_BG

void DOSGFX::updateBg()
{
  static uint32_t last_frame = 0;

  if (frame() <= last_frame + m_frameskip)
    return;

  last_frame = frame();

  if (!m_bg_modified)
    return;

  m_bg_modified = false;

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
          //XXX blit(screen, screen, x+owx, y+owy, t_x, t_y, tsx*4, 1);
          x += tsx;
          tile_x++;
          goto next;
        } else {
          putPixel(x+owx, y+owy, getPixel(t_x, t_y));
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
      if (yy < 0 || yy >= height())
        continue;
      for (int x = 0; x != s->p.w; ++x) {
        int xx = x + s->pos_x;
        if (xx < 0 || xx >= width())
          continue;
        uint8_t p = getPixel(px+x*dx, py+y*dy);
        // draw only non-keyed pixels
        if (p != s->p.key)
          putPixel(xx, yy, p);
      }
    }
  }
}

#ifdef USE_BG_ENGINE
uint8_t DOSGFX::spriteCollision(uint8_t collidee, uint8_t collider)
{
  uint8_t dir = 0x40;	// indicates collision

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
    dir |= psxLeft;
    left = them;
    right = us;
  } else if (them->pos_x + them->p.w > us->pos_x + us->p.w)
    dir |= psxRight;

  const sprite_t *upper = us, *lower = them;
  if (them->pos_y < us->pos_y) {
    dir |= psxUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them->p.h > us->pos_y + us->p.h)
    dir |= psxDown;

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
      int leftpixel = getPixel(leftpx, leftpy);
      int rightpixel = getPixel(rightpx, rightpy);

      if (leftpixel != left->p.key && rightpixel != right->p.key)
        return dir;
    }
  }
  
  // no overlapping pixels
  return 0;
}
#endif	// USE_BG_ENGINE

#endif	// DOS
