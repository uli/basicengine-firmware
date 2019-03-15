#include "../../ttbasic/ttconfig.h"

#ifdef UNIFILE_STDIO

#include "sdfiles.h"
#include <errno.h>

extern "C" {

FILE *_fopen(const char *path, const char *mode)
{
  int flags;
  switch (mode[0]) {
  case 'a':	flags = UFILE_WRITE; break;
  case 'w':	flags = UFILE_OVERWRITE; break;
  case 'r':	flags = UFILE_READ; break;
  default:	errno = EINVAL; return NULL;
  }
  Unifile *f = new Unifile;
  *f = Unifile::open(path, flags);
  if (*f)
    return (FILE *)f;
  delete f;
  return NULL;
}

int _fclose(FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  f->close();
  delete f;
  return 0;
}

size_t _fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  ssize_t ret = f->read((char *)ptr, size * nmemb);
  if (ret < 0)
    ret = 0;
  return ret / size;
}

size_t _fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  ssize_t ret = f->write((char *)ptr, size * nmemb);
  if (ret < 0)
    ret = 0;
  return ret / size;
}

int _fseek(FILE *stream, int offset, int whence) {
  Unifile *f = (Unifile *)stream;
  bool ret;
  switch (whence) {
    case SEEK_SET: ret = f->seekSet(offset); break;
    case SEEK_CUR: ret = f->seekSet(f->position() + offset); break;
    case SEEK_END: ret = f->seekSet(f->fileSize()); break;
    default: ret = false; break;
  }
  return !ret;
}

int _feof(FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  return !f->available();
}

int _getc(FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  return f->read();
}

int _ungetc(int c, FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  // XXX: This is not really what ungetc() does, but it's how it is usually
  // used. YMMV.
  f->seekSet(f->position() - 1);
  return c;
}

int _putc(int c, FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  if (f->write((char)c) < 0)
    return EOF;
  else
    return c;
}

int _fflush(FILE *stream)
{
  Unifile *f = (Unifile *)stream;
  f->sync();
  return 0;
}

int _ferror(FILE *stream)
{
  // XXX: stream status unimplemented
  return 0;
}

void _clearerr(FILE *stream)
{
  // XXX: stream status unimplemented
}

DIR *opendir(const char *name)
{
  Unifile *d = new Unifile;
  *d = Unifile::openDir(name);
  if (!*d) {
    delete d;
    return NULL;
  }
  return (DIR *)d;
}

int closedir(DIR *dir)
{
  Unifile *d = (Unifile *)dir;
  d->close();
  delete d;
  return 0;
}

struct dirent *readdir(DIR *dir)
{
  static struct dirent de;
  Unifile *d = (Unifile *)dir;

  auto ude = d->next();
  if (!ude)
    return NULL;

  de.d_type = ude.is_directory ? DT_DIR : DT_REG;
  memset(de.d_name, 0, sizeof(de.d_name));
  strncpy(de.d_name, ude.name.c_str(), sizeof(de.d_name) - 1);
  
  return &de;
}

int remove(const char *pathname)
{
  if (Unifile::remove(pathname))
    return 0;
  else
    return -1;
}

int rename(const char *oldpath, const char *newpath)
{
  if (Unifile::rename(oldpath, newpath))
    return 0;
  else
    return -1;
}

}

int chdir(const char *path)
{
  if (Unifile::chDir(path))
    return 0;
  else
    return -1;
}

char *getcwd(char *buf, size_t size)
{
  UnifileString cwd = Unifile::cwd();
  memset(buf, 0, size);
  strncpy(buf, cwd.c_str(), size-1);
  return buf;
}

int stat(const char *pathname, struct stat *buf)
{
  Unifile f = Unifile::open(pathname, UFILE_READ);
  if (!f) {
    errno = ENOENT;
    return -1;
  }
  memset(buf, 0, sizeof(struct stat));
  buf->st_size = f.fileSize();
  buf->st_mode = f.isDirectory() ? S_IFDIR : S_IFREG;
  
  return 0;
}

#endif
