/*

 Package: dyncall
 Library: test
 File: test/callback_suite_aggrs/main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "dyncall_callback.h"
#include "globals.h"
#include "../common/platformInit.h"
#include "../common/platformInit.c" /* Impl. for functions only used in this translation unit */




static void print_usage(const char* appName)
{
  printf("usage:\n\
%s [ from [to] ]\n\
where\n\
  from, to: test range (0-based, runs single test if \"to\" is omitted)\n\
options\n\
  -h        help on usage\n\
\n\
", appName);
}


static int find_agg_idx(int* len, const char* sig)
{
  int i;
  for(i=0; i<G_naggs; ++i) {
    const char* agg_sig = G_agg_sigs[i];
    int l = strlen(agg_sig);
    if(len)
      *len = l;
    if(strncmp(agg_sig, sig, l) == 0)
      return i;
  }
  return -1;
}


static int cmp(const char* signature)
{
  char atype;
  const char* sig = signature;
  int pos = 0;
  int s = 0;
  int do_bndck = 1;
  while ( (atype = *sig) != '\0') {
    switch(atype) {
      case '_':  sig += 2; /* skip cconv prefix */                                                                           continue;
      case ')':  ++sig; /* skip ret type separator */ do_bndck = 0; /* no bounds check on retval, as we don't do its copy */ continue;
      case 'v':  s = (sig > signature) && sig[-1] == ')'; /* assure this was the return type */                            break; /*TODO:check that no return-arg was touched.*/
      case 'B':  s = ( V_B[pos] == K_B[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_B[pos], K_B[pos]); break;
      case 'c':  s = ( V_c[pos] == K_c[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_c[pos], K_c[pos]); break;
      case 's':  s = ( V_s[pos] == K_s[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_s[pos], K_s[pos]); break;
      case 'i':  s = ( V_i[pos] == K_i[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_i[pos], K_i[pos]); break;
      case 'j':  s = ( V_j[pos] == K_j[pos] ); if (!s) printf("'%c':%d: %ld != %ld ; ",   atype, pos, V_j[pos], K_j[pos]); break;
      case 'l':  s = ( V_l[pos] == K_l[pos] ); if (!s) printf("'%c':%d: %lld != %lld ; ", atype, pos, V_l[pos], K_l[pos]); break;
      case 'C':  s = ( V_C[pos] == K_C[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_C[pos], K_C[pos]); break;
      case 'S':  s = ( V_S[pos] == K_S[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_S[pos], K_S[pos]); break;
      case 'I':  s = ( V_I[pos] == K_I[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_I[pos], K_I[pos]); break;
      case 'J':  s = ( V_J[pos] == K_J[pos] ); if (!s) printf("'%c':%d: %lu != %lu ; ",   atype, pos, V_J[pos], K_J[pos]); break;
      case 'L':  s = ( V_L[pos] == K_L[pos] ); if (!s) printf("'%c':%d: %llu != %llu ; ", atype, pos, V_L[pos], K_L[pos]); break;
      case 'p':  s = ( V_p[pos] == K_p[pos] ); if (!s) printf("'%c':%d: %p != %p ; ",     atype, pos, V_p[pos], K_p[pos]); break;
      case 'f':  s = ( V_f[pos] == K_f[pos] ); if (!s) printf("'%c':%d: %f != %f ; ",     atype, pos, V_f[pos], K_f[pos]); break;
      case 'd':  s = ( V_d[pos] == K_d[pos] ); if (!s) printf("'%c':%d: %f != %f ; ",     atype, pos, V_d[pos], K_d[pos]); break;
      case '<': /* union */
      case '{': /* struct */
      {
        /* no check: guaranteed to exist, or invoke func would've exited when passing args, above */
        int len, bndck_i;
        int i = find_agg_idx(&len, sig);
        s = ((int(*)(const void*,const void*))G_agg_cmpfuncs[i])(V_a[pos], K_a[pos]);
        if (!s) printf("'%c':%d:  *%p != *%p ; ", atype, pos, V_a[pos], K_a[pos]);
        sig += len-1; /* advance to next arg char; -1 to compensate for ++sig, below */

        /* bounds check */
        for(bndck_i = 0; do_bndck && bndck_i < AGGR_BND_CHECK_PAD; ++bndck_i) {
          if(((unsigned char*)V_a[pos] + G_agg_sizes[i])[bndck_i] != 0xab) {
            printf("'%c':%d:  buffer overflow retrieving aggr arg, target buffer of size %d too small; ", atype, pos, G_agg_sizes[i]);
            s = 0;
          }
        }

        break;
      }
      default: printf("unknown atype '%c' ; ", atype); return 0;
    }
    if(!s) {
      printf("arg mismatch at %d ; ", pos);
      return 0;
    }
    ++sig;
    ++pos;
  }
  return 1;
}


/* handler just copies all received args as well as return value into V_* */
static char handler(DCCallback* that, DCArgs* input, DCValue* output, void* userdata)
{
  const char* signature = (const char*) userdata;
  int pos = 0;
  char ch;

  for(;;) {
    ch = *signature++;
    if (!ch || ch == DC_SIGCHAR_ENDARG) break;
    switch(ch) {
      case DC_SIGCHAR_BOOL:      V_B[pos] = dcbArgBool     (input);           break;
      case DC_SIGCHAR_CHAR:      V_c[pos] = dcbArgChar     (input);           break;
      case DC_SIGCHAR_UCHAR:     V_C[pos] = dcbArgUChar    (input);           break;
      case DC_SIGCHAR_SHORT:     V_s[pos] = dcbArgShort    (input);           break;
      case DC_SIGCHAR_USHORT:    V_S[pos] = dcbArgUShort   (input);           break;
      case DC_SIGCHAR_INT:       V_i[pos] = dcbArgInt      (input);           break;
      case DC_SIGCHAR_UINT:      V_I[pos] = dcbArgUInt     (input);           break;
      case DC_SIGCHAR_LONG:      V_j[pos] = dcbArgLong     (input);           break;
      case DC_SIGCHAR_ULONG:     V_J[pos] = dcbArgULong    (input);           break;
      case DC_SIGCHAR_LONGLONG:  V_l[pos] = dcbArgLongLong (input);           break;
      case DC_SIGCHAR_ULONGLONG: V_L[pos] = dcbArgULongLong(input);           break;
      case DC_SIGCHAR_FLOAT:     V_f[pos] = dcbArgFloat    (input);           break;
      case DC_SIGCHAR_DOUBLE:    V_d[pos] = dcbArgDouble   (input);           break;
      case DC_SIGCHAR_STRING:
      case DC_SIGCHAR_POINTER:   V_p[pos] = dcbArgPointer  (input);           break;
      case DC_SIGCHAR_AGGREGATE:
        /*  bounds check init */
        memset(V_a[pos], 0xab, get_max_aggr_size() + AGGR_BND_CHECK_PAD);
        dcbArgAggr(input, V_a[pos]);
        break;

      case DC_SIGCHAR_CC_PREFIX: ++signature; /* skip cconv prefix */ continue;
      default: assert(0);
    }
    ++pos;
  }

  if(ch == DC_SIGCHAR_ENDARG)
    ch = *signature;

  /* write retval */
  switch(ch) {
    case DC_SIGCHAR_VOID:      /* nothing to set */                    break;
    case DC_SIGCHAR_BOOL:      output->B = K_B[pos];                   break;
    case DC_SIGCHAR_CHAR:      output->c = K_c[pos];                   break;
    case DC_SIGCHAR_UCHAR:     output->C = K_C[pos];                   break;
    case DC_SIGCHAR_SHORT:     output->s = K_s[pos];                   break;
    case DC_SIGCHAR_USHORT:    output->S = K_S[pos];                   break;
    case DC_SIGCHAR_INT:       output->i = K_i[pos];                   break;
    case DC_SIGCHAR_UINT:      output->I = K_I[pos];                   break;
    case DC_SIGCHAR_LONG:      output->j = K_j[pos];                   break;
    case DC_SIGCHAR_ULONG:     output->J = K_J[pos];                   break;
    case DC_SIGCHAR_LONGLONG:  output->l = K_l[pos];                   break;
    case DC_SIGCHAR_ULONGLONG: output->L = K_L[pos];                   break;
    case DC_SIGCHAR_FLOAT:     output->f = K_f[pos];                   break;
    case DC_SIGCHAR_DOUBLE:    output->d = K_d[pos];                   break;
    case DC_SIGCHAR_STRING:
    case DC_SIGCHAR_POINTER:   output->p = K_p[pos];                   break;
    case DC_SIGCHAR_AGGREGATE: dcbReturnAggr(input, output, K_a[pos]); break;
    default: assert(0);
  }

  /* return type info for dyncallback */
  return ch;
}




static int run_test(int id)
{
  const char* signature;
  DCCallback* pcb;
  int result;
  int len_sig;
  char   *dc_sig;
  DCaggr **dc_aggrs;
  int n_aggrs = 0;

  /* index range: [0,nsigs[ */
  signature = G_sigtab[id];
  printf("%d:%s", id, signature);

  len_sig = strlen(signature);

  /* prep dcbNewCallback() args: signature in dyncall format and array of
   * DCaggr* descriptions; use len of verbose test-case sig, as always bigger
   * than canonical/dyncall signature (where aggrs represented as single char)
   * number of aggregates in sig and  */
  dc_sig   = malloc(sizeof(char)   *len_sig);
  dc_aggrs = malloc(sizeof(DCaggr*)*len_sig);

  len_sig = 0; /* len of canonical/dyncall sig */
  while(*signature != '\0') {
    switch(*signature) {
      case '_': signature += 2; /* skip cconv prefix */ break;
      case '<': /* union */
      case '{': /* struct */
      {
        /* find aggregate sig */
        int len;
        int i = find_agg_idx(&len, signature);
        if(i == -1) {
          printf("unknown aggr sig at '%s' ;", signature);
          return 0;
        }
        dc_sig[len_sig++] = DC_SIGCHAR_AGGREGATE;
        dc_aggrs[n_aggrs++] = ((DCaggr*(*)())G_agg_touchAfuncs[i])();
        signature += len; /* advance to next arg char */
        break;
      }
      default:
        dc_sig[len_sig++] = *signature;
        ++signature;
    }
  }
  dc_sig[len_sig] = '\0';


  pcb = dcbNewCallback2(dc_sig, handler, (void*)dc_sig, dc_aggrs);
  assert(pcb != NULL);

  clear_V();

  /* invoke call */
  G_funtab[id]((void*)pcb);

  result = cmp(G_sigtab[id]);

  free(dc_sig);
  free(dc_aggrs);

  printf(":%d\n", result);

  dcbFreeCallback(pcb);

  return result;
}


static int run_all(int from, int to)
{
  int i;
  int failure = 0;
  for(i=from; i<=to ;++i)
      failure |= !( run_test(i) );

  return !failure;
}


jmp_buf jbuf;
void sig_handler(int sig)
{
  longjmp(jbuf, 1);
}


#define Error(X, Y, N) { fprintf(stderr, X, Y); print_usage(N); exit(1); }

int main(int argc, char* argv[])
{
  int from = 0, to = G_ncases-1;
  int i, pos = 0, r = 0;

  signal(SIGABRT, sig_handler);
  signal(SIGILL,  sig_handler);
  signal(SIGSEGV, sig_handler);
#if !defined(DC_WINDOWS)
  signal(SIGBUS,  sig_handler);
#endif

  dcTest_initPlatform();


  /* parse args */
  for(i=1; i<argc; ++i)
  {
    if(argv[i][0] == '-')
    {
      switch(argv[i][1]) {
        case 'h':
        case '?':
          print_usage(argv[0]);
          return 0;
        default:
          Error("invalid option: %s\n\n", argv[i], argv[0]);
      }
    }
    switch(pos++) {
      case 0: from = to = atoi(argv[i]); break;
      case 1:        to = atoi(argv[i]); break;
      default: Error("too many arguments (%d given, 2 allowed)\n\n", pos, argv[0]);
    }
  }
  if(from < 0 || to >= G_ncases || from > to)
      Error("invalid arguments (provided from or to not in order or outside of range [0,%d])\n\n", G_ncases-1, argv[0]);


  init_test_data();
  if(setjmp(jbuf) == 0)
    r = run_all(from, to);
  else
    printf("\n"); /* new line as current might be filled with garbage */

  /* free all DCaggrs created on the fly (backwards b/c they are interdependency-ordered */
  for(i=G_naggs-1; i>=0; --i)
    dcFreeAggr(((DCaggr*(*)())G_agg_touchAfuncs[i])());

  deinit_test_data();

  printf("result: callback_suite_aggrs: %d\n", r);

  dcTest_deInitPlatform();

  return !r;
}

