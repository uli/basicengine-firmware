//
// sdfiles.cpp
// SDカードファイル操作クラス
// 作成日 2017/06/04 by たま吉さん
// 参考情報
// - Topic: SD Timestamp read? (Read 1 time) http://forum.arduino.cc/index.php?topic=114181.0
//

#include "sdfiles.h"
#include <RTClock.h>
#include <time.h>
#include <sdbitmap.h>
#include <string.h>
#include <libBitmap.h>

#define SD_BEGIN() SD.begin(F_CPU/4,cs)
extern RTClock rtc;
void c_puts(const char *s, uint8_t devno=0);
void newline(uint8_t devno=0);

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
uint8_t wildcard_match(char *wildcard, char *target) {
    char *pw = wildcard, *pt = target;
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

// ファイルタイムスタンプコールバック関数
void dateTime(uint16_t* date, uint16_t* time) {
   time_t tt; 
   struct tm* st;
   tt = rtc.getTime();   // 時刻取得
   st = localtime(&tt);  // 時刻型変換
  *date = FAT_DATE(st->tm_year+1900, st->tm_mon+1, st->tm_mday);
  *time = FAT_TIME(st->tm_hour, st->tm_min, st->tm_sec);
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
  SdFile::dateTimeCallback( &dateTime );
  return 0;
}

//
// バイナリファイルのロード
// [引数]
//  fname : ターゲットファイル名
//  ptr   : ロードデータ格納アドレス
//  sz    : ロードデータサイズ
// [戻り値]
//  正常終了             : 0
//  SDカード利用失敗     : SD_ERR_INIT
//  ファイルオープン失敗 : SD_ERR_OPEN_FILE
//  ファイル読み込み失敗 : SD_ERR_READ_FILE
//
uint8_t sdfiles::load(char* fname, uint8_t* ptr, uint16_t sz) {
  File myFile;
  uint8_t rc;
  char head[2];  // ヘッダ

  // ファイルのオープン
  if (SD_BEGIN() == false) 
    return SD_ERR_INIT;     // SDカードの利用失敗

  myFile = SD.open(fname, FILE_READ);
  if (myFile) {
    // データのヘッダーとデータロード
    if (myFile.read(head, 2) && myFile.read(ptr, sz)) {
      rc = 0;
    } else {
      rc = SD_ERR_READ_FILE;
    }
    myFile.close(); 
  } else {
    rc = SD_ERR_OPEN_FILE ; // ファイルオープン失敗
  }
  SD.end();
  return rc;
}

//
// バイナリファイルのセーブ
// [引数]
//  fname : ターゲットファイル名
//  ptr   : ロードデータ格納アドレス
//  sz    : ロードデータサイズ
// [戻り値]
//  正常終了             : 0
//  SDカード利用失敗     : SD_ERR_INIT
//  ファイルオープン失敗 : SD_ERR_OPEN_FILE
//  ファイル書き込み失敗 : SD_ERR_WRITE_FILE
//
uint8_t sdfiles::save(char* fname, uint8_t* ptr, uint16_t sz) {
  File myFile;
  char head[2] = {0,0};
  uint8_t rc = 1;

  // 既存ファイルの削除
  if (SD_BEGIN() == false) 
    return SD_ERR_INIT;

  if (SD.exists(fname))
     SD.remove(fname);

  // ファイルのオープン
  myFile = SD.open(fname, FILE_WRITE);
  if (myFile) {
    // データの保存
    if (myFile.write(head, 2) && myFile.write(ptr, sz)) {
      rc = 0;
    } else {
      rc = SD_ERR_WRITE_FILE;
    }
    myFile.close(); 
  } else {
    rc = SD_ERR_OPEN_FILE;
  }
  SD.end();
  return rc;
}

//
// テキストファイルの出力
// [引数]
//  fname : ターゲットファイル名
//  sline : 出力開始行(0～)
//  ln    : 出力行数(1～)
// [戻り値]
//  SDカード利用失敗      : -SD_ERR_INIT
//  ファイルオープン失敗  : -SD_ERR_OPEN_FILE
//  ファイルでない        : -SD_ERR_NOT_FILE
//  ファイルの終了に達した: 0
//  継続可能              : 1
// 
int8_t sdfiles::textOut(char* fname, int16_t sline, int16_t ln) {
  char str[SD_TEXT_LEN];
  uint16_t cnt = 0;
  uint16_t len;
  uint8_t rc = 0;

  rc = tmpOpen(fname,0);
  if (rc) {
    return -rc;
  }
  
  if (tfile.isDirectory()) {
    rc = -SD_ERR_NOT_FILE;
    tmpClose();
    goto DONE;
  }

  while (1) {
    rc = readLine(str);
    if (!rc) 
     break;
    if (cnt >= sline) {
      c_puts("'");
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
  SD.end();
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
uint8_t sdfiles::flist(char* _dir, char* wildcard) {
  uint16_t cnt = 0;
  uint16_t len;
  uint8_t rc = 0;

 if (SD_BEGIN() == false) 
    return SD_ERR_INIT;

  File dir = SD.open(_dir);
  if (dir) {
    dir.rewindDirectory();
    while (true) {
      File entry =  dir.openNextFile();
      if (!entry) {
        break;
      }
      len = strlen(entry.name());
      if (!wildcard || (wildcard && wildcard_match(wildcard,entry.name()))) {
        if (entry.isDirectory()) {
          c_puts(entry.name());
          c_puts("*");
          len++;
        } else {
          c_puts(entry.name());
        }
        if (cnt % 2) {
          newline();
        } else {
          for (uint8_t i = len; i < 14; i++)
            c_puts(" ");
        }
        cnt++;
      }
      entry.close();
    }
    dir.close();
  } else {
    rc = SD_ERR_OPEN_FILE;
  }
  SD.end();
  newline();
  return rc;
}

//
// 一時ファイルオープン
// [引数]
//  fname : ターゲットファイル名
//  mode  : 0 読込モード、1:書込みモード(新規)
// [戻り値]
//  正常終了              : 0
//  SDカード利用失敗     : SD_ERR_INIT
//  ファイルオープン失敗 : SD_ERR_OPEN_FILE
//  
uint8_t sdfiles::tmpOpen(char* tfname, uint8_t mode) { 
if (SD_BEGIN() == false) 
    return SD_ERR_INIT;
  if(mode) {
    if (SD.exists(tfname))
      SD.remove(tfname);
    tfile = SD.open(tfname, FILE_WRITE);
  } else {
    tfile = SD.open(tfname, FILE_READ);   
  }
  return tfile ? 0:SD_ERR_OPEN_FILE;
}

// 一時ファイルクローズ
uint8_t sdfiles::tmpClose() {
  if (tfile)
    tfile.close();
  SD.end();
  return 0;
}

// 文字列出力
uint8_t sdfiles::puts(char*s) {
  int16_t n = 0;
  if( tfile && s ) {
    n = tfile.write(s);
  }
  return !n;    
}

// 1バイト出力 
uint8_t sdfiles::putch(char c) {
  int16_t n = 0;
  if(tfile) {
    n = tfile.write(c);
  }
  return !n;   
}

// 1バイト読込
int16_t sdfiles::read() {
  if(!tfile) 
    return -1;
  return tfile.read();
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
  int16_t len = 0;
  int16_t rc;
  
  while(1) {
    rc = tfile.read(str, 1);
    if (rc <= 0) break;
    if (*str == 0x0d)
      continue;
    if (*str == 0x0a) {
      rc = len;
      break;
    }
    str++;
    len++;
    if (len == SD_TEXT_LEN)
      break;
  }
  *str = 0;
  return len;
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
  File myFile;
  char head[2];   // ヘッダー
  int8_t rc = -1;
 
  if (SD_BEGIN() == false) 
    return -SD_ERR_INIT;
    
  // ファイルのオープン
  myFile = SD.open(fname, FILE_READ);
  if (myFile) {
    if (myFile.isDirectory()) {
      rc = - SD_ERR_NOT_FILE;
    } else {
      if (! myFile.read(head, 2) )  {
        rc = -SD_ERR_READ_FILE;
      } else {
        if (head[0] == 0 && head[1] == 0) {
          rc = 0;
        } else {
          rc = 1;
        }
      }
    }
    myFile.close();
  } else {
    rc = -SD_ERR_OPEN_FILE;    
  }
  SD.end();
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
uint8_t sdfiles::loadBitmap(char* fname, uint8_t* ptr, int16_t x, int16_t y, int16_t w,int16_t  h, uint8_t mode) {
  uint8_t rc =1;
 
 if (SD_BEGIN() == false) 
    return SD_ERR_INIT;

  sdbitmap bitmap;
  bitmap.setFilename(fname);
  if (!bitmap.open()) {
    if (bitmap.getBitmapEx(ptr, x, y, w,h, mode))
      rc = SD_ERR_READ_FILE;
    else 
      rc = 0;
    bitmap.close(); 
  } else {
    rc = SD_ERR_OPEN_FILE;
  }
  SD.end();
  return rc;
}

//
// ビットマップファイルのグラフィックVRAMへのロード
//  fname : ターゲットファイル名
//  ptr   : ロードデータの格納アドレス(VRAM)
//  bw    : 次のライン先頭のオフセットバイト数
//  bx    : ビットマップ画像の切り出し座標 x
//  by    : ビットマップ画像の切り出し座標 y
//  w     : ビットマップ画像の切り出し幅
//  h     : ビットマップ画像の切り出し高さ
//  mode  : 色モード 0:通常 1：反転
//[戻り値]
//  正常終了             : 0 
//  SDカード利用失敗     : SD_ERR_INIT
//  ファイルオープン失敗 : SD_ERR_OPEN_FILE
//  ファイル読み込み失敗 : SD_ERR_READ_FILE
uint8_t sdfiles::loadBitmapToGVRAM(char* fname, uint8_t* ptr,int16_t bw, int16_t bx, int16_t by, int16_t w, int16_t h,uint8_t mode) {
  uint8_t rc =1;
  
 if (SD_BEGIN() == false) 
    return SD_ERR_INIT;
  sdbitmap bitmap;
  bitmap.setFilename(fname);
  if (!bitmap.open()) {
    if (bitmap.getBitmap(ptr, bx, by, w,h, mode, (uint16_t)bw))
      rc = SD_ERR_READ_FILE;
    else 
      rc = 0;
    bitmap.close(); 
  } else {
    rc = SD_ERR_OPEN_FILE;    
  }
  SD.end();
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
uint8_t sdfiles::mkdir(char* fname) {
  uint8_t rc = 1;
 
  if (SD_BEGIN() == false) {
    return SD_ERR_INIT;
  }

  if (SD.exists(fname)) {
    rc = SD_ERR_OPEN_FILE;    
  } else {
    if(SD.mkdir(fname) == true) {
      rc = 0;
    } else {
      rc = SD_ERR_OPEN_FILE;
    }
  }
  SD.end();
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
 
  if (SD_BEGIN() == false) 
    return SD_ERR_INIT;

  if(SD.rmdir(fname) == true) {
    rc = 0;
  } else {
    rc =  SD_ERR_OPEN_FILE;
  }
  SD.end();
  return rc;
}

/***
// ファイル名の変更
uint8_t sdfiles::rename(char* old_fname,char* new_fname) {
  uint8_t rc = 1;
  
  if (SD_BEGIN() == false) 
    return 1;

  if(SD.rename(old_fname,new_fname) == true)
    rc = 0;
  SD.end();
  return rc;
}
**/
// ファイルの削除
uint8_t sdfiles::remove(char* fname) {
  uint8_t rc = 1;

  if (SD_BEGIN() == false) 
    return 1;
  if(SD.remove(fname) == true)
    rc = 0;
  SD.end();
  return rc;  
}

