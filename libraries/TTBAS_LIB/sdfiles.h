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

#ifdef UNIFILE_USE_SPIFFS
#define FS_NO_GLOBALS
#include <FS.h>
#else
#include <ESP8266FastROMFS.h>
extern FastROMFilesystem fs;
#endif

#define SD_CS       (4)   // SDカードモジュールCS
#define SD_PATH_LEN 64      // ディレクトリパス長
#define SD_TEXT_LEN 255     // テキスト１行最大長さ

#include "../../ttbasic/error.h"
#include "../../ttbasic/BString.h"
typedef BString UnifileString;

#ifndef FILE_OVERWRITE
#define FILE_OVERWRITE	(O_RDWR | O_CREAT | O_TRUNC)
#endif

extern SdFat SD;

bool SD_BEGIN(int mhz = 40);
void SD_END(void);

const char FLASH_PREFIX[] = "/flash";
#define FLASH_PREFIX_LEN 6
const char SD_PREFIX[] = "/sd";
#define SD_PREFIX_LEN 3

class Unifile {
public:
  enum uni_type {
    INVALID, SD, FS, SD_DIR, FS_DIR
  };

  class UniDirEntry {
  public:
    UniDirEntry() {
      size = -1;
      is_directory = false;
    };
    UnifileString name;
    ssize_t size;
    bool is_directory;
    
    operator bool() {
      return size >= 0;
    }
  };

  Unifile() {
    m_type = INVALID;
#ifndef UNIFILE_USE_SPIFFS
    m_fs_file = NULL;
    m_fs_dir = NULL;
#endif
  }
  
  Unifile(File f) {
    m_sd_file = f;
    m_type = SD;
#ifndef UNIFILE_USE_SPIFFS
    m_fs_file = NULL;
    m_fs_dir = NULL;
#endif
  }  
  
#ifdef UNIFILE_USE_SPIFFS
  Unifile(fs::File f) {
    m_fs_file = f;
    m_type = FS;
  }

  Unifile(fs::Dir f) {
    m_fs_dir = f;
    m_type = FS_DIR;
  }
#else
  Unifile(FastROMFile *f) {
    m_fs_file = f;
    if (f)
      m_type = FS;
    else
      m_type = INVALID;
    m_fs_dir = NULL;
  }
  
  Unifile(FastROMFSDir *f) {
    m_fs_dir = f;
    if (f)
      m_type = FS_DIR;
    else
      m_type = INVALID;
    m_fs_file = NULL;
  }
#endif

  bool isDirectory() {
    switch (m_type) {
    case SD: { SD_BEGIN(); bool ret = m_sd_file.isDirectory(); SD_END(); return ret; }
    case FS: return false; /* no directories in SPIFFS */
    default: return false;
    }
  }

  void close() {
    switch (m_type) {
    case SD_DIR:
    case SD: { SD_BEGIN(); m_sd_file.close(); SD_END(); break; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: { noInterrupts(); m_fs_file.close(); interrupts(); break; }
#else
    case FS: { noInterrupts(); m_fs_file->sync(); m_fs_file->close(); interrupts(); break; }
#endif
    default: break;
    }
    m_type = INVALID;
  }

  ssize_t write(char *s) {
    switch (m_type) {
    case SD: { SD_BEGIN(); ssize_t ret = m_sd_file.write(s); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write((uint8_t *)s, strlen(s)); interrupts(); return ret; }
#else
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write((uint8_t *)s, strlen(s)); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  ssize_t write(char *s, size_t sz) {
    switch (m_type) {
    case SD: { SD_BEGIN(); ssize_t ret = m_sd_file.write(s, sz); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write((uint8_t *)s, sz); interrupts(); return ret; }
#else
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write(s, sz); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  ssize_t write(uint8_t c) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.write(c); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write(c); interrupts(); return ret; }
#else
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write(c); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  int read() {
    switch (m_type) {
    case SD: { SD_BEGIN(); int ret = m_sd_file.read(); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.read();
#else
    case FS: return m_fs_file->read();
#endif
    default: return -1;
    }
  }

  ssize_t read(char* buf, size_t size) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.read(buf, size); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.read((uint8_t *)buf, size);
#else
    case FS: return m_fs_file->read((uint8_t *)buf, size);
#endif
    default: return -1;
    }
  }

  ssize_t fgets(char* str, int num) {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.fgets(str, num); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return -1;
#else
    case FS: return -1;
#endif
    default: return -1;
    }
  }

  uint32_t fileSize() {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.fileSize(); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.size();
#else
    case FS: return m_fs_file->size();
#endif
    default: return -1;
    }
  }

  bool seekSet(size_t pos) {
    switch (m_type) {
    case SD: { SD_BEGIN(); bool ret = m_sd_file.seekSet(pos); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.seek(pos, fs::SeekSet);
#else
    case FS: return m_fs_file->seek(pos, SEEK_SET);
#endif
    default: return false;
    }
  }

  size_t position() {
    switch (m_type) {
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.position(); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.position();
#else
    case FS: return m_fs_file->position();
#endif
    default: return -1;
    }
  }
  
  bool available() {
    switch (m_type) {
    case SD: { SD_BEGIN(); bool ret = m_sd_file.available(); SD_END(); return ret; }
#ifdef UNIFILE_USE_SPIFFS
    case FS: return m_fs_file.available();
#else
    case FS: return m_fs_file->available();
#endif
    default: return false;
    }
  }

  UniDirEntry next() {
    UniDirEntry e;
    File sd_entry;
    switch (m_type) {
    case SD_DIR:
      SD_BEGIN();
      sd_entry = m_sd_file.openNextFile();
      if (sd_entry) {
        char name[32];
        sd_entry.getName(name, 32);
        e.name = name;
        e.is_directory = sd_entry.isDirectory();
        e.size = sd_entry.fileSize();
      }
      SD_END();
      break;
    case FS_DIR:
#ifdef UNIFILE_USE_SPIFFS
      if (m_fs_dir.next()) {
        e.name = m_fs_dir.fileName().c_str();
        e.is_directory = false;
        e.size = m_fs_dir.fileSize();
      }
#else
      {
        struct FastROMFSDirent *de = fs.readdir(m_fs_dir);
        if (de) {
          e.name = de->name;
          e.is_directory = false;
          e.size = de->len;
        }
      }
#endif
      break;
    default:
      break;
    }
    return e;
  }

  operator bool() {
    switch (m_type) {
    case SD_DIR:
    case SD: return (bool)m_sd_file;
#ifdef UNIFILE_USE_SPIFFS
    case FS: return (bool)m_fs_file;
    case FS_DIR: return true;
#else
    case FS: return m_fs_file != NULL;
    case FS_DIR: return m_fs_dir != NULL;
#endif
    default: return false;
    }
  }

  static Unifile open(const char *name, uint8_t flags) {
    UnifileString abs_name = path(name);
    if (isSPIFFS(abs_name)) {
      UnifileString spiffs_name = abs_name.substring(FLASH_PREFIX_LEN + 1, 256);
      const char *fl;
      switch (flags) {
      case FILE_WRITE:		fl = "a"; break;
      case FILE_OVERWRITE:	fl = "w"; break;
      case FILE_READ:		fl = "r"; break;
      default:			return Unifile();
      }
#ifdef UNIFILE_USE_SPIFFS
      fs::File f = SPIFFS.open(spiffs_name.c_str(), fl);
#else
      FastROMFile *f = fs.open(spiffs_name.c_str(), fl);
#endif
      if (f)
        return Unifile(f);
      else
        return Unifile();
    } else {
      SD_BEGIN();
      UnifileString sdfat_name = abs_name.substring(SD_PREFIX_LEN, 256);
      File f = ::SD.open(sdfat_name.c_str(), flags);
      SD_END();
      if (f)
        return Unifile(f);
      else
        return Unifile();
    }
  }
  
  static Unifile open(UnifileString &name, uint8_t flags) {
    return open(name.c_str(), flags);
  }

  static bool rename(const char *from, const char *to) {
    UnifileString abs_from = path(from);
    UnifileString abs_to = path(to);
    if (isSPIFFS(abs_from) != isSPIFFS(abs_to))
      return true;
    if (isSPIFFS(abs_from))
#ifdef UNIFILE_USE_SPIFFS
      return SPIFFS.rename(abs_from.c_str() + FLASH_PREFIX_LEN + 1, abs_to.c_str() + FLASH_PREFIX_LEN + 1);
#else
      return fs.rename(abs_from.c_str() + FLASH_PREFIX_LEN + 1, abs_to.c_str() + FLASH_PREFIX_LEN + 1);
#endif
    else {
      SD_BEGIN();
      bool ret = ::SD.rename(abs_from.c_str() + SD_PREFIX_LEN, abs_to.c_str() + SD_PREFIX_LEN);
      SD_END();
      return ret;
    }
  }
  
  static inline bool isSPIFFS(UnifileString file) {
    return file.startsWith(FLASH_PREFIX);
  }

  static UnifileString path(const char *rel) {
    UnifileString absolute;
    if (rel[0] == '/') {
      absolute = rel;
    } else {
      absolute = m_cwd + UnifileString("/") + UnifileString(rel);
    }
    return absolute;
  }

  static bool exists(const char *file) {
    UnifileString abs_file = path(file);
    if (isSPIFFS(abs_file)) {
#ifdef UNIFILE_USE_SPIFFS
      return SPIFFS.exists(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#else
      return fs.exists(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#endif
    } else {
      SD_BEGIN();
      bool ret = ::SD.exists(abs_file.c_str() + SD_PREFIX_LEN);
      SD_END();
      return ret;
    }
  }  

  static bool chDir(const char *p) {
    UnifileString new_cwd = path(p);
    if (!isSPIFFS(new_cwd) && !exists((new_cwd + "/").c_str()))
      return false;
    m_cwd = new_cwd;
    return true;
  }
  
  static const char *cwd() {
    return m_cwd.c_str();
  }

  static bool remove(const char *p) {
    UnifileString abs_file = path(p);
    if (isSPIFFS(abs_file))
#ifdef UNIFILE_USE_SPIFFS
      return SPIFFS.remove(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#else
      return fs.unlink(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#endif
    else {
      SD_BEGIN();
      bool ret = ::SD.remove(abs_file.c_str() + SD_PREFIX_LEN);
      SD_END();
      return ret;
    }
  }

  static Unifile openDir(const char *p) {
    UnifileString abs_path = path(p);
    if (isSPIFFS(abs_path)) {
#ifdef UNIFILE_USE_SPIFFS
      return Unifile(SPIFFS.openDir(abs_path.c_str() + FLASH_PREFIX_LEN + 1));
#else
      return Unifile(fs.opendir(abs_path.c_str() + FLASH_PREFIX_LEN + 1));
#endif      
    } else {
      SD_BEGIN();
      File f = ::SD.open(abs_path == SD_PREFIX ? "/" : abs_path.c_str() + SD_PREFIX_LEN);
      if (!f || !f.isDir()) {
        f.close();
        SD_END();
        return Unifile();
      }
      f.rewindDirectory();
      Unifile uf(f);
      uf.m_type = SD_DIR;
      SD_END();
      return uf;
    }
  }

private:
  File m_sd_file;
#ifdef UNIFILE_USE_SPIFFS
  fs::File m_fs_file;
  fs::Dir m_fs_dir;
#else
  FastROMFile *m_fs_file;
  FastROMFSDir *m_fs_dir;
#endif
  uni_type m_type;

  static UnifileString m_cwd;
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
  uint8_t fcopy(const char* old_fname,const char* new_fname);       // ファイルのコピー

  uint8_t tmpOpen(char* tfname, uint8_t mode);          // 一時ファイルオープン
  uint8_t tmpClose();                                   // 一時ファイルクローズ
  uint8_t puts(char*s);                                 // 文字列出力
  int16_t read();                                       // 1バイト読込
  inline ssize_t tmpRead(char* buf, size_t size) {
    return tfile.read(buf, size);
  }
  uint8_t putch(char c);                                // 1バイト出力 
  int8_t  IsText(char* tfname);                         // ファイルがテキストファイルかチェック
  int16_t readLine(char* str);                          // 1行分読込み
  int8_t  textOut(char* fname, int16_t sline, int16_t ln); // テキストファイルの出力
  inline size_t  tmpSize() {
    return tfile.fileSize();
  }
  inline ssize_t tmpWrite(char *s, size_t sz) {
    return tfile.write(s, sz);
  }

  void setTempFile(Unifile f) {
    tfile = f;
  }

  void fakeTime();
    
  // ビットマップファイルのロード
  uint8_t loadBitmap(char* fname, int32_t &dst_x, int32_t &dst_y, int32_t x, int32_t y, int32_t &w,int32_t &h, int mask = -1);
  uint8_t saveBitmap(char* fname, int32_t src_x, int32_t src_y, int32_t w,int32_t h);
};

#endif
