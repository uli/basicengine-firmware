#include "ttconfig.h"
#include "basic.h"
#include "bstring.h"
#include "error.h"
#include "vs23s010.h"
#include <sdfiles.h>
#include <Updater.h>

extern "C" void startup(void);

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

  Unifile f = Unifile::open(filename, FILE_READ);
  if (!f) {
    err = ERR_FILE_OPEN;
    return;
  }

  newline();
  static const char ays0[] PROGMEM =
    "FIRMWARE UPDATE";
  sc0.setColor(0, col_warn);
  c_puts_P(ays0); newline();
  sc0.setColor(col_normal, 0);

  static const char ays0a[] PROGMEM =
    "Are you absolutely sure you want to overwrite";
  newline(); c_puts_P(ays0a); newline();

  static const char ays1[] PROGMEM =
    "the currently installed firmware";
  c_puts_P(ays1); newline();

  static const char ays2[] PROGMEM = "with ";
  c_puts_P(ays2);
  sc0.setColor(col_warn, 0);
  c_puts(filename.c_str());
  sc0.setColor(col_normal, 0);
  c_putch('?'); newline(); newline();

  static const char ays3[] PROGMEM =
    "If %s is not a working";
  sprintf_P(msg, ays3, filename.c_str());
  c_puts(msg); newline();

  static const char ays3a[] PROGMEM =
    "BASIC Engine firmware image, or if you";
  c_puts_P(ays3a); newline();

  static const char ays4[] PROGMEM =
    "lose power or reset the system during";
  c_puts_P(ays4); newline();

  static const char ays5[] PROGMEM =
    "the upgrade procedure, it will no longer work";
  c_puts_P(ays5); newline();

  static const char ays6[] PROGMEM =
    "until you use a serial programmer to restore";
  c_puts_P(ays6); newline();

  static const char ays7[] PROGMEM =
    "a working firmware image.";
  c_puts_P(ays7); newline(); newline();

  sc0.setColor(col_warn, 0);
  static const char ays8[] PROGMEM =
    "Enter YES to continue, anything else to abort: ";
  c_puts_P(ays8);
  get_input();
  sc0.setColor(col_normal, 0);
  if (strcmp(lbuf, "YES")) {
    static const char aborting[] PROGMEM = "Aborting.";
    c_puts_P(aborting); newline();
    goto out;
  }

  sc0.show_curs(0);

  if (!Update.begin(f.fileSize())) {
    static const char update_start_fail_msg[] PROGMEM =
      "Flash updater init failed, error: ";
    c_puts_P(update_start_fail_msg); putnum(Update.getError(), 0);
    newline();
    goto out;
  }

  static const char staging_update[] PROGMEM = "Staging update: ";
  c_puts_P(staging_update);

  x = sc0.c_x(); y = sc0.c_y();
  sc0.locate(x + 7, y);
  c_putch('/'); putnum(f.fileSize(), 6); c_puts(" bytes");

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
    static const char success_msg[] PROGMEM = "Staging update successful.";
    static const char now_msg[] PROGMEM = "Writing update to flash.";
    static const char warn_msg[] PROGMEM = "DO NOT RESET OR POWER OFF!";
    c_puts_P(success_msg); newline(); newline();
    sc0.setColor(col_warn, 0);
    c_puts_P(now_msg); newline();
    sc0.setColor(0, col_warn);
    c_puts_P(warn_msg); newline();
#ifdef ESP8266_NOWIFI
    // SDKnoWiFi does not have system_restart*(). The only working
    // alternative I could find is triggering the WDT.
    ets_wdt_enable(2,3,3);
    for(;;);
#else
    ESP.reset();	// UNTESTED!
#endif
  } else {
    static const char error_msg[] PROGMEM = "Staging error: ";
    c_puts_P(error_msg); putnum(Update.getError(), 0); newline();
    sc0.show_curs(1);
  }
  return;

out:
  free(buf);
  f.close();
  // XXX: clean up Updater?
  sc0.show_curs(1);
}
