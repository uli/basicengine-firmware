/*

 Package: dyncall
 Library: portasm
 File: portasm/portasm-x64.S
 Description: Portable Assembler Macros for X64
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



/* Common macros. */
#define XCONCAT(A,B) A##B
#if defined(GEN_MASM)
/* MASM syntax. */
.CODE
#  define BEGIN_ASM
#  define END_ASM END
#  define CSYM(X) X
#  define GLOBAL(X)       X PROC
#  define GLOBAL_FRAME(X) X PROC FRAME
#  define BEGIN_PROC(X)       OPTION PROLOGUE:NONE, EPILOGUE:NONE
#  define FRAME_BEGIN_PROC(X) OPTION EPILOGUE:NONE
#  define FRAME_SET(OFFSET, X)  .setframe X, OFFSET
#  define FRAME_PUSH_REG(X)   .pushreg X
#  define FRAME_ENDPROLOG()   .endprolog
#  define END_PROC(X)   X ENDP
#  define PUSH(R) push R
#  define POP(R) pop R
#  define MOV(S,D)  mov D,S
#  define MOVB(S,D) mov D,S
#  define MOVL(S,D) mov D,S
#  define MOVZXQ(S,D) movzx D,S
#  define ADDL(S,D) add D,S
#  define ANDL(S,D) and D,S
#  define SUBL(S,D) sub D,S
#  define SHRL(S,D) shr D,S
#  define SHLQ(S,D) shl D,S
#  define ADD(S,D)  add D,S
#  define AND(S,D)  and D,S
#  define SUB(S,D)  sub D,S
#  define SHR(S,D)  shr D,S
#  define MOVD(S,D) movd D,S
#  define XOR(S,D) xor D,S
#  define RET() ret
#  define CALL_DWORD(R,OFF) call DWORD(R,OFF)
#  define REP(X) rep X
#  define MOVSB movsb
#  define MOVSW movsw
#  define MOVSD(S,D)  movsd D,S
#  define MOVSDX(S,D) movsdx D,S
#  define BYTE(R) byte ptr [R]
#  define DWORD(R,OFF) dword ptr [R+OFF]
#  define QWORD(R,OFF) qword ptr [R+OFF]
#  define LIT(X) X
#  define INT(X) int X
#  define HEX(X) XCONCAT(X,h)
#  define CALL(X) call X
#  define CALL_REG(X) call X
#  define LEA(S,D) lea D,S
#  define SET(K,V) K = V
#  define JE(X) je X
#  define JNE(X) jne X
#  define JMP(X) jmp X
#  define CMPL(A,B) cmp B,A
#  define LOCAL(X) X
/* @@@ check if masm support wanted/needed */
#  define SECTION_NOTE_NXSTACK
#else
/* GNU/SunPro Assembler AT&T Syntax */
	.text
#  define BEGIN_ASM
#  define END_ASM
#  include "../dyncall/dyncall_macros.h"
#  if defined(DC__OS_Darwin)
#    define CSYM(X) _##X
#  else
#    define CSYM(X) X
#  endif
#  define RAX %rax
#  define RBX %rbx
#  define RCX %rcx
#  define RDX %rdx
#  define RSI %rsi
#  define RDI %rdi
#  define RBP %rbp
#  define RSP %rsp
#  define R8   %r8
#  define R8D  %r8d
#  define R9   %r9
#  define R10  %r10
#  define R11  %r11
#  define R12  %r12
#  define R13  %r13
#  define R14  %r14
#  define R15  %r15
#  define XMM0 %xmm0
#  define XMM1 %xmm1
#  define XMM2 %xmm2
#  define XMM3 %xmm3
#  define XMM4 %xmm4
#  define XMM5 %xmm5
#  define XMM6 %xmm6
#  define XMM7 %xmm7
#  define AL   %al
#  define AH   %ah
#  define BL   %bl
#  define BH   %bh
#  define CL   %cl
#  define CH   %ch
#  define DL   %dl
#  define DH   %dh
#  define GLOBAL(X)       .globl CSYM(X)
#  define GLOBAL_FRAME(X) .globl CSYM(X)
#  define BEGIN_PROC(X)       CSYM(X):
#  define FRAME_BEGIN_PROC(X) CSYM(X):
#  define FRAME_SET(OFFSET, X)
#  define FRAME_PUSH_REG(X)
#  define FRAME_ENDPROLOG()
#  define END_PROC(X)
#  define PUSH(R) pushq R
#  define POP(R) popq R
#  define MOV(S,D)  movq S,D
#  define MOVB(S,D) movb S,D
#  define MOVL(S,D) movl S,D
#  define MOVZXQ(S,D) movzbq S,D
#  define ADDL(S,D) addl S,D
#  define ANDL(S,D) andl S,D
#  define SUBL(S,D) subl S,D
#  define SHRL(S,D) shrl S,D
#  define SHLQ(S,D) shlq S,D
#  define ADD(S,D) addq S,D
#  define AND(S,D) andq S,D
#  define SUB(S,D) subq S,D
#  define SHR(S,D) shrq S,D
#  define MOVD(S,D) movd S,D
#  define XOR(S,D) xorq S,D
#  define RET() ret
#  define CALL_DWORD(R,OFF) call *DWORD(R,OFF)
#  define REP(X) rep X
#  define MOVSB movsb
#  define MOVSW movsw
#  define MOVSD(S,D) movsd S,D
#  define BYTE(R)  (R)
#  define DWORD(R,OFF) OFF(R)
#  define QWORD(R,OFF) OFF(R)
#  define LIT(X) $X
#  define INT(X) int X
#  define HEX(X) XCONCAT(0x,X)
#  define CALL(X) call X
#  define CALL_REG(X) call *X
#  define LEA(A,B) lea A,B
#  define CMPL(A,B) cmpl A,B
#  define JE(X)    je X
#  define JNE(X)    jne X
#  define JMP(X)    jmp X
#  define FLDS(X)  flds X
#  define FLDL(X)  fldl X
/* not available on some platforms, e.g. old solaris, so use K=V syntax #  define SET(K,V) .set K,V */
#  define SET(K,V) K = V
#  define LOCAL(X) .X
#endif

