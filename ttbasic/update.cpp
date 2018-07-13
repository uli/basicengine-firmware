/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2018 Ulrich Hecht.
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
#include "basic.h"
#include "BString.h"
#include "error.h"
#include "vs23s010.h"

#ifndef HOSTED

#include <Updater.h>
#include <spi_flash.h>

extern "C" void startup(void);

static void flash_user(BString filename, int sector)
{
  uint8_t *buf;
  int x, y, count;

  // Check that there is at least 32k of memory available,
  // just to be on the safe side.
  if (umm_free_heap_size() < 32768 ||
      !(buf = (uint8_t *)malloc(SPI_FLASH_SEC_SIZE))) {
    err = ERR_OOM;
    return;
  }
  Unifile f = Unifile::open(filename, FILE_READ);
  if (!f) {
    err = ERR_FILE_OPEN;
    return;
  }

  uint32_t size = f.fileSize();
  int end_sector = sector + (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;

  newline();
  PRINT_P("Are you sure you want to overwrite the program\n"
          "currently installed in sectors ");
  putnum(sector, 0); PRINT_P(" to "); putnum(end_sector - 1, 0);
  PRINT_P(" with ");
  c_puts(filename.c_str());
  c_putch('?'); newline(); newline();

  PRINT_P("Enter YES to continue, anything else to abort: ");
  get_input();
  if (strcmp(lbuf, "YES")) {
    PRINT_P("Aborting.\n");
    goto out;
  }

  sc0.show_curs(0);
  PRINT_P("Erasing sector ");
  x = sc0.c_x(); y = sc0.c_y();
  for (int i = sector; i < end_sector; ++i) {
    sc0.locate(x, y);
    putnum(i, 0);
    noInterrupts();
    if (!ESP.flashEraseSector(i)) {
      interrupts();
      newline();
      err = ERR_IO;
      return;
    }
    interrupts();
  }

  newline();
  PRINT_P("Writing data: ");
  x = sc0.c_x(); y = sc0.c_y();
  sc0.locate(x + 7, y);
  c_putch('/'); putnum(f.fileSize(), 6); PRINT_P(" bytes");

  count = 0;
  for (;;) {
    sc0.locate(x, y);
    putnum(count, 6);

    ssize_t redd = f.read((char *)buf, SPI_FLASH_SEC_SIZE);
    if (redd < 0) {
      err = ERR_IO;
      goto out;
    }
    if (!redd)
      break;
    noInterrupts();
    if (!ESP.flashWrite(sector * SPI_FLASH_SEC_SIZE + count, (uint32_t *)buf, redd)) {
      interrupts();
      err = ERR_IO;
      goto out;
    }
    interrupts();
    count += redd;
  }
  newline();

out:
  free(buf);
  f.close();
  sc0.show_curs(1);
}

void iflash()
{
  BString filename;
  bool success;
  uint8_t *buf;
  int x, y, count;
  
  uint8_t col_warn = csp.colorFromRgb(255, 64, 0);
  uint8_t col_normal = csp.colorFromRgb(255, 255, 255);

  // Check that there is at least 32k of memory available,
  // just to be on the safe side.
  if (umm_free_heap_size() < 32768 ||
      !(buf = (uint8_t *)malloc(4096))) {
    err = ERR_OOM;
    return;
  }

  if (!(filename = getParamFname()))
    return;

  int32_t sector = 0;
  if (*cip == I_COMMA) {
    ++cip;
    getParam(sector, 524288 / SPI_FLASH_SEC_SIZE, 1048576 / SPI_FLASH_SEC_SIZE - 1, I_NONE);
    if (err)
      return;
    flash_user(filename, sector);
    return;
  }

  Unifile f = Unifile::open(filename, FILE_READ);
  if (!f) {
    err = ERR_FILE_OPEN;
    return;
  }

  newline();
  sc0.setColor(0, col_warn);
  PRINT_P("FIRMWARE UPDATE\n");
  sc0.setColor(col_normal, 0);

  newline();
  PRINT_P("Are you absolutely sure you want to overwrite\n"
          "the currently installed firmware\n"
          "with ");
  sc0.setColor(col_warn, 0);
  c_puts(filename.c_str());
  sc0.setColor(col_normal, 0);
  c_putch('?'); newline(); newline();

  PRINT_P("If "); c_puts(filename.c_str());
  PRINT_P("is not a working\n"
          "BASIC Engine firmware image, or if you\n"
          "lose power or reset the system during\n"
          "the upgrade procedure, it will no longer work\n"
          "until you use a serial programmer to restore\n"
          "a working firmware image.\n\n");

  sc0.setColor(col_warn, 0);
  PRINT_P("Enter YES to continue, anything else to abort: ");
  get_input();
  sc0.setColor(col_normal, 0);
  if (strcmp(lbuf, "YES")) {
    PRINT_P("Aborting.\n");
    goto out;
  }

  sc0.show_curs(0);

  if (!Update.begin(f.fileSize())) {
    PRINT_P("Flash updater init failed, error: ");
    putnum(Update.getError(), 0);
    newline();
    goto out;
  }

  PRINT_P("Staging update: ");

  x = sc0.c_x(); y = sc0.c_y();
  sc0.locate(x + 7, y);
  c_putch('/'); putnum(f.fileSize(), 6); PRINT_P(" bytes");

  count = 0;
  for (;;) {
    sc0.locate(x, y);
    putnum(count, 6);

    ssize_t redd = f.read((char *)buf, 4096);
    if (redd < 0) {
      err = ERR_IO;
      goto out;
    }
    if (!redd)
      break;
    if (Update.write(buf, redd) != (size_t)redd) {
      err = ERR_IO;
      goto out;
    }
    count += redd;
  }
  newline();

  free(buf);
  f.close();

  success = Update.end();
  if (success) {
    PRINT_P("Staging update successful.\n\n");
    sc0.setColor(col_warn, 0);
    PRINT_P("Writing update to flash.\n");
    sc0.setColor(0, col_warn);
    PRINT_P("DO NOT RESET OR POWER OFF!\n");
#ifdef ESP8266_NOWIFI
    // SDKnoWiFi does not have system_restart*(). The only working
    // alternative I could find is triggering the WDT.
    ets_wdt_enable(2,3,3);
    for(;;);
#else
    ESP.reset();	// UNTESTED!
#endif
  } else {
    PRINT_P("Staging error: ");
    putnum(Update.getError(), 0); newline();
    sc0.show_curs(1);
  }
  return;

out:
  free(buf);
  f.close();
  // XXX: clean up Updater?
  sc0.show_curs(1);
}

#else	// !HOSTED
void iflash() {
  err = ERR_NOT_SUPPORTED;
}
#endif
