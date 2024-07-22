/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_callback_riscv64.S
 Description: Callback Thunk - Implementation for RISCV64
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

/* struct DCCallback
     type       off   size
     ---------|------|------
     DCThunk  |   0  |  32
     handler  |  32  |   8
     userdata |  40  |   8
*/

.align 4
.globl dcCallbackThunkEntry
dcCallbackThunkEntry:

/* called from dcbInitThunk() in dyncall_thunk_riscv64.c */

/* input:
    t5: DCCallback* pcb
    a0..a7   ?? GP regs
    fa0..fa7 ?? FP/SIMD regs
    sp...    ?? arguments on stack

   locals:
     type       off   size
     ---------|------|------
     Frame        0     16
     DCArgs      16    144
     DCValue    160     16

     size              176
     aligned           176

   ra - ret addr
   s0/fp - frame ptr
*/

/* prolog */

	add sp, sp, -176
	sd  ra, 168(sp)
	sd  s0, 160(sp)
	add s0, sp, 176

/* fill DCArgs data (see dyncall_args_riscv64.c) */

	sd  a0, 0(sp)     /* I */
	sd  a1, 8(sp)
	sd  a2, 16(sp)
	sd  a3, 24(sp)
	sd  a4, 32(sp)
	sd  a5, 40(sp)
	sd  a6, 48(sp)
	sd  a7, 56(sp)

	fsd fa0, 64(sp)   /* F */
	fsd fa1, 72(sp)
	fsd fa2, 80(sp)
	fsd fa3, 88(sp)
	fsd fa4, 96(sp)
	fsd fa5, 104(sp)
	fsd fa6, 112(sp)
	fsd fa7, 120(sp)

	sd  s0,   128(sp) /* sp=sp */
	sd  zero, 136(sp) /* i=f=0 */

/* call handler/callback */

	mv   a0, t5       /* DCCallback* pcb      */
	mv   a1, sp       /* DCArgs*     args     */
	add  a2, sp, 144  /* DCValue*    result   */
	ld   a3, 40(t5)   /* void*       userdata */

	ld   t2, 32(t5)
	jalr ra, 0(t2)

	and  t3, a0, 255
	li   t4, 'f'        /* single prec floats need reg's 32 MSBs   */
	beq  t3, t4, .retf  /* all set (on riscv with the D extension) */
	li   t4, 'd'
	beq  t3, t4, .retd

.reti:
	ld   a0, 144(sp)
	j    .ret
.retf:
	flw  fa0, 144(sp)
	j    .ret
.retd:
	fld  fa0, 144(sp)
.ret:

/* epilog */

	ld   ra, 168(sp)
	ld   s0, 160(sp)
	add  sp, sp, 176
	ret

