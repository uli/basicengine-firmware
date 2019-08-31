#include "def.h"

unsigned short sel;
void pds_dpmi_dos_freemem()
{
	struct dosmem_t *dm = &dosmem_t;
	/*unsigned short*/ sel = dm->selector;

#ifdef __DJGPP__
	asm ("movw %0,%%dx" ::"m" (sel));
	asm ("movw $0x101,%ax");
	asm ("int $0x31");

#else   /*WATCOM*/
	_asm {
		mov ax, 101h
		mov dx, sel
		int 31h
	}
#endif
}

char *lin;
struct dosmem_t *pds_dpmi_dos_allocmem(unsigned int size)
{
	struct dosmem_t *dm = &dosmem_t;

#ifdef __DJGPP__
#ifndef ZDM
	if (!__djgpp_nearptr_enable()) return NULL;
#endif

	asm("push %eax");
	asm("push %ebx");
	asm("push %ecx");
	asm("push %edx");
	asm ("movl %0,%%ebx" ::"m" (size));
	asm ("movw $0x100,%ax\n"
	     "addl $16,%ebx\n"
	     "shrl $4,%ebx\n"
	     "int $0x31\n"
	     "jnc go\n"
	     "xorl %edx,%edx\n"
	     "go:\n"
	     "movzx %ax,%eax\n"
	     "shll $4,%eax");
	asm ("movl %%eax,%0" : "=m" (lin));
	asm ("movw %%dx,%0" : "=m" (sel));
	asm("pop %edx");
	asm("pop %ecx");
	asm("pop %ebx");
	asm("pop %eax");

#else   /*WATCOM*/
	_asm {
		mov ax, 100h
		mov ebx, size
		add ebx, 16
		shr ebx, 4
		int 31h
		jnc go
		xor edx, edx
 go:
		movzx eax, ax
		shl eax, 4
		mov lin, eax;
		mov sel, dx
	}
#endif

	dm->linearptr = lin;
	dm->selector = sel;
	return (sel) ? dm : NULL;
}

unsigned long pds_dpmi_map_physical_memory(unsigned long phys_addr, unsigned long memsize)
{
#ifdef ZDM
	__dpmi_meminfo mi;
	mi.address = phys_addr;
	mi.size = memsize;
	if (__dpmi_physical_address_mapping(&mi) != 0) return 0;
	map_selector = __dpmi_allocate_ldt_descriptors(1);
	__dpmi_set_segment_base_address(map_selector, mi.address);
	__dpmi_set_segment_limit(map_selector, mi.size - 1);
	return mi.address;

#else
	union REGS regs;
	memset(&regs, 0, sizeof(union REGS));
	regs.w.ax = 0x0800;
	regs.w.bx = (phys_addr >> 16);
	regs.w.cx = (phys_addr & 0xffff);
	regs.w.di = (memsize & 0xffff);
	regs.w.si = (memsize >> 16);
	int386(0x31, &regs, &regs);
	if (regs.w.cflag) return 0;
	return (((unsigned long)regs.w.bx << 16) | (regs.w.cx & 0xffff))
#ifdef __DJGPP__
	       + __djgpp_conventional_base
#endif
	;
#endif
}

void pds_dpmi_unmap_physycal_memory(unsigned long linear_addr)
{
#ifdef ZDM
	__dpmi_meminfo mi;
	mi.address = linear_addr;
	__dpmi_free_physical_address_mapping(&mi);

#else
	union REGS regs;
	memset(&regs, 0, sizeof(union REGS));
	regs.w.ax = 0x0801;
	regs.w.bx = (linear_addr >> 16);
	regs.w.cx = (linear_addr & 0xffff);
	int386(0x31, &regs, &regs);
#endif
}
