#include "ttconfig.h"
#include "basic.h"
#include "BString.h"
#include "error.h"
#include "vs23s010.h"
#include <sdfiles.h>
#include <Updater.h>
#include <spi_flash.h>

extern "C" void startup(void);

#define PRINT_P(num, msg) \
  static const char _msg_ ## num[] PROGMEM = \
    msg; \
  c_puts_P(_msg_ ## num)
#define PRINTLN_P(num, msg) PRINT_P(num, msg); newline()

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
  PRINTLN_P(us0a, "Are you sure you want to overwrite the program");
  PRINT_P(us1, "currently installed in sectors ");
  putnum(sector, 0); PRINT_P(to, " to "); putnum(end_sector - 1, 0);
  PRINT_P(us2, " with ");
  c_puts(filename.c_str());
  c_putch('?'); newline(); newline();

  PRINT_P(us8, "Enter YES to continue, anything else to abort: ");
  get_input();
  if (strcmp(lbuf, "YES")) {
    PRINTLN_P(usabrt, "Aborting.");
    goto out;
  }

  sc0.show_curs(0);
  PRINT_P(es, "Erasing sector ");
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
  PRINT_P(wd, "Writing data: ");
  x = sc0.c_x(); y = sc0.c_y();
  sc0.locate(x + 7, y);
  c_putch('/'); putnum(f.fileSize(), 6); PRINT_P(usb, " bytes");

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
  char msg[100];
  BString filename;
  bool success;
  uint8_t *buf;
  int x, y, count;
  
  uint8_t col_warn = vs23.colorFromRgb(255, 64, 0);
  uint8_t col_normal = vs23.colorFromRgb(255, 255, 255);

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
  PRINTLN_P(ays0, "FIRMWARE UPDATE");
  sc0.setColor(col_normal, 0);

  newline();
  PRINTLN_P(ays0a, "Are you absolutely sure you want to overwrite");
  PRINTLN_P(ays1, "the currently installed firmware");
  PRINT_P(ays2, "with ");
  sc0.setColor(col_warn, 0);
  c_puts(filename.c_str());
  sc0.setColor(col_normal, 0);
  c_putch('?'); newline(); newline();

  static const char ays3[] PROGMEM =
    "If %s is not a working";
  sprintf_P(msg, ays3, filename.c_str());
  c_puts(msg); newline();

  PRINTLN_P(ays3a, "BASIC Engine firmware image, or if you");
  PRINTLN_P(ays4, "lose power or reset the system during");
  PRINTLN_P(ays5, "the upgrade procedure, it will no longer work");
  PRINTLN_P(ays6, "until you use a serial programmer to restore");
  PRINTLN_P(ays7, "a working firmware image.");
  newline();

  sc0.setColor(col_warn, 0);
  PRINT_P(ays8, "Enter YES to continue, anything else to abort: ");
  get_input();
  sc0.setColor(col_normal, 0);
  if (strcmp(lbuf, "YES")) {
    PRINTLN_P(aysabrt, "Aborting.");
    goto out;
  }

  sc0.show_curs(0);

  if (!Update.begin(f.fileSize())) {
    PRINT_P(aysfail, "Flash updater init failed, error: ");
    putnum(Update.getError(), 0);
    newline();
    goto out;
  }

  PRINT_P(ayssup, "Staging update: ");

  x = sc0.c_x(); y = sc0.c_y();
  sc0.locate(x + 7, y);
  c_putch('/'); putnum(f.fileSize(), 6); PRINT_P(aysb, " bytes");

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
    PRINTLN_P(ayssu, "Staging update successful."); newline();
    sc0.setColor(col_warn, 0);
    PRINTLN_P(ayswr, "Writing update to flash.");
    sc0.setColor(0, col_warn);
    PRINTLN_P(aysdo, "DO NOT RESET OR POWER OFF!");
#ifdef ESP8266_NOWIFI
    // SDKnoWiFi does not have system_restart*(). The only working
    // alternative I could find is triggering the WDT.
    ets_wdt_enable(2,3,3);
    for(;;);
#else
    ESP.reset();	// UNTESTED!
#endif
  } else {
    PRINT_P(aysse, "Staging error: ");
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
