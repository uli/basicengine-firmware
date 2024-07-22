/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_callvm_riscv64.h
 Description: 
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

#ifndef DYNCALL_CALLVM_RISCV64_H
#define DYNCALL_CALLVM_RISCV64_H

#include "dyncall_callvm.h"
#include "dyncall_vector.h"

#define RISCV_NUM_INT_REGISTERS   8
#define RISCV_NUM_FLOAT_REGISTERS 8

typedef struct
{
  DCCallVM mInterface;
  unsigned int i;  /* int register counter */
  unsigned int f;  /* float register counter */
  union {          /* float register buffer */
    DCfloat    S[RISCV_NUM_FLOAT_REGISTERS << 1];
    DCdouble   D[RISCV_NUM_FLOAT_REGISTERS];
    DClonglong I[RISCV_NUM_FLOAT_REGISTERS]; /* helper */
  } u;
  DCulonglong I[RISCV_NUM_INT_REGISTERS]; /* int register buffer */
  DCVecHead mVecHead; /* argument buffer head */
} DCCallVM_riscv64;

#endif /* DYNCALL_CALLVM_RISCV64_H */

