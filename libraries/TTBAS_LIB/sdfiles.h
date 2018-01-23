//
// sdfiles.h
// SDカードファイル操作クラス
// 作成日 2017/06/04 by たま吉さん
// 参考情報
// - Topic: SD Timestamp read? (Read 1 time) http://forum.arduino.cc/index.php?topic=114181.0
//
// 修正履歴
//  2017/07/27, flist()で列数を指定可能
//

#ifndef __sdfiles_h__
#define __sdfiles_h__

#include <Arduino.h>
#include <SdFat.h>
#define FS_NO_GLOBALS
#include <FS.h>

#define SD_CS       (4)   // SDカードモジュールCS
#define SD_PATH_LEN 64      // ディレクトリパス長
#define SD_TEXT_LEN 255     // テキスト１行最大長さ

#include "../../ttbasic/error.h"
#include "../../ttbasic/bstring.h"

#ifndef FILE_OVERWRITE
#define FILE_OVERWRITE	(O_RDWR | O_CREAT | O_TRUNC)
#endif

extern SdFat SD;

bool SD_BEGIN(int mhz = 40);
void SD_END(void);

class Unifile {
public:
  enum uni_type {
    INVALID, SD, FS,
  };

  Unifile() {
    m_type = INVALID;
    m_sd_file = NULL;
  }

  Unifile(File f) {
    m_sd_file = NULL;
    newSdFile();
    *m_sd_file = f;
  }  
  
  Unifile(fs::File f) {
    m_sd_file = NULL;
    newSpiffsFile();
    *m_fs_file = f;
  }

  void newSdFile() {
    cullOldFile();
    m_type = SD;
    m_sd_file = new File();
  }
  
  void newSpiffsFile() {
    cullOldFile();
    m_type = FS;
    m_fs_file = new fs::File();
  }

  bool isDirectory() {
    switch (m_type) {
    case SD: { SD_BEGIN(); bool ret = m_sd_file->isDirectory(); SD_END(); return ret; }
    case FS: return false; /* no directories in SPIFFS */
    default: return false;
    }
  }

  void close() {
    switch (m_type) {
    case SD: { SD_BEGIN(); m_sd_file->close(); SD_END(); return; }
    case FS: m_fs_file->close(); return;
    default: return;
    }
  }

  size_t write(char *s) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->write(s); SD_END(); return ret; }
    case FS: return m_fs_file->write((uint8_t *)s, strlen(s));
    default: return -1;
    }
  }

  size_t write(char *s, size_t sz) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->write(s, sz); SD_END(); return ret; }
    case FS: return m_fs_file->write((uint8_t *)s, sz);
    default: return -1;
    }
  }

  size_t write(uint8_t c) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->write(c); SD_END(); return ret; }
    case FS: return m_fs_file->write(c);
    default: return -1;
    }
  }

  int read() {
    switch (m_type) {
    case SD: { SD_BEGIN(); int ret = m_sd_file->read(); SD_END(); return ret; }
    case FS: return m_fs_file->read();
    default: return -1;
    }
  }

  size_t read(char* buf, size_t size) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->read(buf, size); SD_END(); return ret; }
    case FS: return m_fs_file->read((uint8_t *)buf, size);
    default: return -1;
    }
  }

  size_t fgets(char* str, int num) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->fgets(str, num); SD_END(); return ret; }
    case FS: return -1;
    default: return -1;
    }
  }

  uint32_t fileSize() {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file->fileSize(); SD_END(); return ret; }
    case FS: return m_fs_file->size();
    default: return -1;
    }
  }

  operator bool() {
    switch (m_type) {
    case SD: return (bool)*m_sd_file;
    case FS: return (bool)*m_fs_file;
    default: return false;
    }
  }

  static Unifile open(const char *name, uint8_t flags) {
    if (toupper(name[0]) == 'F' && name[1] == ':') {
      char nam[strlen(name)];
      const char *fl;
      switch (flags) {
      case FILE_WRITE:		fl = "a"; break;
      case FILE_OVERWRITE:	fl = "w"; break;
      case FILE_READ:		fl = "r"; break;
      default:			return Unifile();
      }
      strcpy(nam + 1, name + 2);
      nam[0] = '/';
      Unifile f(SPIFFS.open(nam, fl));
      return f;
    } else {
      SD_BEGIN();
      Unifile f(::SD.open(name, flags));
      SD_END();
      return f;
    }
  }
  
  static Unifile open(BString &name, uint8_t flags) {
    return open(name.c_str(), flags);
  }

  static bool rename(const char *from, const char *to) {
    if (isSPIFFS(from) != isSPIFFS(to))
      return true;
    if (isSPIFFS(from))
      return SPIFFS.rename(from + 2, to + 2);
    else
      return ::SD.rename(from, to);
  }

private:
  void cullOldFile() {
    if (m_sd_file) {
      if (m_type == SD)
        delete m_sd_file;
      else
        delete m_fs_file;
    }
    m_type = INVALID;
    m_sd_file = NULL;
  }
    
  static inline bool isSPIFFS(const char *path) {
    return toupper(path[0]) == 'F' && path[1] == ':';
  }

  union {
    File *m_sd_file;
    fs::File *m_fs_file;
  };
  uni_type m_type;
};

class sdfiles {
private:
  Unifile tfile;
  uint8_t flgtmpOlen;

public:
  uint8_t init(uint8_t cs=SD_CS);                       // 初期設定
  uint8_t load(char* fname, uint8_t* ptr, uint16_t sz); // ファイルのロード
  uint8_t save(char* fname, uint8_t* ptr, uint16_t sz); // ファイルのセーブ
  uint8_t flist(char* _dir, char* wildcard=NULL, uint8_t clmnum=2); // ファイルリスト出力
  uint8_t mkdir(char* fname);                           // ディレクトリの作成
  uint8_t rmdir(char* fname);                           // ディレクトリの削除
  uint8_t rename(char* old_fname,char* new_fname);    // ファイル名の変更
  uint8_t remove(char* fname);                          // ファイルの削除
  uint8_t fcopy(char* old_fname,char* new_fname);       // ファイルのコピー

  uint8_t tmpOpen(char* tfname, uint8_t mode);          // 一時ファイルオープン
  uint8_t tmpClose();                                   // 一時ファイルクローズ
  uint8_t puts(char*s);                                 // 文字列出力
  int16_t read();                                       // 1バイト読込
  uint8_t putch(char c);                                // 1バイト出力 
  int8_t  IsText(char* tfname);                         // ファイルがテキストファイルかチェック
  int16_t readLine(char* str);                          // 1行分読込み
  int8_t  textOut(char* fname, int16_t sline, int16_t ln); // テキストファイルの出力
    
  // ビットマップファイルのロード
  uint8_t loadBitmap(char* fname, int32_t &dst_x, int32_t &dst_y, int32_t x, int32_t y, int32_t &w,int32_t &h);
};

#endif
