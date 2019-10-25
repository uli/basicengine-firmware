// SPDX-License-Identifier: MIT
/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2019 Ulrich Hecht.
 * Copyright (c) 2019 Marko Lukat.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include "ttconfig.h"
#ifdef USE_VS23

#include <joystick.h>

#include "vs23s010.h"
#include "ntsc.h"
#include "lock.h"
#include "colorspace.h"

//#define DISABLE_SPRITE_DRAW

//#define PROFILE_BG
//#define DEBUG_SYNC

//#define DEBUG_PATTERNS

#ifdef DEBUG_PATTERNS
#define dbg_pat(x...) printf(x)
extern "C" size_t umm_free_heap_size(void);
#else
#define dbg_pat(x...)
#endif

uint16_t ICACHE_RAM_ATTR VS23S010::currentLine() {
  uint16_t cl = SpiRamReadRegister(CURLINE) & 0xfff;
  if (m_interlace && cl >= 262)
    cl -= 262;
  return cl;
}

void GROUP(basic_video) VS23S010::setPixel(uint16_t x, uint16_t y, pixel_t c) {
  uint32_t byteaddress = pixelAddr(x, y);
  SpiRamWriteByte(byteaddress, c);
}

pixel_t GROUP(basic_video) VS23S010::getPixel(uint16_t x, uint16_t y) {
  uint32_t byteaddress = pixelAddr(x, y);
  return (pixel_t)SpiRamReadByte(byteaddress);
}

void VS23S010::adjust(int16_t cnt) {
  // XXX: Huh?
}

#ifdef USE_BG_ENGINE
void VS23S010::resetSprites() {
  BGEngine::resetSprites();
  for (int i = 0; i < MAX_SPRITES; ++i) {
    if (m_patterns[i]) {
      free(m_patterns[i]);
      m_patterns[i] = NULL;
    }
    struct sprite_t *s = &m_sprite[i];
    s->pat = NULL;
  }
}
#endif

#include "config.h"

void VS23S010::begin(bool interlace, bool lowpass, uint8_t system) {
  m_vsync_enabled = false;
  m_interlace = interlace;
  m_lowpass = lowpass;
  m_frame = 0;
#ifdef USE_BG_ENGINE
  m_frameskip = 0;
  m_bg_modified = true;
  memset(m_bg, 0, sizeof(m_bg));
  memset(m_patterns, 0, sizeof(m_patterns));
  resetSprites();
#endif

  m_bin.Init(0, 0);

  SpiLock();
  SPI.setFrequency(38000000);
  m_min_spi_div = getSpiClock();
  SPI.setFrequency(11000000);
  m_def_spi_div = getSpiClock();

  m_pal = system != 0;

  m_gpio_state = 0xf;
  SpiRamWriteRegister(0x82, m_gpio_state);

  SpiUnlock();

  csp.setColorConversion(0, 7, 3, 6, true);
  csp.setColorSpace(0);
  m_line_adjust = 0;

  setMode(CONFIG.mode - 1);
}

void VS23S010::end() {
  m_bin.Init(0, 0);
#ifdef USE_BG_ENGINE
  for (int i = 0; i < MAX_BG; ++i) {
    freeBg(i);
  }
#endif
}

void VS23S010::reset() {
  BGEngine::reset();
  setColorSpace(0);
}

bool SMALL VS23S010::setMode(uint8_t mode) {
retry:
#ifdef USE_BG_ENGINE
  m_bg_modified = true;
#endif
  setSyncLine(0);

  memcpy_P(&m_current_mode, &modes()[mode], sizeof(m_current_mode));
  uint32_t spi_div = getSpiClock();
  SPI.setFrequency(m_current_mode.max_spi_freq);
  m_current_mode.max_spi_freq = getSpiClock();
  setSpiClock(spi_div);

  m_last_line = PICLINE_MAX;
  m_first_line_addr = PICLINE_BYTE_ADDRESS(0);
  m_pitch = PICLINE_BYTE_ADDRESS(1) - m_first_line_addr;

#ifdef USE_BG_ENGINE
  resetBgs();
  resetSprites();
#endif

  m_bin.Init(m_current_mode.x, m_last_line - m_current_mode.y);

  videoInit();
  calibrateVsync();
  if (m_pal && F_CPU / m_cycles_per_frame < 45) {
    // We are in PAL mode, but the hardware has an NTSC crystal.
    m_pal = false;
    goto retry;
  } else if (!m_pal && F_CPU / m_cycles_per_frame > 70) {
    // We are in NTSC mode, but the hardware has a PAL crystal.
    m_pal = true;
    goto retry;
  }
  // Start the new frame at the end of the visible screen plus a little extra.
  // Used to be two-thirds down the screen, but that caused more flicker when
  // the rendering load changes drastically.
  setSyncLine(m_current_mode.y + m_current_mode.top + 16);

  // Sony KX-14CP1 and possibly other displays freak out if we start drawing
  // stuff before they had a chance to synchronize with the new mode, so we
  // wait a few frames.
  delay(160);

  return true;
}

void VS23S010::calibrateVsync() {
  uint32_t now, now2, cycles;
  while (currentLine() != 100) {};
  now = ESP.getCycleCount();
  while (currentLine() == 100) {};
  while (currentLine() != 100) {};
  for (;;) {
    now2 = ESP.getCycleCount();
    cycles = now2 - now;
    if (abs((int)m_cycles_per_frame - (int)cycles) < 80000)
      break;
    m_cycles_per_frame = cycles;
    now = now2;
    while (currentLine() == 100) {};
    while (currentLine() != 100) {};
  }
  m_cycles_per_frame = m_cycles_per_frame_calculated = cycles;
}

void ICACHE_RAM_ATTR VS23S010::vsyncHandler(void) {
  uint32_t now = ESP.getCycleCount();
  uint32_t next = now + vs23.m_cycles_per_frame;
#ifdef ESP8266_NOWIFI
  uint16_t line;
  if (!(vs23.m_frame & 15) && !SpiLocked()) {
    line = vs23.currentLine();
    if (line < vs23.m_sync_line) {
      next += (vs23.m_cycles_per_frame / 262) * (vs23.m_sync_line - line);
      vs23.m_cycles_per_frame += 10;
    } else if (line > vs23.m_sync_line) {
      next -= (vs23.m_cycles_per_frame / 262) * (line - vs23.m_sync_line);
      vs23.m_cycles_per_frame -= 10;
    }
    if (vs23.m_cycles_per_frame > vs23.m_cycles_per_frame_calculated * 5 / 4 ||
        vs23.m_cycles_per_frame < vs23.m_cycles_per_frame_calculated * 3 / 4) {
      // If we went completely off the rail (may happen if the interrupt was
      // substantially late at some point), we reset to a sane value.
      vs23.m_cycles_per_frame = vs23.m_cycles_per_frame_calculated;
    }
#ifdef DEBUG_SYNC
    if (vs23.m_sync_line != line) {
      Serial.print("deviation ");
      Serial.print(line - vs23.m_sync_line);
      Serial.print(" at ");
      Serial.println(millis());
    }
#endif
  }
#ifdef DEBUG
  else
    Serial.println("spilocked");
#endif
#endif  // ESP8266_NOWIFI
  vs23.m_frame++;

  // See you next frame:
  timer0_write(next);
}

void VS23S010::setSyncLine(uint16_t line) {
#ifdef USE_BG_ENGINE
  m_bg_modified = true;
#endif
  if (line == 0) {
    if (m_vsync_enabled)
      timer0_detachInterrupt();
    m_vsync_enabled = false;
  } else {
    m_sync_line = line;
    timer0_isr_init();
    timer0_attachInterrupt(&vsyncHandler);
    // Make sure interrupt is triggered soon.
    timer0_write(ESP.getCycleCount() + 100000);
    m_vsync_enabled = true;
  }
}

#ifdef USE_BG_ENGINE

#ifdef HOSTED
#include <hosted_spi.h>
#else
static inline void GROUP(basic_video)
        SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count) {
  uint8_t cmd[count + 4];
  cmd[0] = 3;
  cmd[1] = address >> 16;
  cmd[2] = address >> 8;
  cmd[3] = address;
  VS23_SELECT;
  SPI.transferBytes(cmd, cmd, count + 4);
  VS23_DESELECT;
  os_memcpy(data, cmd + 4, count);
}

static inline void GROUP(basic_video)
        SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len) {
  uint8_t cmd[len + 4];
  cmd[0] = 2;
  cmd[1] = address >> 16;
  cmd[2] = address >> 8;
  cmd[3] = address;
  os_memcpy(&cmd[4], data, len);
  VS23_SELECT;
  SPI.writeBytes(cmd, len + 4);
  VS23_DESELECT;
}
#endif

#define SpiRamWriteBM1Ctrl(a, b, c) SpiRamWriteBMCtrl(BLOCKMVC1, (a), (b), (c))
void GROUP(basic_video) VS23S010::drawBg(struct bg_t *bg, int y1, int y2) {
  //  re/draw tiled background within window limitations
  //
  //  Vertical blitting is limited to 256 lines, with a tile limit of 32 lines there
  //  is no conflict here. Each background section [y1, y2[ is simply drawn line by
  //  line with the top and/or bottom line having potentially less than full height.
  //
  //  Horizontally we have a width limit of 5 pixels which can be reliably transferred.
  //  Below that it primarily depends on the destination alignment how much is in fact
  //  transferred. Which means that tile widths below 5 are upgraded to 5 pixels and
  //  - in the case of the RHS tile - have their start offset adjusted (-ve).
  //
  //  In both cases we end up with unwanted pixel data which is simply overwritten by
  //  the middle tiles or for sufficiently small windows LHS and RHS have to sort this
  //  out among themselves.
  //
  //  = determine LHS, MID and RHS offset/length pairs, there is always a LHS tile
  //    even if it's full width
  //  = draw LHS/RHS first, draw the shorter one first, this leaves us with
  //    - LHS only    ops: 4/%100
  //    - LHS+RHS     ops: 3/%011
  //    - RHS+LHS     ops: 6/%110
  //  = if there're any MIDdle tiles to be drawn, do so
  //
  //  The latter have all the same w/h setup so that's only done once (BMC2). The start
  //  address setup benefits when the bit pattern for bits 0 doesn't change (BMC1 is a
  //  1+5 byte sequence which - when shortened - will reuse the previously latched 6th
  //  byte). Whether 5 or 6 bytes are sent is determined within the BMC1 call (LSB cache).

  int32_t LHS_offset, LHS_length;
  int32_t MID_offset, MID_length;
  int32_t RHS_offset, RHS_length;

  uint8_t LSB, ops = 2;

  int32_t tx, tsx = bg->tile_size_x;
  int32_t ty, tsy = bg->tile_size_y;

  // split hline
  MID_length = bg->win_w;

  LHS_offset = bg->scroll_x % tsx;
  LHS_length = min(tsx - LHS_offset, MID_length);

  MID_length -= LHS_length;

  RHS_offset = 0;
  RHS_length = MID_length % tsx;

  MID_offset = LHS_length;
  MID_length -= RHS_length;

  // determine draw order
  if (RHS_length)
    ops |= 1;
  if (LHS_length > RHS_length)
    ops <<= 1;

  // handle h/w bug, block widths < 5 don't work
  // 4: 6/16, 3: 3/16, 2: 1/16, 1: 0/16
  // ... but they look interesting ...
  if (LHS_length) {
    LHS_length = max(5, LHS_length);
  }

  if (RHS_length) {
    RHS_offset = RHS_length - max(5, RHS_length);
    RHS_length -= RHS_offset;
  }
#if 0
  printf("%3d %3d | %3d %3d | %3d %3d: %02X\n",
         LHS_offset, LHS_length,
         MID_offset, MID_length,
         RHS_offset, RHS_length,
         ops);
#endif
  int32_t offset_y = bg->scroll_y + (y1 - bg->win_y);
  int32_t tile, ROW_height, ROW_offset = offset_y % tsy;

  while (y1 < y2) {
    // destination address for this row
    uint32_t tmp_addr, src_addr, dst_addr = pixelAddr(bg->win_x, y1);

    ROW_height = min(tsy - ROW_offset, y2 - y1);

    // top left map tile indices
    int32_t xs = (bg->scroll_x / tsx) % bg->w;
    int32_t ys = (offset_y / tsy) % bg->h;

    // plot border tiles, left[/right]
    switch (ops) {
    case 3:
      // plot LHS
      // relative x: 0
      if ((tile = bg->tiles[ys * bg->w + xs]) != 0xFF) {
        tx = (tile % bg->pat_w) * tsx + bg->pat_x + LHS_offset;
        ty = (tile / bg->pat_w) * tsy + bg->pat_y + ROW_offset;
        src_addr = pixelAddr(tx, ty);

        LSB = ((src_addr & 1) << 2) | ((dst_addr & 1) << 1) | lowpass();

        while (!blockFinished()) {}
        SpiRamWriteBM1Ctrl(src_addr >> 1, dst_addr >> 1, LSB);
        SpiRamWriteBM2Ctrl(m_pitch - LHS_length, LHS_length, ROW_height - 1);
        startBlockMove();
      }

    case 6:
      // plot RHS
      // relative x: MID_offset + MID_length + RHS_offset
      if ((tile = bg->tiles[ys * bg->w + ((xs + 1 + MID_length / tsx) % bg->w)]) != 0xFF) {
        tx = (tile % bg->pat_w) * tsx + bg->pat_x + RHS_offset;
        ty = (tile / bg->pat_w) * tsy + bg->pat_y + ROW_offset;
        src_addr = pixelAddr(tx, ty);

        tmp_addr = dst_addr + MID_offset + MID_length + RHS_offset;
        LSB = ((src_addr & 1) << 2) | ((tmp_addr & 1) << 1) | lowpass();

        while (!blockFinished()) {}
        SpiRamWriteBM1Ctrl(src_addr >> 1, tmp_addr >> 1, LSB);
        SpiRamWriteBM2Ctrl(m_pitch - RHS_length, RHS_length, ROW_height - 1);
        startBlockMove();
      }

      if (ops & 1)
        break;
    case 4:
      // plot LHS
      // relative x: 0
      if ((tile = bg->tiles[ys * bg->w + xs]) != 0xFF) {
        tx = (tile % bg->pat_w) * tsx + bg->pat_x + LHS_offset;
        ty = (tile / bg->pat_w) * tsy + bg->pat_y + ROW_offset;
        src_addr = pixelAddr(tx, ty);

        LSB = ((src_addr & 1) << 2) | ((dst_addr & 1) << 1) | lowpass();

        while (!blockFinished()) {}
        SpiRamWriteBM1Ctrl(src_addr >> 1, dst_addr >> 1, LSB);
        SpiRamWriteBM2Ctrl(m_pitch - LHS_length, LHS_length, ROW_height - 1);
        startBlockMove();
      }

    default: break;
    }

    while (!blockFinished()) {}
    SpiRamWriteBM2Ctrl(m_pitch - tsx, tsx, ROW_height - 1);

    // plot middle part
    dst_addr += MID_offset;
    for (int i = 0; i < MID_length; i += tsx) {
      if ((tile = bg->tiles[ys * bg->w + (++xs % bg->w)]) != 0xFF) {
        tx = (tile % bg->pat_w) * tsx + bg->pat_x;
        ty = (tile / bg->pat_w) * tsy + bg->pat_y + ROW_offset;
        src_addr = pixelAddr(tx, ty);

        LSB = ((src_addr & 1) << 2) | ((dst_addr & 1) << 1) | lowpass();

        while (!blockFinished()) {}
        SpiRamWriteBM1Ctrl(src_addr >> 1, dst_addr >> 1, LSB);
        startBlockMove();
      }
      dst_addr += tsx;
    }

    // advance
    ROW_offset = 0;
    offset_y += ROW_height;
    y1 += ROW_height;
  }
}

void GROUP(basic_video) VS23S010::updateBg() {
  static uint32_t last_frame = 0;
#ifdef PROFILE_BG
  uint32_t mxx;
  int lines[6];
#endif
  uint16_t pass0_end_line = 0;

  if (m_frame <= last_frame + m_frameskip || !m_bg_modified || SpiLocked())
    return;
  m_bg_modified = false;
  last_frame = m_frame;

  int spi_clock_default = getSpiClock();

  SpiLock();
#ifdef PROFILE_BG
  lines[0] = currentLine();
  mxx = millis();
#endif

  struct bg_t *bg;

  // Every layer needs to end the drawing the first half of the screen at
  // or before the height the previous one did. This indicates the
  // initial height. If an object cannot be drawn exactly up to the
  // current partitioning point, that point will be raised to make sure
  // that the next layer will not overshoot the previous one.
  int last_pix_split_y = m_current_mode.y / 2;

  // Drawing all backgrounds and sprites does not usually fit into the
  // vertical blank period. The screen is therefore redrawn in two passes,
  // top half and bottom half. That way, the top half can be finished
  // before the frame starts and can be displayed while the bottom half is
  // still being drawn.
  for (int pass = 0; pass < 2; ++pass) {
    for (int prio = 0; prio <= MAX_PRIO; ++prio) {
      // Block move programming can be done at max SPI speed.
      setSpiClockMax();

      // Draw enabled backgrounds.
      for (int i = 0; i < MAX_BG; ++i) {
        bg = &m_bg[i];
        if (!bg->enabled)
          continue;
        if (bg->prio != prio)
          continue;

        // In first pass, skip BGs starting below the split line.
        if (pass == 0 && bg->win_y >= last_pix_split_y)
          continue;
        // In second pass, skip BGs ending above the split line.
        if (pass == 1 && bg->win_y + bg->win_h <= last_pix_split_y)
          continue;

        // draw the relevant portion of the background from y1 (incl) to y2 (excl)
        if (pass == 0)
          drawBg(bg, bg->win_y, min(bg->win_y + bg->win_h, last_pix_split_y));
        else
          drawBg(bg, max(bg->win_y, last_pix_split_y), bg->win_y + bg->win_h);

#ifdef PROFILE_BG
        lines[1] = currentLine();
        lines[2] = 0;
#endif
      }

      // Draw sprites.

      // Reduce SPI speed for memory accesses.
      setSpiClock(spi_clock_default);
#ifdef PROFILE_BG
      lines[4] = currentLine();
#endif

      while (!blockFinished()) {}
#ifdef PROFILE_BG
      if (pass == 0)
        lines[3] = currentLine();
#endif

      pixel_t bbuf[MAX_SPRITE_W];
      pixel_t sbuf[MAX_SPRITE_W];

#ifndef DISABLE_SPRITE_DRAW
      for (int sn = 0; sn < MAX_SPRITES; ++sn) {
        struct sprite_t *s = m_sprites_ordered[sn];
        if (!s->enabled || (s->must_reload && s->pat))
          continue;
        if (s->prio != prio)
          continue;
        if (s->pos_x < -s->p.w || s->pos_y < -s->p.h)
          continue;
        if (s->pos_x >= m_current_mode.x || s->pos_y >= m_current_mode.y)
          continue;
        if (pass == 0 && s->pos_y >= last_pix_split_y)
          continue;
        if (pass == 1 && s->pos_y + s->p.h <= last_pix_split_y)
          continue;
        if (s->pat) {
          s->pat->last = m_frame;
          int sx = s->pos_x;
          uint32_t spr_addr =
                  m_first_line_addr + max(0, s->pos_y) * m_pitch + max(0, sx);

          int draw_w = s->p.w;
          int draw_h = s->p.h;

          int offset_y = 0;
          if (s->pos_y < 0) {
            draw_h += s->pos_y;
            offset_y = -s->pos_y;
          } else if (s->pos_y + s->p.h > m_current_mode.y)
            draw_h -= s->pos_y + s->p.h - m_current_mode.y;

          int offset_x = 0;
          if (s->pos_x < 0) {
            draw_w += s->pos_x;
            offset_x = -s->pos_x;
          } else if (s->pos_x + s->p.w > m_current_mode.x)
            draw_w -= s->pos_x + s->p.w - m_current_mode.x;

          // Draw sprites crossing the screen partition in two steps, top half
          // in the first pass, bottom half in the second pass.
          if (pass == 0 && s->pos_y + draw_h > last_pix_split_y)
            draw_h = last_pix_split_y - s->pos_y;
          if (pass == 1 && s->pos_y < last_pix_split_y &&
              s->pos_y + draw_h > last_pix_split_y) {
            draw_h -= last_pix_split_y - s->pos_y;
            offset_y += last_pix_split_y - s->pos_y;
            spr_addr += offset_y * m_pitch;
          }

          for (int sy = 0; sy < draw_h; ++sy) {
            // We try our best to minimize the number of bytes that have to
            // be sent to the VS23.
            struct sprite_line *sl = &s->pat->lines[sy + offset_y];
            int width = sl->len;     // non-BG sprite pixel count
            if (offset_x > sl->off)  // need to clip from the left
              width -= offset_x - sl->off;
            else if (draw_w + offset_x <
                     sl->len + sl->off)  // need to clip from the right
              width -= sl->len + sl->off - draw_w - offset_x;
            if (width <= 0)  // anything left?
              continue;

            uint32_t sa = spr_addr + sy * m_pitch;  // line to draw to
            pixel_t *sb;                            // what to draw
            if (sl->type == LINE_BROKEN) {
              // Copy sprite data to SPI send buffer so it can be masked.
              os_memcpy(sbuf, sl->pixels + sl->off, sl->len);
              sb = sbuf;
            } else {
              // Use data from sprite definition directly.
              sb = sl->pixels + sl->off;
            }
            if (offset_x < sl->off) {
              sa += sl->off - offset_x;
            } else {
              sb += offset_x - sl->off;
            }

            if (sl->type == LINE_BROKEN) {
              // This line has inner transparent pixels; we read the screen
              // to fill in the gaps.

              // Playing fast and loose with data integrity here: we can use
              // much higher SPI speeds than the stable 11 MHz if we accept
              // that there is an occasional corrupted byte, which will
              // be corrected in the next frame.
              setSpiClockRead();
              SpiRamReadBytesFast(sa, (uint8_t *)bbuf, width);
              setSpiClock(spi_clock_default);

              for (int p = 0; p < width; ++p) {
                if (sb[p] == s->p.key)
                  sb[p] = bbuf[p];
              }
            }

            SpiRamWriteBytesFast(sa, (uint8_t *)sb, width);
          }
        } else {
          int w = s->p.w;
          int h = s->p.h;
          int x = s->pos_x;
          int y = s->pos_y;

          int offset_x = 0;
          if (x < 0) {
            w += x;
            offset_x = -x;
            x = 0;
            // Blitter fails to draw less than 4 pixel-wide blocks correctly.
            // If this happens on the left side, we drop the sprite altogether.
            if (w < 4)
              continue;
          } else if (x + w >= m_current_mode.x) {
            w = m_current_mode.x - x;
            // If it happens on the right side, we draw into the border.
            if (w < 4)
              w = 4;
          }
          int offset_y = 0;
          if (y < 0) {
            h += y;
            offset_y = -y;
          } else if (y + h >= m_current_mode.y) {
            h = m_current_mode.y - y;
          }

          // Draw sprites crossing the screen partition in two steps, top half
          // in the first pass, bottom half in the second pass.
          if (pass == 0 && y + h > last_pix_split_y)
            h = last_pix_split_y - y;
          if (pass == 1 && y < last_pix_split_y && y + h > last_pix_split_y) {
            offset_y += last_pix_split_y - y;
            h -= offset_y;
          }
          if (w > 0 && h > 0)
            MoveBlock(s->p.pat_x + s->p.frame_x * s->p.w + offset_x,
                      s->p.pat_y + s->p.frame_y * s->p.h + offset_y,
                      x, y + offset_y, w, h, 0);
        }
      }
#endif
    }  // prio

    // Adjust sync line to avoid flicker due to either the bottom half
    // being drawn too early or the top half being drawn too late.
    if (pass == 0)
      pass0_end_line = currentLine();
    else {
      bool pass0_wraparound = pass0_end_line < m_sync_line;
      uint16_t cl = currentLine();
      if (!(!pass0_wraparound && cl >= pass0_end_line) &&
          cl > m_current_mode.y / 3 &&
          m_sync_line > m_current_mode.y * 2 / 3) {
        m_sync_line--;
#ifdef DEBUG_SYNC
        Serial.printf("sync-- %d\n", m_sync_line);
#endif
      } else if (!pass0_wraparound &&
                 pass0_end_line < m_current_mode.y + m_current_mode.top) {
        m_sync_line++;
#ifdef DEBUG_SYNC
        Serial.printf("sync++ %d\n", m_sync_line);
#endif
      }
    }

#ifdef PROFILE_BG
    Serial.printf("%d %d %d %d %d %d\n", lines[0], lines[1], lines[2], lines[3],
                  lines[4], cl);
#endif
  }  // pass
#ifdef PROFILE_BG
  if (millis() > mxx)
    Serial.println(millis() - mxx);
#endif

  setSpiClock(spi_clock_default);
  SpiUnlock();
}

struct VS23S010::sprite_pattern *
VS23S010::allocateSpritePattern(struct sprite_props *p) {
  void *smem = malloc(p->h * sizeof(struct sprite_line) +  // line meta data
                      p->w * p->h +                        // line pixel data
                      sizeof(struct sprite_pattern));      // pattern meta data
  if (!smem)
    return NULL;
  struct sprite_pattern *pat = (struct sprite_pattern *)smem;
  pixel_t *pix = (pixel_t *)smem + p->h * sizeof(struct sprite_line) +
                 sizeof(struct sprite_pattern);
  for (int i = 0; i < p->h; ++i)
    pat->lines[i].pixels = pix + i * p->w;
  pat->ref = 1;
  pat->last = m_frame;
  return pat;
}

bool VS23S010::loadSpritePattern(uint8_t num) {
  struct sprite_t *s = &m_sprite[num];

  s->must_reload = false;

  if (s->pat) {
    if (memcmp(&s->pat->p, &s->p, sizeof(s->p))) {
      dbg_pat("dumppat %d ref %d\r\n", num, s->pat->ref);
    } else {
      dbg_pat("samepat %d\r\n", num);
      return true;
    }
  }

  for (int i = 0; i < MAX_SPRITES; ++i) {
    struct sprite_pattern *p = m_patterns[i];
    if (p) {
      if (!memcmp(&p->p, &s->p, sizeof(s->p))) {
        p->ref++;
        dbg_pat("matchpat %d for %d ref %d\r\n", i, num, p->ref);
        if (s->pat)
          s->pat->ref--;
        s->pat = p;
        s->p.opaque = false;
        return true;
      } else if (p->ref == 0 && p->last + 30 < m_frame) {
        dbg_pat("gc pat %d\r\n", i);
        free(p);
        m_patterns[i] = NULL;
      }
    }
  }

  struct sprite_pattern *pat = NULL;

  for (int i = 0; i < MAX_SPRITES; ++i) {
    if (!m_patterns[i]) {
      pat = m_patterns[i] = allocateSpritePattern(&s->p);
      if (!pat) {
        s->enabled = false;
        return false;
      }
      dbg_pat("alocpat %d@%p for %d\r\n", i, pat, num);
      break;
    }
  }

  if (!pat) {
    for (int i = 0; i < MAX_SPRITES; ++i) {
      if (m_patterns[i] && m_patterns[i]->ref <= 0) {
        dbg_pat("replpat %d@%p for %d\r\n", i, m_patterns[i], num);
        free(m_patterns[i]);
        pat = m_patterns[i] = allocateSpritePattern(&s->p);
        if (!pat) {
          s->enabled = false;
          return false;
        }
        break;
      }
    }
  }
  if (!pat) {
    dbg_pat("pat FAIL\r\n");
    s->enabled = false;
    return false;
  }

  if (s->pat)
    s->pat->ref--;
  s->pat = pat;

  uint32_t tile_addr = pixelAddr(s->p.pat_x + s->p.frame_x * s->p.w,
                                 s->p.pat_y + s->p.frame_y * s->p.h);

  bool solid_block = true;

  SpiLock();
  int spi_clock_default = getSpiClock();
  setSpiClockRead();
  for (int sy = 0; sy < s->p.h; ++sy) {
    struct sprite_line *p = &pat->lines[sy];

    int pline;
    if (s->p.flip_y)
      pline = s->p.h - 1 - sy;
    else
      pline = sy;

    SpiRamReadBytes(tile_addr + pline * m_pitch, (uint8_t *)p->pixels, s->p.w);

    if (s->p.flip_x) {
      for (int i = 0; i < s->p.w / 2; ++i) {
        pixel_t tmp = p->pixels[s->p.w - 1 - i];
        p->pixels[s->p.w - 1 - i] = p->pixels[i];
        p->pixels[i] = tmp;
      }
    }

    p->off = 0;
    p->len = s->p.w;
    p->type = LINE_SOLID;

    pixel_t *pp = p->pixels;
    while (p->len && *pp == s->p.key) {
      solid_block = false;
      ++pp;
      ++p->off;
      --p->len;
    }

    if (p->len) {
      pp = p->pixels + s->p.w - 1;
      while (*pp == s->p.key) {
        solid_block = false;
        --pp;
        --p->len;
      }
    }

    for (int i = 0; i < p->len; ++i) {
      if (p->pixels[p->off + i] == s->p.key) {
        solid_block = false;
        p->type = LINE_BROKEN;
        break;
      }
    }
#ifdef DEBUG_SPRITES
    Serial.printf("  def line %d off %d len %d type %d\n",
                  sy, p->off, p->len, p->type);
#endif
  }
  setSpiClock(spi_clock_default);
  SpiUnlock();

  os_memcpy(&pat->p, &s->p, sizeof(s->p));
  if (solid_block && !s->p.flip_x && !s->p.flip_y) {
    s->pat->ref--;
    s->pat = NULL;
  }
  s->p.opaque = solid_block;

  return true;
}

uint8_t GROUP(basic_video) VS23S010::spriteCollision(uint8_t collidee,
                                                     uint8_t collider) {
  uint8_t dir = 0x40;  // indicates collision

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
    dir |= joyLeft;
  else if (them->pos_x + them->p.w > us->pos_x + us->p.w)
    dir |= joyRight;

  sprite_t *upper = us, *lower = them;
  if (them->pos_y < us->pos_y) {
    dir |= joyUp;
    upper = them;
    lower = us;
  } else if (them->pos_y + them->p.h > us->pos_y + us->p.h)
    dir |= joyDown;

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

  return dir;
}
#endif  // USE_BG_ENGINE

VS23S010 vs23;
#endif  // USE_VS23
