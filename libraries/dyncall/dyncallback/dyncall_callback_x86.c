/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_callback_x86.c
 Description: Callback - Implementation for x86
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


#include "dyncall_callback.h"
#include "dyncall_alloc_wx.h"
#include "dyncall_thunk.h"
#include "dyncall_args_x86.h"


/* Callback symbol. */
extern void dcCallbackThunkEntry();

struct DCCallback
{
  DCThunk            thunk;         /* offset 0,  size 16 */
  DCCallbackHandler* handler;       /* offset 16 */
  DCArgsVT*          args_vt;       /* offset 20 */
  size_t             stack_cleanup; /* offset 24 */
  void*              userdata;      /* offset 28 */
};


/* compute stacksize for callee cleanup calling conventions:
 *
 * cdecl,stdcall,thiscall_ms,fastcall_ms,fastcall_gnu
 */

static int dcbCleanupSize_x86_cdecl(const DCsigchar* signature)
{
  return 0;
}

static int dcbCleanupSize_x86_std(const DCsigchar* signature)
{
  const DCsigchar* ptr = signature;
  int size = 0;
  DCsigchar ch;
  while( (ch = *ptr++) != DC_SIGCHAR_ENDARG ) {
    switch(ch) {
      case DC_SIGCHAR_BOOL:
      case DC_SIGCHAR_CHAR:
      case DC_SIGCHAR_SHORT:
      case DC_SIGCHAR_INT:
      case DC_SIGCHAR_LONG:
      case DC_SIGCHAR_POINTER:
      case DC_SIGCHAR_UCHAR:
      case DC_SIGCHAR_USHORT:
      case DC_SIGCHAR_UINT:
      case DC_SIGCHAR_ULONG:
      case DC_SIGCHAR_STRING:
      case DC_SIGCHAR_FLOAT:
        size += 4;
        break;
      case DC_SIGCHAR_DOUBLE:
      case DC_SIGCHAR_LONGLONG:
      case DC_SIGCHAR_ULONGLONG:
        size += 8;
        break;
    }
  }
  return size;
}

static int dcbCleanupSize_x86_this_ms(const DCsigchar* signature)
{
  const DCsigchar* ptr = signature;
  int size = 0;
  DCsigchar ch;
  while( (ch = *ptr++) != DC_SIGCHAR_ENDARG )
  {
    switch(ch)
    {
    case DC_SIGCHAR_BOOL:
    case DC_SIGCHAR_CHAR:
    case DC_SIGCHAR_SHORT:
    case DC_SIGCHAR_INT:
    case DC_SIGCHAR_LONG:
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_UCHAR:
    case DC_SIGCHAR_USHORT:
    case DC_SIGCHAR_UINT:
    case DC_SIGCHAR_ULONG:
    case DC_SIGCHAR_STRING:
    case DC_SIGCHAR_FLOAT:
      size += 4;
      break;
    case DC_SIGCHAR_DOUBLE:
    case DC_SIGCHAR_LONGLONG:
    case DC_SIGCHAR_ULONGLONG:
      size += 8;
      break;
    }
  }
  return size;
}

static int dcbCleanupSize_x86_fast_ms(const DCsigchar* signature)
{
  const DCsigchar* ptr = signature;
  int size = 0;
  int regs = 0;
  DCsigchar ch;
  while( (ch = *ptr++) != DC_SIGCHAR_ENDARG )
  {
    switch(ch)
    {
    case DC_SIGCHAR_BOOL:
    case DC_SIGCHAR_CHAR:
    case DC_SIGCHAR_SHORT:
    case DC_SIGCHAR_INT:
    case DC_SIGCHAR_LONG:
    case DC_SIGCHAR_POINTER:
    case DC_SIGCHAR_UCHAR:
    case DC_SIGCHAR_USHORT:
    case DC_SIGCHAR_UINT:
    case DC_SIGCHAR_ULONG:
    case DC_SIGCHAR_STRING:
      if (regs < 2) regs++;
      else size += 4;
      break;
    case DC_SIGCHAR_FLOAT:
      size += 4;
      break;
    case DC_SIGCHAR_DOUBLE:
      size += 8;
      break;
    case DC_SIGCHAR_LONGLONG:
    case DC_SIGCHAR_ULONGLONG:
      size += 8;
      break;
    }
  }
  return size;
}

static int dcbCleanupSize_x86_fast_gnu(const DCsigchar* signature)
{
  const DCsigchar* ptr = signature;
  DCsigchar ch;
  int size = 0;
  int regs = 0;
  while( (ch = *ptr++) != DC_SIGCHAR_ENDARG ) {
    switch(ch) {
      case DC_SIGCHAR_FLOAT:
        size += 4;
        break;
      case DC_SIGCHAR_DOUBLE:
        size += 8;
        break;
      case DC_SIGCHAR_LONGLONG:
      case DC_SIGCHAR_ULONGLONG:
        regs  = 2;
        size += 8;
        break;
      default:
        if (regs < 2) regs++;
        else size += 4;
        break;
    }
  }
  return size;
}


void dcbInitCallback2(DCCallback* pcb, const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  const DCsigchar* ptr;
  int mode;
  pcb->handler = handler;
  pcb->userdata = userdata;

  ptr = signature;

  /* x86 hints: */

  mode = DC_CALL_C_X86_CDECL;

  if(*ptr == DC_SIGCHAR_CC_PREFIX)
  {
    mode = dcGetModeFromCCSigChar(ptr[1]);
    ptr += 2;
  }

  /* x86 configuration: */

  switch(mode) {
    case DC_CALL_C_X86_CDECL:
      pcb->args_vt = &dcArgsVT_default;
      pcb->stack_cleanup = dcbCleanupSize_x86_cdecl(ptr);
      break;
    case DC_CALL_C_X86_WIN32_STD:
      pcb->args_vt = &dcArgsVT_default;
      pcb->stack_cleanup = dcbCleanupSize_x86_std(ptr);
      break;
    case DC_CALL_C_X86_WIN32_FAST_MS:
      pcb->args_vt = &dcArgsVT_fast_ms;
      pcb->stack_cleanup = dcbCleanupSize_x86_fast_ms(ptr);
      break;
    case DC_CALL_C_X86_WIN32_FAST_GNU:
      pcb->args_vt = &dcArgsVT_fast_gnu;
      pcb->stack_cleanup = dcbCleanupSize_x86_fast_gnu(ptr);
      break;
#if defined(DC_WINDOWS) && defined(DC__C_MSVC)
	case DC_CALL_C_DEFAULT_THIS:
#endif
    case DC_CALL_C_X86_WIN32_THIS_MS:
      pcb->args_vt = &dcArgsVT_this_ms;
      pcb->stack_cleanup = dcbCleanupSize_x86_this_ms(ptr);
      break;
  }

#if defined(DC_PLAN9)
  /* HACK for Plan9 - 'reuse' pcb->stack_cleanup as a flag
     to indicate if return value is 64bit. The field is not
     used anyways as the caller is responsible to clean up
     the stack in Plan9. If set to '1' the callback kernel
     takes into account an extra stack-parameter (pointer
     to 64bit return value).
     I thought of introducing a new field, but for one single
     x86 calling convention out of many, it seemed overkill
     to change the struct for everybody else. Maybe renaming
     would be a good idea, though. ~ Tassilo
  */
  while(*ptr) {
    if(*ptr++ == DC_SIGCHAR_ENDARG) {
      pcb->stack_cleanup = (*ptr == DC_SIGCHAR_LONGLONG) || (*ptr == DC_SIGCHAR_ULONGLONG);
      break;
    }
  }
#endif
}


DCCallback* dcbNewCallback2(const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  DCCallback* pcb;
  int err = dcAllocWX(sizeof(DCCallback), (void**) &pcb);
  if(err)
    return NULL;

  dcbInitCallback2(pcb, signature, handler, userdata, aggrs);
  dcbInitThunk(&pcb->thunk, dcCallbackThunkEntry);

  err = dcInitExecWX(pcb, sizeof(DCCallback));
  if(err) {
    dcFreeWX(pcb, sizeof(DCCallback));
    return NULL;
  }

  return pcb;
}

