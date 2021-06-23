#include <stdio.h>
#include <string.h>

#ifdef ALLWINNER_BARE_METAL

// no dirent or stat translation required
#include <dirent.h>

#else

// ===== Translate host dirent to native dirent

// This macro hell allows us to include both the host platform's dirent.h
// and the native dirent.h.

#define dirent   _native_dirent
#define DIR      _native_DIR
#define opendir  _native_opendir
#define closedir _native_closedir
#define readdir  _native_readdir
#include "../include/dirent.h"
#define _native_DT_DIR DT_DIR
#define _native_DT_REG DT_REG
#undef DT_DIR
#undef DT_REG
#undef DIR
#undef opendir
#undef closedir
#undef readdir
#undef dirent
#include <dirent.h>

struct _native_dirent *be_readdir(DIR *dirp) {
  static struct _native_dirent trans_dir;

  struct dirent *dir = readdir(dirp);

  if (dir) {
    strcpy(trans_dir.d_name, dir->d_name);

    switch (dir->d_type) {
    case DT_REG: trans_dir.d_type = _native_DT_REG; break;
    case DT_DIR: trans_dir.d_type = _native_DT_DIR; break;
    default: trans_dir.d_type = 0;
    }

    return &trans_dir;
  }

  return NULL;
}

// ===== Translate host {f,l,}stat() to native {f,l,}stat()

#include <sys/stat.h>

// discount version of newlib's struct stat
struct _native_stat {
  dev_t         st_dev;
  ino_t         st_ino;
  mode_t        st_mode;
  nlink_t       st_nlink;
  uid_t         st_uid;
  gid_t         st_gid;
  dev_t         st_rdev;
  off_t         st_size;
};

// XXX: Does not translate timestamps.
static void translate_stat(struct _native_stat *nsb, struct stat *sb) {
  memset(nsb, 0, sizeof(struct _native_stat));
  nsb->st_mode = sb->st_mode;
  nsb->st_size = sb->st_size;
}

static int _native_stat(const char *pathname, struct _native_stat *statbuf) {
  struct stat sb;
  int ret = stat(pathname, &sb);
  if (!ret)
    translate_stat(statbuf, &sb);
  return ret;
}

static int _native_fstat(int fd, struct _native_stat *statbuf) {
  struct stat sb;
  int ret = fstat(fd, &sb);
  if (!ret)
    translate_stat(statbuf, &sb);
  return ret;
}

static int _native_lstat(const char *pathname, struct _native_stat *statbuf) {
  struct stat sb;
  int ret = lstat(pathname, &sb);
  if (!ret)
    translate_stat(statbuf, &sb);
  return ret;
}

#endif // ALLWINNER_BARE_METAL


extern void be_exit(int ret);
extern int c_printf(const char *f, ...);

#include "ttconfig.h"
#include "basic_native.h"  // provides struct symtab

// ===== include all the stuff we are going to export

#include "eb_basic.h"
#include "eb_bg.h"
#include "eb_config.h"
#include "eb_conio.h"
#include "eb_io.h"
#include "eb_file.h"
#include "eb_img.h"
#include "eb_input.h"
#include "eb_native.h"
#include "eb_sound.h"
#include "eb_sys.h"
#include "eb_video.h"
#include "mcurses.h"
#include <ctype.h>
#include <errno.h>
#include <fnmatch.h>
#include <libgen.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <sts_mixer.h>
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_image_write.h>

// ===== CPU architecture-specific exports

#ifdef __x86_64__
extern void __va_arg(void);
extern void __va_start(void);
extern void __fixunsxfdi(void);
extern void __fixunssfdi(void);
extern void __fixunsdfdi(void);
#endif

#ifdef __arm__
extern void __aeabi_d2lz(void);
extern void __aeabi_d2ulz(void);
extern void __aeabi_dadd(void);
extern void __aeabi_drsub(void);
extern void __aeabi_dsub(void);
extern void __aeabi_f2d(void);
extern void __aeabi_f2lz(void);
extern void __aeabi_f2ulz(void);
extern void __aeabi_i2d(void);
extern void __aeabi_idiv(void);
extern void __aeabi_idiv0(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_l2d(void);
extern void __aeabi_l2f(void);
extern void __aeabi_lasr(void);
extern void __aeabi_ldiv0(void);
extern void __aeabi_ldivmod(void);
extern void __aeabi_llsl(void);
extern void __aeabi_llsr(void);
extern void __aeabi_memcpy(void);
extern void __aeabi_memcpy4(void);
extern void __aeabi_memcpy8(void);
extern void __aeabi_memmove(void);
extern void __aeabi_memmove4(void);
extern void __aeabi_memmove8(void);
extern void __aeabi_memset(void);
extern void __aeabi_ui2d(void);
extern void __aeabi_uidiv(void);
extern void __aeabi_uidivmod(void);
extern void __aeabi_ul2d(void);
extern void __aeabi_ul2f(void);
extern void __aeabi_uldivmod(void);
#endif

#ifdef SDL

// We are running on a random C library, but our native code uses newlib
// header files, which define macros to access errno and stdio streams. We
// therefore have to emulate the underlying implementation for things to
// work correctly.

static int *__errno(void) { return &errno; }

// discount version of newlib's reent structure
struct fake_reent {
  int _errno;
  FILE *_stdin, *_stdout, *_stderr;
};
static struct fake_reent fake_reent;

struct fake_reent *__getreent(void) {
  fake_reent._stdin = stdin;
  fake_reent._stdout = stdout;
  fake_reent._stderr = stderr;
  return &fake_reent;
}

#endif

#define S(n) { #n, (void *)n },
#define R(n, m) { #n, (void *)m },

const struct symtab export_syms[] = {
#include "export_syms.h"
     { 0, 0 },
};

#undef S
#undef R
