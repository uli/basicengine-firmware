#include "SdFat.h"

using namespace sdfat;

SdFat sdfat::SD;

char full_path[1024];

const char *apsd(const char *p) {
    sprintf(full_path, FS_PREFIX "/sd%s", p);
    printf("FP %s\n",full_path);
    return full_path;
}
const char *apfs(const char *p) {
    sprintf(full_path, FS_PREFIX "/flash%s", p);
    return full_path;
}

File File::openNextFile(uint8_t mode) {
  if (!dir.get())
    return File();
  struct dirent *de;
  do {
    de = readdir(dir.get());
  } while (de && (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")));
  if (!de)
    return File();
  BString ap = full_name + "/" + de->d_name;
  return SD.open(ap.c_str(), mode);
}

int sdfat::smart_fclose(FILE *stream)
{
  if (stream)
    return fclose(stream);
  else
    return EOF;
}

int sdfat::smart_closedir(DIR *dirp)
{
  if (dirp)
    return closedir(dirp);
  else
    return -1;
}
