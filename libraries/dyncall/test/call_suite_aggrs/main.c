/*

 Package: dyncall
 Library: test
 File: test/call_suite_aggrs/main.c
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

#include "dyncall.h"
#include "globals.h"
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "../common/platformInit.h"
#include "../common/platformInit.c" /* Impl. for functions only used in this translation unit */


static void* G_callvm;


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

static int invoke(const char *signature, void* t)
{
  DCCallVM   * p = (DCCallVM*) G_callvm;
  const char * sig = signature;
  const char * rtype;
  DCaggr *     rtype_a = NULL;
  int          rtype_size = 0;
  funptr       rtype_a_cmp = NULL;
  char         atype;
  int          pos = 0;
  int          s = 0;

  clear_V();

  dcReset(p);

  /* locate return type in sig; if no ')' separator, test failed */
  rtype = strchr(sig, ')');
  if(!rtype) {
    printf("cannot locate rtype in sig '%s' ;", signature);
    return 0;
  }

  ++rtype;
  if(*rtype == '{' || *rtype == '<') {
    int i = find_agg_idx(NULL, rtype);
    if(i == -1) {
      printf("unknown rtype aggr sig at '%s' ;", rtype);
      return 0;
    }

    rtype_size = G_agg_sizes[i];
    rtype_a_cmp = G_agg_cmpfuncs[i];
    rtype_a = ((DCaggr*(*)())G_agg_touchAfuncs[i])();
    dcBeginCallAggr(p, rtype_a);
  }


  while ( (atype = *sig) != ')') {
    switch(atype) {
      case 'B': dcArgBool    (p,K_B[pos]); break;
      case 'c': dcArgChar    (p,K_c[pos]); break;
      case 's': dcArgShort   (p,K_s[pos]); break;
      case 'i': dcArgInt     (p,K_i[pos]); break;
      case 'j': dcArgLong    (p,K_j[pos]); break;
      case 'l': dcArgLongLong(p,K_l[pos]); break;
      case 'C': dcArgChar    (p,K_C[pos]); break;
      case 'S': dcArgShort   (p,K_S[pos]); break;
      case 'I': dcArgInt     (p,K_I[pos]); break;
      case 'J': dcArgLong    (p,K_J[pos]); break;
      case 'L': dcArgLongLong(p,K_L[pos]); break;
      case 'p': dcArgPointer (p,K_p[pos]); break;
      case 'f': dcArgFloat   (p,K_f[pos]); break;
      case 'd': dcArgDouble  (p,K_d[pos]); break;
      case '<': /* union */
      case '{': /* struct */
      {
        /* find aggregate sig */
        int len;
        DCaggr *ag;
        int i = find_agg_idx(&len, sig);
        if(i == -1) {
          printf("unknown aggr sig at '%s' ;", sig);
          return 0;
        }
        ag = ((DCaggr*(*)())G_agg_touchAfuncs[i])();
        dcArgAggr(p, ag, K_a[pos]);
        sig += len-1; /* advance to next arg char */
        break;
      }
      default: printf("unknown atype '%c' (1) ;", atype); return 0;
    }
    ++pos;
    ++sig;
  }

  switch(*rtype)
  {
    case 'v':                          dcCallVoid(p,t); s=1;             break; /*TODO:check that no return-arg was touched.*/
    case 'B': s = (                    dcCallBool    (p,t) == K_B[pos]); break;
    case 'c': s = (                    dcCallChar    (p,t) == K_c[pos]); break;
    case 's': s = (                    dcCallShort   (p,t) == K_s[pos]); break;
    case 'i': s = (                    dcCallInt     (p,t) == K_i[pos]); break;
    case 'j': s = (                    dcCallLong    (p,t) == K_j[pos]); break;
    case 'l': s = (                    dcCallLongLong(p,t) == K_l[pos]); break;
    case 'C': s = ((unsigned char)     dcCallChar    (p,t) == K_C[pos]); break;
    case 'S': s = ((unsigned short)    dcCallShort   (p,t) == K_S[pos]); break;
    case 'I': s = ((unsigned int)      dcCallInt     (p,t) == K_I[pos]); break;
    case 'J': s = ((unsigned long)     dcCallLong    (p,t) == K_J[pos]); break;
    case 'L': s = ((unsigned long long)dcCallLongLong(p,t) == K_L[pos]); break;
    case 'p': s = (                    dcCallPointer (p,t) == K_p[pos]); break;
    case 'f': s = (                    dcCallFloat   (p,t) == K_f[pos]); break;
    case 'd': s = (                    dcCallDouble  (p,t) == K_d[pos]); break;
    case '<': /* union */
    case '{': /* struct */
    {
      /* bound check memory adjacent to returned aggregate, to check for overflows by dcCallAggr */
      long long* adj_ll = (get_max_aggr_size() - rtype_size) > sizeof(long long) ? (long long*)((char*)V_a[pos] + rtype_size) : NULL;
      if(adj_ll)
        *adj_ll = 0x0123456789abcdefLL;

      s = ((int(*)(const void*,const void*))rtype_a_cmp)(dcCallAggr(p, t, rtype_a, V_a[pos]), K_a[pos]);

      if(adj_ll && *adj_ll != 0x0123456789abcdefLL) {
        printf("writing rval overflowed into adjacent memory;");
        return 0;
      }
      break;
    }
    default: printf("unknown rtype '%s'", rtype); return 0;
  }

  if (!s) { printf("rval wrong;"); return 0; }

  /* test V_* array against values passed to func: */
  sig = signature;
  pos = 0;
  while ( (atype = *sig) != ')') {
    switch(atype) {
      case 'B': s = ( V_B[pos] == K_B[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_B[pos], K_B[pos]); break;
      case 'c': s = ( V_c[pos] == K_c[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_c[pos], K_c[pos]); break;
      case 's': s = ( V_s[pos] == K_s[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_s[pos], K_s[pos]); break;
      case 'i': s = ( V_i[pos] == K_i[pos] ); if (!s) printf("'%c':%d: %d != %d ; ",     atype, pos, V_i[pos], K_i[pos]); break;
      case 'j': s = ( V_j[pos] == K_j[pos] ); if (!s) printf("'%c':%d: %ld != %ld ; ",   atype, pos, V_j[pos], K_j[pos]); break;
      case 'l': s = ( V_l[pos] == K_l[pos] ); if (!s) printf("'%c':%d: %lld != %lld ; ", atype, pos, V_l[pos], K_l[pos]); break;
      case 'C': s = ( V_C[pos] == K_C[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_C[pos], K_C[pos]); break;
      case 'S': s = ( V_S[pos] == K_S[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_S[pos], K_S[pos]); break;
      case 'I': s = ( V_I[pos] == K_I[pos] ); if (!s) printf("'%c':%d: %u != %u ; ",     atype, pos, V_I[pos], K_I[pos]); break;
      case 'J': s = ( V_J[pos] == K_J[pos] ); if (!s) printf("'%c':%d: %lu != %lu ; ",   atype, pos, V_J[pos], K_J[pos]); break;
      case 'L': s = ( V_L[pos] == K_L[pos] ); if (!s) printf("'%c':%d: %llu != %llu ; ", atype, pos, V_L[pos], K_L[pos]); break;
      case 'p': s = ( V_p[pos] == K_p[pos] ); if (!s) printf("'%c':%d: %p != %p ; ",     atype, pos, V_p[pos], K_p[pos]); break;
      case 'f': s = ( V_f[pos] == K_f[pos] ); if (!s) printf("'%c':%d: %f != %f ; ",     atype, pos, V_f[pos], K_f[pos]); break;
      case 'd': s = ( V_d[pos] == K_d[pos] ); if (!s) printf("'%c':%d: %f != %f ; ",     atype, pos, V_d[pos], K_d[pos]); break;
      case '<': /* union */
      case '{': /* struct */
      {
        /* no check: guaranteed to exist, or invoke func would've exited when passing args, above */
        int len;
        int i = find_agg_idx(&len, sig);
        s = ((int(*)(const void*,const void*))G_agg_cmpfuncs[i])(V_a[pos], K_a[pos]);
        if (!s) printf("'%c':%d:  *%p != *%p ; ", atype, pos, V_a[pos], K_a[pos]);
        sig += len-1; /* advance to next arg char */
        break;
      }
      default: printf("unknown atype '%c' ; ", atype); return 0;
    }
    if (!s) {
      printf("arg mismatch at %d ; ", pos);
      return 0;
    }
    ++sig;
    ++pos;
  }
  return 1;
}

static int run_test(int i)
{
  char const * sig;
  void * target;
  int success;
  sig = G_sigtab[i];
  target = (void*) G_funtab[i];
  printf("%d:%s:",i,sig);
  success = invoke(sig,target);
  printf("%d\n",success);
  return success;
}

static int run_all()
{
  int i;
  int failure = 0;
  for(i=0;i<G_ncases;++i)
    failure |= !( run_test(i) );

  return !failure;
}


jmp_buf jbuf;
void sig_handler(int sig)
{
  longjmp(jbuf, 1);
}


int main(int argc, char* argv[])
{
  int r = 0, i;

  signal(SIGABRT, sig_handler);
  signal(SIGILL,  sig_handler);
  signal(SIGSEGV, sig_handler);
#if !defined(DC_WINDOWS)
  signal(SIGBUS,  sig_handler);
#endif

  dcTest_initPlatform();

  init_test_data();
  G_callvm = (DCCallVM*) dcNewCallVM(32768);

  dcReset(G_callvm);
  if(setjmp(jbuf) == 0)
    r = run_all();
  else
    printf("\n"); /* new line as current might be filled with garbage */

  /* free all DCaggrs created on the fly (backwards b/c they are interdependency-ordered */
  for(i=G_naggs-1; i>=0; --i)
    dcFreeAggr(((DCaggr*(*)())G_agg_touchAfuncs[i])());

  dcFree(G_callvm);
  deinit_test_data();

  printf("result: call_suite_aggrs: %d\n", r);

  dcTest_deInitPlatform();

  return !r;
}

