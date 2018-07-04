#ifndef _SDFAT_H
#define _SDFAT_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FILE_READ O_RDONLY
#define FILE_WRITE (O_RDWR | O_CREAT | O_APPEND)
#define FILE_OVERWRITE	(O_RDWR | O_CREAT | O_TRUNC)
#define O_READ O_RDONLY

#define FS_PREFIX "./vfs"

const char *apsd(const char *p);
const char *apfs(const char *p);

typedef struct {
  uint16_t creationDate;
  uint16_t creationTime;
} dir_t;

class SdFile {
public:
  static void dateTimeCallback(
    void (*dateTime)(uint16_t* date, uint16_t* time)) {
  }
};

class SdFat;

class File {
  friend class SdFat;
public:
  File() {
    fp = NULL;
  }
  bool close() {
    if (fp) {
      int ret = fclose(fp);
      fp = NULL;
      return ret;
    } else
      return false;
  }
  bool sync() {
    return false;
  }
  int write(const void* buf, size_t nbyte) {
    return -1;
  }
  int write(const char* str) {
    return write(str, strlen(str));
  }
  int write(uint8_t b) {
    return write(&b, 1);
  }
  int read(void* buf, size_t nbyte) {
    if (!fp)
      return -1;
    return fread(buf, 1, nbyte, fp);
  }
  int read() {
    uint8_t b;
    return read(&b, 1) == 1 ? b : -1;
  }
  int16_t fgets(char* str, int16_t num, char* delim = 0) {
    return -1;
  }
  uint32_t fileSize() const {
    if (!fp)
      return 0;
    size_t c = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size_t s = ftell(fp);
    fseek(fp, c, SEEK_SET);
    return s;
  }
  bool seekSet(uint32_t pos) {
    if (!fp)
      return false;
    else
      return fseek(fp, pos, SEEK_SET) == 0;
  }
  uint32_t available() {
    size_t c = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size_t e = ftell(fp);
    fseek(fp, c, SEEK_SET);
    return e-c;
  }
  int position() {
    if (!fp)
      return -1;
    else
      return ftell(fp);
  }
  int peek() {
    return -1;
  }
  File openNextFile(uint8_t mode = O_READ) {
    File tmpFile;
    return tmpFile;
  }
  bool getName(char* name, size_t size) {
    return false;
  }
  bool isDirectory() const {
    return false;
  }
  bool isDir() const {
    return isDirectory();
  }
  void rewindDirectory() {
  }
  bool dirEntry(dir_t* dst) {
    return false;
  }
  operator bool() {
    return fp != NULL;
  }
private:
  FILE *fp;
};

#define SD_SCK_MHZ(x) 0

class SdFat {
public:
  File open(const char *path, int mode = FILE_READ) {
    File tmpFile;
    char mod[2];
    mod[1]=0;
    switch (mode) {
    case FILE_READ:	mod[0] = 'r'; break;
    case FILE_OVERWRITE:mod[0] = 'w'; break;
    case FILE_WRITE:	mod[0] = 'a'; break;
    default:		mod[0] = 'r'; break;
    }
    tmpFile.fp = fopen(apsd(path), mod);
    return tmpFile;
  }
  bool rename(const char *oldPath, const char *newPath) {
    return false;
  }
  bool exists(const char* path) {
    struct stat st;
    return lstat(apsd(path), &st) == 0;
  }
  bool remove(const char* path) {
    return false;
  }
  bool begin(uint8_t csPin = 0, int foo = 0) {
    return true;
  }
  bool mkdir(const char* path, bool pFlag = true) {
    return false;
  }
  bool rmdir(const char* path) {
    return false;
  }
};

/** date field for FAT directory entry
 * \param[in] year [1980,2107]
 * \param[in] month [1,12]
 * \param[in] day [1,31]
 *
 * \return Packed date for dir_t entry.
 */
static inline uint16_t FAT_DATE(uint16_t year, uint8_t month, uint8_t day) {
  return (year - 1980) << 9 | month << 5 | day;
}
/** year part of FAT directory date field
 * \param[in] fatDate Date in packed dir format.
 *
 * \return Extracted year [1980,2107]
 */
static inline uint16_t FAT_YEAR(uint16_t fatDate) {
  return 1980 + (fatDate >> 9);
}
/** month part of FAT directory date field
 * \param[in] fatDate Date in packed dir format.
 *
 * \return Extracted month [1,12]
 */
static inline uint8_t FAT_MONTH(uint16_t fatDate) {
  return (fatDate >> 5) & 0XF;
}
/** day part of FAT directory date field
 * \param[in] fatDate Date in packed dir format.
 *
 * \return Extracted day [1,31]
 */
static inline uint8_t FAT_DAY(uint16_t fatDate) {
  return fatDate & 0X1F;
}
/** time field for FAT directory entry
 * \param[in] hour [0,23]
 * \param[in] minute [0,59]
 * \param[in] second [0,59]
 *
 * \return Packed time for dir_t entry.
 */
static inline uint16_t FAT_TIME(uint8_t hour, uint8_t minute, uint8_t second) {
  return hour << 11 | minute << 5 | second >> 1;
}
/** hour part of FAT directory time field
 * \param[in] fatTime Time in packed dir format.
 *
 * \return Extracted hour [0,23]
 */
static inline uint8_t FAT_HOUR(uint16_t fatTime) {
  return fatTime >> 11;
}
/** minute part of FAT directory time field
 * \param[in] fatTime Time in packed dir format.
 *
 * \return Extracted minute [0,59]
 */
static inline uint8_t FAT_MINUTE(uint16_t fatTime) {
  return (fatTime >> 5) & 0X3F;
}
/** second part of FAT directory time field
 * Note second/2 is stored in packed time.
 *
 * \param[in] fatTime Time in packed dir format.
 *
 * \return Extracted second [0,58]
 */
static inline uint8_t FAT_SECOND(uint16_t fatTime) {
  return 2*(fatTime & 0X1F);
}

extern SdFat SD;

#endif
