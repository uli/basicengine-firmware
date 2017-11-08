#include "ttconfig.h"

#if USE_VS23 == 1

#include "vs23s010.h"
#include "ntsc.h"
#include "lock.h"

//#define DISABLE_BG_LEFT_COL
//#define DISABLE_BG_MIDDLE
//#define DISABLE_BG_RIGHT_COL

//#define DISABLE_SPRITE_SAVE
//#define DISABLE_SPRITE_RESTORE
//#define DISABLE_SPRITE_DRAW

void VS23S010::setPixel(uint16_t x, uint16_t y, uint8_t c)
{
  uint32_t byteaddress = pixelAddr(x, y);
  SpiRamWriteByte(byteaddress, c);
}

void VS23S010::adjust(int16_t cnt)
{
  // XXX: Huh?
}

#include <SPI.h>

static int absolute_min_spi_div;
void VS23S010::begin()
{
  m_vsync_enabled = false;
  for (int i = 0; i < VS23_MAX_SPRITES; ++i) {
    m_sprites_ordered[i] = &m_sprite[i];
    m_sprite[i].enabled = false;
    m_sprite[i].old_enabled = false;
    m_sprite[i].transparent = false;
  }

  m_bin.Init(0, 0);

  SpiLock();
  for (int i = 0; i < numModes; ++i) {
    SPI.setFrequency(modes[i].max_spi_freq);
    modes[i].max_spi_freq = SPI1CLK;
  }
  SPI.setFrequency(38000000);
  absolute_min_spi_div = SPI1CLK;
  SPI.setFrequency(11000000);
  SpiUnlock();
}

void VS23S010::end()
{
  m_bin.Init(0, 0);
}

void VS23S010::setMode(uint8_t mode)
{
  setSyncLine(0);
  m_current_mode = &modes[mode];
  m_last_line = PICLINE_MAX;
  m_first_line_addr = PICLINE_BYTE_ADDRESS(0);
  m_pitch = PICLINE_BYTE_ADDRESS(1) - m_first_line_addr;

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
  m_cycles_per_frame = ESP.getCycleCount() - now;
}

void ICACHE_RAM_ATTR VS23S010::vsyncHandler(void)
{
  uint32_t now = ESP.getCycleCount();
  uint32_t next = now + vs23.m_cycles_per_frame;
  uint16_t line;
  if (!SpiLocked()) {
    line = vs23.currentLine();
    if (line < vs23.m_sync_line) {
      next += (vs23.m_cycles_per_frame / 262) * (vs23.m_sync_line-line);
      vs23.m_cycles_per_frame += 10;
    } else if (line > vs23.m_sync_line) {
      next -= (vs23.m_cycles_per_frame / 262) * (line-vs23.m_sync_line);
      vs23.m_cycles_per_frame -= 10;
    }
#ifdef DEBUG
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
  if (line == 0) {
    if (m_vsync_enabled)
      timer0_detachInterrupt();
    m_vsync_enabled = false;
  } else {
    m_sync_line = line;
    timer0_isr_init();
    timer0_attachInterrupt(&vsyncHandler);
    // Make sure interrupt is triggered soon.
    timer0_write(ESP.getCycleCount()+100);
    m_vsync_enabled = true;
  }
}

bool VS23S010::defineBg(uint8_t bg_idx, uint16_t width, uint16_t height,
                     uint8_t tile_size_x, uint8_t tile_size_y,
                     uint16_t pat_x, uint16_t pat_y, uint16_t pat_w)
{
  struct bg_t *bg = &m_bg[bg_idx];

  bg->enabled = false;

  if (bg->tiles)
    free(bg->tiles);
  bg->tiles = (uint8_t *)calloc(width * height, 1);
  if (!bg->tiles)
    return true;

  for (int i=0; i < width*height; ++i)
    bg->tiles[i] = 64+(i&7);

  bg->pat_x = pat_x;
  bg->pat_y = pat_y;
  bg->pat_w = pat_w;
  bg->tile_size_x = tile_size_x;
  bg->tile_size_y = tile_size_y;
  bg->w = width;
  bg->h = height;
  bg->scroll_x = bg->scroll_y = 5;
  bg->old_scroll_x = bg->old_scroll_y = 0;
  bg->win_x = bg->win_y = 0;
  bg->win_w = m_current_mode->x;
  bg->win_h = m_current_mode->y;
  return false;
}

void VS23S010::enableBg(uint8_t bg)
{
  if (m_bg[bg].tiles) {
    m_bg[bg].force_redraw = true;
    m_bg[bg].enabled = true;
  }
}

void VS23S010::disableBg(uint8_t bg)
{
  m_bg[bg].enabled = false;
}

void VS23S010::freeBg(uint8_t bg_idx)
{
  struct bg_t *bg = &m_bg[bg_idx];
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
  bg->force_redraw = true;
}

static inline void ICACHE_RAM_ATTR MoveBlockAddr(uint32_t byteaddress2, uint32_t dest_addr)
{
      // XXX: What about PYF?
      //SpiRamWriteBMCtrl(0x34, byteaddress2 >> 1, dest_addr >> 1, ((dest_addr & 1) << 1) | ((byteaddress2 & 1) << 2));
      uint8_t req[5] = { 0x34, byteaddress2 >> 9, byteaddress2 >> 1, dest_addr >> 9, dest_addr >> 1 };
      VS23_SELECT;
      SPI.writeBytes(req, 5);
      VS23_DESELECT;
      while (!blockFinished()) {}
      //SpiRamWriteBM3Ctrl(0x36);
      VS23_SELECT;
      SPI.write(0x36);
      VS23_DESELECT;
}

static inline void ICACHE_RAM_ATTR SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count)
{
  
  data[0] = 3;
  data[1] = address >> 16;
  data[2] = address >> 8;
  data[3] = address;
  VS23_SELECT;
  SPI.transferBytes(data, data, count+4);
  VS23_DESELECT;
}

static inline void ICACHE_RAM_ATTR SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len)
{
  data[0] = 2;
  data[1] = address >> 16;
  data[2] = address >> 8;
  data[3] = address;
  VS23_SELECT;
  SPI.writeBytes(data, len+4);
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
                            uint32_t skip_x,
                            uint32_t skip_y)
{
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
    dest_addr_start = win_start_addr + ((yy - tile_start_y) * tsy - ypoff) * m_pitch + bg->win_x - tile_start_x * tsx - xpoff;

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
}

void ICACHE_RAM_ATTR VS23S010::drawBgBottom(struct bg_t *bg,
                                                   int tile_start_x,
                                                   int tile_end_x,
                                                   int tile_end_y,
                                                   uint32_t xpoff,
                                                   uint32_t ypoff,
                                                   uint32_t skip_x)
{
  uint32_t tile;
  uint32_t tx, ty;
  uint32_t tsx = bg->tile_size_x;
  uint32_t tsy = bg->tile_size_y;
  uint8_t bg_w = bg->w;    
  uint8_t bg_h = bg->h;
  uint32_t pw = bg->pat_w;
  uint32_t byteaddress1, byteaddress2;
  int draw_w;

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
      // XXX: What about PYF?
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
}

#define SPRITE_BACKING_X(sn) ((sn) * VS23_MAX_SPRITE_W % m_current_mode->x)
#define SPRITE_BACKING_Y(sn) (m_last_line - VS23_MAX_SPRITE_H * (VS23_MAX_SPRITES * VS23_MAX_SPRITE_W / m_current_mode->x + 1))

void ICACHE_RAM_ATTR VS23S010::updateBg()
{
  static uint32_t last_frame = 0;
  uint32_t mxx;
  int lines[6];
  uint32_t tile;
  uint32_t tx, ty;
  uint32_t byteaddress2;
  int dest_addr_start;
  uint32_t dest_addr, pat_start_addr, win_start_addr;

  if (m_frame == last_frame || SpiLocked())
    return;
  last_frame = m_frame;

  int spi_clock_default = SPI1CLK;

  SpiLock();
  lines[0] = currentLine();
  for (int pass = 0; pass < 2; ++pass) {
  for (int i = 0; i < VS23_MAX_BG; ++i) {
    struct bg_t *bg = &m_bg[i];
    if (!bg->enabled)
      continue;

    SPI1CLK = absolute_min_spi_div;
    mxx = millis();

    int tile_start_y = bg->scroll_y / bg->tile_size_y;
    int tile_end_y = tile_start_y + (bg->win_h + bg->tile_size_y-1) / bg->tile_size_y + 1;
    int tile_start_x = bg->scroll_x / bg->tile_size_x;
    int tile_end_x = tile_start_x + (bg->win_w + bg->tile_size_x-1) / bg->tile_size_x + 1;

    uint32_t tsx = bg->tile_size_x;
    uint32_t tsy = bg->tile_size_y;
    uint32_t xpoff = bg->scroll_x % tsx;
    uint32_t ypoff = bg->scroll_y % tsy;
    uint32_t pw = bg->pat_w;
    pat_start_addr = pixelAddr(bg->pat_x, bg->pat_y);
    win_start_addr = pixelAddr(bg->win_x, bg->win_y);
    uint8_t bg_w = bg->w;    
    uint8_t bg_h = bg->h;

    // Find split point at a tile boundary
    uint32_t tile_split_y = tile_start_y + (tile_end_y - tile_start_y) / 2;
    uint32_t pix_split_y = ((tile_end_y - tile_start_y) / 2) * tsy - ypoff;

    int scroll_dx = bg->scroll_x - bg->old_scroll_x;
    int scroll_dy = bg->scroll_y - bg->old_scroll_y;

    dest_addr_start = win_start_addr + (m_pitch * pix_split_y * pass) - tile_start_x * tsx - xpoff;
    if (bg->force_redraw || abs(bg->scroll_x - bg->old_scroll_x) > 8 || abs(bg->scroll_y - bg->old_scroll_y) > 8) {
      if (pass == 0)
        drawBgTop(bg, dest_addr_start, pat_start_addr, tile_start_x, tile_start_y, tile_end_x, xpoff, ypoff);

      drawBg(bg, dest_addr_start, pat_start_addr, win_start_addr,
             tile_start_x, tile_start_y,
             tile_end_x, pass ? tile_end_y : tile_split_y+2, xpoff, ypoff, 0, pass ? (tile_split_y - tile_start_y) : 1);

      if (pass == 1)
        drawBgBottom(bg, tile_start_x, tile_end_x, tile_end_y, xpoff, ypoff, 0);
      
      scroll_dy = 0;
      lines[1] = currentLine();
      lines[2] = 0;
    } else {
      int old_tile_start_y = bg->old_scroll_y / tsy;
      int old_tile_end_y = old_tile_start_y + (bg->win_h + tsy-1) / tsy + 1;
      int old_tile_start_x = bg->old_scroll_x / tsx;
      int old_tile_end_x = old_tile_start_x + (bg->win_w + tsx-1) / tsx + 1;
      uint32_t old_xpoff = bg->old_scroll_x % tsx;
      uint32_t old_ypoff = bg->old_scroll_y % tsy;
      int old_dest_addr_start = win_start_addr - old_tile_start_x * tsx - old_xpoff;

      lines[1] = currentLine();

      uint8_t x_dir, y_dir;
      uint32_t src_x, src_y, dst_x, dst_y;
      uint32_t w, h;
      bool draw_left = false;
      bool draw_right = false;
      bool draw_top = false;
      bool draw_bottom = false;

      if (scroll_dx == 0 && scroll_dy == 0)
        goto restore_backing;

      if (bg->scroll_y > bg->old_scroll_y) {
        y_dir = 0;
        dst_y = 0;
        src_y = bg->scroll_y - bg->old_scroll_y;
        h = bg->win_h - src_y;
        draw_bottom = true;
      } else {
        if (bg->scroll_y != bg->old_scroll_y) {
          draw_top = true;
          y_dir = 1;
        } else
          y_dir = 0;
        dst_y = bg->old_scroll_y - bg->scroll_y;
        src_y = 0;
        h = bg->win_h - dst_y;
      }
      if (bg->scroll_x > bg->old_scroll_x) {
        x_dir = 0;
        dst_x = 0;
        src_x = bg->scroll_x - bg->old_scroll_x;
        w = bg->win_w - src_x;
        draw_right = true;
      } else {
        if (bg->scroll_x != bg->old_scroll_x) {
          x_dir = 1;
          draw_left = true;
        } else
          x_dir = 0;
        dst_x = bg->old_scroll_x - bg->scroll_x;
        src_x = 0;
        w = bg->win_w - dst_x;
      }
      
      if (y_dir) {
        src_x += w - 1;
        dst_x += w - 1;
        src_y += h - 1;
        dst_y += h - 1;
      }
      
      if (w > 255) {
        if (x_dir == 0) {
          if (pass == 0) {
            if (y_dir) {
              MoveBlock(bg->win_x, pix_split_y, 0, SPRITE_BACKING_Y(0)-8, w/2, -scroll_dy, 0);
              MoveBlock(bg->win_x+w/2, pix_split_y, w/2, SPRITE_BACKING_Y(0)-8, w/2, -scroll_dy, 0);
              // XXX: Why is this necessary? In a "backwards after forwards" case, MoveBlock()
              // rewrites 0x34, but then waits for completion. Shouldn't that be safe?
              while (!blockFinished()) {}
            }
            MoveBlock(src_x, src_y - (y_dir ? (h - pix_split_y) : 0), dst_x, dst_y - (y_dir ? (h - pix_split_y) : 0), w/2, pix_split_y, y_dir);
            MoveBlock(src_x + (y_dir ? -w/2 : w/2), src_y - (y_dir ? (h - pix_split_y) : 0), dst_x + (y_dir ? -w/2 : w/2), dst_y - (y_dir ? (h - pix_split_y) : 0), w/2, pix_split_y, y_dir);
          } else {
            MoveBlock(src_x, src_y + (y_dir ? 0 : pix_split_y), dst_x, dst_y + (y_dir ? 0 : pix_split_y), w/2, h-pix_split_y, y_dir);
            MoveBlock(src_x + (y_dir ? -w/2 : w/2), src_y + (y_dir ? 0 : pix_split_y), dst_x + (y_dir ? -w/2 : w/2), dst_y + (y_dir ? 0 : pix_split_y), w/2, h-pix_split_y, y_dir);
            if (y_dir) {
              while (!blockFinished()) {}
              MoveBlock(scroll_dx > 0 ? scroll_dx : 0, SPRITE_BACKING_Y(0)-8, bg->win_x-(scroll_dx < 0 ? scroll_dx : 0), pix_split_y-scroll_dy, w/2, -scroll_dy, 0);
              MoveBlock((scroll_dx > 0 ? scroll_dx : 0) + w/2, SPRITE_BACKING_Y(0)-8, bg->win_x-(scroll_dx < 0 ? scroll_dx : 0)+w/2, pix_split_y-scroll_dy, w/2, -scroll_dy, 0);
            }
          }
        } else {
          if (pass == 0) {
            if (y_dir) {
              MoveBlock(bg->win_x, pix_split_y, 0, SPRITE_BACKING_Y(0)-8, w/2, -scroll_dy, 0);
              MoveBlock(bg->win_x+w/2, pix_split_y, w/2, SPRITE_BACKING_Y(0)-8, w/2, -scroll_dy, 0);
              // XXX: Why is this necessary? In a "backwards after forwards" case, MoveBlock()
              // rewrites 0x34, but then waits for completion. Shouldn't that be safe?
              while (!blockFinished()) {}
            }
            MoveBlock(src_x + (y_dir ? -w/2 : w/2), src_y - (y_dir ? (h - pix_split_y) : 0), dst_x + (y_dir ? -w/2 : w/2), dst_y - (y_dir ? (h - pix_split_y) : 0), w/2, pix_split_y, y_dir);
            MoveBlock(src_x, src_y - (y_dir ? (h - pix_split_y) : 0), dst_x, dst_y - (y_dir ? (h - pix_split_y) : 0), w/2, pix_split_y, y_dir);
          } else {
            MoveBlock(src_x + (y_dir ? -w/2 : w/2), src_y + (y_dir ? 0 : pix_split_y), dst_x + (y_dir ? -w/2 : w/2), dst_y + (y_dir ? 0 : pix_split_y), w/2, h-pix_split_y, y_dir);
            MoveBlock(src_x, src_y + (y_dir ? 0 : pix_split_y), dst_x, dst_y + (y_dir ? 0 : pix_split_y), w/2, h-pix_split_y, y_dir);
            if (y_dir) {
              MoveBlock(scroll_dx > 0 ? scroll_dx : 0, SPRITE_BACKING_Y(0)-8, bg->win_x-(scroll_dx < 0 ? scroll_dx : 0), pix_split_y-scroll_dy, w/2, -scroll_dy, 0);
              MoveBlock((scroll_dx > 0 ? scroll_dx : 0) + w/2, SPRITE_BACKING_Y(0)-8, bg->win_x-(scroll_dx < 0 ? scroll_dx : 0)+w/2, pix_split_y-scroll_dy, w/2, -scroll_dy, 0);
            }
          }
        }
      } else {
        if (pass == 0) {
          if (y_dir) {
            MoveBlock(bg->win_x, pix_split_y, 0, SPRITE_BACKING_Y(0)-8, w, -scroll_dy, 0);
            // XXX: Why is this necessary? In a "backwards after forwards" case, MoveBlock()
            // rewrites 0x34, but then waits for completion. Shouldn't that be safe?
            while (!blockFinished()) {}
          }
          MoveBlock(src_x, src_y - (y_dir ? (h - pix_split_y) : 0), dst_x, dst_y - (y_dir ? (h - pix_split_y) : 0), w, pix_split_y, y_dir);
        } else {
          MoveBlock(src_x, src_y + (y_dir ? 0 : pix_split_y), dst_x, dst_y + (y_dir ? 0 : pix_split_y), w, h-pix_split_y, y_dir);
          if (y_dir) {
            MoveBlock(scroll_dx > 0 ? scroll_dx : 0, SPRITE_BACKING_Y(0)-8, bg->win_x-(scroll_dx < 0 ? scroll_dx : 0), pix_split_y-scroll_dy, w, -scroll_dy, 0);
          }
        }
      }
        
      lines[2] = currentLine();

      while (!blockFinished()) {}

      if (pass == 0 && draw_top) {
        drawBgTop(bg, dest_addr_start, pat_start_addr, tile_start_x, tile_start_y, tile_end_x, xpoff, ypoff);
      }
      if (draw_left) {
        if (!draw_top && pass == 0) {
          drawBgTop(bg, dest_addr_start, pat_start_addr, tile_start_x, tile_start_y, tile_start_x + 2, xpoff, ypoff);
          while (!blockFinished()) {}
        }

        drawBg(bg, dest_addr_start, pat_start_addr, win_start_addr,
               tile_start_x, tile_start_y,
               tile_start_x + 2, pass ? tile_end_y : tile_split_y+1,
               xpoff, ypoff,
               0, pass ? (tile_split_y-tile_start_y) : 1);

        if (!draw_bottom && pass == 1)
          drawBgBottom(bg, tile_start_x, tile_start_x + 1, tile_end_y, xpoff, ypoff, 0);
      }
      if (draw_right) {
        if (!draw_top && pass == 0) {
          drawBgTop(bg, dest_addr_start, pat_start_addr, tile_end_x - 2, tile_start_y, tile_end_x, xpoff, ypoff);
          while (!blockFinished()) {}
        }

        drawBg(bg, dest_addr_start, pat_start_addr, win_start_addr,
               tile_start_x, tile_start_y,
               tile_end_x, pass ? tile_end_y : tile_split_y+1,
               xpoff, ypoff,
               tile_end_x - tile_start_x - 2, pass ? (tile_split_y-tile_start_y) : 1);

        if (!draw_bottom && pass == 1)
          drawBgBottom(bg, tile_start_x, tile_end_x, tile_end_y, xpoff, ypoff, tile_end_x - tile_start_x - 2);
      }
      if (draw_bottom && pass == 1) {
        if (ypoff)
          drawBgBottom(bg, tile_start_x, tile_end_x, tile_end_y, xpoff, ypoff, 0);
        else
          drawBg(bg, dest_addr_start, pat_start_addr, win_start_addr, tile_start_x, tile_start_y, tile_end_x, tile_end_y, xpoff, ypoff, 0, tile_end_y-tile_start_y-2);
      }

restore_backing:
      for (int sn = VS23_MAX_SPRITES-1; sn >= 0; --sn) {
#ifndef DISABLE_SPRITE_RESTORE
        struct sprite_t *s = m_sprites_ordered[sn];
        int sx_adj = s->old_pos_x - scroll_dx;
        int sy_adj = s->old_pos_y - scroll_dy;
        if (!s->old_enabled)
          continue;
        if (sx_adj > m_current_mode->x || sy_adj > m_current_mode->y)
          continue;
        if (sx_adj < -s->w || sy_adj < -s->h)
          continue;
        if (pass == 0 && sy_adj >= pix_split_y-scroll_dy)
          continue;
        if (pass == 1 && sy_adj + s->h <= pix_split_y-scroll_dy)
          continue;

        int w = s->w;
        int h = s->h;
        if (sx_adj < 0) {
          w += sx_adj;
          sx_adj = 0;
        } else if (sx_adj + w >= m_current_mode->x) {
          w = m_current_mode->x - sx_adj;
        }
        if (sy_adj < 0) {
          h += sy_adj;
          sy_adj = 0;
        } else if (sy_adj + h >= m_current_mode->y) {
          h = m_current_mode->y - sy_adj;
        }

        // Restore sprites crossing the screen partition in two steps, top half
        // in the first pass, bottom half in the second pass.

        // Sprite restoring, saving and drawing have to agree on a common boundary;
        // The position to which sprite saving can be done depends on the vertical
        // scrolling delta, so the other two operations have to take it into
        // account as well, even though they do not depend on it by themselves.
        int offset_y = 0;
        if (pass == 0 && sy_adj + h > pix_split_y-scroll_dy)
          h = pix_split_y-scroll_dy - sy_adj;
        if (pass == 1 && sy_adj < pix_split_y-scroll_dy && sy_adj + h > pix_split_y-scroll_dy) {
          offset_y = pix_split_y-scroll_dy - sy_adj;
          h -= offset_y;
        }

        if (w > 0 && h > 0)
          MoveBlock(SPRITE_BACKING_X(sn), SPRITE_BACKING_Y(sn) + offset_y,
                    sx_adj, sy_adj + offset_y, w, h, 0);
#endif
      }

    }

    while (!blockFinished()) {}
    if (pass == 0) lines[3] = currentLine();

    uint8_t bbuf[VS23_MAX_SPRITE_W+4];
    uint8_t sbuf[VS23_MAX_SPRITE_W+4];
    uint32_t sprite_pat_start_addr = piclineByteAddress(0);

#ifndef DISABLE_SPRITE_SAVE
    for (int sn = 0; sn < VS23_MAX_SPRITES; ++sn) {
      struct sprite_t *s = m_sprites_ordered[sn];
      if (!s->enabled)
        continue;
      if (pass == 0 && s->pos_y >= pix_split_y-scroll_dy)
        continue;
      if (pass == 1 && s->pos_y + s->h <= pix_split_y-scroll_dy)
        continue;

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

      // Save sprites crossing the screen partition in two steps, top half
      // in the first pass, bottom half in the second pass.
      int offset_y = 0;
      if (pass == 0 && y + s->h > pix_split_y-scroll_dy)
        h = pix_split_y-scroll_dy - y;
      if (pass == 1 && y < pix_split_y-scroll_dy && y + h > pix_split_y-scroll_dy) {
        offset_y = pix_split_y-scroll_dy - y;
        h -= offset_y;
      }
      
      if (w > 0 && h > 0)
        MoveBlock(x, y + offset_y, SPRITE_BACKING_X(sn), SPRITE_BACKING_Y(sn) + offset_y, w, h, 0);
    }
#endif

    // Reduce speed for memory accesses.
    SPI1CLK = spi_clock_default;
    lines[4] = currentLine();
    
#ifndef DISABLE_SPRITE_DRAW
    for (int sn = 0; sn < VS23_MAX_SPRITES; ++sn) {
      struct sprite_t *s = m_sprites_ordered[sn];
      s->old_pos_x = s->pos_x;
      s->old_pos_y = s->pos_y;
      s->old_enabled = s->enabled;
      if (!s->enabled)
        continue;
      if (s->pos_x < -s->w || s->pos_y < -s->h)
        continue;
      if (s->pos_x >= m_current_mode->x || s->pos_y >= m_current_mode->y)
        continue;
      if (pass == 0 && s->pos_y >= pix_split_y-scroll_dy)
        continue;
      if (pass == 1 && s->pos_y + s->h <= pix_split_y-scroll_dy)
        continue;
      if (s->transparent) {
        int sx = s->pos_x;
        uint32_t spr_addr = m_first_line_addr + max(0, s->pos_y) * m_pitch + max(0, sx);
        //uint32_t tile_addr = sprite_pat_start_addr + s->pat_y*m_pitch + s->pat_x;

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
        if (pass == 0 && s->pos_y + draw_h > pix_split_y-scroll_dy)
          draw_h = pix_split_y-scroll_dy - s->pos_y;
        if (pass == 1 && s->pos_y < pix_split_y-scroll_dy && s->pos_y + draw_h > pix_split_y-scroll_dy) {
          draw_h -= pix_split_y-scroll_dy - s->pos_y;
          offset_y = pix_split_y-scroll_dy - s->pos_y;
          spr_addr += offset_y * m_pitch;
        }

        for (int sy = 0; sy < draw_h; ++sy) {
          // Copy sprite data to SPI send buffer.
          memcpy(sbuf+4, s->pattern + (sy+offset_y)*s->w, s->w);

          // Playing fast and loose with data integrity here: we can use
          // much higher SPI speeds than the stable 11 MHz if we accept
          // that there is an occasional corrupted byte, which will
          // be corrected in the next frame.
          SPI1CLK = m_current_mode->max_spi_freq;
          SpiRamReadBytesFast(spr_addr + sy*m_pitch, bbuf, draw_w);
          SPI1CLK = spi_clock_default;

          for (int p = 4; p < draw_w+4; ++p) {
            if (!sbuf[p])
              sbuf[p] = bbuf[p];
          }

          SpiRamWriteBytesFast(spr_addr + sy*m_pitch, sbuf, draw_w);
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
        if (pass == 0 && y + h > pix_split_y-scroll_dy)
          h = pix_split_y-scroll_dy - y;
        if (pass == 1 && y < pix_split_y-scroll_dy && y + h > pix_split_y-scroll_dy) {
          offset_y = pix_split_y-scroll_dy - y;
          h -= offset_y;
        }
        if (w > 0 && h > 0)
          MoveBlock(s->pat_x, s->pat_y + offset_y, x, y + offset_y, w, h, 0);
      }
    }
#endif
    lines[5] = currentLine();

    if (pass == 1) {
      bg->old_scroll_x = bg->scroll_x;
      bg->old_scroll_y = bg->scroll_y;
      bg->force_redraw = false;
    }
    Serial.println(millis() - mxx);
  }
  } // pass

  SPI1CLK = spi_clock_default;
  SpiUnlock();
}

void VS23S010::defineSprite(uint8_t num, uint16_t pat_x, uint16_t pat_y, uint8_t w, uint8_t h)
{
  struct sprite_t *s = &m_sprite[num];
  s->pat_x = pat_x;
  s->pat_y = pat_y;
  s->pos_x = s->old_pos_x = 0;
  s->pos_y = s->old_pos_y = 0;
  if ((w != s->w || h != s->h) && s->pattern) {
    free(s->pattern);
    s->pattern = NULL;
  }
  s->w = w;
  s->h = h;
  if (!s->pattern) {
    s->pattern = (uint8_t *)malloc(w * h);
  }

  uint8_t *p = s->pattern;
  uint32_t tile_addr = pixelAddr(pat_x, pat_y);
  for (int sy = 0; sy < s->h; ++sy, p+=w) {
    SpiRamReadBytes(tile_addr + sy*m_pitch, p, w);
  }

  s->enabled = true;
  s->old_enabled = false;
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
}

void VS23S010::setBgTile(uint8_t bg_idx, uint16_t x, uint16_t y, uint8_t t)
{
  struct bg_t *bg = &m_bg[bg_idx];
  int tile_scroll_x = bg->scroll_x / bg->tile_size_x;
  int tile_scroll_y = bg->scroll_y / bg->tile_size_y;
  int tile_w = bg->win_w / bg->tile_size_x;
  int tile_h = bg->win_h / bg->tile_size_y;
  
  bg->tiles[y * bg->w + x] = t;
  if (x >= tile_scroll_x &&
      x < tile_scroll_x + tile_w &&
      y >= tile_scroll_y &&
      y < tile_scroll_y + tile_h) {
    bg->force_redraw = true;
  }
}

bool VS23S010::allocBacking(int w, int h, int &x, int &y)
{
  Rect r = m_bin.Insert(w, h, false, GuillotineBinPack::RectBestAreaFit, GuillotineBinPack::SplitMinimizeArea);
  x = r.x; y = r.y + m_current_mode->y;
  return r.height != 0;
}

void VS23S010::freeBacking(int x, int y, int w, int h)
{
  Rect r;
  r.x = x; r.y = y - m_current_mode->y; r.width = w; r.height = h;
  m_bin.Free(r, true);
}

VS23S010 vs23;
#endif
