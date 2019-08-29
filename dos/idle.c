// From https://stackoverflow.com/questions/31625723/waiting-in-dos-using-djgpp-alternatives-to-busy-wait

#include <stdio.h>
#include <dpmi.h>
#include <errno.h>

static unsigned int idle_type;

static int
haveDosidle(void)
{
    __dpmi_regs regs;
    regs.x.ax = 0x1680;
    __dpmi_int(0x28, &regs);
    return regs.h.al ? 0 : 1;
}

void init_idle()
{
    puts("checking idle methods:");

    fputs("yield (int 0x2f 0x1680): ", stdout);
    errno = 0;
    __dpmi_yield();

    if (errno)
    {
        puts("not supported.");
    }
    else
    {
        puts("supported.");
	idle_type = 1;
	return;
    }

    fputs("idle (int 0x28 0x1680): ", stdout);

    if (!haveDosidle())
    {
        puts("not supported.");
    }
    else
    {
        puts("supported.");
	idle_type = 2;
	return;
    }

    unsigned int ring;
    fputs("ring-0 HLT instruction: ", stdout);
    __asm__ ("mov %%cs, %0\n\t"
             "and $3, %0" : "=r" (ring));

    if (ring)
    {
        printf("not supported. (running in ring-%u)\n", ring);
    }
    else
    {
        puts("supported. (running in ring-0)");
	idle_type = 3;
	return;
    }

    idle_type = 0;
}

void yield()
{
    switch (idle_type) {
	case 0: return;
	case 1: __dpmi_yield(); return;
	case 2: haveDosidle(); return;
	case 3: asm("hlt"); return;
    }
}
