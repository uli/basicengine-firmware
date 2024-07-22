/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_aggregate_x64.c
 Description:
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


#if defined(DC_UNIX)

#define DC_ONE_8BYTE      8
#define DC_TWO_8BYTES   2*8
#define DC_EIGHT_8BYTES 8*8

/* helper - long long mask with each byte being X */
#define LLBYTE(X) ((X)&0xFFULL)
#define SYSVC_CHECK_ALL_CLASSES(X) ((LLBYTE(X)<<56)|(LLBYTE(X)<<48)|(LLBYTE(X)<<40)|(LLBYTE(X)<<32)|(LLBYTE(X)<<24)|(LLBYTE(X)<<16)|(LLBYTE(X)<<8)|LLBYTE(X))


static DCuchar dc_merge_sysv_classes(DCuchar clz, DCuchar new_class)
{
    if(clz == SYSVC_NONE)
      return new_class;

    if(new_class == SYSVC_NONE)
      return clz;

    if(clz == SYSVC_MEMORY || new_class == SYSVC_MEMORY)
      return SYSVC_MEMORY;

    if(clz == SYSVC_INTEGER || new_class == SYSVC_INTEGER)
      return SYSVC_INTEGER;

    /* @@@AGGR implement when implementing x87 types
    if((clz & (SYSVC_X87|SYSVC_X87UP|SYSVC_COMPLEX_X87)) || (new_class & (SYSVC_X87|SYSVC_X87UP|SYSVC_COMPLEX_X87)))
      return SYSVC_MEMORY;*/

    return SYSVC_SSE;
}


static DCuchar dc_get_sysv_class_for_8byte(const DCaggr *ag, int index, int base_offset)
{
  int qword_offset = index * DC_ONE_8BYTE, i;
  DCuchar clz = SYSVC_NONE;

  for(i=0; i<ag->n_fields; ++i) {
    const DCfield *f = ag->fields + i;
    DCsize offset = base_offset + f->offset;

    /* field outside of qword at index? */
    if(offset >= (qword_offset + DC_ONE_8BYTE) || (offset + f->size * f->array_len) <= qword_offset)
      continue;

    /* if field is unaligned, class is MEMORY */
    assert((f->alignment & (f->alignment - 1)) == 0);      /* f->alignment required to be a power of 2*/
    if(f->alignment && (offset & (f->alignment - 1)) != 0) /* offset not a multiple of (power of 2) f->alignment? */
      return SYSVC_MEMORY;

    DCuchar new_class = SYSVC_NONE;

    switch (f->type) {
      case DC_SIGCHAR_BOOL:
      case DC_SIGCHAR_CHAR:
      case DC_SIGCHAR_UCHAR:
      case DC_SIGCHAR_SHORT:
      case DC_SIGCHAR_USHORT:
      case DC_SIGCHAR_INT:
      case DC_SIGCHAR_UINT:
      case DC_SIGCHAR_LONG:
      case DC_SIGCHAR_ULONG:
      case DC_SIGCHAR_LONGLONG:
      case DC_SIGCHAR_ULONGLONG:
      case DC_SIGCHAR_STRING:
      case DC_SIGCHAR_POINTER:
        new_class = SYSVC_INTEGER;
        break;
      case DC_SIGCHAR_FLOAT:
      case DC_SIGCHAR_DOUBLE:
        new_class = SYSVC_SSE;
        break;
      case DC_SIGCHAR_AGGREGATE:
        /* skip empty structs */
        if(f->size)
        {
          /* aggregate arrays need to be checked per element, as an aggregate can be composed of
           * multiple types, potentially split across an 8byte; loop only over parts within 8byte */
          int j = (qword_offset-offset) / f->size;
          int k = DC_ONE_8BYTE / f->size + j + 1; /* +1 for split array elements at end of 8byte */
          if(k > f->array_len)
            k = f->array_len;

          for(; j<k; ++j)
            new_class = dc_merge_sysv_classes(new_class, dc_get_sysv_class_for_8byte(f->sub_aggr, index, offset + f->size*j));
        }
        break;
      /*case DClongdouble, DCcomplexfloat DCcomplexdouble DCcomplexlongdouble etc... -> x87/x87up/complexx87 classes @@@AGGR implement */
    }

    if (clz == new_class)
      continue;

    clz = dc_merge_sysv_classes(clz, new_class);
  }

  return clz;
}


static void dc_get_sysv_classes_for_aggr(const DCaggr *ag, DCuchar *classes)
{
  int i;

#if 1 /* this is the optimized version that only respects types supported by dyncall */

  if(ag->size > DC_TWO_8BYTES) { /* @@@AGGR not checking if a field is unaligned */
    classes[0] = SYSVC_MEMORY;
    return;
  }

  /* abi doc: "If one of the classes is MEMORY, the whole argument is passed in memory." */
  classes[0] = dc_get_sysv_class_for_8byte(ag, 0, 0);
  if(classes[0] != SYSVC_MEMORY) {
    classes[1] = dc_get_sysv_class_for_8byte(ag, 1, 0);
    if(classes[1] == SYSVC_MEMORY)
      classes[0] = SYSVC_MEMORY;
    else
      classes[2] = SYSVC_NONE;
  }
  /* @@@AGGR what would happen with alignment-enforced padding >= 8? Then no field would cover the eightbyte @@@test */

#else /* this would be the version following the ABI more closely, to be implemented fully or partly when those types get supported by dyncall */

  /* abi doc: "If the size of an object is larger than eight qwords, or it
   *           contains unaligned fields, it has class MEMORY."
   * note:     ABI specs <v1.0 (2018) specify "four qwords", instead (b/c _m512 was added, later) */ /* @@@AGGR not checking if a field is unaligned */
  if(ag->size > DC_EIGHT_8BYTES) {
    classes[0] = SYSVC_MEMORY;
    return;
  }

  /* classify fields according to each of max 8 qwords */
  for(i = 0; i < DC_SYSV_MAX_NUM_CLASSES; ++i) {
    classes[i] = dc_get_sysv_class_for_8byte(ag, i, 0);

    /* abi doc: "If one of the classes is MEMORY, the whole argument is passed in memory." */
    if(classes[i] == SYSVC_MEMORY) {
      classes[0] = SYSVC_MEMORY;
      return;
    }

    /* stop eightbyte classification on first SYSVC_NONE returned */
    /* @@@AGGR what would happen with alignment-enforced padding >= 8? Then no field would cover the eightbyte @@@test */
    if(classes[i] == SYSVC_NONE)
      break;
  }

  /* Do post merger cleanup */

  /* abi doc: "If X87UP is not preceded by X87, the whole argument is passed in memory." */
  for(i = 1; i < DC_SYSV_MAX_NUM_CLASSES; ++i) {
    if (classes[i-1] == SYSVC_X87 && classes[i] != SYSVC_X87UP) {
      classes[0] = SYSVC_MEMORY;
      return;
    }
  }

  /* abi doc: "If the size of the aggregate exceeds two qwords and the first eightbyte isn't
   *           SSE or any other eightbyte isn't SSEUP, the whole argument is passed in memory." */
  if(ag->size > DC_TWO_8BYTES) {
    DClonglong mask = SYSVC_CHECK_ALL_CLASSES(SYSVC_SSEUP|SYSVC_NONE) ^ (LLBYTE(SYSVC_SSE|SYSVC_SSEUP|SYSVC_NONE)<<56);
    if((*(DClonglong*)ag->sysv_classes & mask) != *(DClonglong*)ag->sysv_classes) {
      classes[0] = SYSVC_MEMORY;
      return;
    }
  }

  /* abi doc: "If SSEUP is not preceded by SSE or SSEUP, it is converted to SSE." */
  for(i = 1; i < DC_SYSV_MAX_NUM_CLASSES; ++i) {
    DCuchar clz = classes[i];
    if(classes[i] == SYSVC_SSEUP && !(classes[i-1] & (SYSVC_SSE|SYSVC_SSEUP)))
      classes[i] = SYSVC_SSE;
  }

#endif
}


void dcFinishAggr(DCaggr *ag)
{
  dc_get_sysv_classes_for_aggr(ag, ag->sysv_classes);

  /* @@@AGGR implement when implementing x87 types
  for(i=0; ag->sysv_classes[i] && i<DC_SYSV_MAX_NUM_CLASSES; ++i)
    assert((ag->sysv_classes[i] & (SYSVC_MEMORY|SYSVC_INTEGER|SYSVC_SSE)) && "Unsupported System V class detected in struct");*/
}

#else

void dcFinishAggr(DCaggr *ag)
{
}

#endif

