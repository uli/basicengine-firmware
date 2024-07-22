/*

 Package: dyncall
 Library: test
 File: test/callback_suite_aggrs/cases.c
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

#include "globals.h"

#define write_V_v(X, v)    (         v);
#define write_V_B(X, v)    (V_B[X] = v);
#define write_V_c(X, v)    (V_c[X] = v);
#define write_V_s(X, v)    (V_s[X] = v);
#define write_V_i(X, v)    (V_i[X] = v);
#define write_V_j(X, v)    (V_j[X] = v);
#define write_V_l(X, v)    (V_l[X] = v);
#define write_V_C(X, v)    (V_C[X] = v);
#define write_V_S(X, v)    (V_S[X] = v);
#define write_V_I(X, v)    (V_I[X] = v);
#define write_V_J(X, v)    (V_J[X] = v);
#define write_V_L(X, v)    (V_L[X] = v);
#define write_V_p(X, v)    (V_p[X] = v);
#define write_V_f(X, v)    (V_f[X] = v);
#define write_V_d(X, v)    (V_d[X] = v);
#define write_V_a(X, v, t) (*(t*)V_a[X] = v);

#define v void
#define X(CH,T) typedef T CH;
DEF_TYPES
#undef X


#define AF(c,t,i,n)   dcAggrField(a,c,offsetof(t,i),n);
#define AFa(t,i,n,f)  dcAggrField(a,DC_SIGCHAR_AGGREGATE,offsetof(t,i),n,f_touch##f());

#include "dyncall.h"
#include <string.h>


/* Plan9 pcc and MSVC (when using C) do not allow empty structs */
#if defined(DC__C_MSVC) || defined(DC__OS_Plan9)
#  include "nonemptyaggrs.h"
#else
#  include "cases.h"
#endif

int G_ncases = sizeof(G_sigtab)/sizeof(G_sigtab[0]);
int G_naggs  = sizeof(G_agg_sigs)/sizeof(G_agg_sigs[0]);

