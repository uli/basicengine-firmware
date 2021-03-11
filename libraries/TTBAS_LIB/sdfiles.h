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

#include "../../ttbasic/ttconfig.h"

#include "../../ttbasic/error.h"
#include "../../ttbasic/BString.h"
typedef BString UnifileString;

#ifdef LOWMEM
#define SD_PATH_LEN 64      // Directory path length
#define SD_TEXT_LEN 255     // Maximum length of one line of text
#else
#define SD_PATH_LEN 1024      // Directory path length
#define SD_TEXT_LEN 4096     // Maximum length of one line of text
#endif

#if defined(UNIFILE_USE_SDFAT) || \
    defined(UNIFILE_USE_FASTROMFS) || \
    defined(UNIFILE_USE_OLD_SPIFFS) || \
    defined(UNIFILE_USE_NEW_SPIFFS) || \
    defined(UNIFILE_USE_NEW_SD)

#define USE_UNIFILE

#if defined(UNIFILE_USE_SDFAT) && defined(UNIFILE_USE_NEW_SD)
#error only one of SdFat and SD can be enabled
#endif

#if defined(UNIFILE_USE_OLD_SPIFFS) && defined(UNIFILE_USE_NEW_SPIFFS)
#error old and new SPIFFS interfaces cannot be used at the same time
#endif

#undef UNIFILE_USE_FS
#if defined(UNIFILE_USE_NEW_SPIFFS) || defined(UNIFILE_USE_NEW_SD)
#define UNIFILE_USE_FS
#endif

#ifdef UNIFILE_USE_SDFAT
#include <SPI.h>
// This relies on having SdFat wrapped in a namespace.
#include <SdFat.h>
#endif

#if defined(UNIFILE_USE_FS)
#include <SD.h>
#include <SPIFFS.h>
#endif

#ifdef UNIFILE_USE_OLD_SPIFFS
#define FS_NO_GLOBALS
#include <FS.h>
#elif defined(UNIFILE_USE_FASTROMFS)
#include <ESP8266FastROMFS.h>
extern FastROMFilesystem fs;
#endif

#ifndef FILE_OVERWRITE
#define FILE_OVERWRITE	(O_RDWR | O_CREAT | O_TRUNC)
#endif

#ifdef UNIFILE_USE_FS
#define UFILE_READ      42
#define UFILE_WRITE     43
#define UFILE_OVERWRITE 44
#else
#define UFILE_WRITE     FILE_WRITE
#define UFILE_READ      FILE_READ
#define UFILE_OVERWRITE FILE_OVERWRITE
#endif

#ifdef UNIFILE_USE_SDFAT
extern sdfat::SdFat sdf;
#endif

bool SD_BEGIN(int mhz = 40);
void SD_END(void);

extern const char FLASH_PREFIX[];
#define FLASH_PREFIX_LEN 6
const char SD_PREFIX[] = "/sd";
#define SD_PREFIX_LEN 3

class Unifile {
public:
  enum uni_type {
    INVALID, SD, SDFAT, FS, SD_DIR, SDFAT_DIR, FS_DIR
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
#ifdef UNIFILE_USE_FASTROMFS
    m_fs_file = NULL;
    m_fs_dir = NULL;
#endif
  }

#ifdef UNIFILE_USE_FS
  Unifile(File f) {
    m_sd_file = f;
    m_type = SD;
#ifdef UNIFILE_USE_FASTROMFS
    m_fs_file = NULL;
    m_fs_dir = NULL;
#endif
  }
#endif

#ifdef UNIFILE_USE_SDFAT
  Unifile(sdfat::File f) {
    m_sdfat_file = f;
    m_type = SDFAT;
#ifdef UNIFILE_USE_FASTROMFS
    m_fs_file = NULL;
    m_fs_dir = NULL;
#endif
  }
#endif

#ifdef UNIFILE_USE_OLD_SPIFFS
  Unifile(fs::File f) {
    m_fs_file = f;
    m_type = FS;
  }

  Unifile(fs::Dir f) {
    m_fs_dir = f;
    m_type = FS_DIR;
  }
#elif defined(UNIFILE_USE_FASTROMFS)
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
    case SD_DIR:
    case SDFAT_DIR:
    case FS_DIR:
      return true;
    default: return false;
    }
  }

  void close() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT_DIR:
    case SDFAT: { SD_BEGIN(); m_sdfat_file.close(); SD_END(); break; }
#endif
#ifdef UNIFILE_USE_FS
    case SD_DIR:
    case SD: { SD_BEGIN(); m_sd_file.close(); SD_END(); break; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: { noInterrupts(); m_fs_file.close(); interrupts(); break; }
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: { noInterrupts(); m_fs_file->sync(); m_fs_file->close(); interrupts(); break; }
#endif
    default: break;
    }
    m_type = INVALID;
  }

  void sync() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT_DIR:
    case SDFAT: { SD_BEGIN(); m_sdfat_file.sync(); SD_END(); break; }
#endif
#ifdef UNIFILE_USE_FS
    case SD_DIR:
    case SD: { SD_BEGIN(); m_sd_file.flush(); SD_END(); break; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: break;
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: { noInterrupts(); m_fs_file->sync(); interrupts(); break; }
#endif
    default: break;
    }
  }
    
  ssize_t write(char *s) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); ssize_t ret = m_sdfat_file.write(s); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: { SD_BEGIN(); ssize_t ret = m_sd_file.write((uint8_t *)s, strlen(s)); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write((uint8_t *)s, strlen(s)); interrupts(); return ret; }
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write((uint8_t *)s, strlen(s)); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  ssize_t write(char *s, size_t sz) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); ssize_t ret = m_sdfat_file.write(s, sz); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: { SD_BEGIN(); ssize_t ret = m_sd_file.write((uint8_t *)s, sz); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write((uint8_t *)s, sz); interrupts(); return ret; }
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write(s, sz); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  ssize_t write(uint8_t c) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); size_t ret = m_sdfat_file.write(c); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_FS
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.write(c); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: { noInterrupts(); ssize_t ret = m_fs_file.write(c); interrupts(); return ret; }
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: { noInterrupts(); ssize_t ret = m_fs_file->write(c); interrupts(); return ret; }
#endif
    default: return -1;
    }
  }

  int read() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); int ret = m_sdfat_file.read(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_FS
    case SD: { SD_BEGIN(); int ret = m_sd_file.read(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.read();
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->read();
#endif
    default: return -1;
    }
  }

  ssize_t read(char *buf, size_t size) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); size_t ret = m_sdfat_file.read(buf, size); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.read((uint8_t *)buf, size); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.read((uint8_t *)buf, size);
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->read((uint8_t *)buf, size);
#endif
    default: return -1;
    }
  }

  ssize_t fgets(char *str, int num) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); size_t ret = m_sdfat_file.fgets(str, num); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: {
      SD_BEGIN();
      String s = m_sd_file.readStringUntil('\n');
      strcpy(str, s.substring(num - 1).c_str());
      size_t ret = strlen(str);
      SD_END();
      return ret;
    }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return -1;
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return -1;
#endif
    default: return -1;
    }
  }

  uint32_t fileSize() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); size_t ret = m_sdfat_file.fileSize(); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.size(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.size();
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->size();
#endif
    default: return -1;
    }
  }

  bool seekSet(size_t pos) {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); bool ret = m_sdfat_file.seekSet(pos); SD_END(); return ret; }
#endif
#if defined(UNIFILE_USE_FS)
    case SD: { SD_BEGIN(); bool ret = m_sd_file.seek(pos); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.seek(pos, fs::SeekSet);
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->seek(pos, SEEK_SET);
#endif
    default: return false;
    }
  }

  size_t position() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); size_t ret = m_sdfat_file.position(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_FS
    case SD: { SD_BEGIN(); size_t ret = m_sd_file.position(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.position();
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->position();
#endif
    default: return -1;
    }
  }

  bool available() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); bool ret = m_sdfat_file.available(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_FS
    case SD: { SD_BEGIN(); bool ret = m_sd_file.available(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.available();
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->available();
#endif
    default: return false;
    }
  }

  int peek() {
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT: { SD_BEGIN(); int ret = m_sdfat_file.peek(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_FS
    case SD: { SD_BEGIN(); int ret = m_sd_file.peek(); SD_END(); return ret; }
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return m_fs_file.peek();
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file->peek();
#endif
    default: return -1;
    }
  }

  UniDirEntry next() {
    UniDirEntry e;
    switch (m_type) {
#ifdef UNIFILE_USE_SDFAT
    case SDFAT_DIR: {
      SD_BEGIN();
      sdfat::File sd_entry = m_sdfat_file.openNextFile();
      if (sd_entry) {
        char name[32];
        sd_entry.getName(name, 32);
        e.name = name;
        e.is_directory = sd_entry.isDirectory();
        e.size = sd_entry.fileSize();
      }
      SD_END();
      break;
    }
#endif
#ifdef UNIFILE_USE_FS
    case SD_DIR: {
      SD_BEGIN();
      File sd_entry = m_sd_file.openNextFile();
      if (sd_entry) {
        e.name = sd_entry.name();
        e.name = e.name.substring(e.name.lastIndexOf('/') + 1);
        e.is_directory = sd_entry.isDirectory();
        e.size = sd_entry.size();
      }
      SD_END();
      break;
    }
#endif
    case FS_DIR:
#ifdef UNIFILE_USE_OLD_SPIFFS
      if (m_fs_dir.next()) {
        e.name = m_fs_dir.fileName().c_str();
        e.is_directory = false;
        e.size = m_fs_dir.fileSize();
      }
#elif defined(UNIFILE_USE_FASTROMFS)
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
#ifdef UNIFILE_USE_SDFAT
    case SDFAT_DIR:
    case SDFAT: return (bool)m_sdfat_file;
#endif
#ifdef UNIFILE_USE_FS
    case SD_DIR:
    case SD: return (bool)m_sd_file;
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
    case FS: return (bool)m_fs_file;
    case FS_DIR: return true;
#elif defined(UNIFILE_USE_FASTROMFS)
    case FS: return m_fs_file != NULL;
    case FS_DIR: return m_fs_dir != NULL;
#endif
    default: return false;
    }
  }

  static Unifile open(const char *name, int flags) {
    UnifileString abs_name = path(name);
    if (isSPIFFS(abs_name)) {
      UnifileString spiffs_name = abs_name.substring(FLASH_PREFIX_LEN
#ifndef UNIFILE_USE_NEW_SPIFFS
                                                                      + 1
#endif
                                                                          , 256);
      const char *fl;
      switch (flags) {
      case UFILE_WRITE:     fl = "a"; break;
      case UFILE_OVERWRITE: fl = "w"; break;
      case UFILE_READ:      fl = "r"; break;
      default:              return Unifile();
      }
#ifdef UNIFILE_USE_OLD_SPIFFS
      fs::File f = SPIFFS.open(spiffs_name.c_str(), fl);
#elif defined(UNIFILE_USE_FASTROMFS)
      FastROMFile *f = fs.open(spiffs_name.c_str(), fl);
#elif defined(UNIFILE_USE_NEW_SPIFFS)
      File f = SPIFFS.open(spiffs_name.c_str(), fl);
#else
      File f;
#endif
      if (f)
        return Unifile(f);
      else
        return Unifile();
    } else {
      SD_BEGIN();
      UnifileString sdfat_name = abs_name.substring(SD_PREFIX_LEN, 256);
#ifdef UNIFILE_USE_SDFAT
      sdfat::File f = sdf.open(sdfat_name.c_str(), flags);
#elif defined(UNIFILE_USE_NEW_SD)
      const char *fl;
      switch (flags) {
      case UFILE_WRITE:     fl = "a"; break;
      case UFILE_OVERWRITE: fl = "w"; break;
      case UFILE_READ:      fl = "r"; break;
      default:              return Unifile();
      }
      File f = ::SD.open(sdfat_name.c_str(), fl);
#endif
      SD_END();
      if (f)
        return Unifile(f);
      else
        return Unifile();
    }
  }

  static Unifile open(const UnifileString &name, int flags) {
    return open(name.c_str(), flags);
  }

  static bool rename(const char *from, const char *to) {
    UnifileString abs_from = path(from);
    UnifileString abs_to = path(to);
    if (isSPIFFS(abs_from) != isSPIFFS(abs_to))
      return true;
    if (isSPIFFS(abs_from))
#ifdef UNIFILE_USE_OLD_SPIFFS
      return SPIFFS.rename(abs_from.c_str() + FLASH_PREFIX_LEN + 1,
                           abs_to.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_FASTROMFS)
      return fs.rename(abs_from.c_str() + FLASH_PREFIX_LEN + 1,
                       abs_to.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_NEW_SPIFFS)
      return SPIFFS.rename(abs_from.c_str() + FLASH_PREFIX_LEN,
                           abs_to.c_str() + FLASH_PREFIX_LEN);
#else
      return false;
#endif
    else {
      SD_BEGIN();
#ifdef UNIFILE_USE_SDFAT
      bool ret = sdf.rename(abs_from.c_str() + SD_PREFIX_LEN,
                            abs_to.c_str() + SD_PREFIX_LEN);
#elif defined(UNIFILE_USE_NEW_SD)
      bool ret = ::SD.rename(abs_from.c_str() + SD_PREFIX_LEN,
                             abs_to.c_str() + SD_PREFIX_LEN);
#else
      ret = false;
#endif
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
    } else if (rel[0] == '.' && rel[1] == 0) {
      absolute = m_cwd;
    } else {
      while (rel[0] == '.' && rel[1] == '/')
        rel += 2;
      absolute = m_cwd + UnifileString("/") + UnifileString(rel);
    }
    return absolute;
  }

  static bool exists(const char *file) {
    UnifileString abs_file = path(file);
    if (isSPIFFS(abs_file)) {
#ifdef UNIFILE_USE_OLD_SPIFFS
      return SPIFFS.exists(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_FASTROMFS)
      return fs.exists(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_NEW_SPIFFS)
      return SPIFFS.exists(abs_file.c_str() + FLASH_PREFIX_LEN);
#endif
    } else {
      SD_BEGIN();
#ifdef UNIFILE_USE_SDFAT
      bool ret = sdf.exists(abs_file.c_str() + SD_PREFIX_LEN);
#elif defined(UNIFILE_USE_NEW_SD)
      bool ret = ::SD.exists(abs_file.substring(SD_PREFIX_LEN, abs_file.length()-1).c_str());
#endif
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
#ifdef UNIFILE_USE_OLD_SPIFFS
      return SPIFFS.remove(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_FASTROMFS)
      return fs.unlink(abs_file.c_str() + FLASH_PREFIX_LEN + 1);
#elif defined(UNIFILE_USE_NEW_SPIFFS)
      return SPIFFS.remove(abs_file.c_str() + FLASH_PREFIX_LEN);
#else
      return false;
#endif
    else {
      SD_BEGIN();
#ifdef UNIFILE_USE_SDFAT
      bool ret = sdf.remove(abs_file.c_str() + SD_PREFIX_LEN);
#elif defined(UNIFILE_USE_NEW_SD)
      bool ret = ::SD.remove(abs_file.c_str() + SD_PREFIX_LEN);
#endif
      SD_END();
      return ret;
    }
  }

  static Unifile openDir(const char *p) {
    UnifileString abs_path = path(p);
    if (isSPIFFS(abs_path)) {
#ifdef UNIFILE_USE_OLD_SPIFFS
      return Unifile(SPIFFS.openDir(abs_path.c_str() + FLASH_PREFIX_LEN + 1));
#elif defined(UNIFILE_USE_FASTROMFS)
      return Unifile(fs.opendir(abs_path.c_str() + FLASH_PREFIX_LEN + 1));
#elif defined(UNIFILE_USE_NEW_SPIFFS)
      while (abs_path.length() > FLASH_PREFIX_LEN + 1 &&
             abs_path.endsWith(UnifileString('/')))
        abs_path = abs_path.substring(0, abs_path.length() - 1);
      Unifile uf = Unifile(SPIFFS.open(abs_path.c_str() + FLASH_PREFIX_LEN));
      uf.m_type = SD_DIR;
      return uf;
#else
      return Unifile();
#endif
    } else {
      SD_BEGIN();
#ifdef UNIFILE_USE_NEW_SD
      while (abs_path.endsWith(UnifileString('/')))
        abs_path = abs_path.substring(0, abs_path.length() - 1);
      File f = ::SD.open(abs_path == SD_PREFIX ? "/" : abs_path.c_str() + SD_PREFIX_LEN);
#elif defined(UNIFILE_USE_SDFAT)
      sdfat::File f = sdf.open(abs_path == SD_PREFIX ? "/" : abs_path.c_str() + SD_PREFIX_LEN);
#endif
      if (!f ||
#ifdef UNIFILE_USE_SDFAT
          !f.isDir()
#elif defined(UNIFILE_USE_NEW_SD)
          !f.isDirectory()
#endif
      ) {
        f.close();
        SD_END();
        return Unifile();
      }
      f.rewindDirectory();
      Unifile uf(f);
#ifdef UNIFILE_USE_SDFAT
      uf.m_type = SDFAT_DIR;
#elif defined(UNIFILE_USE_NEW_SD)
      uf.m_type = SD_DIR;
#endif
      SD_END();
      return uf;
    }
  }

private:
#ifdef UNIFILE_USE_SDFAT
  sdfat::File m_sdfat_file;
#endif
#ifdef UNIFILE_USE_FS
  File m_sd_file;
#endif
#ifdef UNIFILE_USE_OLD_SPIFFS
  fs::File m_fs_file;
  fs::Dir m_fs_dir;
#elif defined(UNIFILE_USE_FASTROMFS)
  FastROMFile *m_fs_file;
  FastROMFSDir *m_fs_dir;
#endif
  uni_type m_type;

  static UnifileString m_cwd;
};

#else  // use Unifile
#include <sys/stat.h>
#include <dirent.h>

#ifdef __DJGPP__

static const char FLASH_PREFIX[] = "A:";
#define FLASH_PREFIX_LEN 2
const char SD_PREFIX[] = "C:";
#define SD_PREFIX_LEN    2

#endif

#endif  // use Unifile

class sdfiles {
private:
  FILE *tfile;
  uint8_t flgtmpOlen;

public:
  uint8_t init(uint8_t cs);                       // 初期設定
  uint8_t flist(char* _dir, char* wildcard=NULL, uint8_t clmnum=2); // ファイルリスト出力
  uint8_t mkdir(const char* fname);                           // ディレクトリの作成
  uint8_t rmdir(const char* fname);                           // ディレクトリの削除
  uint8_t rename(char* old_fname,char* new_fname);    // ファイル名の変更
  uint8_t fcopy(const char* old_fname,const char* new_fname);       // ファイルのコピー
  int8_t compare(const char *one, const char *two);

  uint8_t tmpOpen(char* tfname, uint8_t mode);          // 一時ファイルオープン
  uint8_t tmpClose();                                   // 一時ファイルクローズ
  uint8_t puts(char*s);                                 // 文字列出力
  int16_t read();                                       // 1バイト読込
  inline ssize_t tmpRead(char* buf, size_t size) {
    return fread(buf, 1, size, tfile);
  }
  uint8_t putch(char c);                                // 1バイト出力 
  int8_t  IsText(char* tfname);                         // ファイルがテキストファイルかチェック
  int16_t readLine(char* str);                          // 1行分読込み
  int8_t  textOut(char* fname, int16_t sline, int16_t ln); // テキストファイルの出力
  inline size_t  tmpSize() {
    size_t now = ftell(tfile);
    fseek(tfile, 0, SEEK_END);
    size_t end = ftell(tfile);
    fseek(tfile, now, SEEK_SET);
    return end;
  }
  inline ssize_t tmpWrite(char *s, size_t sz) {
    return fwrite(s, 1, sz, tfile);
  }

  void setTempFile(FILE *f) {
    tfile = f;
  }

  void fakeTime();

  // ビットマップファイルのロード
  uint8_t loadBitmap(char *fname, int32_t &dst_x, int32_t &dst_y, int32_t x,
                     int32_t y, int32_t &w, int32_t &h, uint32_t mask = (uint32_t)-1);
  uint8_t saveBitmap(char *fname, int32_t src_x, int32_t src_y, int32_t w,
                     int32_t h);
};

#endif
