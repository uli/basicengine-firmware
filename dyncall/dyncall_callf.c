/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_callf.c
 Description: formatted call C interface (extension module)
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



#include "dyncall_callf.h"


static void handle_mode(DCCallVM* vm, const DCsigchar** sigptr)
{
  if(*((*sigptr)+1) != '\0') {
    DCint mode = dcGetModeFromCCSigChar(*(*sigptr)++);
    if(mode != DC_ERROR_UNSUPPORTED_MODE)
      dcMode(vm, mode);
  }
}


/* Shareable implementation for argument binding used in ArgF and CallF below. */
static void dcArgF_impl(DCCallVM* vm, const DCsigchar** sigptr, va_list args)
{
  DCsigchar ch;
  while((ch=*(*sigptr)++) != '\0' && ch != DC_SIGCHAR_ENDARG) {
    switch(ch) {
      /* calling convention modes */
      case DC_SIGCHAR_CC_PREFIX: handle_mode(vm, sigptr); break;
      /* types */
      case DC_SIGCHAR_BOOL:      dcArgBool    (vm, (DCbool)           va_arg(args, DCint     )); break;
      case DC_SIGCHAR_CHAR:      dcArgChar    (vm, (DCchar)           va_arg(args, DCint     )); break;
      case DC_SIGCHAR_UCHAR:     dcArgChar    (vm, (DCchar)(DCuchar)  va_arg(args, DCint     )); break;
      case DC_SIGCHAR_SHORT:     dcArgShort   (vm, (DCshort)          va_arg(args, DCint     )); break;
      case DC_SIGCHAR_USHORT:    dcArgShort   (vm, (DCshort)(DCushort)va_arg(args, DCint     )); break;
      case DC_SIGCHAR_INT:       dcArgInt     (vm, (DCint)            va_arg(args, DCint     )); break;
      case DC_SIGCHAR_UINT:      dcArgInt     (vm, (DCint)(DCuint)    va_arg(args, DCint     )); break;
      case DC_SIGCHAR_LONG:      dcArgLong    (vm, (DClong)           va_arg(args, DClong    )); break;
      case DC_SIGCHAR_ULONG:     dcArgLong    (vm, (DCulong)          va_arg(args, DClong    )); break;
      case DC_SIGCHAR_LONGLONG:  dcArgLongLong(vm, (DClonglong)       va_arg(args, DClonglong)); break;
      case DC_SIGCHAR_ULONGLONG: dcArgLongLong(vm, (DCulonglong)      va_arg(args, DClonglong)); break;
      case DC_SIGCHAR_FLOAT:     dcArgFloat   (vm, (DCfloat)          va_arg(args, DCdouble  )); break;
      case DC_SIGCHAR_DOUBLE:    dcArgDouble  (vm, (DCdouble)         va_arg(args, DCdouble  )); break;
      case DC_SIGCHAR_POINTER:   dcArgPointer (vm, (DCpointer)        va_arg(args, DCpointer )); break;
      case DC_SIGCHAR_STRING:    dcArgPointer (vm, (DCpointer)        va_arg(args, DCpointer )); break;
      case DC_SIGCHAR_AGGREGATE: {
        /* aggregates expect 2 va args, a DCaggr*, then a ptr to the aggregate */
        DCaggr* ag = va_arg(args, DCaggr*);
        dcArgAggr(vm, ag, va_arg(args, DCpointer));
        break;
      }
    }
  }
}

void dcVArgF(DCCallVM* vm, const DCsigchar* signature, va_list args)
{
  dcArgF_impl(vm, &signature, args);
}

void dcArgF(DCCallVM* vm, const DCsigchar* signature, ...)
{
  va_list va;
  va_start(va, signature);
  dcVArgF(vm,signature,va);
  va_end(va);
}


/* msvc introduced C99'w va_copy() late (in 2013 w/ msvc 18.00); plan9 APE does
 * not have it either; luckily given their va_list being only a ptr in both
 * cases, work around the issue for older versions */
#if (defined(DC__C_MSVC) || defined(DC__OS_Plan9)) && !defined(va_copy)
	#define va_copy(dst, src) ((dst)=(src))
#endif


void dcVCallF(DCCallVM* vm, DCValue* result, DCpointer funcptr, const DCsigchar* signature, va_list args)
{
  DCaggr* ret_ag = NULL; /* only needed for when func returns an aggregate      */
  const DCsigchar* ptr = signature;

  /* need preparatory call if return type is an aggregate, so check end of sig */
  /* @@@ugly */
  while(*ptr && ptr[1]) ++ptr;
  if(*ptr == DC_SIGCHAR_AGGREGATE) {
    va_list args_;
    va_copy(args_, args);

    /* iterate va_list to get return type related args*/
    ptr = signature;
    while(*ptr) {
      switch(*ptr++) {
        /* calling convention modes */
        case DC_SIGCHAR_CC_PREFIX: handle_mode(vm, &ptr); break; /* needs handling before dcBeginCallAggr */
        /* types */
        case DC_SIGCHAR_BOOL:
        case DC_SIGCHAR_CHAR:
        case DC_SIGCHAR_UCHAR:
        case DC_SIGCHAR_SHORT:
        case DC_SIGCHAR_USHORT:
        case DC_SIGCHAR_INT:
        case DC_SIGCHAR_UINT:      va_arg(args_, DCint     ); break;
        case DC_SIGCHAR_LONG:
        case DC_SIGCHAR_ULONG:     va_arg(args_, DClong    ); break;
        case DC_SIGCHAR_LONGLONG:
        case DC_SIGCHAR_ULONGLONG: va_arg(args_, DClonglong); break;
        case DC_SIGCHAR_FLOAT:
        case DC_SIGCHAR_DOUBLE:    va_arg(args_, DCdouble  ); break;
        case DC_SIGCHAR_POINTER:
        case DC_SIGCHAR_STRING:    va_arg(args_, DCpointer ); break;
        case DC_SIGCHAR_AGGREGATE:
          /* aggregate as retval expects 2 more va args, a DCaggr*, then a ptr to the aggregate */
          ret_ag = va_arg(args_, DCaggr*);
          result->p = va_arg(args_, DCpointer);
          break;
      }
    }
    dcBeginCallAggr(vm, ret_ag);

	va_end(args_);
  }

  /* push args */
  ptr = signature;
  dcArgF_impl(vm, &ptr, args);

  /* call */
  switch(*ptr) {
    case DC_SIGCHAR_VOID:                   dcCallVoid             (vm,funcptr); break;
    case DC_SIGCHAR_BOOL:       result->B = dcCallBool             (vm,funcptr); break;
    case DC_SIGCHAR_CHAR:       result->c = dcCallChar             (vm,funcptr); break;
    case DC_SIGCHAR_UCHAR:      result->C = (DCuchar)dcCallChar    (vm,funcptr); break;
    case DC_SIGCHAR_SHORT:      result->s = dcCallShort            (vm,funcptr); break;
    case DC_SIGCHAR_USHORT:     result->S = dcCallShort            (vm,funcptr); break;
    case DC_SIGCHAR_INT:        result->i = dcCallInt              (vm,funcptr); break;
    case DC_SIGCHAR_UINT:       result->I = dcCallInt              (vm,funcptr); break;
    case DC_SIGCHAR_LONG:       result->j = dcCallLong             (vm,funcptr); break;
    case DC_SIGCHAR_ULONG:      result->J = dcCallLong             (vm,funcptr); break;
    case DC_SIGCHAR_LONGLONG:   result->l = dcCallLongLong         (vm,funcptr); break;
    case DC_SIGCHAR_ULONGLONG:  result->L = dcCallLongLong         (vm,funcptr); break;
    case DC_SIGCHAR_FLOAT:      result->f = dcCallFloat            (vm,funcptr); break;
    case DC_SIGCHAR_DOUBLE:     result->d = dcCallDouble           (vm,funcptr); break;
    case DC_SIGCHAR_POINTER:    result->p = dcCallPointer          (vm,funcptr); break;
    case DC_SIGCHAR_STRING:     result->Z = (DCstring)dcCallPointer(vm,funcptr); break;
    case DC_SIGCHAR_AGGREGATE: {
      result->p = dcCallAggr(vm, funcptr, ret_ag, result->p);
      break;
    }
  }
}

void dcCallF(DCCallVM* vm, DCValue* result, DCpointer funcptr, const DCsigchar* signature, ...)
{
  va_list va;
  va_start(va, signature);
  dcVCallF(vm,result,funcptr,signature,va);
  va_end(va);
}

