#include "ttconfig.h"
#include "Psx.h"

#if USE_VS23 == 1

#include "vs23s010.h"
#include "ntsc.h"
#include "lock.h"

#ifndef os_memcpy
#define os_memcpy memcpy
#endif

//#define DISABLE_BG_LEFT_COL
//#define DISABLE_BG_MIDDLE
//#define DISABLE_BG_RIGHT_COL

//#define DISABLE_SPRITE_DRAW

//#define DISABLE_BG_TOP
//#define DISABLE_BG_CENTER
//#define DISABLE_BG_BOTTOM

//#define PROFILE_BG
//#define DEBUG_SYNC

void VS23S010::setPixel(uint16_t x, uint16_t y, uint8_t c)
{
  uint32_t byteaddress = pixelAddr(x, y);
  SpiRamWriteByte(byteaddress, c);
}

void VS23S010::adjust(int16_t cnt)
{
  // XXX: Huh?
}

void VS23S010::resetSprites()
{
  m_bg_modified = true;
  for (int i = 0; i < VS23_MAX_SPRITES; ++i) {
    struct sprite_t *s = &m_sprite[i];
    if (s->pattern)
      freeSpritePattern(s);
    m_sprites_ordered[i] = s;
    s->enabled = false;
    s->transparent = true;
    s->pos_x = s->pos_y = 0;
    s->frame_x = s->frame_y = 0;
    s->w = s->h = 8;
  }
}

void VS23S010::resetBgs()
{
  m_bg_modified = true;
  for (int i=0; i < VS23_MAX_BG; ++i) {
    struct bg_t *bg = &m_bg[i];
    freeBg(i);
    bg->tile_size_x = 8;
    bg->tile_size_y = 8;
    bg->pat_x = 0;
    bg->pat_y = m_current_mode->y + 8;
    bg->pat_w = m_current_mode->x / bg->tile_size_x;
  }
}

void VS23S010::begin()
{
  m_frameskip = 0;
  m_bg_modified = true;
  m_vsync_enabled = false;
  resetSprites();

  m_bin.Init(0, 0);

  SpiLock();
  for (int i = 0; i < numModes; ++i) {
    SPI.setFrequency(modes[i].max_spi_freq);
    modes[i].max_spi_freq = getSpiClock();
  }
  SPI.setFrequency(38000000);
  m_min_spi_div = getSpiClock();
  SPI.setFrequency(11000000);
  m_def_spi_div = getSpiClock();

  m_gpio_state = 0xf;
  SpiRamWriteRegister(0x82, m_gpio_state);

  SpiUnlock();
  
  setColorConversion(0, 7, 3, 6, true);
}

void VS23S010::end()
{
  m_bin.Init(0, 0);
  for (int i = 0; i < VS23_MAX_BG; ++i) {
    freeBg(i);
  }
}

void VS23S010::reset()
{
  m_bg_modified = true;
  resetSprites();
  resetBgs();
  m_bin.Init(m_current_mode->x, m_current_mode->y);
  setColorSpace(0);
}

void SMALL VS23S010::setMode(uint8_t mode)
{
  m_bg_modified = true;
  setSyncLine(0);
  m_current_mode = &modes[mode];
  m_last_line = PICLINE_MAX;
  m_first_line_addr = PICLINE_BYTE_ADDRESS(0);
  m_pitch = PICLINE_BYTE_ADDRESS(1) - m_first_line_addr;

  resetBgs();
  resetSprites();

  m_bin.Init(m_current_mode->x, m_last_line - m_current_mode->y);

  SpiRamVideoInit();
  calibrateVsync();
  setSyncLine(m_current_mode->y + m_current_mode->top + 16);
}

void VS23S010::calibrateVsync()
{
  uint32_t now;
  while (currentLine() != 100) {};
  now = ESP.getCycleCount();
  while (currentLine() == 100) {};
  while (currentLine() != 100) {};
  m_cycles_per_frame = m_cycles_per_frame_calculated = ESP.getCycleCount() - now;
}

void ICACHE_RAM_ATTR VS23S010::vsyncHandler(void)
{
  uint32_t now = ESP.getCycleCount();
  uint32_t next = now + vs23.m_cycles_per_frame;
  uint16_t line;
  if (!(vs23.m_frame & 15) && !SpiLocked()) {
    line = vs23.currentLine();
    if (line < vs23.m_sync_line) {
      next += (vs23.m_cycles_per_frame / 262) * (vs23.m_sync_line-line);
      vs23.m_cycles_per_frame += 10;
    } else if (line > vs23.m_sync_line) {
      next -= (vs23.m_cycles_per_frame / 262) * (line-vs23.m_sync_line);
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
      Serial.print(line-vs23.m_sync_line);
      Serial.print(" at ");
      Serial.println(millis());
    }
#endif
  }
#ifdef DEBUG
  else
    Serial.println("spilocked");
#endif
  vs23.m_frame++;

  // See you next frame:
  timer0_write(next);
#ifndef ESP8266_NOWIFI
  // Any attempt to disable the software watchdog is subverted by the SDK
  // by re-enabling it as soon as it gets the chance. This is the only
  // way to avoid watchdog resets if you actually want to do anything
  // with your system that is not wireless bullshit.
  system_soft_wdt_feed();
#endif
}

void VS23S010::setSyncLine(uint16_t line)
{
  m_bg_modified = true;
  if (line == 0) {
    if (m_vsync_enabled)
      timer0_detachInterrupt();
    m_vsync_enabled = false;
  } else {
    m_sync_line = line;
    timer0_isr_init();
    timer0_attachInterrupt(&vsyncHandler);
    // Make sure interrupt is triggered soon.
    timer0_write(ESP.getCycleCount()+100000);
    m_vsync_enabled = true;
  }
}

bool VS23S010::setBgSize(uint8_t bg_idx, uint16_t width, uint16_t height)
{
  struct bg_t *bg = &m_bg[bg_idx];

  m_bg_modified = true;
  bg->enabled = false;

  if (bg->tiles)
    free(bg->tiles);
  bg->tiles = (uint8_t *)calloc(width * height, 1);
  if (!bg->tiles)
    return true;

  for (int i=0; i < width*height; ++i)
    bg->tiles[i] = 64+(i&7);

  bg->w = width;
  bg->h = height;
  bg->scroll_x = bg->scroll_y = 0;
  bg->win_x = bg->win_y = 0;
  bg->win_w = m_current_mode->x;
  bg->win_h = m_current_mode->y;
  return false;
}

void VS23S010::enableBg(uint8_t bg)
{
  m_bg_modified = true;
  if (m_bg[bg].tiles) {
    m_bg[bg].enabled = true;
  }
}

void VS23S010::disableBg(uint8_t bg)
{
  m_bg_modified = true;
  m_bg[bg].enabled = false;
}

void VS23S010::freeBg(uint8_t bg_idx)
{
  struct bg_t *bg = &m_bg[bg_idx];
  m_bg_modified = true;
  bg->enabled = false;
  if (bg->tiles) {
    free(bg->tiles);
    bg->tiles = NULL;
  }
}

void VS23S010::setBgWin(uint8_t bg_idx, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  struct bg_t *bg = &m_bg[bg_idx];
  bg->win_x = x;
  bg->win_y = y;
  bg->win_w = w;
  bg->win_h = h;
  m_bg_modified = true;
}

static inline void ICACHE_RAM_ATTR MoveBlockAddr(uint32_t byteaddress2, uint32_t dest_addr)
{
  // XXX: What about PYF?
  uint8_t req[5] = {
    0x34,
    (uint8_t)(byteaddress2 >> 9),
    (uint8_t)(byteaddress2 >> 1),
    (uint8_t)(dest_addr >> 9),
    (uint8_t)(dest_addr >> 1)
  };

  VS23_SELECT;
  SPI.writeBytes(req, 5);
  VS23_DESELECT;
  while (!blockFinished()) {}
  VS23S010::startBlockMove();
}

static inline void ICACHE_RAM_ATTR SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count)
{
  uint8_t cmd[count+4];
  cmd[0] = 3;
  cmd[1] = address >> 16;
  cmd[2] = address >> 8;
  cmd[3] = address;
  VS23_SELECT;
  SPI.transferBytes(cmd, cmd, count+4);
  VS23_DESELECT;
  os_memcpy(data, cmd+4, count);
}

static inline void ICACHE_RAM_ATTR SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len)
{
  uint8_t cmd[len+4];
  cmd[0] = 2;
  cmd[1] = address >> 16;
  cmd[2] = address >> 8;
  cmd[3] = address;
  os_memcpy(&cmd[4], data, len);
  VS23_SELECT;
  SPI.writeBytes(cmd, len+4);
  VS23_DESELECT;
}

void ICACHE_RAM_ATTR VS23S010::drawBg(struct bg_t *bg,
                                      int dest_addr_start,
                                      uint32_t pat_start_addr,
                                      uint32_t win_start_addr,
                                      int tile_start_x,
                                      int tile_start_y,
                                      int tile_end_x,
                                      int tile_end_y,
                                      uint32_t xpoff,
                                      uint32_t ypoff,
                                      int skip_x,
                                      int skip_y)
{
#ifndef DISABLE_BG_CENTER
  // This code draws into the "extra" bytes following each picture line.
  // Doing so keeps us from having to give border tiles special treatment:
  // their invisible parts are simply drawn where they cannot be seen.
  // It also helps to avoid narrow block moves (less than 4 bytes wide),
  // which often don't work as expected.

  // XXX: This means that window width must be a multiple of tile width.

  uint32_t tile;
  uint32_t tx, ty;
  uint32_t byteaddress2, dest_addr;
  uint32_t tsx = bg->tile_size_x;
  uint32_t tsy = bg->tile_size_y;
  uint8_t bg_w = bg->w;
  uint8_t bg_h = bg->h;
  uint32_t pw = bg->pat_w;
  int draw_w;

  // Set up the LSB of the start/dest addresses; they don't change for
  // middle tiles, so we can omit the last byte of the request.
  while (!blockFinished()) {}
  SpiRamWriteBMCtrl(0x34, 0, 0, ((dest_addr_start & 1) << 1) | ((pat_start_addr & 1) << 2));

  for (int yy = tile_start_y+skip_y; yy < tile_end_y-1; ++yy) {
    dest_addr_start = win_start_addr + ((yy - tile_start_y) * tsy - ypoff) * m_pitch - tile_start_x * tsx - xpoff;

#ifndef DISABLE_BG_LEFT_COL
    // Left tile
    tile = bg->tiles[(yy % bg_h) * bg_w + (tile_start_x+skip_x) % bg_w];
    tx = (tile % pw) * tsx;
    ty = (tile / pw) * tsy;

    dest_addr = dest_addr_start + (tile_start_x+skip_x) * tsx;
    byteaddress2 = pat_start_addr + ty*m_pitch + tx;

    draw_w = tsx;
    if (xpoff >= 8) {
      draw_w -=8;
      dest_addr += 8;
      byteaddress2 += 8;
    }
    while (!blockFinished()) {}
    SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, tsy-1);
    MoveBlockAddr(byteaddress2, dest_addr);
#endif

#ifndef DISABLE_BG_MIDDLE
    // Middle tiles
    while (!blockFinished()) {}
    SpiRamWriteBM2Ctrl(m_pitch-tsx, tsx, tsy-1);
    for (int xx = tile_start_x+skip_x+1; xx < tile_end_x-1; ++xx) {
      tile = bg->tiles[(yy % bg_h) * bg_w + xx % bg_w];
      tx = (tile % pw) * tsx;
      ty = (tile / pw) * tsy;

      dest_addr = dest_addr_start + xx * tsx;
      byteaddress2 = pat_start_addr + ty*m_pitch + tx;

      MoveBlockAddr(byteaddress2, dest_addr);
    }
#endif

#ifndef DISABLE_BG_RIGHT_COL
    // Right tile
    tile = bg->tiles[(yy % bg_h) * bg_w + (tile_end_x-1) % bg_w];
    tx = (tile % pw) * tsx;
    ty = (tile / pw) * tsy;

    dest_addr = dest_addr_start + (tile_end_x-1) * tsx;
    byteaddress2 = pat_start_addr + ty*m_pitch + tx;

    draw_w = tsx;
    if (xpoff < tsx - 8)
      draw_w -=8;

    while (!blockFinished()) {}
    SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, tsy-1);
    MoveBlockAddr(byteaddress2, dest_addr);
#endif
  }
  while (!blockFinished()) {}
#endif
}

void ICACHE_RAM_ATTR VS23S010::drawBgTop(struct bg_t *bg,
                                         int dest_addr_start,
                                         uint32_t pat_start_addr,
                                         int tile_start_x,
                                         int tile_start_y,
                                         int tile_end_x,
                                         uint32_t xpoff,
                                         uint32_t ypoff)
{
#ifndef DISABLE_BG_TOP
  uint32_t tile;
  uint32_t tx, ty;
  uint32_t byteaddress2, dest_addr;
  uint32_t tsx = bg->tile_size_x;
  uint32_t tsy = bg->tile_size_y;
  uint8_t bg_w = bg->w;
  uint8_t bg_h = bg->h;
  uint32_t pw = bg->pat_w;
  int draw_w;

  // Set up the LSB of the start/dest addresses; they don't change for
  // middle tiles, so we can omit the last byte of the request.
  while (!blockFinished()) {}
  SpiRamWriteBMCtrl(0x34, 0, 0, ((dest_addr_start & 1) << 1) | ((pat_start_addr & 1) << 2));

#ifndef DISABLE_BG_LEFT_COL
  tile = bg->tiles[(tile_start_y % bg_h) * bg_w + tile_start_x % bg_w];
  tx = (tile % pw) * tsx;
  ty = (tile / pw) * tsy + ypoff;

  dest_addr = dest_addr_start + tile_start_x * tsx;
  byteaddress2 = pat_start_addr + ty * m_pitch + tx;

  draw_w = tsx;
  if (xpoff >= 8) {
    draw_w -=8;
    dest_addr += 8;
    byteaddress2 += 8;
  }

  SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, tsy-ypoff-1);
  MoveBlockAddr(byteaddress2, dest_addr);
#endif

#ifndef DISABLE_BG_MIDDLE
  while (!blockFinished()) {}
  SpiRamWriteBM2Ctrl(m_pitch-tsx, tsx, tsy-ypoff-1);
  for (int xx = tile_start_x+1; xx < tile_end_x-1; ++xx) {
    tile = bg->tiles[(tile_start_y % bg_h) * bg_w + xx % bg_w];
    tx = (tile % pw) * tsx;
    ty = (tile / pw) * tsy + ypoff;

    dest_addr = dest_addr_start + xx * tsx;
    byteaddress2 = pat_start_addr + ty * m_pitch + tx;
    MoveBlockAddr(byteaddress2, dest_addr);
  }
#endif

#ifndef DISABLE_BG_RIGHT_COL
  tile = bg->tiles[(tile_start_y % bg_h) * bg_w + (tile_end_x - 1) % bg_w];
  tx = (tile % pw) * tsx;
  ty = (tile / pw) * tsy + ypoff;

  dest_addr = dest_addr_start + (tile_end_x - 1) * tsx;
  byteaddress2 = pat_start_addr + ty * m_pitch + tx;

  draw_w = tsx;
  if (xpoff < tsx - 8) {
    draw_w -=8;
  }
  while (!blockFinished()) {}
  SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, tsy-ypoff-1);
  MoveBlockAddr(byteaddress2, dest_addr);
#endif

  while (!blockFinished()) {}
#endif
}

void ICACHE_RAM_ATTR VS23S010::drawBgBottom(struct bg_t *bg,
                                            int tile_start_x,
                                            int tile_end_x,
                                            int tile_end_y,
                                            uint32_t xpoff,
                                            uint32_t ypoff,
                                            int skip_x)
{
#ifndef DISABLE_BG_BOTTOM
  uint32_t tile;
  uint32_t tx, ty;
  uint32_t tsx = bg->tile_size_x;
  uint32_t tsy = bg->tile_size_y;
  uint8_t bg_w = bg->w;
  uint8_t bg_h = bg->h;
  uint32_t pw = bg->pat_w;
  uint32_t byteaddress1, byteaddress2;
  int draw_w;

  ypoff = (ypoff + bg->win_h) % tsy;
  // Bottom line
  if (ypoff) {
    int ba1a = pixelAddr(bg->win_x - xpoff, bg->win_y + bg->win_h - ypoff);
    int ba2a = pixelAddr(bg->pat_x, bg->pat_y);

    while (!blockFinished()) {}
    SpiRamWriteBMCtrl(0x34, 0, 0, ((ba1a & 1) << 1) | ((ba2a & 1) << 2));

#ifndef DISABLE_BG_LEFT_COL
    if (tile_start_x + skip_x < tile_end_x) {
      tile = bg->tiles[((tile_end_y-1) % bg_h) * bg_w + (tile_start_x+skip_x) % bg_w];
      tx = (tile % pw) * tsx;
      ty = (tile / pw) * tsy;
      byteaddress1 = ba1a + ((tile_start_x+skip_x) - tile_start_x) * tsx;
      byteaddress2 = ba2a + ty * m_pitch + tx;
      draw_w = tsx;
      if (xpoff >= 8) {
	draw_w -= 8;
	byteaddress1 += 8;
	byteaddress2 += 8;
      }
      SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, ypoff-1);
      MoveBlockAddr(byteaddress2, byteaddress1);
    }
#endif

#ifndef DISABLE_BG_MIDDLE
    // Set pitch, width and height; same for all tiles
    while (!blockFinished()) {}
    SpiRamWriteBM2Ctrl(m_pitch-tsx, tsx, ypoff-1);
    // Set up the LSB of the start/dest addresses; they don't change for
    // middle tiles, so we can omit the last byte of the request.
    for (int xx = tile_start_x+skip_x+1; xx < tile_end_x-1; ++xx) {
      tile = bg->tiles[((tile_end_y-1) % bg_h) * bg_w + xx % bg_w];
      tx = (tile % pw) * tsx;
      ty = (tile / pw) * tsy;
      byteaddress1 = ba1a + (xx - tile_start_x) * tsx;
      byteaddress2 = ba2a + ty * m_pitch + tx;
      while (!blockFinished()) {}
      // XXX: What about PYF?
      MoveBlockAddr(byteaddress2, byteaddress1);
    }
#endif

#ifndef DISABLE_BG_RIGHT_COL
    if (tile_start_x + skip_x < tile_end_x - 1) {
      tile = bg->tiles[((tile_end_y-1) % bg_h) * bg_w + (tile_end_x - 1) % bg_w];
      tx = (tile % pw) * tsx;
      ty = (tile / pw) * tsy;
      byteaddress1 = ba1a + ((tile_end_x - 1) - tile_start_x) * tsx;
      byteaddress2 = ba2a + ty * m_pitch + tx;
      draw_w = tsx;
      if (xpoff < tsx - 8)
	draw_w -= 8;
      while (!blockFinished()) {}
      SpiRamWriteBM2Ctrl(m_pitch-draw_w, draw_w, ypoff-1);
      // XXX: What about PYF?
      MoveBlockAddr(byteaddress2, byteaddress1);
    }
#endif
  }
#endif
}

void ICACHE_RAM_ATTR VS23S010::updateBg()
{
  static uint32_t last_frame = 0;
#ifdef PROFILE_BG
  uint32_t mxx;
  int lines[6];
#endif
  int dest_addr_start;
  uint32_t pat_start_addr, win_start_addr;
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
  int tsx, tsy;
  int xpoff, ypoff;
  int tile_start_x, tile_start_y;
  int tile_end_x, tile_end_y;
  int tile_split_y;
  int last_pix_split_y, pix_split_y;

  // Every layer needs to end the drawing the first half of the screen at
  // or before the height the previous one did. This indicates the
  // initial height. If an object cannot be drawn exactly up to the
  // current partitioning point, that point will be raised to make sure
  // that the next layer will not overshoot the previous one.
  last_pix_split_y = m_current_mode->y / 2;

  int bg_tile_start_y[VS23_MAX_BG];
  int bg_tile_end_y[VS23_MAX_BG];
  int bg_tile_split_y[VS23_MAX_BG];
  int bg_pix_split_y[VS23_MAX_BG];
  tsy = -1;
  for (int i = 0; i < VS23_MAX_BG; ++i) {
    bg = &m_bg[i];
    if (!bg->enabled)
      continue;
    tsx = bg->tile_size_x;
    tsy = bg->tile_size_y;
    xpoff = bg->scroll_x % tsx;
    ypoff = bg->scroll_y % tsy;

    tile_start_y = bg->scroll_y / tsy;
    tile_end_y = tile_start_y + (bg->win_h + ypoff) / tsy + 1;
    tile_split_y = tile_start_y + (last_pix_split_y - bg->win_y) / tsy;
    pix_split_y = (tile_split_y - tile_start_y) * tsy - ypoff + bg->win_y;

    // make sure pix_split_y is less or equal than last_pix_split_y
    while (pix_split_y > last_pix_split_y) {
      --tile_split_y;
      pix_split_y -= tsy;
    }

#ifdef DEBUG
    Serial.printf("bg %d win %d,%d sx/y %d,%d tile start %d(%dpx) end %d(%dpx) split %d(%dpx) pix split %dpx ypoff %dpx\n",
                  i, bg->win_x, bg->win_y, bg->scroll_x, bg->scroll_y, tile_start_y, tile_start_y*tsy,
                  tile_end_y, tile_end_y*tsy, tile_split_y, tile_split_y*tsy, pix_split_y, ypoff);
#endif
    // make sure next layer stops at or before this one
    if (pix_split_y < last_pix_split_y)
      last_pix_split_y = pix_split_y;

    // make sure drawing does not start/end before/after the actual window
    if (tile_split_y < tile_start_y)
      tile_split_y = tile_start_y;
    if (tile_split_y > tile_end_y)
      tile_split_y = tile_end_y;
    
    bg_tile_start_y[i] = tile_start_y;
    bg_tile_end_y[i] = tile_end_y;
    bg_tile_split_y[i] = tile_split_y;
    bg_pix_split_y[i] = pix_split_y;
  }

  // Drawing all backgrounds and sprites does not usually fit into the
  // vertical blank period. The screen is therefore redrawn in two passes,
  // top half and bottom half. That way, the top half can be finished
  // before the frame starts and can be displayed while the bottom half is
  // still being drawn.
  for (int pass = 0; pass < 2; ++pass) {
    // Block move programming can be done at max SPI speed.
    setSpiClockMax();

    // Draw enabled backgrounds.
    if (tsy != -1) for (int i = 0; i < VS23_MAX_BG; ++i) {
      bg = &m_bg[i];
      if (!bg->enabled)
	continue;

      if (pass == 0 && bg->win_y >= bg_pix_split_y[i])
        continue;
      if (pass == 1 && bg->win_y + bg->win_h <= bg_pix_split_y[i])
        continue;

      tsx = bg->tile_size_x;
      tsy = bg->tile_size_y;
      xpoff = bg->scroll_x % tsx;
      ypoff = bg->scroll_y % tsy;

      tile_start_y = bg_tile_start_y[i];
      tile_end_y = bg_tile_end_y[i];
      tile_split_y = bg_tile_split_y[i];
      pix_split_y = bg_pix_split_y[i];

      tile_start_x = bg->scroll_x / bg->tile_size_x;
      tile_end_x = tile_start_x + (bg->win_w + bg->tile_size_x-1) / bg->tile_size_x + 1;

      pat_start_addr = pixelAddr(bg->pat_x, bg->pat_y);
      win_start_addr = pixelAddr(bg->win_x, bg->win_y);

      dest_addr_start = win_start_addr + (m_pitch * (pix_split_y - bg->win_y) * pass) - tile_start_x * tsx - xpoff;

      // drawBg() does not handle partial lines. If a BG ends just before the split, we have to let
      // drawBgBottom() handle the last line; otherwise, drawBg() has to draw an extra line.
      int overdraw_y = 0;
      if (pass == 0 && bg->win_y + bg->win_h >= bg_pix_split_y[i])
        overdraw_y = 1;

      if (pass == 0)
	drawBgTop(bg, dest_addr_start, pat_start_addr, tile_start_x, tile_start_y, tile_end_x, xpoff, ypoff);

      drawBg(bg, dest_addr_start, pat_start_addr, win_start_addr,
             tile_start_x, tile_start_y,
             tile_end_x, pass ? tile_end_y : tile_split_y+overdraw_y, xpoff, ypoff, 0, pass ? (tile_split_y - tile_start_y) : 1);

      if (pass == 1 || overdraw_y == 0)
	drawBgBottom(bg, tile_start_x, tile_end_x, tile_end_y, xpoff, ypoff, 0);

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

    uint8_t bbuf[VS23_MAX_SPRITE_W];
    uint8_t sbuf[VS23_MAX_SPRITE_W];

#ifndef DISABLE_SPRITE_DRAW
    for (int sn = 0; sn < VS23_MAX_SPRITES; ++sn) {
      struct sprite_t *s = m_sprites_ordered[sn];
      if (!s->enabled)
        continue;
      if (s->pos_x < -s->w || s->pos_y < -s->h)
        continue;
      if (s->pos_x >= m_current_mode->x || s->pos_y >= m_current_mode->y)
        continue;
      if (pass == 0 && s->pos_y >= last_pix_split_y)
        continue;
      if (pass == 1 && s->pos_y + s->h <= last_pix_split_y)
        continue;
      if (s->transparent) {
        int sx = s->pos_x;
        uint32_t spr_addr = m_first_line_addr + max(0, s->pos_y) * m_pitch + max(0, sx);

        int draw_w = s->w;
        int draw_h = s->h;

        if (s->pos_y < 0)
          draw_h += s->pos_y;
        else if (s->pos_y + s->h > m_current_mode->y)
          draw_h -= s->pos_y + s->h - m_current_mode->y;

        if (s->pos_x < 0)
          draw_w += s->pos_x;
        else if (s->pos_x + s->w > m_current_mode->x)
          draw_w -= s->pos_x + s->w - m_current_mode->x;

        // Draw sprites crossing the screen partition in two steps, top half
        // in the first pass, bottom half in the second pass.
        int offset_y = 0;
        if (pass == 0 && s->pos_y + draw_h > last_pix_split_y)
          draw_h = last_pix_split_y - s->pos_y;
        if (pass == 1 && s->pos_y < last_pix_split_y && s->pos_y + draw_h > last_pix_split_y) {
          draw_h -= last_pix_split_y - s->pos_y;
          offset_y = last_pix_split_y - s->pos_y;
          spr_addr += offset_y * m_pitch;
        }

        for (int sy = 0; sy < draw_h; ++sy) {
          struct sprite_line *sl = &s->pattern[sy+offset_y];
          // Copy sprite data to SPI send buffer.
          os_memcpy(sbuf, sl->pixels, s->w);

          if (sl->type == LINE_BROKEN) {
            // This line has inner transparent pixels; we read the screen
            // to fill in the gaps.

            // Playing fast and loose with data integrity here: we can use
            // much higher SPI speeds than the stable 11 MHz if we accept
            // that there is an occasional corrupted byte, which will
            // be corrected in the next frame.
            setSpiClockRead();
            SpiRamReadBytesFast(spr_addr + sy*m_pitch + sl->off, bbuf, sl->len);
            setSpiClock(spi_clock_default);

            for (int p = 0; p < sl->len; ++p) {
              if (!sbuf[p + sl->off])
                sbuf[p+sl->off] = bbuf[p];
            }
          }

          SpiRamWriteBytesFast(spr_addr + sy*m_pitch + sl->off, sbuf + sl->off, sl->len);
        }
      } else {
        int w = s->w;
        int h = s->h;
        int x = s->pos_x;
        int y = s->pos_y;

        if (x < 0) {
          w += x;
          x = 0;
        } else if (x + w >= m_current_mode->x) {
          w = m_current_mode->x - x;
        }
        if (y < 0) {
          h += y;
          y = 0;
        } else if (y + h >= m_current_mode->y) {
          h = m_current_mode->y - y;
        }

        // Draw sprites crossing the screen partition in two steps, top half
        // in the first pass, bottom half in the second pass.
        int offset_y = 0;
        if (pass == 0 && y + h > last_pix_split_y)
          h = last_pix_split_y - y;
        if (pass == 1 && y < last_pix_split_y && y + h > last_pix_split_y) {
          offset_y = last_pix_split_y - y;
          h -= offset_y;
        }
        if (w > 0 && h > 0)
          MoveBlock(s->pat_x, s->pat_y + offset_y, x, y + offset_y, w, h, 0);
      }
    }
#endif

    // Adjust sync line to avoid flicker due to either the bottom half
    // being drawn too early or the top half being drawn too late.
    if (pass == 0)
      pass0_end_line = currentLine();
    else {
      bool pass0_wraparound = pass0_end_line < m_sync_line;
      uint16_t cl = currentLine();
      if (!(!pass0_wraparound && cl >= pass0_end_line) &&
          cl > m_current_mode->y / 3 && 
          m_sync_line > m_current_mode->y * 2 / 3) {
        m_sync_line--;
#ifdef DEBUG_SYNC
        Serial.printf("sync-- %d\n", m_sync_line);
#endif
      } else if (!pass0_wraparound && pass0_end_line < m_current_mode->y + m_current_mode->top) {
        m_sync_line++;
#ifdef DEBUG_SYNC
        Serial.printf("sync++ %d\n", m_sync_line);
#endif
      }
    }

#ifdef PROFILE_BG
    Serial.printf("%d %d %d %d %d %d\n", lines[0], lines[1], lines[2], lines[3], lines[4], cl);
#endif
  } // pass
#ifdef PROFILE_BG
  if (millis() > mxx)
    Serial.println(millis() - mxx);
#endif

  setSpiClock(spi_clock_default);
  SpiUnlock();
}

void VS23S010::allocateSpritePattern(struct sprite_t *s)
{
  s->pattern = (struct sprite_line *)malloc(s->h * sizeof(struct sprite_line));
  for (int i = 0; i < s->h; ++i)
    s->pattern[i].pixels = (uint8_t *)malloc(s->w);
}

void VS23S010::freeSpritePattern(struct sprite_t *s)
{
  for (int i = 0; i < s->h; ++i)
    free(s->pattern[i].pixels);
  free(s->pattern);
  s->pattern = NULL;
}

void VS23S010::resizeSprite(uint8_t num, uint8_t w, uint8_t h)
{
  struct sprite_t *s = &m_sprite[num];
  if ((w != s->w || h != s->h) && s->pattern) {
    freeSpritePattern(s);
  }
  s->w = w;
  s->h = h;
  if (!s->pattern) {
    allocateSpritePattern(s);
  }
  loadSpritePattern(num);
  m_bg_modified = true;
}

void VS23S010::loadSpritePattern(uint8_t num)
{
  struct sprite_t *s = &m_sprite[num];
  uint32_t tile_addr = pixelAddr(s->pat_x + s->frame_x * s->w,
                                 s->pat_y + s->frame_y * s->h);

  if (!s->pattern)
    allocateSpritePattern(s);

  bool solid_block = true;

  for (int sy = 0; sy < s->h; ++sy) {
    struct sprite_line *p = &s->pattern[sy];
    SpiRamReadBytes(tile_addr + sy*m_pitch, p->pixels, s->w);

    p->off = 0;
    p->len = s->w;
    p->type = LINE_SOLID;

    uint8_t *pp = p->pixels;
    while (*pp == 0 && p->len) {
      solid_block = false;
      ++pp;
      ++p->off;
      --p->len;
    }

    if (p->len) {
      pp = p->pixels + s->w - 1;
      while (*pp == 0) {
	solid_block = false;
	--pp;
	--p->len;
      }
    }

    for (int i = 0; i < p->len; ++i) {
      if (p->pixels[p->off + i] == 0) {
	p->type = LINE_BROKEN;
	break;
      }
    }
#ifdef DEBUG_SPRITES
    Serial.printf("  def line %d off %d len %d type %d\n", sy, p->off, p->len, p->type);
#endif
  }

  s->transparent = !solid_block;
}

void VS23S010::setSpriteFrame(uint8_t num, uint8_t frame_x, uint8_t frame_y)
{
  m_sprite[num].frame_x = frame_x;
  m_sprite[num].frame_y = frame_y;
  loadSpritePattern(num);
  m_bg_modified = true;
}

void VS23S010::setSpritePattern(uint8_t num, uint16_t pat_x, uint16_t pat_y)
{
  struct sprite_t *s = &m_sprite[num];
  s->pat_x = pat_x;
  s->pat_y = pat_y;
  s->frame_x = s->frame_y = 0;

  loadSpritePattern(num);
  m_bg_modified = true;
#ifdef DEBUG_SPRITES
  Serial.printf("defined %d\n", num); Serial.flush();
#endif
}

void VS23S010::enableSprite(uint8_t num)
{
  struct sprite_t *s = &m_sprite[num];
  if (!s->pattern)
    loadSpritePattern(num);
  s->enabled = true;
  m_bg_modified = true;
}

void VS23S010::disableSprite(uint8_t num)
{
  struct sprite_t *s = &m_sprite[num];
  s->enabled = false;
  m_bg_modified = true;
}

int VS23S010::cmp_sprite_y(const void *one, const void *two)
{
  return (*(struct sprite_t**)one)->pos_y - (*(struct sprite_t **)two)->pos_y;
}

void VS23S010::moveSprite(uint8_t num, int16_t x, int16_t y)
{
  m_sprite[num].pos_x = x;
  m_sprite[num].pos_y = y;
  qsort(m_sprites_ordered, VS23_MAX_SPRITES, sizeof(struct sprite_t *), cmp_sprite_y);
  m_bg_modified = true;
}

void VS23S010::setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t)
{
  struct bg_t *bg = &m_bg[bg_idx];
  int tile_scroll_x = bg->scroll_x / bg->tile_size_x;
  int tile_scroll_y = bg->scroll_y / bg->tile_size_y;
  int tile_w = bg->win_w / bg->tile_size_x;
  int tile_h = bg->win_h / bg->tile_size_y;

  bg->tiles[(y % bg->h) * bg->w + (x % bg->w)] = t;
  if (x >= tile_scroll_x &&
      x < tile_scroll_x + tile_w &&
      y >= tile_scroll_y &&
      y < tile_scroll_y + tile_h) {
    m_bg_modified = true;
  }
}

bool VS23S010::allocBacking(int w, int h, int &x, int &y)
{
  Rect r = m_bin.Insert(w, h, false, GuillotineBinPack::RectBestAreaFit, GuillotineBinPack::Split256);
  x = r.x; y = r.y + m_current_mode->y;
  return r.height != 0;
}

void VS23S010::freeBacking(int x, int y, int w, int h)
{
  Rect r;
  r.x = x; r.y = y - m_current_mode->y; r.width = w; r.height = h;
  m_bin.Free(r, true);
}

void VS23S010::spriteTileCollision(uint8_t sprite, uint8_t bg_idx, uint8_t *tiles, uint8_t num_tiles)
{
  sprite_t *spr = &m_sprite[sprite];
  bg_t *bg = &m_bg[bg_idx];
  int tsx = bg->tile_size_x;
  int tsy = bg->tile_size_y;
  uint8_t res[num_tiles];
  
  // Intialize results with "no collision".
  memset(res, 0, num_tiles);
  
  // Check if sprite is overlapping with background at all.
  if (spr->pos_x + spr->w <= bg->win_x ||
      spr->pos_x >= bg->win_x + bg->win_w ||
      spr->pos_y + spr->h <= bg->win_y ||
      spr->pos_y >= bg->win_y + bg->win_h)
    return;
  
  // Calculate pixel coordinates of top/left sprite corner in relation to
  // the background's origin.
  int bg_left_x = bg->scroll_x - bg->win_x + spr->pos_x;
  int bg_top_y = bg->scroll_y - bg->win_y + spr->pos_y;

  // Calculate offset, width and height in tiles.
  // round down top/left
  int bg_first_tile_off = bg_left_x / tsx + bg->w * (bg_top_y / tsy);
  // round up width/height
  int bg_tile_width = (spr->w + tsx - 1) / tsx;
  int bg_tile_height =  (spr->h + tsy - 1) / tsy;
  
  int bg_last_tile_off = bg_first_tile_off + bg_tile_width + bg_tile_height * bg->w;
  
  // Iterate over all tiles overlapping the sprite and record in what way they are
  // overlapping.
  // For every line
  for (int t = bg_first_tile_off, ty = bg_top_y; t < bg_last_tile_off; t += bg->w, ty += tsy) {
    // For every column
    for (int tt = t, tx = bg_left_x; tt < t + bg->w; ++tt, tx += tsx) {
      // For every tile code to be checked
      for (int m = 0; m < num_tiles; ++m) {
        if (tiles[m] == bg->tiles[tt % (bg->w*bg->h)]) {
          res[m] = 0x40;	// indicates collision in general
          if (tx < spr->pos_x)
            res[m] |= psxLeft;
          else if (tx + tsx > spr->pos_x + spr->w)
            res[m] |= psxRight;
          if (ty < spr->pos_y)
            res[m] |= psxUp;
          else if (ty + tsy > spr->pos_y + spr->h)
            res[m] |= psxDown;
        }
      }
    }
  }
  os_memcpy(tiles, res, num_tiles);
}

uint8_t VS23S010::spriteTileCollision(uint8_t sprite, uint8_t bg, uint8_t tile)
{
  spriteTileCollision(sprite, bg, &tile, 1);
  return tile;
}

uint8_t VS23S010::spriteCollision(uint8_t collidee, uint8_t collider)
{
  uint8_t dir = 0x40;	// indicates collision

  sprite_t *us = &m_sprite[collidee];
  sprite_t *them = &m_sprite[collider];
  
  if (us->pos_x + us->w < them->pos_x)
    return 0;
  if (them->pos_x + them->w < us->pos_x)
    return 0;
  if (us->pos_y + us->h < them->pos_y)
    return 0;
  if (them->pos_y + them->h < us->pos_y)
    return 0;
  
  // sprite frame as bounding box; we may want something more flexible...
  if (them->pos_x < us->pos_x)
    dir |= psxLeft;
  else if (them->pos_x + them->w > us->pos_x + us->w)
    dir |= psxRight;
  if (them->pos_y < us->pos_y)
    dir |= psxUp;
  else if (them->pos_y + them->h > us->pos_y + us->h)
    dir |= psxDown;

  return dir;
}

VS23S010 vs23;
#endif
