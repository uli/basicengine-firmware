/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_args_riscv64.c
 Description: Callback's Arguments VM - Implementation for RISCV64
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
#include "dyncall_args.h"

#include <stdint.h>

typedef union {
  struct { double value; } d;
  struct { float  value; } f;
} DCFPU_t;

struct DCArgs
{
  /* buffers and stack-pointer: */
  uint64_t  I[8];
  DCFPU_t   F[8];
  uint64_t* sp;
  
  /* counters: */
  int i;
  int f;
};

DClonglong dcbArgLongLong (DCArgs* p) {
  return  (p->i < 8) ? p->I[p->i++] : *(p->sp)++;
}

DCdouble  dcbArgDouble (DCArgs* p) {
  if (p->f < 8)
    return p->F[p->f++].d.value;
  else if (p->i < 8)
    return ((DCFPU_t*)&(p->I[p->i++]))->d.value;
  else
    return *(double*)(p->sp++);
}
DCfloat   dcbArgFloat  (DCArgs* p) {
  if (p->f < 8)
    return p->F[p->f++].f.value;
  else if (p->i < 8)
    return ((DCFPU_t*)&(p->I[p->i++]))->f.value;
  else
    return *(float*)(p->sp++);
}


DClong      dcbArgLong     (DCArgs* p) { return (DClong)   dcbArgLongLong(p); }
DCint       dcbArgInt      (DCArgs* p) { return (DCint)    dcbArgLongLong(p); }
DCshort     dcbArgShort    (DCArgs* p) { return (DCshort)  dcbArgLongLong(p); }
DCchar      dcbArgChar     (DCArgs* p) { return (DCchar)   dcbArgLongLong(p); }
DCbool      dcbArgBool     (DCArgs* p) { return dcbArgLongLong(p) & 0x1; }
DCpointer   dcbArgPointer  (DCArgs* p) { return (DCpointer)dcbArgLongLong(p); }

DCuint      dcbArgUInt     (DCArgs* p) { return (DCuint)     dcbArgInt(p);      }
DCuchar     dcbArgUChar    (DCArgs* p) { return (DCuchar)    dcbArgChar(p);     }
DCushort    dcbArgUShort   (DCArgs* p) { return (DCushort)   dcbArgShort(p);    }
DCulong     dcbArgULong    (DCArgs* p) { return (DCulong)    dcbArgLong(p);     }
DCulonglong dcbArgULongLong(DCArgs* p) { return (DCulonglong)dcbArgLongLong(p); }

DCpointer   dcbArgAggr     (DCArgs* p, DCpointer target)                   { /* @@@AGGR not impl */ return NULL; }
void        dcbReturnAggr  (DCArgs *args, DCValue *result, DCpointer ret)  { /* @@@AGGR not impl */ }

