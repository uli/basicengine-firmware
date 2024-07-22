/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_args_x64.c
 Description: Callback's Arguments VM - Implementation for x64
 License:

   Copyright (c) 2007-2024 Daniel Adler <dadler@uni-goettingen.de>,
                           Tassilo Philipp <tphilipp@potion-studios.com>

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



#include "dyncall_args.h"
#include "dyncall_callvm_x64.h" /* reuse DCRegCount_x64 and DCRegData_x64_s */
#include "dyncall_aggregate.h"

#include <assert.h>
#include <string.h>


struct DCArgs
{
  /* state */
  int64*          stack_ptr;              /* offset 0 */
  DCRegCount_x64  reg_count;              /* offset 8, size:win 4, size:*nix 8 */
#if defined(DC_WINDOWS)
  int             pad_w;                  /* alignment helper for win/x64 */
#endif
  int             aggr_return_register;   /* offset 16 */
  int             pad;                    /* offset 20 */
  DCaggr**        aggrs;                  /* offset 24 */

  /* reg data */
  DCRegData_x64_s reg_data;               /* offset 32 */
};


static int64* arg_i64(DCArgs* args)
{
  args->reg_count.i += (args->reg_count.i == args->aggr_return_register);

  if (args->reg_count.i < numIntRegs)
    return &args->reg_data.i[args->reg_count.i++];
  else
    return args->stack_ptr++;
}


static double* arg_f64(DCArgs* args)
{
#if defined(DC_WINDOWS)
  args->reg_count.f += (args->reg_count.f == args->aggr_return_register);
#endif
  if (args->reg_count.f < numFloatRegs)
    return &args->reg_data.f[args->reg_count.f++];
  else
    return (double*)args->stack_ptr++;
}



DClonglong  dcbArgLongLong (DCArgs* p) { return *arg_i64(p); }
DCint       dcbArgInt      (DCArgs* p) { return (int)   dcbArgLongLong(p); }
DClong      dcbArgLong     (DCArgs* p) { return (long)  dcbArgLongLong(p); }
DCchar      dcbArgChar     (DCArgs* p) { return (char)  dcbArgLongLong(p); }
DCshort     dcbArgShort    (DCArgs* p) { return (short) dcbArgLongLong(p); }
DCbool      dcbArgBool     (DCArgs* p) { return dcbArgInt(p) != 0; }

DCuint      dcbArgUInt     (DCArgs* p) { return (DCuint)      dcbArgInt(p);      }
DCuchar     dcbArgUChar    (DCArgs* p) { return (DCuchar)     dcbArgChar(p);     }
DCushort    dcbArgUShort   (DCArgs* p) { return (DCushort)    dcbArgShort(p);    }
DCulong     dcbArgULong    (DCArgs* p) { return (DCulong)     dcbArgLong(p);     }
DCulonglong dcbArgULongLong(DCArgs* p) { return (DCulonglong) dcbArgLongLong(p); }


DCpointer   dcbArgPointer  (DCArgs* p) { return (DCpointer)   dcbArgLongLong(p); }

DCdouble    dcbArgDouble   (DCArgs* p) { return *arg_f64(p); }
DCfloat     dcbArgFloat    (DCArgs* p) { return *(float*)arg_f64(p); }

DCpointer   dcbArgAggr     (DCArgs* p, DCpointer target)
{
  int i;
  DCaggr *ag = *(p->aggrs++);

  if(!ag) {
    /* non-trivial aggr: retrieve as ptr, user is supposed to make copy */
    return dcbArgPointer(p);
  }

#if defined(DC_UNIX)
  DCRegCount_x64 n_regs = { p->reg_count.i, p->reg_count.f };

  if(ag->sysv_classes[0] != SYSVC_MEMORY) {
    /* reclassify aggr w/ respect to remaining regs, might have been passed entirely via the stack */
    for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i) {
      DCuchar clz = ag->sysv_classes[i];
      n_regs.i += (clz == SYSVC_INTEGER);
      n_regs.f += (clz == SYSVC_SSE);
      /* @@@AGGR implement when implementing x87 types */
    }
  }

  if(ag->sysv_classes[0] == SYSVC_MEMORY || (n_regs.i > numIntRegs) || (n_regs.f > numFloatRegs))
  {
     memcpy(target, p->stack_ptr, ag->size);
     p->stack_ptr = p->stack_ptr + ((ag->size + (sizeof(DClonglong)-1)) >> 3); /* advance to next full stack slot */
  }
  else
  {
    for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i)
    {
      size_t s = ag->size - i*8;
      s = s<8?s:8;

      switch (ag->sysv_classes[i])
      {
        case SYSVC_INTEGER:
          {
            DClonglong l = dcbArgLongLong(p);
            memcpy((DClonglong*)target + i, &l, s);
          }
          break;

        case SYSVC_SSE:
          if(s == 8)
            *(((DCdouble*)target) + i) = dcbArgDouble(p);
          else if(s == 4)
            *(DCfloat*)(((DCdouble*)target) + i) = dcbArgFloat (p);
          else
            assert(DC_FALSE && "SYSV aggregate floating point slot mismatch (unexpected size of fp field)");
          break;

        /* @@@AGGR implement when implementing x87 types */
        default:
            assert(DC_FALSE && "unsupported SYSV aggregate slot classification"); /* shouldn't be reached, as we check for unupported classes earlier */
      }
    }
  }

#else

  switch (ag->size) {
    case 1: *(DCchar    *)target = dcbArgChar    (p); break;
    case 2: *(DCshort   *)target = dcbArgShort   (p); break;
    case 4: *(DClong    *)target = dcbArgLong    (p); break;
    case 8: *(DClonglong*)target = dcbArgLongLong(p); break;
    default: memcpy(target, dcbArgPointer(p), ag->size); break;
  }
#endif

  return target;
}


void dcbReturnAggr(DCArgs *args, DCValue *result, DCpointer ret)
{
  int i;
  DCaggr *ag = *(args->aggrs++);

  if(args->aggr_return_register >= 0) {
    DCpointer dest = (DCpointer) args->reg_data.i[args->aggr_return_register];
    if(ag)
      memcpy(dest, ret, ag->size);
    else {
      /* non-trivial aggr: all we can do is to provide the ptr to the output space, user has to make copy */
    }
    result->p = dest;
  } else {
#if defined(DC_UNIX)
    /* A 16 byte struct would be sufficient for System V (because at most two of the four registers can be full). */
    /* But then it's much more complicated to copy the result to the correct registers in assembly. */
    typedef struct {
      DClonglong r[2]; /* rax, rdx   */
      DCdouble   x[2]; /* xmm0, xmm1 */
    } DCRetRegs_SysV;

    /* a max of 2 regs are used in this case, out of rax, rdx, xmm0 and xmm1 */
    /* space for 4 qwords is pointed to by (DCRetRegs_SysV*)result */
    DClonglong *intRegs = ((DCRetRegs_SysV*)result)->r;
    DCdouble   *sseRegs = ((DCRetRegs_SysV*)result)->x;
    for(i=0; ag->sysv_classes[i] && i<2/*guaranteed*/; ++i) {
      switch (ag->sysv_classes[i]) {
        case SYSVC_INTEGER: *(intRegs++) = ((DClonglong*)ret)[i]; break;
        case SYSVC_SSE:     *(sseRegs++) = ((DCdouble  *)ret)[i]; break;
        /* @@@AGGR implement when implementing x87 types, might lead to more than 2 regs (e.g. _m512) */
        default: assert(DC_FALSE && "Should never be reached because we check for unupported classes earlier");
      }
    }
#else
    /* copy aggregate (guaranteed to be <= 8b by call conv, as no hidden ptr) into result */
    assert(ag->size <= 8 && "aggregate info mismatch for return type");
    memcpy(result, ret, ag->size);
#endif
  }
}

