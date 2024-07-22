/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_aggregate.c
 Description: C interface to compute struct size
 License:

   Copyright (c) 2021-2022 Tassilo Philipp <tphilipp@potion-studios.com>

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
#include "dyncall_signature.h"
#include "dyncall_aggregate.h"
#include "dyncall_alloc.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>



#if defined(DC__Arch_AMD64) && defined(DC_UNIX)
#  include "dyncall_aggregate_x64.c"
#else
static void dcFinishAggr(DCaggr *ag)
{
}
#endif


DCaggr* dcNewAggr(DCsize maxFieldCount, DCsize size)
{
	DCaggr* ag = (DCaggr*)dcAllocMem(sizeof(DCaggr) + maxFieldCount * sizeof(DCfield));
	ag->n_fields = 0;
	ag->size = size;
	ag->alignment = 0;
	return ag;
}


void dcAggrField(DCaggr* ag, DCsigchar type, DCint offset, DCsize array_len, ...)
{
	DCfield *f = ag->fields + (ag->n_fields++);
	f->type = type;
	f->offset = offset;
	f->array_len = array_len;
	f->sub_aggr = NULL;
	switch(type) {
		case DC_SIGCHAR_BOOL:       f->size = sizeof(DCbool);     break;
		case DC_SIGCHAR_CHAR:
		case DC_SIGCHAR_UCHAR:      f->size = sizeof(DCchar);     break;
		case DC_SIGCHAR_SHORT:
		case DC_SIGCHAR_USHORT:     f->size = sizeof(DCshort);    break;
		case DC_SIGCHAR_INT:
		case DC_SIGCHAR_UINT:       f->size = sizeof(DCint);      break;
		case DC_SIGCHAR_LONG:
		case DC_SIGCHAR_ULONG:      f->size = sizeof(DClong);     break;
		case DC_SIGCHAR_LONGLONG:
		case DC_SIGCHAR_ULONGLONG:  f->size = sizeof(DClonglong); break;
		case DC_SIGCHAR_FLOAT:      f->size = sizeof(DCfloat);    break;
		case DC_SIGCHAR_DOUBLE:     f->size = sizeof(DCdouble);   break;
		case DC_SIGCHAR_POINTER:
		case DC_SIGCHAR_STRING:     f->size = sizeof(DCpointer);  break;
		case DC_SIGCHAR_AGGREGATE:
		{
			va_list ap;
			va_start(ap, array_len);
			f->sub_aggr = va_arg(ap, const DCaggr*);
			va_end(ap);

			f->size = f->sub_aggr->size;
			f->alignment = f->sub_aggr->alignment; /* field inherit's sub aggrs alignment*/
			break;
		}
		default:
			assert(0);
	}

	if(type != DC_SIGCHAR_AGGREGATE)
		f->alignment = f->size;

	/* aggr's field alignment is relative to largest field size */
	if(ag->alignment < f->alignment)
		ag->alignment = f->alignment;
}


void dcCloseAggr(DCaggr* ag)
{
	dcFinishAggr(ag);
}


void dcFreeAggr(DCaggr* ag)
{
	dcFreeMem(ag);
}

