#ifndef _ERROR_H
#define _ERROR_H

// エラーコード
enum {
  ERR_OK,
  ERR_DIVBY0,
  ERR_VOF,
  ERR_SOR,
  ERR_IBUFOF, ERR_LBUFOF,
  ERR_GSTKOF, ERR_GSTKUF,
  ERR_LSTKOF, ERR_LSTKUF,
  ERR_NEXTWOV, ERR_NEXTUM, ERR_FORWOV, ERR_FORWOTO,
  ERR_LETWOV, ERR_IFWOC,
  ERR_ULN,
  ERR_PAREN, ERR_VWOEQ,
  ERR_COM,
  ERR_VALUE, // 追加
  ERR_RANGE, // 追加
  ERR_NOPRG, // 追加
  ERR_SYNTAX,
  ERR_SYS,
  ERR_CTR_C,
  ERR_LONG,                 // 追加
  ERR_EEPROM_OUT_SIZE,      // 追加
  ERR_EEPROM_BAD_ADDRESS,   // 追加
  ERR_EEPROM_BAD_FLASH,     // 追加
  ERR_EEPROM_NOT_INIT,      // 追加
  ERR_EEPROM_NO_VALID_PAGE, // 追加
  ERR_FILE_WRITE,            // 追加 V0.83
  ERR_FILE_READ,             // 追加 V0.83
  ERR_GPIO,                  // 追加 V0.83
  ERR_LONGPATH,              // 追加 V0.83
  ERR_FILE_OPEN,             // 追加 V0.83
  ERR_SD_NOT_READY,          // 追加 V0.83
  ERR_BAD_FNAME,             // 追加 V0.83
  ERR_NOT_SUPPORTED,         // 追加 V0.84
  ERR_SCREEN_FULL,
  ERR_OOM,
};

#define SD_ERR_INIT		ERR_SD_NOT_READY
#define SD_ERR_OPEN_FILE	ERR_FILE_OPEN
#define SD_ERR_NOT_FILE		ERR_BAD_FNAME
#define SD_ERR_READ_FILE	ERR_FILE_READ
#define SD_ERR_WRITE_FILE	ERR_FILE_WRITE

#endif
