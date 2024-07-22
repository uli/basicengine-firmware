/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_callvm_riscv64.c
 Description: RISC-V ABI implementation
 License:

   Copyright (c) 2023 Jun Jeon <yjeon@netflix.com>

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


#include "dyncall_callvm_riscv64.h"
#include "dyncall_alloc.h"


void dcCall_riscv64(DCpointer target, DCpointer data, DCsize size, DCpointer regdata);


static void reset(DCCallVM* in_p)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_p;
  p->i = 0;
  p->f = 0;
  dcVecReset(&p->mVecHead);
  /* single precision fp values in 64b float registers require all 32 high bits *
   * to be set: set all bits on init, will be overwritten when doubles are used */
  for(int i=0; i<RISCV_NUM_FLOAT_REGISTERS; ++i)
    p->u.I[i] = -1;
}


static void deinit(DCCallVM* in_self)
{
  dcFreeMem(in_self);
}



static void a_i64(DCCallVM* in_self, DClonglong x)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_self;
  if (p->i < RISCV_NUM_INT_REGISTERS) {
    p->I[p->i] = x;
    p->i++;
  } else {
    dcVecAppend(&p->mVecHead, &x, sizeof(DClonglong));
  }
}

static void a_bool    (DCCallVM* self, DCbool    x) { a_i64(self, (DClonglong)x); }
static void a_char    (DCCallVM* self, DCchar    x) { a_i64(self, x); }
static void a_short   (DCCallVM* self, DCshort   x) { a_i64(self, x); }
static void a_int     (DCCallVM* self, DCint     x) { a_i64(self, x); }
static void a_long    (DCCallVM* self, DClong    x) { a_i64(self, x); }
static void a_pointer (DCCallVM* self, DCpointer x) { a_i64(self, (DClonglong) x ); }

static void a_float(DCCallVM* in_p, DCfloat x)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_p;

  if (p->f < RISCV_NUM_FLOAT_REGISTERS) {
    /* trivial case; just use float arg reg */
    p->u.S[p->f << 1] = x;
    ++p->f;
  } else if (p->i < RISCV_NUM_INT_REGISTERS) {
    /* risc-v will use int arg reg to pass float args */
    p->I[p->i] = (unsigned)*(DCint*)&x;
    ++p->i;
  } else {
    /* now everything has to go on stack */
    dcVecAppend(&p->mVecHead, &x, sizeof(DCfloat));
    dcVecSkip(&p->mVecHead, 4);        /* align to 8-bytes */
  }
}

static void a_double(DCCallVM* in_p, DCdouble x)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_p;
  if (p->f < RISCV_NUM_FLOAT_REGISTERS) {
    /* trivial case; just use float arg reg */
    p->u.D[p->f] = x;
    ++p->f;
  } else if (p->i < RISCV_NUM_INT_REGISTERS) {
    /* risc-v will use int arg reg to pass float args */
    p->I[p->i] = *(DClonglong*)&x;
    ++p->i;
  } else {
    /* now everything has to go on stack */
    dcVecAppend(&p->mVecHead, &x, sizeof(DCdouble));
  }
}


/* for variadic args, push everything onto stack, according to the RISC-V calling convention */

static void var_i64(DCCallVM* in_self, DClonglong x)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_self;
  /* first use int registers, then spill over onto stack */
  if (p->i < RISCV_NUM_INT_REGISTERS) {
    p->I[p->i] = *(DClonglong*)&x;
    p->i++;
  } else {
    dcVecAppend(&p->mVecHead, &x, sizeof(DClonglong));
  }
}

static void var_bool    (DCCallVM* self, DCbool    x) { var_i64(self, (DClonglong)x); }
static void var_char    (DCCallVM* self, DCchar    x) { var_i64(self, (DClonglong)x); }
static void var_short   (DCCallVM* self, DCshort   x) { var_i64(self, (DClonglong)x); }
static void var_int     (DCCallVM* self, DCint     x) { var_i64(self, (DClonglong)x); }
static void var_long    (DCCallVM* self, DClong    x) { var_i64(self, (DClonglong)x); }
static void var_pointer (DCCallVM* self, DCpointer x) { var_i64(self, (DClonglong)x); }

static void var_double(DCCallVM* in_p, DCdouble x)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_p;
  /* first use int registers, then spill over onto stack */
  if (p->i < RISCV_NUM_INT_REGISTERS) {
    p->I[p->i] = *(DClonglong*)&x;
    p->i++;
  } else {
    dcVecAppend(&p->mVecHead, &x, sizeof(DCdouble));
  }
}

static void var_float(DCCallVM* in_p, DCfloat x)
{
  var_double(in_p, (DCdouble)x);
}


void call(DCCallVM* in_p, DCpointer target)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)in_p;

  /*
  ** copy 'size' argument is given in number of 16-byte 'pair' blocks.
     sp is always 16-byte aligned
     ref: https://riscv.org/wp-content/uploads/2015/01/riscv-calling.pdf, page 3
  */
  dcCall_riscv64(target, dcVecData(&p->mVecHead), ( dcVecSize(&p->mVecHead) + 15 ) & -16, &p->u.S[0]);
}

static void mode(DCCallVM* in_self, DCint mode);

DCCallVM_vt vt_riscv64 =
{
  &deinit
, &reset
, &mode
, &a_bool
, &a_char
, &a_short
, &a_int
, &a_long
, &a_i64
, &a_float
, &a_double
, &a_pointer
, NULL /* argAggr */
, (DCvoidvmfunc*)       &call
, (DCboolvmfunc*)       &call
, (DCcharvmfunc*)       &call
, (DCshortvmfunc*)      &call
, (DCintvmfunc*)        &call
, (DClongvmfunc*)       &call
, (DClonglongvmfunc*)   &call
, (DCfloatvmfunc*)      &call
, (DCdoublevmfunc*)     &call
, (DCpointervmfunc*)    &call
, NULL /* callAggr */
, NULL /* beginAggr */
};

DCCallVM_vt vt_riscv64_varargs =
{
  &deinit
, &reset
, &mode
, &var_bool
, &var_char
, &var_short
, &var_int
, &var_long
, &var_i64
, &var_float
, &var_double
, &var_pointer
, NULL /* argAggr */
, (DCvoidvmfunc*)       &call
, (DCboolvmfunc*)       &call
, (DCcharvmfunc*)       &call
, (DCshortvmfunc*)      &call
, (DCintvmfunc*)        &call
, (DClongvmfunc*)       &call
, (DClonglongvmfunc*)   &call
, (DCfloatvmfunc*)      &call
, (DCdoublevmfunc*)     &call
, (DCpointervmfunc*)    &call
, NULL /* callAggr */
, NULL /* beginAggr */
};

static void mode(DCCallVM* in_self, DCint mode)
{
  DCCallVM_riscv64* self = (DCCallVM_riscv64*)in_self;
  DCCallVM_vt* vt;

  switch(mode) {
    case DC_CALL_C_DEFAULT:
    case DC_CALL_C_RISCV64:
    case DC_CALL_C_ELLIPSIS:
      vt = &vt_riscv64;
      break;
    case DC_CALL_C_ELLIPSIS_VARARGS:
      vt = &vt_riscv64_varargs;
      break;
    default:
      self->mInterface.mError = DC_ERROR_UNSUPPORTED_MODE;
      return;
  }
  dc_callvm_base_init(&self->mInterface, vt);
}

/* Public API. */
DCCallVM* dcNewCallVM(DCsize size)
{
  DCCallVM_riscv64* p = (DCCallVM_riscv64*)dcAllocMem(sizeof(DCCallVM_riscv64)+size);

  mode((DCCallVM*)p, DC_CALL_C_DEFAULT);

  dcVecInit(&p->mVecHead, size);
  reset((DCCallVM*)p);

  return (DCCallVM*)p;
}

