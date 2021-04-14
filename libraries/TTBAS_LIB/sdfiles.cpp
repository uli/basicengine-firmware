//
// sdfiles.cpp
// SDカードファイル操作クラス
// 作成日 2017/06/04 by たま吉さん
// 参考情報
// - Topic: SD Timestamp read? (Read 1 time) http://forum.arduino.cc/index.php?topic=114181.0
//
// 修正履歴
//  2017/07/27, flist()で列数を指定可能
//

#include "../../ttbasic/ttconfig.h"
#include "sdfiles.h"
#include "../../ttbasic/basic.h"
#include "../../ttbasic/lock.h"
#include "../../ttbasic/video.h"
#include <string.h>

#include <time.h>

#ifdef UNIFILE_USE_SDFAT
sdfat::SdFat sdf;
#endif

#ifdef UNIFILE_USE_FASTROMFS
FastROMFilesystem fs;
#endif
static bool sdfat_initialized = false;
static int m_mhz = 0;
static uint8_t cs = 0;

#ifdef USE_UNIFILE
UnifileString Unifile::m_cwd;

const char FLASH_PREFIX[] = "/flash";
#endif

bool SD_BEGIN(int mhz) {
  if (mhz != m_mhz) {
    m_mhz = mhz;
    sdfat_initialized = false;
  }
  SpiLock();
  if (!sdfat_initialized) {
#ifdef UNIFILE_USE_SDFAT
    sdfat_initialized = sdf.begin(cs, SD_SCK_MHZ(mhz));
#elif defined(UNIFILE_USE_NEW_SD)
    sdfat_initialized = SD.begin(SS, SPI, mhz * 1000000);
#endif
  } else {
    // Cannot use SPI.setFrequency() here because it's too slow; this method
    // is called before every access. We instead use the mechanism
    // for fast SPI frequency switching from the VS23S010 driver.
    vs23.setSpiClockMax();
  }
  return sdfat_initialized;
}

void SD_END(void) {
  // Cannot use SPI.setFrequency() here because it's too slow.
  vs23.setSpiClockWrite();
  SpiUnlock();
}

// Wildcard matching (use the linked source below)
// http://qiita.com/gyu-don/items/5a640c6d2252a860c8cd
//
// [argument]
//	wildcard: Judgment condition (wildcard *,? available)
//	target: Character string to be judged
// [Return value]
//	Match: 0
//	not match: other than 0
//
uint8_t wildcard_match(const char *wildcard, const char *target) {
  const char *pw = wildcard, *pt = target;
  while (1) {
    if (*pt == 0)
      return *pw == 0;
    else if (*pw == 0)
      return 0;
    else if (*pw == '*') {
      return *(pw + 1) == 0 || wildcard_match(pw, pt + 1) ||
             wildcard_match(pw + 1, pt);
    } else if (*pw == '?' || (*pw == *pt)) {
      pw++;
      pt++;
      continue;
    } else
      return 0;
  }
}

#include <Time.h>

#ifdef UNIFILE_USE_SDFAT
using namespace sdfat;
#endif

// File timestamp callback function
#ifdef UNIFILE_USE_SDFAT
void dateTime(uint16_t *date, uint16_t *time) {
  time_t tt = now();
  *date = FAT_DATE(year(tt), month(tt), day(tt));
  *time = FAT_TIME(hour(tt), minute(tt), second(tt));
}
#endif

void sdfiles::fakeTime() {
#ifdef UNIFILE_USE_SDFAT
  SD_BEGIN();
  auto dir = sdf.open("/");
  sdfat::File file;
  if (!dir || !dir.isDir()) {
    SD_END();
    return;
  }
  while ((file = dir.openNextFile())) {
    dir_t dirent;
    if (!file.dirEntry(&dirent))
      continue;
    uint16_t date = dirent.creationDate;
    uint16_t time = dirent.creationTime;
    time_t tt = now();
    if (FAT_YEAR(date) < year(tt))
      continue;
    if (FAT_YEAR(date) == year(tt)) {
      if (FAT_MONTH(date) < month(tt))
        continue;
      if (FAT_MONTH(date) == month(tt)) {
        if (FAT_DAY(date) < day(tt))
          continue;
        if (FAT_DAY(date) == day(tt)) {
          if (FAT_HOUR(time) < hour(tt))
            continue;
          if (FAT_HOUR(time) == hour(tt)) {
            if (FAT_MINUTE(time) < minute(tt))
              continue;
            if (FAT_MINUTE(time) == minute(tt)) {
              if (FAT_SECOND(time) <= second(tt))
                continue;
            }
          }
        }
      }
    }
    setTime(FAT_HOUR(time), FAT_MINUTE(time), FAT_SECOND(time), FAT_DAY(date),
            FAT_MONTH(date), FAT_YEAR(date));
  }
  SD_END();
#endif
}

//
// Initial setting
// [argument]
//	_cs: SD card cs pin number
// [Return value]
//	0
//
uint8_t sdfiles::init(uint8_t _cs) {
  cs = _cs;
  flgtmpOlen = false;
#ifdef UNIFILE_USE_SDFAT
  sdfat::SdFile::dateTimeCallback(&dateTime);
#endif
  return 0;
}

//
// Output text file
// [Arguments]
//  fname: target file name
//  sline: Output start line (0-)
//  ln   : Number of output lines (1-)
// [Return value]
//  SD card use failure : -SD_ERR_INIT
//  Failed to open file : -SD_ERR_OPEN_FILE
//  not a file          : -SD_ERR_NOT_FILE
//  End of file reached : 0
//  possible to continue: 1
//
int8_t sdfiles::textOut(char *fname, int16_t sline, int16_t ln) {
  char str[SD_TEXT_LEN];
  uint16_t cnt = 0;
  uint8_t rc = 0;

  rc = tmpOpen(fname, 0);
  if (rc) {
    return -rc;
  }

  while (1) {
    rc = readLine(str);
    if (!rc)
      break;
    if (cnt >= sline) {
      c_puts(str);
      newline();
    }
    cnt++;
    if (cnt >= sline + ln) {
      rc = 1;
      break;
    }
  }
  tmpClose();

  return rc;
}

//
// File list output
// [argument]
//	_dir: File path
//	wildcard: wildcard (NULL: not specified)
// [Return value]
//	Normal end: 0
//	SD card use failure: SD_ERR_INIT
//	SD_ERR_OPEN_FILE: File open error
//
uint8_t sdfiles::flist(char *_dir, char *wildcard, uint8_t clmnum) {
  uint16_t cnt = 0;
  uint16_t len;
  uint8_t rc = 0;
  uint64_t total_size = 0;
  UnifileString fname;

  if (strlen(_dir) == 0)
    _dir = (char *)".";

  DIR *dir = _opendir(_dir);

  if (dir) {
    while (true) {
      dirent *next = _readdir(dir);
      if (!next)
        break;

      // skip "." and ".."
      if (!strcmp_P(next->d_name, PSTR(".")) ||
          !strcmp_P(next->d_name, PSTR("..")))
        continue;

      fname = next->d_name;
      len = fname.length();
      if (!wildcard || (wildcard && wildcard_match(wildcard, fname.c_str()))) {
        // Reduce SPI clock while doing screen writes.
        vs23.setSpiClockWrite();
        struct stat st;
        _stat((BString(_dir) + BString(F("/")) + fname).c_str(), &st);
        putnum(st.st_size, 10);
        c_putch(' ');
        total_size += st.st_size;
        if (next->d_type == DT_DIR) {
          c_puts(fname.c_str());
          c_puts("*");
          len++;
        } else {
          c_puts(fname.c_str());
        }
        newline();
        if (err)
          break;
        cnt++;
      }
    }
    _closedir(dir);
  } else {
    rc = SD_ERR_OPEN_FILE;
  }

  newline();
  putnum(cnt, 0);
  PRINT_P(" files, ");
  putnum(total_size, 0);
  PRINT_P(" bytes.\n");

  return rc;
}

//
// Open temporary file
// [Arguments]
//  fname: target file name
//  mode : 0 read mode, 1: write mode (new)
// [Return value]
//  Normal completion  : 0
//  SD card use failure: SD_ERR_INIT
//  Failed to open file: SD_ERR_OPEN_FILE
//
uint8_t sdfiles::tmpOpen(char *tfname, uint8_t mode) {
  if (mode) {
    tfile = fopen(tfname, "w");
  } else {
    tfile = fopen(tfname, "r");
  }

  if (tfile)
    return 0;

  return SD_ERR_OPEN_FILE;
}

uint8_t sdfiles::tmpClose() {
  if (tfile) {
    fclose(tfile);
    tfile = NULL;
  }

  return 0;
}

uint8_t sdfiles::puts(char *s) {
  int16_t n = 0;

  if (tfile && s) {
    n = fputs(s, tfile);
  }

  return !n;
}

// 1 byte output
uint8_t sdfiles::putch(char c) {
  int16_t n = 0;

  if (tfile) {
    n = putc(c, tfile);
  }

  return !n;
}

// 1 byte read
int16_t sdfiles::read() {
  if (!tfile)
    return -1;
  return getc(tfile);
}

//
// read one line
// [argument]
//	str: Read data storage address
// [Return value]
//	0: no data
//	Other than 0: Number of bytes read
//
int16_t sdfiles::readLine(char *str) {
  int len = 0;
  int16_t rc = 0;

  while (!feof(tfile)) {
    rc = getc(tfile);
    if (rc <= 0)
      break;
    *str = rc;
    if (*str == 0x0d)
      continue;
    if (*str == 0x0a)
      break;
    str++;
    len++;
    if (len == SD_TEXT_LEN)
      break;
  }

  *str = 0;
  return rc;
}

//
// check if the file is a text file
// [argument]
//	fname: target file name
// [Return value]
//	Normal end: 0 Binary format 1: Text format
//	SD card use failure:-SD_ERR_INIT
//	File open failure:-SD_ERR_OPEN_FILE
//	file read failure:-SD_ERR_READ_FILE
//	not a file:-SD_ERR_NOT_FILE
//
int8_t sdfiles::IsText(char *fname) {
  FILE *myFile;
  char head[2];  // ヘッダー
  int8_t rc = -1;

  // ファイルのオープン
  myFile = fopen(fname, "r");
  if (myFile) {
    if (fread(head, 1, 2, myFile) != 2) {
      rc = -SD_ERR_READ_FILE;
    } else {
      if (head[0] == 0 && head[1] == 0) {
        rc = 0;
      } else {
        rc = 1;
      }
    }
    fclose(myFile);
  } else {
    rc = -SD_ERR_OPEN_FILE;
  }

  return rc;
}

//
// Load bitmap file
// [argument]
// 	fname: target file name
//	ptr: Load data storage address
//	x: cutout coordinate of bitmap image x
//	y: Bit map image cutout coordinates y
//	w: Bitmap image cutout width
//	h: Bitmap image cutout height
//	mode: Color mode 0: Normal 1: Inverted
//[Return value]
//	Normal end: 0
//	SD card use failure: SD_ERR_INIT
//	file open failure: SD_ERR_OPEN_FILE
//	file read failure: SD_ERR_READ_FILE
//
FILE *pcx_file = NULL;

#define DR_PCX_NO_STDIO
#define DR_PCX_IMPLEMENTATION
#include "../../ttbasic/dr_pcx.h"

static size_t read_image_bytes(void *user_data, void *buf, size_t bytesToRead) {
  if (!pcx_file)
    return -1;
  if (buf == (void *)-1)
    return ftell(pcx_file);
  else if (buf == (void *)-2)
    return fseek(pcx_file, bytesToRead, SEEK_SET) == 0;
  return fread((char *)buf, 1, bytesToRead, pcx_file);
}

#ifdef TRUE_COLOR
extern "C" {
#include <stb_image.h>
}

uint8_t sdfiles::loadImage(FILE *img_file,
                           int32_t img_w, int32_t img_h,
                           int32_t &dst_x, int32_t &dst_y,
                           int32_t x, int32_t y,
                           int32_t &w, int32_t &h,
                           pixel_t mask) {
  uint8_t rc = 1;
  int components;
  uint32_t *data = 0;
  fseek(img_file, 0, SEEK_SET);

  if (w == -1) {
    if (h != -1) {
      rc = ERR_RANGE;
      goto out;
    }
    w = img_w - x;
    h = img_h - y;
  }
  if (dst_x == -1) {
    if (dst_y != -1) {
      rc = ERR_RANGE;
      goto out;
    }
    if (!vs23.allocBacking(w, h, (int &)dst_x, (int &)dst_y)) {
      rc = ERR_SCREEN_FULL;
      goto out;
    }
  }

  if (dst_x + w > vs23.width() || dst_y + h > vs23.lastLine()) {
    rc = ERR_RANGE;
    goto out;
  }

  data = (uint32_t *)stbi_load_from_file(img_file, (int *)&img_w, (int *)&img_h, &components, 4);

  if (data) {
    rc = 0;

    // Blit image to screen.
    for (int dy = dst_y; dy < dst_y + h; ++y, ++dy) {
      int sx = x;

      for (int dx = dst_x; dx < dst_x + w; ++sx, ++dx) {
        uint32_t d = data[y*img_w + sx];
        pixel_t p = csp.colorFromRgb(d, d >> 8, d >> 16);
        if ((d >> 24) > 0x80 && (mask == 0 || p != mask))
          vs23.setPixel(dx, dy, p);
      }
    }

    free(data);
  } else
    rc = SD_ERR_READ_FILE;  // XXX: or OOM...

out:
  fclose(img_file);
  return rc;
}
#endif

uint8_t sdfiles::loadBitmap(char *fname, int32_t &dst_x, int32_t &dst_y,
                            int32_t x, int32_t y, int32_t &w, int32_t &h,
                            uint32_t mask) {
  uint8_t rc = 1;
  int width, height, components;

  pcx_file = fopen(fname, "r");

  if (!pcx_file) {
    return SD_ERR_OPEN_FILE;
  }

#ifdef TRUE_COLOR
  if (stbi_info_from_file(pcx_file, &width, &height, &components)) {
    rc = loadImage(pcx_file, width, height, dst_x, dst_y, x, y, w, h, mask);
    pcx_file = NULL;
    return rc;
  }
#endif

  if (w == -1) {
    if (h != -1) {
      rc = ERR_RANGE;
      goto out;
    }
    if (!drpcx_info(read_image_bytes, NULL, &width, &height, &components)) {
      rc = SD_ERR_READ_FILE;
      goto out;
    }
    fseek(pcx_file, 0, SEEK_SET);
    w = width - x;
    h = height - y;
  }
  if (dst_x == -1) {
    if (dst_y != -1) {
      rc = ERR_RANGE;
      goto out;
    }
    if (!vs23.allocBacking(w, h, (int &)dst_x, (int &)dst_y)) {
      rc = ERR_SCREEN_FULL;
      goto out;
    }
  }

  if (dst_x + w > vs23.width() || dst_y + h > vs23.lastLine()) {
    rc = ERR_RANGE;
    goto out;
  }

  if (drpcx_load(read_image_bytes, NULL, false, &width, &height, &components, 0,
                 dst_x, dst_y, x, y, w, h, mask))
    rc = 0;
  else
    rc = SD_ERR_READ_FILE;  // XXX: or OOM...

out:
  fclose(pcx_file);
  pcx_file = NULL;
  return rc;
}

#ifdef TRUE_COLOR
extern "C" {
#include <stb_image_write.h>
}
#endif

uint8_t sdfiles::saveBitmap(char *fname, int32_t src_x, int32_t src_y,
                            int32_t w, int32_t h) {
  if (strlen(fname) < 4) {
    return ERR_BAD_FNAME;
  }

  char *ext = fname + strlen(fname) - 3;
  int ret;

  if (!strcasecmp(ext, "pcx"))
    return saveBitmapPcx(fname, src_x, src_y, w, h);
#ifndef TRUE_COLOR
  else
    return ERR_BAD_FNAME;	// only PCX supported
#endif

#ifdef TRUE_COLOR
  pixel_t *data = (pixel_t *)malloc(w * h * 4);
  if (!data)
    return ERR_OOM;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      uint8_t r, g, b, a;
      vs23.rgbaFromColor(vs23.getPixel(x, y), r, g, b, a);
      data[y * w + x] = r | (g << 8) | (b << 16) | (a << 24);
    }
  }

  if (!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg"))
    ret = stbi_write_jpg(fname, w, h, 4, (const void *)data, 95);
  else if (!strcasecmp(ext, "png"))
    ret = stbi_write_png(fname, w, h, 4, (const void *)data, w * sizeof(pixel_t));
  else if (!strcasecmp(ext, "bmp"))
    ret = stbi_write_bmp(fname, w, h, 4, (const void *)data);
  else if (!strcasecmp(ext, "tga"))
    ret = stbi_write_tga(fname, w, h, 4, (const void *)data);
  else {
    free(data);
    return ERR_BAD_FNAME;
  }

  free(data);

  if (ret)
    return 0;
  else
    return ERR_IO;	// ...I guess
#endif
}

uint8_t sdfiles::saveBitmapPcx(char *fname, int32_t src_x, int32_t src_y,
                               int32_t w, int32_t h) {
#ifdef TRUE_COLOR
  return ERR_NOT_SUPPORTED;
#else
  uint8_t rc = 0;
  drpcx_header hdr;

  memset(&hdr, 0, sizeof(hdr));
  hdr.header = 10;
  hdr.version = 5;
  hdr.encoding = 1;
  hdr.bpp = 8;
  hdr.right = w - 1;
  hdr.bottom = h - 1;
  hdr.hres = hdr.vres = 300;
  hdr.bitPlanes = 1;
  hdr.bytesPerLine = w;
  hdr.paletteType = 1;

  pcx_file = fopen(fname, "w");
  if (!pcx_file) {
    return ERR_FILE_OPEN;
  }

  fwrite((char *)&hdr, 1, sizeof(hdr), pcx_file);

  for (int yy = src_y; yy < src_y + h; ++yy) {
    uint8_t rle_count = 1;
    uint8_t rle_last = vs23.getPixel(src_x, yy);
    for (int xx = src_x + 1; xx < src_x + w; ++xx) {
      uint8_t px = vs23.getPixel(xx, yy);
      if (px == rle_last && rle_count < 63)
        rle_count++;
      else {
        // flush last rle group
        if (rle_count == 1 && rle_last < 0xc0)
          putc(rle_last, pcx_file);
        else {
          putc(rle_count | 0xc0, pcx_file);
          putc(rle_last, pcx_file);
        }
        rle_count = 1;
        rle_last = px;
      }
    }
    // flush last rle group
    if (rle_count == 1 && rle_last < 0xc0)
      putc(rle_last, pcx_file);
    else {
      putc(rle_count | 0xc0, pcx_file);
      putc(rle_last, pcx_file);
    }
  }

  putc(0x0c, pcx_file);  // palette marker

  uint8_t *pal = csp.paletteData(csp.getColorSpace());
  // PROGMEM, not safe to pass it directly to FS functions
  for (int i = 0; i < 256 * 3; ++i)
    putc((char)pgm_read_byte(&pal[i]), pcx_file);

  fclose(pcx_file);
  pcx_file = NULL;

  return rc;
#endif
}

//
// Create directory
// [argument]
// 	fname: file name
// [Return value]
// 	Normal end: 0
// 	SD card use failure: SD_ERR_INIT
// 	file open error: SD_ERR_OPEN_FILE
//
uint8_t sdfiles::mkdir(const char *fname) {
  uint8_t rc = 1;

#ifdef USE_UNIFILE
  UnifileString abs_name = Unifile::path(fname);

  // This is a NOP for SPIFFS.
  if (Unifile::isSPIFFS(abs_name))
    return 0;

  if (SD_BEGIN() == false) {
    return SD_ERR_INIT;
  }

  fname = abs_name.c_str() +
          (abs_name.startsWith(SD_PREFIX) ? SD_PREFIX_LEN : 0);

#ifdef UNIFILE_USE_SDFAT
  if (sdf.exists(fname)) {
#else
  if (SD.exists(fname)) {
#endif
    rc = SD_ERR_OPEN_FILE;
  } else {
#ifdef UNIFILE_USE_SDFAT
    if (sdf.mkdir(fname) == true) {
#else
    if (SD.mkdir(fname) == true) {
#endif
      rc = 0;
    } else {
      rc = SD_ERR_OPEN_FILE;
    }
  }
  SD_END();
#else  // USE_UNIFILE
  rc = !mkdir(fname);
#endif
  return rc;
}

// delete directory
// [argument]
// 	fname: file name
// [Return value]
// 	Normal end: 0
// 	SD card use failure: SD_ERR_INIT
// 	file open error: SD_ERR_OPEN_FILE
//
uint8_t sdfiles::rmdir(const char *fname) {
  uint8_t rc = 1;

#ifdef USE_UNIFILE
  UnifileString abs_name = Unifile::path(fname);

  // This is a NOP for SPIFFS.
  if (Unifile::isSPIFFS(abs_name))
    return 0;

  if (SD_BEGIN() == false)
    return SD_ERR_INIT;

  fname = abs_name.c_str() +
          (abs_name.startsWith(SD_PREFIX) ? SD_PREFIX_LEN : 0);

#ifdef UNIFILE_USE_SDFAT
  if (sdf.rmdir(fname) == true) {
#else
  if (SD.rmdir(fname) == true) {
#endif
    rc = 0;
  } else {
    rc = SD_ERR_OPEN_FILE;
  }
  SD_END();
#else
  rc = !rmdir(fname);
#endif
  return rc;
}

#define COPY_BUFFER_SIZE 512

uint8_t sdfiles::fcopy(const char *srcp, const char *dstp) {
  FILE *src = fopen(srcp, "r");
  if (!src)
    return SD_ERR_OPEN_FILE;
  FILE *dst = fopen(dstp, "w");
  if (!dst) {
    fclose(src);
    return SD_ERR_OPEN_FILE;
  }
  uint8_t rc = 0;
  char buf[COPY_BUFFER_SIZE];
  for (;;) {
    size_t redd = fread(buf, 1, COPY_BUFFER_SIZE, src);
    if (redd == 0 && ferror(src)) {
      rc = SD_ERR_READ_FILE;
      goto out;
    } else if (redd == 0)
      break;

    if (fwrite(buf, 1, redd, dst) != redd) {
      rc = SD_ERR_WRITE_FILE;
      goto out;
    }
  }
out:
  fclose(src);
  fclose(dst);
  return rc;
}

int8_t sdfiles::compare(const char *one, const char *two) {
  int8_t ret = 0;
  FILE *fone = fopen(one, "r");
  FILE *ftwo = fopen(two, "r");
  if (!fone || !ftwo) {
    err = ERR_FILE_OPEN;
    goto out;
  }
  char buf1[128];
  char buf2[128];
  while (!feof(fone) && !feof(ftwo)) {
    size_t one_bytes = fread(buf1, 1, 128, fone);
    size_t two_bytes = fread(buf2, 1, 128, ftwo);
    if (one_bytes != two_bytes) {
      ret = one_bytes < two_bytes ? -1 : 1;
      goto out;
    }
    int cmp = memcmp(buf1, buf2, one_bytes);
    if (cmp) {
      ret = cmp;
      goto out;
    }
  }
  if (feof(fone) != feof(ftwo)) {
    if (!feof(fone))
      ret = 1;
    else
      ret = -1;
  }
out:
  if (fone)
    fclose(fone);
  if (ftwo)
    fclose(ftwo);
  return ret;
}
