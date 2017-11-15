#ifndef _ERROR_H
#define _ERROR_H

#define ESTR(n,s) n,

// エラーコード
enum {
#include "errdef.h"
};

#define SD_ERR_INIT		ERR_SD_NOT_READY
#define SD_ERR_OPEN_FILE	ERR_FILE_OPEN
#define SD_ERR_NOT_FILE		ERR_BAD_FNAME
#define SD_ERR_READ_FILE	ERR_FILE_READ
#define SD_ERR_WRITE_FILE	ERR_FILE_WRITE

#endif
