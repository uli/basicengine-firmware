/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_callback_ppc64.c
 Description: Callback - Implementation Header for ppc64
 License:

   Copyright (c) 2014-2016 Masanori Mitsugi <mitsugi@linux.vnet.ibm.com>,
                      2022 Tassilo Philipp <tphilipp@potion-studios.com>

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


/* Callback symbol. */
extern void dcCallbackThunkEntry();

struct DCCallback                   /*       ELF v1      |       ELF v2      */
{                                   /* ------------------+------------------ */
  DCThunk            thunk;         /* offset  0 size 64 | offset  0 size 48 */
  DCCallbackHandler* handler;       /* offset 64 size  8 | offset 48 size  8 */
  size_t             stack_cleanup; /* offset 72 size  8 | offset 56 size  8 */
  void*              userdata;      /* offset 80 size  8 | offset 64 size  8 */
};


void dcbInitCallback2(DCCallback* pcb, const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  pcb->handler  = handler;
  pcb->userdata = userdata;
}


DCCallback* dcbNewCallback2(const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  DCCallback* pcb;
  int err = dcAllocWX(sizeof(DCCallback), (void**)&pcb);
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

