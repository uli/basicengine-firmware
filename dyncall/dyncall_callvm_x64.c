/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_callvm_x64.c
 Description: 
 License:

   Copyright (c) 2007-2022 Daniel Adler <dadler@uni-goettingen.de>, 
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




/* MS Windows x64 calling convention, AMD64 SystemV ABI. */


#include "dyncall_callvm_x64.h"
#include "dyncall_alloc.h"
#include "dyncall_aggregate.h"

#include <stdint.h>
#include <string.h>
#include <assert.h>


/* 
** x64 SystemV calling convention 
**
** - hybrid return-type call (bool ... pointer)
**
*/

#if defined(DC_UNIX)
extern void dcCall_x64_sysv(DCsize stacksize, DCpointer stackdata, DCpointer regdata_i, DCpointer regdata_f, DCpointer target);
extern void dcCall_x64_sysv_aggr(DCsize stacksize, DCpointer stackdata, DCpointer regdata_i, DCpointer regdata_f, DCpointer target, DCpointer ret_regs);
#else
extern void dcCall_x64_win64(DCsize stacksize, DCpointer stackdata, DCpointer regdata, DCpointer target);
extern void dcCall_x64_win64_aggr(DCsize stacksize, DCpointer stackdata, DCpointer regdata, DCpointer target, DCpointer aggr_mem);
#endif
extern void dcCall_x64_syscall_sysv(DCpointer argdata, DCpointer target);




static void dc_callvm_free_x64(DCCallVM* in_self)
{
  dcFreeMem(in_self);
}


static void dc_callvm_reset_x64(DCCallVM* in_self)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;
  dcVecReset(&self->mVecHead);
  self->mRegCount.i = self->mRegCount.f = 0;
  self->mAggrReturnReg = -1;
#if defined(DC_WINDOWS)
  self->mpAggrVecCopies = ((DCchar*)dcVecData(&self->mVecHead)) + self->mVecHead.mTotal;
#endif
}




static void dc_callvm_argLongLong_x64(DCCallVM* in_self, DClonglong x)
{
  /* A long long always has 64 bits on the supported x64 platforms (lp64 on unix and llp64 on windows). */
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

  self->mRegCount.i += (self->mRegCount.i == self->mAggrReturnReg);

  if(self->mRegCount.i < numIntRegs)
    self->mRegData.i[self->mRegCount.i++] = x;
  else
    dcVecAppend(&self->mVecHead, &x, sizeof(DClonglong));
}


static void dc_callvm_argBool_x64(DCCallVM* in_self, DCbool x)
{
  dc_callvm_argLongLong_x64(in_self, (DClonglong)x);
}


static void dc_callvm_argChar_x64(DCCallVM* in_self, DCchar x)
{
  dc_callvm_argLongLong_x64(in_self, x);
}


static void dc_callvm_argShort_x64(DCCallVM* in_self, DCshort x)
{
  dc_callvm_argLongLong_x64(in_self, x);
}


static void dc_callvm_argInt_x64(DCCallVM* in_self, DCint x)
{
  dc_callvm_argLongLong_x64(in_self, x);
}


static void dc_callvm_argLong_x64(DCCallVM* in_self, DClong x)
{
  dc_callvm_argLongLong_x64(in_self, x);
}


static void dc_callvm_argDouble_x64(DCCallVM* in_self, DCdouble x)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

#if defined(DC_WINDOWS) 
  self->mRegCount.f += (self->mRegCount.f == self->mAggrReturnReg);
#endif

  if(self->mRegCount.f < numFloatRegs)
    self->mRegData.f[self->mRegCount.f++] = x;
  else
    dcVecAppend(&self->mVecHead, &x, sizeof(DCdouble));
}


static void dc_callvm_argFloat_x64(DCCallVM* in_self, DCfloat x)
{
  /* Although not promoted to doubles, floats are stored with 64bits in this API.*/
  union {
    DCdouble d;
    DCfloat  f;
  } f;
  f.f = x;

  dc_callvm_argDouble_x64(in_self, f.d);
}


static void dc_callvm_argPointer_x64(DCCallVM* in_self, DCpointer x)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

  self->mRegCount.i += (self->mRegCount.i == self->mAggrReturnReg);

  if(self->mRegCount.i < numIntRegs)
    *(DCpointer*)&self->mRegData.i[self->mRegCount.i++] = x;
  else
    dcVecAppend(&self->mVecHead, &x, sizeof(DCpointer));
}


static void dc_callvm_argAggr_x64(DCCallVM* in_self, const DCaggr* ag, const void* x)
{
  int i;
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

  if (!ag) {
    /* non-trivial aggrs (C++) are passed via pointer (win and sysv callconv),
     * copy has to be provided by user, as dyncall cannot do such copies*/
    dc_callvm_argPointer_x64(in_self, (DCpointer)x);
    return;
  }

#if defined(DC_UNIX)
  DCRegCount_x64 n_regs = { self->mRegCount.i, self->mRegCount.f };

  if(ag->sysv_classes[0] != SYSVC_MEMORY) {
    /* reclassify aggr w/ respect to remaining regs, might need to pass it all via the stack */
    for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i) {
      DCuchar clz = ag->sysv_classes[i];
      n_regs.i += (clz == SYSVC_INTEGER);
      n_regs.f += (clz == SYSVC_SSE);
      /* @@@AGGR implement when implementing x87 types */
    }
  }

  if(ag->sysv_classes[0] == SYSVC_MEMORY || (n_regs.i > numIntRegs) || (n_regs.f > numFloatRegs))
  {
     dcVecAppend(&self->mVecHead, x, ag->size);
     dcVecSkip(&self->mVecHead, (ag->size + (sizeof(DClonglong)-1) & -sizeof(DClonglong)) - ag->size); /* realign to qword */
     return;
  }

  for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i)
  {
    switch (ag->sysv_classes[i]) {
      case SYSVC_INTEGER: dc_callvm_argLongLong_x64(in_self, ((DClonglong*)x)[i]); break;
      case SYSVC_SSE:     dc_callvm_argDouble_x64  (in_self, ((DCdouble  *)x)[i]); break;
      /* @@@AGGR implement when implementing x87 types */
    }
  }

#else

  switch (ag->size) {
    case 1:  dc_callvm_argChar_x64    (in_self, *(DCchar    *)x); break;
    case 2:  dc_callvm_argShort_x64   (in_self, *(DCshort   *)x); break;
    case 4:  dc_callvm_argLong_x64    (in_self, *(DClong    *)x); break;
    case 8:  dc_callvm_argLongLong_x64(in_self, *(DClonglong*)x); break;
    default:
      /* pass the aggr indirectly via hidden pointer; requires caller-made copy
       * to mimic pass-by-value semantics (or a call that modifies the param
       * would corrupt the source aggr)
       * place those copies at the end of the param vector (aligned to 16b for
       * this calling convention); it's a bit of a hack, but should be safe: in
       * any case the vector has to be big enough to hold all params */
      self->mpAggrVecCopies = (void*)((intptr_t)((DCchar*)self->mpAggrVecCopies - ag->size) & -16);
      x = memcpy(self->mpAggrVecCopies, x, ag->size);
      dc_callvm_argPointer_x64(in_self, (DCpointer)x);
      break;
  }
#endif
}


/* Call. */
static void dc_callvm_call_x64(DCCallVM* in_self, DCpointer target)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;
#if defined(DC_UNIX)
  dcCall_x64_sysv(
#else
  dcCall_x64_win64(
#endif
    dcVecSize(&self->mVecHead),  /* Size of stack data.                           */
    dcVecData(&self->mVecHead),  /* Pointer to stack arguments.                   */
    self->mRegData.i,            /* Pointer to register arguments (ints on SysV). */
#if defined(DC_UNIX)
    self->mRegData.f,            /* Pointer to floating point register arguments. */
#endif
    target
  );
}


static void dc_callvm_begin_aggr_x64(DCCallVM* in_self, const DCaggr *ag)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

  assert(self->mRegCount.i == 0 && self->mRegCount.f == 0 && "dc_callvm_begin_aggr_x64 should be called before any function arguments are declared");
#if defined(DC_UNIX)
  if (!ag || (ag->sysv_classes[0] == SYSVC_MEMORY)) {
#else
  if (!ag || ag->size > 8 || /*not a power of 2?*/(ag->size & (ag->size - 1))) {
#endif 
    /* pass pointer to aggregate as hidden first argument */
    self->mAggrReturnReg = 0;
  }
}


#if defined(DC_WINDOWS)
static void dc_callvm_begin_aggr_x64_win64_this(DCCallVM* in_self, const DCaggr *ag)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

  assert(self->mRegCount.i == 0 && self->mRegCount.f == 0 && "dc_callvm_begin_aggr_x64_win64_this should be called before any function arguments are declared");

  /* thiscall: this-ptr comes first, then pointer to aggregate as hidden (second) argument */
  self->mAggrReturnReg = 1;
}
#endif


static void dc_callvm_call_x64_aggr(DCCallVM* in_self, DCpointer target, const DCaggr *ag, DCpointer ret)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;

#if defined(DC_UNIX)

  if (self->mAggrReturnReg != -1) {
    /* call regular dcCall_x64_sysv here, w/ pointer to the aggr in first arg */
    self->mRegData.i[self->mAggrReturnReg] = (int64)ret;

    dcCall_x64_sysv(
      dcVecSize(&self->mVecHead),  /* rdi: Size of stack data.                           */
      dcVecData(&self->mVecHead),  /* rsi: Pointer to stack arguments.                   */
      self->mRegData.i,            /* rdx: Pointer to register arguments (ints on SysV). */
      self->mRegData.f,            /* rcx: Pointer to floating point register arguments. */
      target                       /* r8 */
    );
  } else {
    int i;
    DCchar ret_regs[32];           /* 4 qwords: 2 for ints, 2 for floats */
    DCchar *ret_regs_i = ret_regs+0;
    DCchar *ret_regs_f = ret_regs+16;
    DCsize st_size = ag->size;
    DCchar* dst = (char*)ret;
    dcCall_x64_sysv_aggr(
      dcVecSize(&self->mVecHead),  /* rdi: Size of stack data.                           */
      dcVecData(&self->mVecHead),  /* rsi: Pointer to stack arguments.                   */
      self->mRegData.i,            /* rdx: Pointer to register arguments (ints on SysV). */
      self->mRegData.f,            /* rcx: Pointer to floating point register arguments. */
      target,                      /* r8 */
      ret_regs                     /* r9 */
    );
    /* reassemble aggr to be returned from reg data */
    for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i) {
      DCchar** src;
      int ll = 8;
      switch(ag->sysv_classes[i]) {
        case SYSVC_INTEGER:  src = &ret_regs_i; break;
        case SYSVC_SSE:      src = &ret_regs_f; break;
        /* @@@AGGR implement when implementing x87 types */
      }
      while(ll-- && st_size--)
        *dst++ = *(*src)++;
    }
  }

#else

  if (self->mAggrReturnReg != -1) {
    /* call regular dcCall_x64_sysv here, w/ pointer to the aggr in first arg */
    self->mRegData.i[self->mAggrReturnReg] = (int64)ret;

    dcCall_x64_win64(
      dcVecSize(&self->mVecHead),  /* rcx: Size of stack data.           */
      dcVecData(&self->mVecHead),  /* rdx: Pointer to stack arguments.   */
      self->mRegData.i,            /* r8:  Pointer to register arguments */
      target                       /* r9 */
    );
  } else {
    DCchar ret_reg[8];             /* 1 qword */
    DCsize st_size = ag->size;     /* guaranteed to be <= 8 */
    DCchar* dst = (char*)ret;
    DCchar* src = ret_reg;
    dcCall_x64_win64_aggr(
      dcVecSize(&self->mVecHead),  /* rcx: Size of stack data.           */
      dcVecData(&self->mVecHead),  /* rdx: Pointer to stack arguments.   */
      self->mRegData.i,            /* r8:  Pointer to register arguments */
      target,                      /* r9 */
      ret_reg                      /* stack */
    );
    while(st_size--)
      *dst++ = *src++;
  }

#endif
}


static void dc_callvm_mode_x64(DCCallVM* in_self, DCint mode);

DCCallVM_vt gVT_x64 =
{
  &dc_callvm_free_x64
, &dc_callvm_reset_x64
, &dc_callvm_mode_x64
, &dc_callvm_argBool_x64
, &dc_callvm_argChar_x64
, &dc_callvm_argShort_x64
, &dc_callvm_argInt_x64
, &dc_callvm_argLong_x64
, &dc_callvm_argLongLong_x64
, &dc_callvm_argFloat_x64
, &dc_callvm_argDouble_x64
, &dc_callvm_argPointer_x64
, &dc_callvm_argAggr_x64
, (DCvoidvmfunc*)     &dc_callvm_call_x64
, (DCboolvmfunc*)     &dc_callvm_call_x64
, (DCcharvmfunc*)     &dc_callvm_call_x64
, (DCshortvmfunc*)    &dc_callvm_call_x64
, (DCintvmfunc*)      &dc_callvm_call_x64
, (DClongvmfunc*)     &dc_callvm_call_x64
, (DClonglongvmfunc*) &dc_callvm_call_x64
, (DCfloatvmfunc*)    &dc_callvm_call_x64
, (DCdoublevmfunc*)   &dc_callvm_call_x64
, (DCpointervmfunc*)  &dc_callvm_call_x64
, (DCaggrvmfunc*)     &dc_callvm_call_x64_aggr
, (DCbeginaggrvmfunc*)&dc_callvm_begin_aggr_x64
};


#if defined(DC_WINDOWS)
/* --- win64 thiscalls ------------------------------------------------------------- */

DCCallVM_vt gVT_x64_win64_this =
{
  &dc_callvm_free_x64
, &dc_callvm_reset_x64
, &dc_callvm_mode_x64
, &dc_callvm_argBool_x64
, &dc_callvm_argChar_x64
, &dc_callvm_argShort_x64
, &dc_callvm_argInt_x64
, &dc_callvm_argLong_x64
, &dc_callvm_argLongLong_x64
, &dc_callvm_argFloat_x64
, &dc_callvm_argDouble_x64
, &dc_callvm_argPointer_x64
, &dc_callvm_argAggr_x64
, (DCvoidvmfunc*)     &dc_callvm_call_x64
, (DCboolvmfunc*)     &dc_callvm_call_x64
, (DCcharvmfunc*)     &dc_callvm_call_x64
, (DCshortvmfunc*)    &dc_callvm_call_x64
, (DCintvmfunc*)      &dc_callvm_call_x64
, (DClongvmfunc*)     &dc_callvm_call_x64
, (DClonglongvmfunc*) &dc_callvm_call_x64
, (DCfloatvmfunc*)    &dc_callvm_call_x64
, (DCdoublevmfunc*)   &dc_callvm_call_x64
, (DCpointervmfunc*)  &dc_callvm_call_x64
, (DCaggrvmfunc*)     &dc_callvm_call_x64_aggr
, (DCbeginaggrvmfunc*)&dc_callvm_begin_aggr_x64_win64_this
};

#endif

/* --- syscall ------------------------------------------------------------- */

#if defined(DC_UNIX)
void dc_callvm_call_x64_syscall_sysv(DCCallVM* in_self, DCpointer target)
{
  DCCallVM_x64* self;

  /* syscalls can have up to 6 args, required to be "Only values of class INTEGER or class MEMORY" (from */
  /* SysV manual), so we can use self->mRegData.i directly; verify this has space for at least 6 values, though. */
  assert(numIntRegs >= 6);

  self = (DCCallVM_x64*)in_self;
  dcCall_x64_syscall_sysv(self->mRegData.i, target);
}

DCCallVM_vt gVT_x64_syscall_sysv =
{
  &dc_callvm_free_x64
, &dc_callvm_reset_x64
, &dc_callvm_mode_x64
, &dc_callvm_argBool_x64
, &dc_callvm_argChar_x64
, &dc_callvm_argShort_x64
, &dc_callvm_argInt_x64
, &dc_callvm_argLong_x64
, &dc_callvm_argLongLong_x64
, &dc_callvm_argFloat_x64
, &dc_callvm_argDouble_x64
, &dc_callvm_argPointer_x64
, NULL /* argAggr */
, (DCvoidvmfunc*)       &dc_callvm_call_x64_syscall_sysv
, (DCboolvmfunc*)       &dc_callvm_call_x64_syscall_sysv
, (DCcharvmfunc*)       &dc_callvm_call_x64_syscall_sysv
, (DCshortvmfunc*)      &dc_callvm_call_x64_syscall_sysv
, (DCintvmfunc*)        &dc_callvm_call_x64_syscall_sysv
, (DClongvmfunc*)       &dc_callvm_call_x64_syscall_sysv
, (DClonglongvmfunc*)   &dc_callvm_call_x64_syscall_sysv
, (DCfloatvmfunc*)      &dc_callvm_call_x64_syscall_sysv
, (DCdoublevmfunc*)     &dc_callvm_call_x64_syscall_sysv
, (DCpointervmfunc*)    &dc_callvm_call_x64_syscall_sysv
, NULL /* callAggr */
, NULL /* beginAggr */
};
#endif



/* mode */

static void dc_callvm_mode_x64(DCCallVM* in_self, DCint mode)
{
  DCCallVM_x64* self = (DCCallVM_x64*)in_self;
  DCCallVM_vt* vt;

  switch(mode) {
    case DC_CALL_C_DEFAULT:
#if defined(DC_UNIX)
    case DC_CALL_C_DEFAULT_THIS:
    case DC_CALL_C_X64_SYSV: /* = DC_CALL_C_X64_SYSV_THIS */
#else
    case DC_CALL_C_X64_WIN64:
#endif
    case DC_CALL_C_ELLIPSIS:
    case DC_CALL_C_ELLIPSIS_VARARGS:
      vt = &gVT_x64;
      break;
#if defined(DC_WINDOWS)
    case DC_CALL_C_DEFAULT_THIS:
    case DC_CALL_C_X64_WIN64_THIS:
      vt = &gVT_x64_win64_this;
      break;
#endif
    case DC_CALL_SYS_DEFAULT:
#if defined(DC_UNIX)
    case DC_CALL_SYS_X64_SYSCALL_SYSV:
      vt = &gVT_x64_syscall_sysv; break;
#else
      self->mInterface.mError = DC_ERROR_UNSUPPORTED_MODE; return;
#endif
    default:
      self->mInterface.mError = DC_ERROR_UNSUPPORTED_MODE; 
      return;
  }
  dc_callvm_base_init(&self->mInterface, vt);
}

/* Public API. */
DCCallVM* dcNewCallVM(DCsize size)
{
  DCCallVM_x64* p = (DCCallVM_x64*)dcAllocMem(sizeof(DCCallVM_x64)+size);

  dc_callvm_mode_x64((DCCallVM*)p, DC_CALL_C_DEFAULT);

  /* Since we store register parameters in DCCallVM_x64 directly, adjust the stack size. */
  size -= sizeof(DCRegData_x64);
  size = (((signed long)size) < 0) ? 0 : size;
  dcVecInit(&p->mVecHead, size);
  dc_callvm_reset_x64((DCCallVM*)p);

  return (DCCallVM*)p;
}

