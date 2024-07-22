/*

 Package: dyncall
 Library: dyncall
 File: dyncall/dyncall_call_riscv64.S
 Description: Call Kernel for RISCV64 Architecture
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


/* ============================================================================
   DynCall Call Kernel for RISCV64 Architecture 
   ----------------------------------------------------------------------------
   C Interface:
     dcCall_riscv64 (DCpointer target, DCpointer data, DCsize size, DCfloat* regdata);
*/

.text
/*
   DynCall Back-End riscv64
   
   Supports:
   - arch=rv64g, abi=lp64d
*/ 

.globl dcCall_riscv64
dcCall_riscv64:
.align 2

/* input:
     a0: target   (address of target)
     a1: data     (address of stack copy data)
     a2: size     (number of 'pair' 16-byte units)
     a3: regdata  (address of register data)
*/

/* prolog: */

	add  sp, sp, -16 /* alloc frame */
	sd   ra, 0(sp)   /* save ret addr */
	sd   s0, 8(sp)   /* save stk ptr */
	mv   s0, sp      /* set cur stk ptr */

/* load 64-bit floating-point registers */
        
	fld  fa0, 0(a3)
	fld  fa1, 8(a3)
	fld  fa2, 16(a3)
	fld  fa3, 24(a3)
	fld  fa4, 32(a3)
	fld  fa5, 40(a3)
	fld  fa6, 48(a3)
	fld  fa7, 56(a3)

/* copy to stack */

	sub  sp, sp, a2     /* create call-frame */
	
	mv   t1, zero       /* t1: cnt = 0 */
	mv   t2, a1         /* t2: read pointer = data */
	mv   t3, sp         /* t3: write pointer = sp */

.next:
	bge  t1, a2, .done
	
	ld   t4, 0(t2)      /* read 8b from mem */
	sd   t4, 0(t3)      /* write 8b to mem */

	/* advance the pointers */
	add  t2, t2, 8
	add  t3, t3, 8
	add  t1, t1, 8

	j    .next

.done:

/* rescue temp int registers */

	mv   t1, a0         /* t1: target */
	add  t2, a3, 64     /* t2: integer reg buffer */

/* load 64-bit integer registers ( 8 x 64-bit ) */

	/* load register set */

	ld   a0, 0(t2)
	ld   a1, 8(t2)
	ld   a2, 16(t2)
	ld   a3, 24(t2)
	ld   a4, 32(t2)
	ld   a5, 40(t2)
	ld   a6, 48(t2)
	ld   a7, 56(t2)

/* call target: */

	jalr ra, 0(t1)

/* epilog: */

	mv   sp, s0

	ld   ra, 0(sp)
	ld   s0, 8(sp)
	add  sp, sp, 16

	ret

