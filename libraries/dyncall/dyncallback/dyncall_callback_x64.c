/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_callback_x64.c
 Description: Callback - Implementation for x64
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
#include "dyncall_aggregate.h"
#include "dyncall_thunk.h"


/* Callback symbol. */
extern void dcCallback_x64_sysv();
extern void dcCallback_x64_win64();

struct DCCallback
{
  DCThunk            thunk;                /* offset 0,  size 24 */
  DCCallbackHandler* handler;              /* offset 24 */
  void*              userdata;             /* offset 32 */
  DCint              aggr_return_register; /* offset 40 */
  DCint              pad;                  /* offset 44 */
  DCaggr *const *    aggrs;                /* offset 48 */
};


void dcbInitCallback2(DCCallback* pcb, const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  const DCsigchar *ch = signature;
  int mode = DC_CALL_C_DEFAULT;
  DCint num_aggrs = 0;

  pcb->handler              = handler;
  pcb->userdata             = userdata;
  pcb->aggrs                = NULL;
  pcb->aggr_return_register = -2; /* default, = no aggr as ret value */

  if(*ch == DC_SIGCHAR_CC_PREFIX)
  {
    mode = dcGetModeFromCCSigChar(ch[1]);
    ch += 2;
  }

  while(*ch)
    num_aggrs += (*(ch++) == DC_SIGCHAR_AGGREGATE);

  if(num_aggrs)
  {
    pcb->aggrs = aggrs;
  
    if (ch != signature && *(ch - 1) == DC_SIGCHAR_AGGREGATE) {
      const DCaggr *ag = pcb->aggrs[num_aggrs - 1];
  
#if defined(DC_UNIX)
      if (!ag || (ag->sysv_classes[0] == SYSVC_MEMORY)) {
#else
      if (mode == DC_CALL_C_DEFAULT_THIS) {
        pcb->aggr_return_register = 1;
      } else if (!ag || ag->size > 8 || /*not a power of 2?*/(ag->size & (ag->size - 1))) {
#endif 
        /* we need to "return" this aggr as a hidden pointer (first arg) */
        pcb->aggr_return_register = 0;
      } else {
        pcb->aggr_return_register = -1; /* small aggr, returned in register */
      }
    }
  }
}


DCCallback* dcbNewCallback2(const DCsigchar* signature, DCCallbackHandler* handler, void* userdata, DCaggr *const * aggrs)
{
  int err;
  DCCallback* pcb;
  err = dcAllocWX(sizeof(DCCallback), (void**) &pcb);
  if(err)
    return NULL;

  dcbInitCallback2(pcb, signature, handler, userdata, aggrs);

#if defined (DC__OS_Win64)
  dcbInitThunk(&pcb->thunk, dcCallback_x64_win64); 
#else
  dcbInitThunk(&pcb->thunk, dcCallback_x64_sysv); 
#endif

  err = dcInitExecWX(pcb, sizeof(DCCallback));
  if(err) {
    dcFreeWX(pcb, sizeof(DCCallback));
    return NULL;
  }

  return pcb;
}

