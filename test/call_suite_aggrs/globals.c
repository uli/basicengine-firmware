/*

 Package: dyncall
 Library: test
 File: test/call_suite_aggrs/globals.c
 Description:
 License:

   Copyright (c) 2022 Tassilo Philipp <tphilipp@potion-studios.com>

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

#include <stdlib.h>
#include "globals.h"
#include <float.h>
#include <string.h>

#define X(CH,T) T *V_##CH; T *K_##CH;
DEF_TYPES
#undef X

/* intentional misalignment of test aggregates (use only positive numbers);
 * crashes/exceptions (e.g. sigbus on some platforms) when using values > 0
 * might reveal missing aggr-by-val copies in the implementation */
#define AGGR_MISALIGN 1 /* @@@AGGR make configurable */

static double rand_d()      { return ( ( (double) rand() )  / ( (double) RAND_MAX ) ); }

/* fill mem with random values, make sure no float aligned memory location
 * results in a NaN, as they always compare to false; so avaid all ones in
 * exporent (for simplicity we just look at first 7 exponent bits and make sure
 * they aren't all set, which would work for all IEEE754 precision formats) */
static void rand_mem__fp_friendly(void* p, size_t s)
{
  int i;
  for(i = 0; i<s; ++i) {
    char* c = (char*)p;
    c[i] = (char)rand(); /* slowish, byte by byte, but whatev */
    if((c[i]&0x7f) == 0x7f)
      c[i] ^= 1;
  }
}

int get_max_aggr_size()
{
  static int s = 0;
  int i;
  if(s == 0) {
    for(i=0; i<G_naggs; ++i)
      if(G_agg_sizes[i] > s)
        s = G_agg_sizes[i];
  }
  return s;
}

void init_test_data()
{
  int i;
  int maxaggrsize = get_max_aggr_size();
#define X(CH,T) V_##CH = (T*) malloc(sizeof(T)*(G_maxargs+1)); K_##CH = (T*) malloc(sizeof(T)*(G_maxargs+1));
DEF_TYPES
#undef X

  for(i=0;i<G_maxargs+1;++i) {
    K_B[i] = (DCbool)            ((int)rand_d() & 1);
    K_c[i] = (char)              (((rand_d()-0.5)*2) * (1<<7));
    K_s[i] = (short)             (((rand_d()-0.5)*2) * (1<<(sizeof(short)*8-1)));
    K_i[i] = (int)               (((rand_d()-0.5)*2) * (1<<(sizeof(int)*8-2)));
    K_j[i] = (long)              (((rand_d()-0.5)*2) * (1L<<(sizeof(long)*8-2)));
    K_l[i] = (long long)         (((rand_d()-0.5)*2) * (1LL<<(sizeof(long long)*8-2)));
    K_C[i] = (unsigned char)     (((rand_d()-0.5)*2) * (1<<7));
    K_S[i] = (unsigned short)    (((rand_d()-0.5)*2) * (1<<(sizeof(short)*8-1)));
    K_I[i] = (unsigned int)      (((rand_d()-0.5)*2) * (1<<(sizeof(int)*8-2)));
    K_J[i] = (unsigned long)     (((rand_d()-0.5)*2) * (1L<<(sizeof(long)*8-2)));
    K_L[i] = (unsigned long long)(((rand_d()-0.5)*2) * (1LL<<(sizeof(long long)*8-2)));
    K_p[i] = (void*)(long)       (((rand_d()-0.5)*2) * (1LL<<(sizeof(void*)*8-1)));
    K_f[i] = (float)             (rand_d() * FLT_MAX);
    K_d[i] = (double)            (((rand_d()-0.5)*2) * DBL_MAX);
    K_a[i] = malloc(maxaggrsize+AGGR_MISALIGN);
    rand_mem__fp_friendly(K_a[i], maxaggrsize+AGGR_MISALIGN);
    K_a[i] = (char*)K_a[i]+AGGR_MISALIGN;
  }
}

void clear_V()
{
  static int aggr_init = 0;
  int maxaggrsize = get_max_aggr_size();

  int i;
  for(i=0;i<G_maxargs+1;++i) {
    if(aggr_init)
      free((char*)V_a[i]-AGGR_MISALIGN);
#define X(CH,T) V_##CH[i] = (T) 0;
DEF_TYPES
#undef X
    V_a[i] = malloc(maxaggrsize+AGGR_MISALIGN);
    memset(V_a[i], 0, maxaggrsize+AGGR_MISALIGN);
    V_a[i] = (char*)V_a[i]+AGGR_MISALIGN;
  }
  aggr_init = 1;
}

void deinit_test_data()
{
  int i;
  for(i=0;i<G_maxargs+1;++i) {
    if(V_a[i]) free((char*)V_a[i]-AGGR_MISALIGN);
    if(K_a[i]) free((char*)K_a[i]-AGGR_MISALIGN);
  }

#define X(CH,T) free(V_##CH); free(K_##CH);
DEF_TYPES
#undef X
}

