#include <stdio.h>

#ifdef __GLIBC__
// avoid special C++ prototype for memrchr() in glibc
#undef __cplusplus
extern "C" {
#include <string.h>
}
#define __cplusplus 201103L
#endif

#include "basic.h"
#include "basic_native.h"

typedef unsigned int pixel_t;
typedef unsigned int ipixel_t;

#include "eb_conio.h"
#include "eb_sys.h"
#include "eb_video.h"
#include <stdarg.h>
#include <libgen.h>
#include <sys/fcntl.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define S(n) { #n, (void *)n },
#define R(n, m) { #n, (void *)m },

#ifdef __x86_64__
extern "C" void __va_arg(void);
extern "C" void __va_start(void);

static int *__errno(void) { return &errno; }
#endif

void delayMicroseconds(unsigned int us);

const struct symtab export_syms[] = {
#include "export_syms.h"
     { 0, 0 },
};

#undef S
#undef R
