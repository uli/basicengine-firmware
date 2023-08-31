/*
   Everything that has to do with time, memory, non-BASIC code execution,
   and other functions related to the system underlying the interpreter.
 */

#include "basic.h"
#include "basic_native.h"
#include "credits.h"
#include "eb_video.h"
#include "eb_sys.h"

// **** RTC用宣言 ********************
#ifdef USE_INNERRTC
#include <Time.h>
#endif

void basic_init_environment() {
#ifdef __x86_64__
  setenv("HOSTTYPE", "x86_64", 1);
#elif defined(__arm__)
  setenv("HOSTTYPE", "arm", 1);
#else
  setenv("HOSTTYPE", "unknown", 1);
#endif

#ifdef SDL
  setenv("HOME", getenv("ENGINEBASIC_ROOT"), 1);
#elif defined(JAILHOUSE)
  // XXX: shouldn't that be the same for H3 without Jailhouse
  setenv("HOME", "/sd", 1);
#else
  setenv("HOME", "/", 1);
#endif

  setenv("TERM", "ansi", 1);
}

/***bf sys ENVIRON$
Returns the value of an environment variable.
\usage e$ = ENVIRON$(varname$)
\args
@varname$	name of environment variable
\ret
Value of the specified environment variable.
***/
BString Basic::senviron() {
  BString name;
  if (checkOpen())
    return name;
  name = istrexp();
  if (checkClose())
    return name;
  return BString(getenv(name.c_str()));
}

/***bc sys WAIT
Pause for a specific amount of time.
\usage WAIT ms
\args
@ms	time in milliseconds
\ref TICK()
***/
void Basic::iwait() {
  uint32_t tm;
  if (getParam(tm, 0, UINT32_MAX, I_NONE))
    return;
  eb_wait(tm);
}

void *BASIC_INT sanitize_addr(intptr_t vadr, int type) {
#ifdef ESP8266
  // Unmapped memory, causes exception
  if (vadr < 0x20000000UL) {
    E_ERR(VALUE, "unmapped address");
    return NULL;
  }
  // IRAM, flash: 32-bit only
  if ((vadr >= 0x40100000UL && vadr < 0x40300000UL) && type != 2) {
    E_ERR(VALUE, "non-32-bit access");
    return NULL;
  }
#else
  // anything goes
#endif
  if ((type == 1 && (vadr & 1)) ||
      (type == 2 && (vadr & 3))) {
    E_ERR(VALUE, "misaligned address");
    return NULL;
  }
  return (void *)vadr;
}

/***bf sys PEEK
Read a byte of data from an address in memory.
\usage v = PEEK(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
Memory at `addr` must allow byte-wise access.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEKD() PEEKW() PEEK$()
***/
/***bf sys PEEKW
Read a half-word (16 bits) of data from an address in memory.
\usage v = PEEKW(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
Memory at `addr` must allow byte-wise access, and `addr` must be 2-byte
aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEK() PEEKD() PEEK$()
***/
/***bf sys PEEKD
Read a word (32 bits) of data from an address in memory.
\usage v = PEEKD(addr)
\args
@addr	memory address
\ret Content of memory address.
\note
`addr` must be 4-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEK() PEEKW() PEEK$()
***/
uint32_t Basic::ipeek(int type) {
  uint32_t value = 0;
  intptr_t vadr;
  void *radr;

  if (checkOpen())
    return 0;
  vadr = iexp();
  if (checkClose())
    return 0;
  radr = sanitize_addr(vadr, type);
  if (radr) {
    switch (type) {
    case 0: value = *(uint8_t *)radr; break;
    case 1: value = *(uint16_t *)radr; break;
    case 2: value = *(uint32_t *)radr; break;
    default: err = ERR_SYS; break;
    }
  } else
    err = ERR_RANGE;
  return value;
}

/***bc sys POKE
Write a byte to an address in memory.
\usage POKE addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow byte-wise access.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKED POKEW POKE$
***/
/***bc sys POKEW
Write a half-word (16 bits) to an address in memory.
\usage POKEW addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow half-word-wise access.
It must be 2-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKE POKED POKE$
***/
/***bc sys POKED
Write a word (32 bits) to an address in memory.
\usage POKED addr, value
\args
@addr	memory address
@value	value to be written
\note
`addr` must be mapped writable and must allow half-word-wise access.
It must be 4-byte aligned.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKE POKEW POKE$
***/
void BASIC_FP Basic::do_poke(int type) {
  void *adr;
  uint32_t value;
  intptr_t vadr;

  // アドレスの指定
  vadr = iexp();
  if (err)
    return;
  if (*cip != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }

  // 例: 1,2,3,4,5 の連続設定処理
  do {
    adr = sanitize_addr(vadr, type);
    if (!adr) {
      err = ERR_RANGE;
      break;
    }
    cip++;  // 中間コードポインタを次へ進める
    if (getParam(value, I_NONE))
      return;
    switch (type) {
    case 0: *((uint8_t *)adr) = value; vadr += 1; break;
    case 1: *((uint16_t *)adr) = value; vadr += 2; break;
    case 2: *((uint32_t *)adr) = value; vadr += 4; break;
    default: err = ERR_SYS; break;
    }
  } while (*cip == I_COMMA);
}

void BASIC_FP Basic::ipoke() {
  do_poke(0);
}
void BASIC_FP Basic::ipokew() {
  do_poke(1);
}
void BASIC_FP Basic::ipoked() {
  do_poke(2);
}

/***bc sys POKE$
Write a byte string to an address in memory.
\usage POKE$ addr, data$
\args
@addr	memory address
@data$	data to be written
\note
* `addr` must be mapped writable and must allow byte-wise access.
* When writing a string that is to be used by a C function you have to make
  sure to add a `0`-byte at the end, e.g. `"text" + CHR$(0)`.
\bugs
Sanity checks for `addr` are insufficient.
\ref POKE POKED POKEW
***/
void Basic::ipokestr() {
  intptr_t vadr = iexp();
  if (err)
    return;
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return;
  }

  BString out_str = istrexp();
  uint8_t *out = (uint8_t *)vadr;

  uint8_t *in = (uint8_t *)out_str.c_str();
  for (int i = 0; i < out_str.length(); ++i, ++in, ++out) {
    *out = *in;
  }
}

/***bc sys SYS
Call a machine language routine.

WARNING: Using this command incorrectly may crash the system, or worse.
\usage SYS addr
\args
@addr	a memory address mapped as executable
\bugs
No sanity checks are performed on the address.
***/
void Basic::isys() {
  void (*sys)() = (void (*)())(uintptr_t)iexp();
  sys();
}

/***bc sys SET DATE
Sets the current date and time.
\usage SET DATE year, month, day, hour, minute, second
\args
@year	numeric expression [`1900` to `2036`]
@month	numeric expression [`1` to `12`]
@day	numeric expression [`1` to `31`]
@hour	numeric expression [`0` to `23`]
@minute	numeric expression [`0` to `59`]
@second	numeric expression [`0` to `61`]
\bugs
It is unclear why the maximum value for `second` is 61 and not 59.
\ref DATE GET_DATE
***/
void Basic::isetDate() {
#ifdef USE_INNERRTC
  int32_t p_year, p_mon, p_day;
  int32_t p_hour, p_min, p_sec;

  if ( getParam(p_year, 1900,2036, I_COMMA) ) return;  // 年
  if ( getParam(p_mon,     1,  12, I_COMMA) ) return;  // 月
  if ( getParam(p_day,     1,  31, I_COMMA) ) return;  // 日
  if ( getParam(p_hour,    0,  23, I_COMMA) ) return;  // 時
  if ( getParam(p_min,     0,  59, I_COMMA) ) return;  // 分
  // 61? WTF?
  if ( getParam(p_sec,     0,  61, I_NONE)) return;  // 秒

  setTime(p_hour, p_min, p_sec, p_day, p_mon, p_year);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void Basic::iset() {
  int32_t flag, val;

  if (*cip == I_DATE) {
    ++cip;
    isetDate();
    return;
  } else if (*cip == I_FLAGS) {
    ++cip;
    if (getParam(flag, 0, 0, I_COMMA))
      return;
    if (getParam(val, 0, 1, I_NONE))
      return;

    switch (flag) {
    case 0:	math_exceptions_disabled = val; break;
    default:	break;
    }
  } else {
    SYNTAX_T(_("expected DATE or FLAGS"));
  }
}

/***bc sys GET DATE
Get the current date.

\usage GET DATE year, month, day, weekday
\args
@year		numeric variable
@month		numeric variable
@day		numeric variable
@weekday	numeric variable
\ret The current date in the given numeric variables.
\bugs
Only supports scalar variables, not array or list members.

WARNING: The syntax and semantics of this command are not consistent with
other BASIC implementations and may be changed in future releases.
\ref DATE GET_TIME SET_DATE
***/
void Basic::igetDate() {
#ifdef USE_INNERRTC
  int16_t index;
  time_t tt = now();

  int v[] = {
    year(tt),
    month(tt),
    day(tt),
    weekday(tt),
  };

  for (uint8_t i = 0; i < 4; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++;
      index = *cip;               // 変数インデックスの取得
      nvar.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if (i != 3) {
      if (*cip != I_COMMA) {      // ','のチェック
        err = ERR_SYNTAX;
        return;
      }
      cip++;
    }
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc sys GET TIME
Get the current time.

\usage GET TIME hour, minute, second
\args
@hour	numeric variable
@minute	numeric variable
@second	numeric variable
\ret The current time in the given numeric variables.
\bugs
Only supports scalar variables, not array or list members.

WARNING: The syntax and semantics of this command are not consistent with
other BASIC implementations and may be changed in future releases.
\ref SET_DATE
***/
void Basic::igetTime() {
#ifdef USE_INNERRTC
  int16_t index;
  time_t tt = now();

  int v[] = {
    hour(tt),
    minute(tt),
    second(tt),
  };

  for (uint8_t i = 0; i < 3; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++;
      index = *cip;               // 変数インデックスの取得
      nvar.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if (i != 2) {
      if (*cip != I_COMMA) {      // ','のチェック
        err = ERR_SYNTAX;
        return;
      }
      cip++;
    }
  }
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

void Basic::iget() {
  if (*cip == I_DATE) {
    ++cip;
    igetDate();
  } else if (*cip == I_TIME) {
    ++cip;
    igetTime();
  } else {
    SYNTAX_T(_("expected DATE or TIME"));
  }
}

/***bc sys DATE
Prints the current date and time.

WARNING: The semantics of this command are not consistent with other BASIC
implementations and may be changed in future releases.
\usage DATE
\ref GET_DATE SET_DATE
***/
// XXX: 32 byte jump table
void NOJUMP Basic::idate() {
#ifdef USE_INNERRTC
  time_t tt = now();

  putnum(year(tt), -4);
  c_putch('/');
  putnum(month(tt), -2);
  c_putch('/');
  putnum(day(tt), -2);
  PRINT_P(" [");
  switch (weekday(tt)) {
  case 1: PRINT_P("Sun"); break;
  case 2: PRINT_P("Mon"); break;
  case 3: PRINT_P("Tue"); break;
  case 4: PRINT_P("Wed"); break;
  case 5: PRINT_P("Thu"); break;
  case 6: PRINT_P("Fri"); break;
  case 7: PRINT_P("Sat"); break;
  };
  PRINT_P("] ");
  putnum(hour(tt), -2);
  c_putch(':');
  putnum(minute(tt), -2);
  c_putch(':');
  putnum(second(tt), -2);
  newline();
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc sys CREDITS
Prints information on Engine BASIC's components, its authors and
licensing conditions.
\usage CREDITS
***/
void Basic::icredits() {
  c_puts_P(__credits);
}

/***bc sys XYZZY
Run Z-machine program.
\usage XYZZY file_name$
\args
@file_name$	name of a z-code file
***/
void Basic::ixyzzy() {
  if (!is_strexp()) {
    PRINT_P("Nothing happens.\n");
    return;
  }
  BString game = getParamFname();
  if (err)
    return;
  struct stat st;
  if (_stat(game.c_str(), &st)) {
    err = ERR_FILE_OPEN;
    return;
  }

  std::list<BString> args;
  args.push_back("frotz");
  args.push_back(game);
  shell_list(args);
}

/***bf sys SYS
Retrieve internal system addresses and parameters.
\usage a = SYS(item)
\args
@item	number of the item of information to be retrieved
\ret Requested information.
\sec ITEMS
The following internal information can be retrieved using `SYS()`:
\table
| `0` | memory address of BASIC program buffer
| `1` | memory address of current font
| `2` | system type; `0` for original (ESP8266), `1` for Shuttle (ESP32),
        `2` for NG (H3 bare-metal), `3` for LT (Linux-based)
\endtable
***/
num_t Basic::nsys() {
  int32_t item = getparam();
  if (err)
    return 0;
  switch (item) {
  case 0:	return (num_t)(intptr_t)listbuf;
  case 1:	return (num_t)(intptr_t)sc0.getfontadr();
  case 2:
#ifdef ESP8266
                return 0;
#elif defined(ESP32)
                return 1;
#elif defined(H3)
                return 2;
#elif defined(__linux__)
                return 3;
#else
#warning undefined system
                return -1;
#endif
  default:	E_VALUE(0, 2); return 0;
  }
}

#ifdef __DJGPP__
#include <dpmi.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

uint64_t SMALL Basic::getFreeMemory() {
#ifdef ESP8266
  return umm_free_heap_size();
#elif defined(H3)
  return sys_mem_free();
#elif defined(__DJGPP__)
  return _go32_dpmi_remaining_physical_memory();
#elif defined(__linux__)
  struct sysinfo si;
  if (!sysinfo(&si)) {
    return (si.freeram + si.bufferram) * si.mem_unit;
  } else {
    return -1;
  }
#else
  return -1;
#endif
}

/***bf bas FREE
Get free memory size.
\usage bytes = FREE
\ret Number of bytes free.
\note
The alternative syntax `FREE()` is supported for backwards compatibility
with earlier versions of Engine BASIC.
***/
num_t BASIC_FP Basic::nfree() {
  // backwards compatibility with FREE() syntax
  if (*cip == I_OPEN && cip[1] == I_CLOSE)
    cip += 2;

  return getFreeMemory();
}

/***bf sys TICK
Returns the elapsed time since power-on.
\usage tim = TICK([unit])
\args
@unit	unit of time [`0` for milliseconds, `1` for seconds, default: `0`]
\ret Time elapsed.
***/
num_t BASIC_FP Basic::ntick() {
  num_t value;
  if ((*cip == I_OPEN) && (*(cip + 1) == I_CLOSE)) {
    // 引数無し
    value = 0;
    cip += 2;
  } else {
    value = getparam(); // 括弧の値を取得
    if (err)
      return 0;
  }
  if (value == 0) {
    value = millis();              // 0～INT32_MAX ms
  } else if (value == 1) {
    value = millis() / 1000;       // 0～INT32_MAX s
  } else {
    E_VALUE(0, 1);
  }
  return value;
}

num_t BASIC_FP Basic::npeek() {
  return ipeek(0);
}
num_t BASIC_FP Basic::npeekw() {
  return ipeek(1);
}
num_t BASIC_FP Basic::npeekd() {
  return ipeek(2);
}

/***bf sys PEEK$
Reads a byte string of data from an address in memory.
\usage data$ = PEEK$(addr, length)
\args
@addr	memory address
@length	number of bytes to read, or 0
\ret Byte string containing `length` bytes of data starting at `addr`.
\note
* Memory at `addr` must allow byte-wise access.
* When `length` is `0`, data is read until a `0`-byte is encountered. Use
  this to read C-style strings.
\bugs
Sanity checks for `addr` are insufficient.
\ref PEEK() PEEKD() PEEKW()
***/
BString Basic::speek() {
  uint32_t value = 0;
  int len = 0;
  intptr_t vadr;
  char *radr;
  BString in;

  if (checkOpen())
    return in;
  vadr = iexp();
  if (*cip++ != I_COMMA) {
    E_SYNTAX(I_COMMA);
    return in;
  }
  if (getParam(len, I_CLOSE))
    return in;
  if (len < 0) {
    err = ERR_RANGE;
    return in;
  }

  radr = (char *)sanitize_addr(vadr, 1);

  if (len == 0) {
    while (*radr) {
      in.concat(*radr++);
    }
  } else {
    for (int i = 0; i < len; ++i) {
      in.concat(*radr++);
    }
  }

  return in;
}

/***bc sys SYSINFO
Displays internal information about the system.

WARNING: This command may be renamed in the future to reduce namespace
pollution.
\usage SYSINFO
\desc
`SYSINFO` displays the following information:

* Program size,
* statistics on scalar, array and list variables, both numeric and string,
* loop and return stack depths,
* CPU stack pointer address,
* free memory size and
* video timing information (nominal and actual cycles per frame).
***/
void SMALL Basic::isysinfo() {
  char top = 't';
  intptr_t adr = (intptr_t)&top;

  PRINT_P("Program size: ");
  putnum(size_list, 0);
  newline();

  newline();
  PRINT_P("Variables:\n");

  PRINT_P(" Numerical: ");
  putnum(nvar.size(), 0);
  PRINT_P(", ");
  putnum(num_arr.size(), 0);
  PRINT_P(" arrays, ");
  putnum(num_lst.size(), 0);
  PRINT_P(" lists\n");

  PRINT_P(" Strings:   ");
  putnum(svar.size(), 0);
  PRINT_P(", ");
  putnum(str_arr.size(), 0);
  PRINT_P(" arrays, ");
  putnum(str_lst.size(), 0);
  PRINT_P(" lists\n");

  newline();
  PRINT_P("Loop stack:   ");
  putnum(lstki, 0);
  newline();
  PRINT_P("Return stack: ");
  putnum(gstki, 0);
  newline();

  // スタック領域先頭アドレスの表示
  newline();
  PRINT_P("CPU stack: ");
  putHexnum(adr, sizeof(intptr_t));
  newline();

#ifdef USE_VS23
  newline();
  PRINT_P("Video timing: ");
  putint(vs23.cyclesPerFrame(), 0);
  PRINT_P(" cpf (");
  putint(vs23.cyclesPerFrameCalculated(), 0);
  PRINT_P(" nominal)\n");
#endif
}

#ifdef ESP8266
#include <eboot_command.h>
#include <spi_flash.h>
#endif
/***bc sys BOOT
Reboots the system.
\usage BOOT
\note
Only implemented on the H3 platform.
***/
void Basic::iboot() {
#if defined(H3)
#ifdef JAILHOUSE
  system("reboot");
#else
  sys_reset();
#endif
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc sys CPUSPEED
Sets the CPU clock frequency.
\usage CPUSPEED speed
\args
@speed	CPU speed in percent of maximum.
\note
* Only implemented on the H3 platform.
* Will not generate an error on platforms that do not support it.
* When `speed` is `-1` the CPU speed will be reset to the power-on default.
***/
void Basic::icpuspeed() {
  int percent = iexp();
  if (err)
    return;
  eb_set_cpu_speed(percent);
}

void syspanic(const char *txt) {
  redirect_output_file = -1;
  c_puts_P(txt);
  PRINT_P("\nSystem halted");
  Serial.println(txt);
  Serial.println(F("System halted"));
  for (;;);
}

void Basic::isystem() {
#ifdef __unix__
  ::exit(0);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

/***bc sys INSTALL
Installs a packaged module to the system module directory.
\usage INSTALL filename$
\args
@filename$	name of a packaged module file
\ref LISTMOD LOADMOD
***/
void Basic::iinstall() {
  BString mod = getParamFname();
  eb_install_module(mod.c_str());
}

/***bc sys LOADMOD
Loads a module from the system module directory.
\usage LOADMOD module$
\args
@module$	name of an installed module
\ref INSTALL LISTMOD
***/
void Basic::iloadmod() {
  BString mod = istrexp();
  eb_load_module(mod.c_str());
}

/***bc sys LISTMOD
Shows the names of all modules currently loaded.
\usage LISTMOD
\ref INSTALL LOADMOD
***/
void Basic::ilistmod() {
  const char *name;
  for (int i = 0; (name = eb_module_name(i)); ++i) {
    c_printf("%s\n", name);
  }
}

/***bc sys #REQUIRE
Ensures that a given module is loaded.
\usage #REQUIRE "module_name"
\args
@"module_name"	module name
\desc
Checks if a module called `module_name` has alredy been loaded. If not, it
will search for the module in `/sys/modules/module_name` and load it. If no
module is found, it will generate an error.
\note
The behavior of `#REQUIRE` differs from that of other BASIC commands. It is
executed as soon as it is spotted by the interpreter, for instance when
loading a program.

This means that `#REQUIRE` will already generate an error when loading a
program if the specified module cannot be found.

When executed during normal program flow, `#REQUIRE` will not do anything.
\ref INSTALL LOADMOD
***/
void Basic::irequire() {
  istrexp();
}

#include <list>

#ifdef __unix__
#include <pty.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifdef JAILHOUSE
#include <errno.h>
#include <sys/wait.h>

// no termios.h in newlib
struct winsize {
        unsigned short ws_row;
        unsigned short ws_col;
        unsigned short ws_xpixel;
        unsigned short ws_ypixel;
};

extern "C" pid_t jhlibc_forkptyexec(int *, struct winsize *, char * const *);
#endif

#if defined(__unix__) || defined(JAILHOUSE)
int shell_list(std::list<BString>& args) {
  int fd;
  int wstatus = 0;

  struct winsize ws = {
    (unsigned short)eb_csize_height(),
    (unsigned short)eb_csize_width(),
    (unsigned short)eb_psize_width(),
    (unsigned short)eb_psize_height()
  };

#ifdef JAILHOUSE

  std::vector<const char *> argsp;

  for (auto &s : args)
    argsp.push_back(s.c_str());

  argsp.push_back(NULL);

  pid_t pid = jhlibc_forkptyexec(&fd, &ws, (char * const *)argsp.data());

#else

  pid_t pid = forkpty(&fd, NULL, NULL, &ws);

  if (pid == 0) {
    // shell
    unsetenv("DISPLAY");
    setenv("TERM", "cons25-debian", 1);
    setenv("LANG", "en_US.UTF-8", 1);
    setenv("HOME", "/sd", 1);
    if (args.size() == 0)
      execl("/bin/sh", "sh", NULL);
    else if (args.size() == 1)
      execl("/bin/sh", "sh", "-c", args.front().c_str(), (char *) 0);
    else
      exec_list(args);
    _exit(1);
  }
  else
#endif	// JAILHOUSE
  if (pid < 0) {
    // error
    err = ERR_COM;
  } else {
    // us
#ifndef JAILHOUSE	// JH libc server will do that for us
    fcntl(fd, F_SETFL, O_NONBLOCK);
#endif

    char buf[256] = { 0 };

    eb_show_cursor(1);

    for (;;) {
      int ret = read(fd, buf, 256);
      if (ret < 0) {
        if (errno != EAGAIN)
          break;
#ifdef JAILHOUSE
        yield();
#else
        usleep(10000);
#endif
      } else if (ret == 0) {
        break;
      } else if (ret > 0) {
        for (int i = 0; i < ret; ++i)
          eb_term_putch(buf[i]);
      }

      process_events();

      int ch;
      ret = 0;

      while ((ch = eb_term_getch()) >= 0 && ret < 256) {
        buf[ret++] = ch;
      }

      if (ret > 0)
        write(fd, buf, ret);
    }
    close(fd);
    wait(&wstatus);

    if (WEXITSTATUS(wstatus) != 0)
      err = ERR_OS;
  }

  return WEXITSTATUS(wstatus);
}
#else	// __unix__
void shell_list(std::list<BString>& args) {
  err = ERR_NOT_SUPPORTED;
}
#endif // __unix__

/***bc sys SHELL
Runs operating system commands.
\usage SHELL [<command>[, <argument> ...]] [OFF]
\args
@command	command to run
@argument	argument to pass to the command
\desc
Runs `command`, or an interactive shell if `command` is not specified.

`command` is executed by the operating system shell if there are no
`argument` parameters, allowing shell syntax constructs to be used.

If one or more `argument` parameters are specified,
the command will be executed directly.

If the `OFF` keyword is added, Engine BASIC will shut down its graphics,
sound and input system to allow the executed program to use these
facilities.
\note
* `command`-only commands are executed using `execl("/bin/sh", "sh", ...)`.
* Commands with `argument` parameters are executed using
  `execvp("<command>", "<command>", "<argument">, ...)`.
***/
void Basic::ishell() {
#if defined(__unix__) || defined(JAILHOUSE)
  std::list<BString> args;
  bool screen_off = false;

  while (!end_of_statement()) {
    args.push_back(istrexp());
    if (*cip == I_COMMA)
      ++cip;
    else if (*cip == I_OFF) {
#ifdef JAILHOUSE
      err = ERR_NOT_SUPPORTED;
      return;
#else
      ++cip;
      screen_off = true;
      break;
#endif
    }
  }

#ifdef JAILHOUSE
  shell_list(args);
#else
  if (screen_off) {
    vs23.end();
    run_list(args);
    vs23.restart();
  } else {
    shell_list(args);
  }
#endif

#else
  err = ERR_NOT_SUPPORTED;
#endif
}

#if !defined(__linux__) && !defined(JAILHOUSE)
void Basic::idtbload() {
  err = ERR_NOT_SUPPORTED;
}
#endif
