/*
   Everything that has to do with time, memory, non-BASIC code execution,
   and other functions related to the system underlying the interpreter.
 */

#include "basic.h"
#include "credits.h"

// **** RTC用宣言 ********************
#ifdef USE_INNERRTC
  #include <Time.h>
#endif

/***bc sys WAIT
Pause for a specific amount of time.
\usage WAIT ms
\args
@ms	time in milliseconds
\ref TICK()
***/
void Basic::iwait() {
  int32_t tm;
  if ( getParam(tm, 0, INT32_MAX, I_NONE) ) return;
  uint32_t end = tm + millis();
  while (millis() < end) {
    process_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      break;
    }
    yield();
  }
}

void* BASIC_INT sanitize_addr(uint32_t vadr, int type) {
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
\ref PEEKD() PEEKW()
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
\ref PEEK() PEEKD()
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
\ref PEEK() PEEKW()
***/
int32_t Basic::ipeek(int type) {
  int32_t value = 0, vadr;
  void* radr;

  if (checkOpen()) return 0;
  if ( getParam(vadr, I_NONE) ) return 0;
  if (checkClose()) return 0;
  radr = sanitize_addr(vadr, type);
  if (radr) {
    switch (type) {
    case 0: value = *(uint8_t*)radr; break;
    case 1: value = *(uint16_t*)radr; break;
    case 2: value = *(uint32_t*)radr; break;
    default: err = ERR_SYS; break;
    }
  }
  else
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
\ref POKED POKEW
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
\ref POKE POKED
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
\ref POKE POKEW
***/
void BASIC_FP Basic::do_poke(int type) {
  void* adr;
  int32_t value;
  int32_t vadr;

  // アドレスの指定
  vadr = iexp(); if(err) return;
  if(*cip != I_COMMA) { E_SYNTAX(I_COMMA); return; }

  // 例: 1,2,3,4,5 の連続設定処理
  do {
    adr = sanitize_addr(vadr, type);
    if (!adr) {
      err = ERR_RANGE;
      break;
    }
    cip++;          // 中間コードポインタを次へ進める
    if (getParam(value, I_NONE)) return;
    switch (type) {
    case 0: *((uint8_t*)adr) = value; break;
    case 1: *((uint16_t *)adr) = value; break;
    case 2: *((uint32_t *)adr) = value; break;
    default: err = ERR_SYS; break;
    }
    vadr++;
  } while(*cip == I_COMMA);
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
    if (getParam(flag, 0, 0, I_COMMA)) return;
    if (getParam(val, 0, 1, I_NONE)) return;

    switch (flag) {
    case 0:	math_exceptions_disabled = val; break;
    default:	break;
    }
  } else {
    SYNTAX_T("exp DATE or FLAGS");
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

  for (uint8_t i=0; i <4; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      nvar.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if(i != 3) {
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

  for (uint8_t i=0; i <3; i++) {
    if (*cip == I_VAR) {          // 変数の場合
      cip++; index = *cip;        // 変数インデックスの取得
      nvar.var(index) = v[i];
      cip++;
    } else {
      err = ERR_SYNTAX;           // 変数・配列でない場合はエラーとする
      return;
    }
    if(i != 2) {
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
    SYNTAX_T("exp DATE or TIME");
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
void __attribute__((optimize ("no-jump-tables"))) Basic::idate() {
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
#include <azip.h>
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
  AZIP azip;
  azip.load(game.c_str());
  azip.run();
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
\endtable
***/
num_t Basic::nsys() {
  int32_t item = getparam();
  if (err)
    return 0;
  switch (item) {
  case 0:	return (uint32_t)listbuf;
  case 1:	return (uint32_t)sc0.getfontadr();
  default:	E_VALUE(0, 1); return 0;
  }
}

#ifdef __DJGPP__
#include <dpmi.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

uint64_t SMALL Basic::getFreeMemory()
{
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
\usage bytes = FREE()
\ret Number of bytes free.
***/
num_t BASIC_FP Basic::nfree() {
  if (checkOpen()||checkClose()) return 0;
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
    cip+=2;
  } else {
    value = getparam(); // 括弧の値を取得
    if (err)
      return 0;
  }
  if(value == 0) {
    value = millis();              // 0～INT32_MAX ms
  } else if (value == 1) {
    value = millis()/1000;         // 0～INT32_MAX s
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
  uint32_t adr = (uint32_t)&top;

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
  putHexnum(adr, 8);
  newline();

  // SRAM未使用領域の表示
  PRINT_P("SRAM Free: ");
#ifdef ESP8266
  putnum(umm_free_heap_size(), 0);
#else
  putnum(9999, 0);
#endif
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
Boots the system from the specified flash page.
\usage BOOT page
\args
@page	flash page to boot from [`0` to `255`]
\note
The `BOOT` command does not verify if valid firmware code is installed at
the given flash page. Use with caution.
***/
void Basic::iboot() {
#if !defined(HOSTED) && defined(ESP8266)
  int32_t sector;
  if (getParam(sector, 0, 1048576 / SPI_FLASH_SEC_SIZE - 1, I_NONE))
    return;
  eboot_command ebcmd;
  ebcmd.action = ACTION_LOAD_APP;
  ebcmd.args[0] = sector * SPI_FLASH_SEC_SIZE;
  eboot_command_write(&ebcmd);
#ifdef ESP8266_NOWIFI
  // SDKnoWiFi does not have system_restart*(). The only working
  // alternative I could find is triggering the WDT.
  ets_wdt_enable(2,1,1);
  for(;;);
#else
  ESP.reset();        // UNTESTED!
#endif
#else
  err = ERR_NOT_SUPPORTED;
#endif
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
  exit(0);
#else
  err = ERR_NOT_SUPPORTED;
#endif
}

#include "lua_defs.h"

lua_State *lua = NULL;

static void lhook(lua_State *L, lua_Debug *ar)
{
  int c;
  (void)ar;
  if ((c = sc0.peekKey())) {
        if (process_hotkeys(c)) {
          luaL_error(L, "interrupted!");
        }
  }
  process_events();
}

void Basic::ilua()
{
  lua = luaL_newstate();
  if (!lua) {
    err = ERR_SYS;
    return;
  }
  luaL_openlibs(lua);
  luaopen_be(lua);
  luaopen_video(lua);
  lua_sethook(lua, lhook, LUA_MASKCOUNT, 1000);
}
