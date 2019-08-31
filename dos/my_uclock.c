/* Modified from the original to try RDTSC even if the OS is not NT+. */

/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 2001 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <libc/stubs.h>
#include <time.h>
#include <errno.h>
#include <pc.h>
#include <libc/farptrgs.h>
#include <go32.h>
#include <dpmi.h>
#include <libc/bss.h>
#include <dos.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/exceptn.h>

typedef void (*SignalHandler) (int);

/* Catch rdtsc exception if opcode not legal and always return 0LL */
static void catch_rdtsc(int val)
{
  short *eip = (short *)__djgpp_exception_state->__eip;
  if (*eip == 0x310f) {					/* rdtsc opcode */
    __djgpp_exception_state->__eip += 2;		/* skip over it */
    __djgpp_exception_state->__edx = 0;			/* EDX = 0 */
    longjmp(__djgpp_exception_state, 0);		/* EAX = 0 */
  }
  return;
}

static int uclock_bss = -1;

/* tics = about 18.2 * 65536 (1,192,755)

   actually, it's   0x1800b0 tics/day (FAQ)
                  / 24*60*60 sec/day
                  * 65536 utics/tic
		  = 1,193,180 utics/sec */

uclock_t
my_uclock(void)
{
  static uclock_t base = 0;
  static unsigned long last_tics = 0;
  unsigned char lsb, msb;
  unsigned long tics, otics;
  uclock_t rv;

  _farsetsel(_dos_ds);

  if (1) {//_os_trueversion == 0x532) {	/* Windows NT, 2000, XP */
    static double multiplier;
    static unsigned long btics;

    if (uclock_bss != __bss_count) {
      SignalHandler saveold;
      saveold = signal(SIGILL, catch_rdtsc);
      tics = _farnspeekl(0x46c);
      while ( (btics = _farnspeekl(0x46c)) == tics);
      base = _rdtsc();
      signal(SIGILL, saveold);
      if (base == 0)
        multiplier = 0.0;
      else {
        while ( (tics = _farnspeekl(0x46c)) == btics);
        if (tics < btics) tics = btics + 1;		/* Midnight */
        multiplier = ((tics - btics)*65536) / (double)(_rdtsc() - base);
        uclock_bss = __bss_count;
      }
    }
    if (multiplier != 0.0) {
      rv = (_rdtsc() - base) * multiplier;
      tics = _farnspeekl(0x46c) - btics;
      while (tics <= 0) tics += 0x1800b0;		/* Midnight */
      if ((unsigned long)(rv >> 16) != tics) {		/* Recalibrate */
        rv = ((uclock_t)tics) << 16;
        multiplier = (tics*65536) / (double)(_rdtsc() - base);
      }
      return rv;
    }
  }

  if (uclock_bss != __bss_count)
  {
    int e = errno;

    /* switch the timer to mode 2 (rate generator) */
    /* rather than mode 3 (square wave), which doesn't count linearly. */

    outportb(0x43, 0x34);
    outportb(0x40, 0xff);
    outportb(0x40, 0xff);

    base = 0;
    last_tics = 0;
    uclock_bss = __bss_count;

    /* It seems like Windows 9X virtualization of the timer device
       delays the actual execution of the above command until the
       next timer tick.  Or maybe it only consults the actual device
       once per tick.  In any case, the values returned during the
       first 55 msec after the timer was reprogrammed still look as
       if the timer worked in mode 3.  So we simply wait for one clock
       tick when we run on Windows.  */
    otics = _farnspeekl(0x46c);
    do {
      errno = 0;
      __dpmi_yield();	/* will set errno to ENOSYS on plain DOS */
    } while (errno == 0 && _farnspeekl(0x46c) == otics);
    errno = e;
  }

  /* Make sure the numbers we get are consistent */
  do {
    otics = _farnspeekl(0x46c);
    outportb(0x43, 0x00);
    lsb = inportb(0x40);
    msb = inportb(0x40);
    tics = _farnspeekl(0x46c);
  } while (otics != tics);

  /* calculate absolute time */
  msb ^= 0xff;
  lsb ^= 0xff;
  rv = ((uclock_t)tics << 16) | (msb << 8) | lsb;

  if (base == 0L)
    base = rv;

  if (last_tics > tics) /* midnight happened */
    base -= 0x1800b00000LL;

  last_tics = tics;

  /* return relative time */
  return rv - base;
}
