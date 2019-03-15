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

UnifileString Unifile::m_cwd;

const char FLASH_PREFIX[] = "/flash";

bool SD_BEGIN(int mhz)
{
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

void SD_END(void)
{
  // Cannot use SPI.setFrequency() here because it's too slow.
  vs23.setSpiClockWrite();
  SpiUnlock();
}

// ワイルドカードでのマッチング(下記のリンク先のソースを利用)
// http://qiita.com/gyu-don/items/5a640c6d2252a860c8cd
//
// [引数]
//  wildcard : 判定条件(ワイルドカード *,?の利用可能)
//  target   : 判定対象文字列
// [戻り値]
// 一致     : 0
// 不一致   : 0以外
// 
uint8_t wildcard_match(const char *wildcard, const char *target) {
    const char *pw = wildcard, *pt = target;
    while(1){
        if(*pt == 0) return *pw == 0;
        else if(*pw == 0) return 0;
        else if(*pw == '*'){
            return *(pw+1) == 0 || wildcard_match(pw, pt + 1)
                                || wildcard_match(pw + 1, pt);
        }
        else if(*pw == '?' || (*pw == *pt)){
            pw++;
            pt++;
            continue;
        }
        else return 0;
    }
}

#include <Time.h>

#ifdef UNIFILE_USE_SDFAT
using namespace sdfat;
#endif

// ファイルタイムスタンプコールバック関数
#ifdef UNIFILE_USE_SDFAT
void dateTime(uint16_t* date, uint16_t* time) {
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
    setTime(FAT_HOUR(time), FAT_MINUTE(time), FAT_SECOND(time),
            FAT_DAY(date), FAT_MONTH(date), FAT_YEAR(date));
  }
  SD_END();
#endif
}

//
// 初期設定
// [引数]
//  _cs : SDカードのcsピン番号
// [戻り値]
//  0
uint8_t  sdfiles::init(uint8_t _cs) {
  cs = _cs;
  flgtmpOlen = false;
#ifdef UNIFILE_USE_SDFAT
  sdfat::SdFile::dateTimeCallback( &dateTime );
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
int8_t sdfiles::textOut(char* fname, int16_t sline, int16_t ln) {
  char str[SD_TEXT_LEN];
  uint16_t cnt = 0;
  uint8_t rc = 0;

  rc = tmpOpen(fname,0);
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
    if (cnt >=sline+ln) {
      rc = 1;
      break;
    }
  }
  tmpClose();

DONE:
  return rc; 
}

//
// ファイルリスト出力
// [引数]
//  _dir             : ファイルパス
//  wildcard         : ワイルドカード(NULL:指定なし)
// [戻り値]
//  正常終了         : 0
//  SDカード利用失敗 : SD_ERR_INIT
//  SD_ERR_OPEN_FILE : ファイルオープンエラー 
//
uint8_t sdfiles::flist(char* _dir, char* wildcard, uint8_t clmnum) {
  uint16_t cnt = 0;
  uint16_t len;
  uint8_t rc = 0;
  uint64_t total_size = 0;
  UnifileString fname;

  DIR *dir = opendir(_dir);

  if (dir) {
    while (true) {
      dirent *next = readdir(dir);
      if (!next)
        break;
      fname = next->d_name;
      len = fname.length();
      if (!wildcard || (wildcard && wildcard_match(wildcard,fname.c_str()))) {
        // Reduce SPI clock while doing screen writes.
        vs23.setSpiClockWrite();
        struct stat st;
        stat((BString(_dir) + BString(F("/")) + fname).c_str(), &st);
        putnum(st.st_size, 10); c_putch(' ');
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
    closedir(dir);
  } else {
    rc = SD_ERR_OPEN_FILE;
  }

  newline();
  putnum(cnt, 0); PRINT_P(" files, ");
  putnum(total_size, 0); PRINT_P(" bytes.\n");

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
uint8_t sdfiles::tmpOpen(char* tfname, uint8_t mode) { 
  if(mode) {
    tfile = fopen(tfname, "w");
  } else {
    tfile = fopen(tfname, "r");   
  }

  if (tfile)
    return 0;

  return SD_ERR_OPEN_FILE;
}

// 一時ファイルクローズ
uint8_t sdfiles::tmpClose() {
  if (tfile) {
    fclose(tfile);
    tfile = NULL;
  }

  return 0;
}

// 文字列出力
uint8_t sdfiles::puts(char*s) {
  int16_t n = 0;

  if( tfile && s ) {
    n = fputs(s, tfile);
  }

  return !n;    
}

// 1バイト出力 
uint8_t sdfiles::putch(char c) {
  int16_t n = 0;

  if(tfile) {
    n = putc(c, tfile);
  }

  return !n;   
}

// 1バイト読込
int16_t sdfiles::read() {
  if(!tfile) 
    return -1;
  return getc(tfile);
}

//
// 1行分読込み
// [引数]
// str :読み取りデータ格納アドレス
// [戻り値]
//  0    :  データなし
//  0以外:  読み込んだバイト数
//
int16_t sdfiles::readLine(char* str) {
  int len = 0;
  int16_t rc = 0;
  
  while(!feof(tfile)) {
    rc = getc(tfile);
    if (rc <= 0) break;
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
// ファイルがテキストファイルかチェック  
// [引数]
//  fname : ターゲットファイル名
// [戻り値]
//  正常終了             : 0 バイナリ形式 1:テキスト形式
//  SDカード利用失敗     : - SD_ERR_INIT
//  ファイルオープン失敗 : - SD_ERR_OPEN_FILE
//  ファイル読み込み失敗 : - SD_ERR_READ_FILE
//  ファイルでない       : - SD_ERR_NOT_FILE
//
int8_t sdfiles::IsText(char* fname) {
  FILE *myFile;
  char head[2];   // ヘッダー
  int8_t rc = -1;
 
  // ファイルのオープン
  myFile = fopen(fname, "r");
  if (myFile) {
    if (fread(head, 1, 2, myFile) != 2)  {
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
// ビットマップファイルのロード
// [引数]
//  fname : ターゲットファイル名
//  ptr   : ロードデータの格納アドレス
//  x     : ビットマップ画像の切り出し座標 x
//  y     : ビットマップ画像の切り出し座標 y
//  w     : ビットマップ画像の切り出し幅
//  h     : ビットマップ画像の切り出し高さ
//  mode  : 色モード 0:通常 1：反転
//[戻り値]
//  正常終了             : 0 
//  SDカード利用失敗     : SD_ERR_INIT
//  ファイルオープン失敗 : SD_ERR_OPEN_FILE
//  ファイル読み込み失敗 : SD_ERR_READ_FILE
// 

FILE *pcx_file = NULL;

#define DR_PCX_NO_STDIO
#define DR_PCX_IMPLEMENTATION
#include "../../ttbasic/dr_pcx.h"

static size_t read_image_bytes(void *user_data, void *buf, size_t bytesToRead)
{
  if (!pcx_file)
    return -1;
  if (buf == (void *)-1)
    return ftell(pcx_file);
  else if (buf == (void *)-2)
    return fseek(pcx_file, bytesToRead, SEEK_SET) == 0;
  return fread((char *)buf, 1, bytesToRead, pcx_file);
}

uint8_t sdfiles::loadBitmap(char* fname, int32_t &dst_x, int32_t &dst_y, int32_t x, int32_t y, int32_t &w,int32_t &h, int mask) {
  uint8_t rc =1;

  pcx_file = fopen(fname, "r");
  if (!pcx_file) {
    return SD_ERR_OPEN_FILE;
  }

  int width, height, components;

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
    if (!vs23.allocBacking(w, h, (int&)dst_x, (int&)dst_y)) {
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
    rc = SD_ERR_READ_FILE;	// XXX: or OOM...

out:
  fclose(pcx_file);
  pcx_file = NULL;
  return rc;
}

uint8_t sdfiles::saveBitmap(char* fname, int32_t src_x, int32_t src_y, int32_t w,int32_t h)
{
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
  
  putc(0x0c, pcx_file);	// palette marker

  uint8_t *pal = csp.paletteData(csp.getColorSpace());
  // PROGMEM, not safe to pass it directly to FS functions
  for (int i = 0; i < 256*3; ++i)
    putc((char)pgm_read_byte(&pal[i]), pcx_file);
  
  fclose(pcx_file);
  pcx_file = NULL;

  return rc;
}

//
// ディレクトリの作成
// [引数]
//  fname                  : ファイル名
// [戻り値]
//  正常終了               : 0
//  SDカード利用失敗       : SD_ERR_INIT
//  ファイルオープンエラー : SD_ERR_OPEN_FILE 
//
uint8_t sdfiles::mkdir(const char* fname) {
  uint8_t rc = 1;
 
  // This is a NOP for SPIFFS.
  if (Unifile::isSPIFFS(fname))
    return 0;

  UnifileString abs_name = Unifile::path(fname);

  fname = abs_name.c_str() + SD_PREFIX_LEN;

  if (SD_BEGIN() == false) {
    return SD_ERR_INIT;
  }

#ifdef UNIFILE_USE_SDFAT
  if (sdf.exists(fname)) {
#else
  if (SD.exists(fname)) {
#endif
    rc = SD_ERR_OPEN_FILE;    
  } else {
#ifdef UNIFILE_USE_SDFAT
    if(sdf.mkdir(fname) == true) {
#else
    if(SD.mkdir(fname) == true) {
#endif
      rc = 0;
    } else {
      rc = SD_ERR_OPEN_FILE;
    }
  }
  SD_END();
  return rc;
}

// ディレクトリの削除
// [引数]
//  fname                  : ファイル名
// [戻り値]
//  正常終了               : 0
//  SDカード利用失敗       : SD_ERR_INIT
//  ファイルオープンエラー : SD_ERR_OPEN_FILE 
//
uint8_t sdfiles::rmdir(char* fname) {
  uint8_t rc = 1;
 
  // This is a NOP for SPIFFS.
  if (Unifile::isSPIFFS(fname))
    return 0;

  if (SD_BEGIN() == false) 
    return SD_ERR_INIT;

  if (strncmp(fname, SD_PREFIX, SD_PREFIX_LEN))
    return SD_ERR_OPEN_FILE;

  fname += SD_PREFIX_LEN;

#ifdef UNIFILE_USE_SDFAT
  if(sdf.rmdir(fname) == true) {
#else
  if(SD.rmdir(fname) == true) {
#endif
    rc = 0;
  } else {
    rc =  SD_ERR_OPEN_FILE;
  }
  SD_END();
  return rc;
}

// ファイル名の変更
uint8_t sdfiles::rename(char* old_fname,char* new_fname) {
  uint8_t rc = 1;
  
  if(Unifile::rename(old_fname,new_fname) == true)
    rc = 0;

  return rc;
}

#define COPY_BUFFER_SIZE 512

uint8_t sdfiles::fcopy(const char *srcp, const char *dstp)
{
  Unifile src = Unifile::open(srcp, UFILE_READ);
  if (!src)
    return SD_ERR_OPEN_FILE;
  Unifile dst = Unifile::open(dstp, UFILE_OVERWRITE);
  if (!dst) {
    src.close();
    return SD_ERR_OPEN_FILE;
  }
  uint8_t rc = 0;
  char buf[COPY_BUFFER_SIZE];
  for (;;) {
    ssize_t redd = src.read(buf, COPY_BUFFER_SIZE);
    if (redd < 0) {
      rc = SD_ERR_READ_FILE;
      goto out;
    } else if (redd == 0)
      break;

    if (dst.write(buf, redd) != redd) {
      rc = SD_ERR_WRITE_FILE;
      goto out;
    }
  }
out:
  src.close();
  dst.close();
  return rc;
}

int8_t sdfiles::compare(const char *one, const char *two)
{
  int8_t ret = 0;
  Unifile fone = Unifile::open(one, UFILE_READ);
  Unifile ftwo = Unifile::open(two, UFILE_READ);
  if (!fone || !ftwo) {
    err = ERR_FILE_OPEN;
    goto out;
  }
  char buf1[128];
  char buf2[128];
  while (fone.available() && ftwo.available()) {
    size_t one_bytes = fone.read(buf1, 128);
    size_t two_bytes = ftwo.read(buf2, 128);
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
  if (fone.available() != ftwo.available()) {
    if (fone.available())
      ret = 1;
    else
      ret = -1;
  }
out:
  fone.close();
  ftwo.close();
  return ret;
}
