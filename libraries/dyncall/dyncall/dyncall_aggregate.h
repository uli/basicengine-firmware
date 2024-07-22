/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_aggregate.h
 Description: C interface to compute struct size
 License:

   Copyright (c) 2021-2022 Tassilo Philipp <tphilipp@potion-studios.com>

   Permission to use, copy, modify, and distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/


#ifndef DYNCALL_STRUCT_H
#define DYNCALL_STRUCT_H

#include "dyncall.h"

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(DC_UNIX) && defined(DC__Arch_AMD64)

/* x64 param classification - only used for aggregates, so comments might be aggregate specific */
#  define DC_SYSV_MAX_NUM_CLASSES 8  /* max num of aggregate qwords to be classified; constant defined by call conv */
#  define SYSVC_NONE             0  /* end of classification !code relies on this being 0! */
#  define SYSVC_INTEGER      (1<<0) /* (signed and unsigned) _Bool/bool, char, short, int, long, long long and pointers (also __int128, but treated as two longs) */
#  define SYSVC_SSE          (1<<1) /* float, double and __m64, as well as least significant half of __float128 and __m128 (also complex float/double, but treated as two float/double) */
#  define SYSVC_SSEUP        (1<<2) /* @@@AGGR currently unsupported/unused: most significant half of __float128 and __m128, most significant parts of __m256 and __m512 */
#  define SYSVC_X87          (1<<3) /* @@@AGGR currently unsupported/unused: 64bit mantissa of type long double (80bit x87 extended precision format) */
#  define SYSVC_X87UP        (1<<4) /* @@@AGGR currently unsupported/unused: 16bit exponent plus 6 bytes of padding of type long double (80bit x87 extended precision format) */
#  define SYSVC_COMPLEX_X87  (1<<5) /* @@@AGGR currently unsupported/unused: complex long double */
#  define SYSVC_MEMORY       (1<<6) /* for everything not fitting or allowed in regs given call conv (if class[0] == SYSVC_MEMORY, shortcut to pass entire aggregate via memory) */

#endif


typedef struct DCfield_ {
	DCsize offset, size, alignment, array_len;
	DCsigchar type;
	const DCaggr* sub_aggr;
} DCfield;

struct DCaggr_ {
	DCsize size, n_fields, alignment;
#if defined(DC_UNIX) && defined(DC__Arch_AMD64)
	DCuchar sysv_classes[DC_SYSV_MAX_NUM_CLASSES]; /* !code relies on this to be 64 bits! */
#endif
	DCfield fields[];
};



#ifdef __cplusplus
}
#endif

#endif /* DYNCALL_H */

