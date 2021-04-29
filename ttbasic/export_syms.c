#include <stdio.h>
#include <string.h>

// Wrap functions that pass platform-specific data structures ONLY if:
// - no alternative standard function exists (counter-example:
//   fopen()/fileno() can be used instead of open())
// - they cannot be replaced with a simple Engine BASIC-specific alternative
//   (counter-example: eb_file_exists() etc. fulfill the typical use of
//   stat())

// This macro hell allows us to include both the host platform's dirent.h
// and the native dirent.h.

#ifdef ALLWINNER_BARE_METAL

// no translation required
#include <dirent.h>

#else

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

#endif // ALLWINNER_BARE_METAL

extern void be_exit(int ret);
extern int c_printf(const char *f, ...);

#include "ttconfig.h"
#include "basic_native.h"

#include "eb_config.h"
#include "eb_conio.h"
#include "eb_file.h"
#include "eb_sys.h"
#include "eb_video.h"
#include "mcurses.h"
#include <stdarg.h>
#include <libgen.h>
#include <sys/fcntl.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <wchar.h>

#define S(n) { #n, (void *)n },
#define R(n, m) { #n, (void *)m },

#ifdef SDL
extern void __va_arg(void);
extern void __va_start(void);

static int *__errno(void) { return &errno; }

// emulate newlib's reent structure
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

#ifdef __arm__
extern void __aeabi_idiv(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_lasr(void);
extern void __aeabi_ldivmod(void);
extern void __aeabi_llsl(void);
extern void __aeabi_llsr(void);
extern void __aeabi_memcpy4(void);
extern void __aeabi_memcpy8(void);
extern void __aeabi_uidiv(void);
#endif

const struct symtab export_syms[] = {
#include "export_syms.h"
     { 0, 0 },
};

#undef S
#undef R
