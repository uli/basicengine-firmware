/*

 Package: dyncall
 Library: dyncallback
 File: dyncallback/dyncall_callback_x64.S
 Description: Callback Thunk entry for x64 (portasm version)
 License:

   Copyright (c) 2011-2022 Daniel Adler <dadler@uni-goettingen.de>,
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



#include "../portasm/portasm-x64.S"

/* structure sizes */

SET(DCThunk_size,24)
SET(DCArgs_size_win64,96) /* 8 (stack_ptr) + 4 (reg_count) + 4 (pad_w) + 4 (aggr_return_register) + 4 (pad) + 8*4 (int regs) + 8 (aggrs) + 8*4 (float regs) */
SET(DCArgs_size_sysv,144) /* 8 (stack_ptr) + 8 (reg_count) +             4 (aggr_return_register) + 4 (pad) + 8*6 (int regs) + 8 (aggrs) + 8*8 (float regs) */
SET(DCValue_size,8)
SET(DCRetRegs_SystemV_size,32)

/* frame local variable offsets relative to %rbp */

SET(FRAME_arg0_win64,48)
SET(FRAME_arg0_sysv,16)
SET(FRAME_return,8)
SET(FRAME_parent,0)

/* struct DCCallback layout, relative to ptr passed to functions below via RAX */

SET(CTX_thunk,0)
SET(CTX_handler,24)
SET(CTX_userdata,32)
SET(CTX_aggr_ret_reg,40)
SET(CTX_pad,44)
SET(CTX_aggrs_pp,48)
SET(DCCallback_size,56)



GLOBAL(dcCallback_x64_sysv)
BEGIN_PROC(dcCallback_x64_sysv)

	PUSH(RBP)
	MOV(RSP,RBP)

	/* initialize DCArgs */

	/* float parameters (8 registers spill to DCArgs.reg_data) */
	SUB(LIT(8*8),RSP)
	MOVSD(XMM7, QWORD(RSP,8*7))		/* DCArgs offset 136: float parameter 7 */
	MOVSD(XMM6, QWORD(RSP,8*6))		/* DCArgs offset 128: float parameter 6 */
	MOVSD(XMM5, QWORD(RSP,8*5))		/* DCArgs offset 120: float parameter 5 */
	MOVSD(XMM4, QWORD(RSP,8*4))		/* DCArgs offset 112: float parameter 4 */
	MOVSD(XMM3, QWORD(RSP,8*3))		/* DCArgs offset 104: float parameter 3 */
	MOVSD(XMM2, QWORD(RSP,8*2))		/* DCArgs offset  96: float parameter 2 */
	MOVSD(XMM1, QWORD(RSP,8*1))		/* DCArgs offset  88: float parameter 1 */
	MOVSD(XMM0, QWORD(RSP,8*0))		/* DCArgs offset  80: float parameter 0 */

	/* integer parameters (6 registers spill to DCArgs.reg_data) */
	PUSH(R9)				/* DCArgs offset 72: parameter 5 */
	PUSH(R8)				/* DCArgs offset 64: parameter 4 */
	PUSH(RCX)				/* DCArgs offset 56: parameter 3 */
	PUSH(RDX)				/* DCArgs offset 48: parameter 2 */
	PUSH(RSI)				/* DCArgs offset 40: parameter 1 */
	PUSH(RDI)				/* DCArgs offset 32: parameter 0 */

	MOV(QWORD(RAX, CTX_aggrs_pp), R8)
	PUSH(R8)				/* DCArgs offset 24: **aggrs */

	/* get val of aggr_return_register from DCCallback struct into r8's LSBs, clear */
	/* MSBs, write DCarg's pad and  aggr_return_register at once */
	MOVL(DWORD(RAX, CTX_aggr_ret_reg), R8D)	/* implicitly zeroes the high bits of R8 */
	PUSH(R8)				/* DCArgs offset 16: pad=0, aggr_return_register=DCCallback.aggr_return_register */
	/* register counts for integer/pointer and float regs */
	PUSH(LIT(0))				/* DCArgs offset 12: fcount */
						/* DCArgs offset  8: icount */

	LEA(QWORD(RBP,FRAME_arg0_sysv),RDX)	/* DCArgs offset  0: *stack_ptr */
	PUSH(RDX)

	MOV(RSP,RSI)				/* arg 1 RSI : DCArgs* */

	/* stack space for DCValue or DCRetRegs_SysV (passed to handler as DCValue*) and padding */
	SUB(LIT(4*8),RSP)			/* 4 qwords for DCRetRegs_SysV */

	/* call handler(*ctx, *args, *value, *userdata) - stack must be 16b aligned, here */
	MOV(RAX,RDI)				/* arg 0 RDI : DCCallback* (RAX) */
	MOV(QWORD(RDI,CTX_userdata),RCX)	/* arg 3 RCX : userdata* */
	MOV(RSP,RDX)				/* arg 2 RDX : DCValue*  */

	CALL_REG(QWORD(RAX,CTX_handler))

	/* get info about return type, use to select how to store reg-based retval */
	CMPL(LIT(-2/*see C*/), DWORD(RSP, 48)) /* rsp+48 = where r8 (aggr_return_register) was pushed */

	/* if retval is small aggregate via regs */
	JE(CSYM(scalar_retval))

		MOV(QWORD(RSP,0),RAX)
		MOV(QWORD(RSP,8),RDX)
		MOVSD(QWORD(RSP,16),XMM0) /* @@@AGGR needed to be put in xmm in this case? @@@ also not what doc/appendix says, actually */
		MOVSD(QWORD(RSP,24),XMM1) /* @@@AGGR needed to be put in xmm in this case? @@@ also not what doc/appendix says, actually */

	/* else (retval is int, float, or ptr to aggregate) */
	JMP(CSYM(epilog))
CSYM(scalar_retval):

		/* pass return type via registers, handle ints and floats */
		MOV(QWORD(RSP,0),RAX)
		MOVD(RAX,XMM0)

CSYM(epilog):

	MOV(RBP,RSP)
	POP(RBP)
	RET()

END_PROC(dcCallback_x64_sysv)


GLOBAL_FRAME(dcCallback_x64_win64)
FRAME_BEGIN_PROC(dcCallback_x64_win64)

	PUSH(RBP)
	FRAME_PUSH_REG(RBP)
	MOV(RSP,RBP)
	FRAME_SET(0, RBP)
	FRAME_ENDPROLOG()

	/* initialize DCArgs */

	/* float parameters (4 registers spill to DCArgs.reg_data) */
	SUB(LIT(4*8),RSP)
	MOVSD(XMM3, QWORD(RSP,8*3))		/* DCArgs offset  88: float parameter 3 */
	MOVSD(XMM2, QWORD(RSP,8*2))		/* DCArgs offset  80: float parameter 2 */
	MOVSD(XMM1, QWORD(RSP,8*1))		/* DCArgs offset  72: float parameter 1 */
	MOVSD(XMM0, QWORD(RSP,8*0))		/* DCArgs offset  64: float parameter 0 */

	/* integer parameters (4 registers spill to DCArgs.reg_data) */
	PUSH(R9)				/* DCArgs offset 56: parameter 3 */
	PUSH(R8)				/* DCArgs offset 48: parameter 2 */
	PUSH(RDX)				/* DCArgs offset 40: parameter 1 */
	PUSH(RCX)				/* DCArgs offset 32: parameter 0 */

	MOV(QWORD(RAX, CTX_aggrs_pp), R8)
	PUSH(R8)				/* DCArgs offset 24: **aggrs */

	/* get val of aggr_return_register from DCCallback struct into r8's LSBs, clear */
	/* MSBs, write DCarg's pad and  aggr_return_register at once */
	MOVL(DWORD(RAX, CTX_aggr_ret_reg), R8D)	/* implicitly zeroes the high bits of R8 */
	PUSH(R8)				/* DCArgs offset 16: pad=0, aggr_return_register=DCCallback.aggr_return_register */

	/* register counts for integer/pointer and float regs */
	PUSH(LIT(0))				/* DCArgs offset 12: pad_w */
						/* DCArgs offset  8: reg_count */

	LEA(QWORD(RBP,FRAME_arg0_win64),RDX)	/* DCArgs offset  0: *stack_ptr */
	PUSH(RDX)

	MOV(RSP,RDX)				/* arg 1 RDX : DCArgs* */

	/* space for retval (also aligns stack to 16b) */
	SUB(LIT(2*8),RSP)

	/* call handler(*ctx, *args, *value, *userdata) - stack must be 16b aligned, here */
	MOV(RAX,RCX)				/* arg 0 RCX : DCCallback* (RAX) */
	MOV(QWORD(RAX,CTX_userdata),R9)		/* arg 3 R9  : userdata* */
	MOV(RSP,R8)				/* arg 2 R8  : DCValue*  */

	/* spill area */
	SUB(LIT(4*8),RSP)			/* 4 qwords for spill area */

	CALL_REG(QWORD(RAX,CTX_handler))

	/* pass return type via registers, handle ints and floats */
	MOV(QWORD(RSP,4*8),RAX)
	MOVD(RAX,XMM0)

	MOV(RBP,RSP)
	POP(RBP)
	RET()

END_PROC(dcCallback_x64_win64)


END_ASM

/* vim: set ts=8: */

