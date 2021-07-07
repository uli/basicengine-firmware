// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "basic.h"
#include "eb_input.h"

bool event_pad_enabled;
index_t event_pad_proc_idx[MAX_PADS];
int event_pad_last[MAX_PADS];

#include <joystick.h>

void SMALL basic_init_input() {
#ifdef USE_PSX_GPIO
  joy.setupPins(PSX_DATA_PIN, PSX_CMD_PIN, PSX_ATTN_PIN, PSX_CLK_PIN,
                PSX_DELAY);
#else
  joy.begin();
#endif
}

uint8_t BASIC_FP process_hotkeys(uint16_t c, bool dont_dump) {
  if (c == SC_KEY_CTRL_C) {
    err = ERR_CTR_C;
  } else if (c == SC_KEY_PRINT) {
    sc0.saveScreenshot();
  } else
    return 0;

  if (!dont_dump)
    sc0.get_ch();
  return err;
}

int32_t BASIC_FP iinkey() {
  return eb_inkey();
}

#ifdef USE_PSX_GPIO
#define JOY_MASK 0xffff
#else
#define JOY_MASK 0x3ffff
#endif

/***bf io PAD
Get the state of the game controller(s) and cursor pad.
\desc
`PAD()` can be used to query actual game controllers, or a "virtual" controller
simulated with cursor and letter keys on the keyboard.

The special controller number `0` returns the combined state of all (real and
virtual) controllers, making it easier to write programs that work with and
without a game controller.
\usage state = PAD(num[, state])
\args
@num Number of the game controller: +
     `0`: all controllers combined +
     `1`: cursor pad +
     `2`: Joystick/joypad controller
@state	`0`: current button state (default) +
        `1`: button-change events
\ret
Bit field representing the button states of the requested controller(s). The
value is the sum of any of the following bit values:
\table header
| Bit value | Joystick/joypad Controller | Keyboard
| `1` (aka `<<LEFT>>`) | kbd:[&#x25c4;] button | kbd:[Left] key
| `2` (aka `<<DOWN>>`) | kbd:[&#x25bc;] button | kbd:[Down] key
| `4` (aka `<<RIGHT>>`) | kbd:[&#x25ba;] button | kbd:[Right] key
| `8` (aka `<<UP>>`) | kbd:[&#x25b2;] button | kbd:[Up] key
| `16` | kbd:[Start] button | n/a
| `32` | kbd:[Select] button | n/a
| `256` | kbd:[&#x25a1;] button | kbd:[Z] key
| `512` | kbd:[&#x2715;] button | kbd:[X] key
| `1024` | kbd:[&#x25cb;] button | kbd:[S] key
| `2048` | kbd:[&#x25b3;] button | kbd:[A] key
| `4096` | kbd:[R1] button | n/a
| `8192` | kbd:[L1] button | n/a
| `16384` | kbd:[R2] button | n/a
| `32768` | kbd:[L2] button | n/a
\endtable

Depending on `state`, these bit values are set if the respective buttons are
currently pressed (`0`) or newly pressed (`1`).

If `state` is `1`, additional values are returned:

* `RET(1)` contains buttons that are newly released,
* `RET(2)` contains the current button state.
\note
WARNING: Using `PAD()` to retrieve button events (`state` is not `0`) interacts with
the event handling via `ON PAD`. It is not recommended to use both at the
same time.
\ref ON_PAD KEY() UP DOWN LEFT RIGHT
***/
num_t BASIC_INT Basic::npad() {
  int32_t num;
  int32_t state = 0;

  if (checkOpen())
    return 0;

  if (getParam(num, 0, 2, I_NONE))
    return 0;

  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(state, 0, 3, I_NONE))
      return 0;
  }

  if (checkClose())
    return 0;

  int ps = eb_pad_state(num);
  if (state) {
    int cs = ps ^ event_pad_last[num];
    retval[1] = cs & ~ps;
    retval[2] = ps;
    event_pad_last[num] = ps;
    ps &= cs;
  }
  return ps;
}

/***bf io KEY
Get the state of the specified keyboard key.
\usage state = KEY(num)
\args
@num Key code [`0` to `255`]
\ret
`1` if the key is held, `0` if it is not.

The key codes are detailed in the table below.
Availability of each key depends on the Engine BASIC platform.

\table header
| Key			| Key code	| Notes
| kbd:[Left Alt]	| 1		|
| kbd:[Left Shift]	| 2		|
| kbd:[Left Ctrl]	| 3		|
| kbd:[Right Shift]	| 4		|
| kbd:[Right Alt]	| 5		|
| kbd:[Right Ctrl]	| 6		|
| kbd:[Left GUI]	| 7		|
| kbd:[Right GUI]	| 8		|
| kbd:[NumLock]		| 9		|
| kbd:[Scroll Lock]	| 10		|
| kbd:[Caps Lock]	| 11		|
| kbd:[Print Screen]	| 12		|
| kbd:[`]		| 13		|
| kbd:[Insert]		| 14		|
| kbd:[Home]		| 15		|
| kbd:[Pause]		| 16		|
| kbd:[Romaji]		| 17		| Japanese keyboard, not available on SDL
| kbd:[App]		| 18		| Japanese keyboard, not available on SDL
| kbd:[Henkan]		| 19		| Japanese keyboard, not available on SDL
| kbd:[Muhenkan]	| 20		| Japanese keyboard, not available on SDL
| kbd:[Page Up]		| 21		|
| kbd:[Page Down]	| 22		|
| kbd:[End]		| 23		|
| kbd:[Left Arrow]	| 24		|
| kbd:[Up_Arrow]	| 25		|
| kbd:[Right Arrow]	| 26		|
| kbd:[Down Arrow]	| 27		|
| kbd:[ESC]		| 30		|
| kbd:[Tab]		| 31		|
| kbd:[Enter]		| 32		|
| kbd:[Backspace]	| 33		|
| kbd:[Delete]		| 34		|
| kbd:[Space]		| 35		|
| kbd:[']		| 36		|
| kbd:[:]		| 37		|
| kbd:[,]		| 38		|
| kbd:[-]		| 39		|
| kbd:[.]		| 40		|
| kbd:[/]		| 41		|
| kbd:[[]		| 42		|
| kbd:[\]]		| 43		|
| kbd:[&#x5c;]		| 44		|
| kbd:[\]]		| 45		| Japanese keyboard, not available on SDL
| kbd:[=]		| 46		|
| kbd:[Ro]		| 47		| Japanese keyboard, not available on SDL
| kbd:[0]		| 48		|
| kbd:[1]		| 49		|
| kbd:[2]		| 50		|
| kbd:[3]		| 51		|
| kbd:[4]		| 52		|
| kbd:[5]		| 53		|
| kbd:[6]		| 54		|
| kbd:[7]		| 55		|
| kbd:[8]		| 56		|
| kbd:[9]		| 57		|
| kbd:[Pipe2]		| 58		| <<unclear,not well defined>>, not available on SDL
| kbd:[A]		| 65		|
| kbd:[B]		| 66		|
| kbd:[C]		| 67		|
| kbd:[D]		| 68		|
| kbd:[E]		| 69		|
| kbd:[F]		| 70		|
| kbd:[G]		| 71		|
| kbd:[H]		| 72		|
| kbd:[I]		| 73		|
| kbd:[J]		| 74		|
| kbd:[K]		| 75		|
| kbd:[L]		| 76		|
| kbd:[M]		| 77		|
| kbd:[N]		| 78		|
| kbd:[O]		| 79		|
| kbd:[P]		| 80		|
| kbd:[Q]		| 81		|
| kbd:[R]		| 82		|
| kbd:[S]		| 83		|
| kbd:[T]		| 84		|
| kbd:[U]		| 85		|
| kbd:[V]		| 86		|
| kbd:[W]		| 87		|
| kbd:[X]		| 88		|
| kbd:[Y]		| 89		|
| kbd:[Z]		| 90		|
| Keypad kbd:[=]	| 94		|
| Keypad kbd:[Enter]	| 95		|
| Keypad kbd:[0]	| 96		|
| Keypad kbd:[1]	| 97		|
| Keypad kbd:[2]	| 98		|
| Keypad kbd:[3]	| 99		|
| Keypad kbd:[4]	| 100		|
| Keypad kbd:[5]	| 101		|
| Keypad kbd:[6]	| 102		|
| Keypad kbd:[7]	| 103		|
| Keypad kbd:[8]	| 104		|
| Keypad kbd:[9]	| 105		|
| Keypad kbd:[*]	| 106		|
| Keypad kbd:[+]	| 107		|
| Keypad kbd:[,]	| 108		| Brazilian keyboard, not available on SDL
| Keypad kbd:[-]	| 109		|
| Keypad kbd:[.]	| 110		|
| Keypad kbd:[/]	| 111		|
| kbd:[F1]		| 112		|
| kbd:[F2]		| 113		|
| kbd:[F3]		| 114		|
| kbd:[F4]		| 115		|
| kbd:[F5]		| 116		|
| kbd:[F6]		| 117		|
| kbd:[F7]		| 118		|
| kbd:[F8]		| 119		|
| kbd:[F9]		| 120		|
| kbd:[F10]		| 121		|
| kbd:[F11]		| 122		|
| kbd:[F12]		| 123		|
| kbd:[Power]		| 145		|
\endtable
\note
Key codes are fixed and do not change with the configured keyboard layout.
\bugs
* Not all keys available on international (especially Japanese) keyboard are
  correctly mapped on all platforms.
* [[unclear]] Some key definitions are not well-defined. The kbd:[Pipe2] key is documented
  as belonging to a US layout keyboard, but US layouts do not have a separate
  kbd:[|] key.
* There is currently no keyboard event handling similar to <<ON PAD>> for game pad
  input.
\ref PAD()
***/
num_t BASIC_INT Basic::nkey() {
  int32_t scancode;

  if (checkOpen() ||
      getParam(scancode, I_CLOSE))
    return 0;

  return eb_key_state(scancode);
}

void BASIC_INT Basic::event_handle_pad() {
  for (int i = 0; i < MAX_PADS; ++i) {
    if (event_pad_proc_idx[i] == NO_PROC)
      continue;
    int new_state = eb_pad_state(i);
    int old_state = event_pad_last[i];
    event_pad_last[i] = new_state;
    if (new_state != old_state) {
      init_stack_frame();
      push_num_arg(i);
      push_num_arg(new_state ^ old_state);
      do_call(event_pad_proc_idx[i]);
      // Have to end here so the handler can be executed.
      // Events on other pads will be processed next time.
      return;
    }
  }
}

/***bn bas INKEY$
Reads a character from the keyboard.
\usage c$ = INKEY$
\ret
Returns either

* an empty string if there is no keyboard input,
* a single-character string for regular keys,
* a two-character string for extended keys.

An "extended key" is a key that does not have a common ASCII representation,
such as cursor or function keys.
\ref INKEY
***/
BString BASIC_INT Basic::sinkey() {
  int32_t c = iinkey();
  if (c > 0 && c < 0x100) {
    return BString((char)c);
  } else if (c >= 0x100) {
    return BString((char)(c >> 8)) + BString((char)(c & 255));
  } else
    return BString();
}

/***bn io UP
Value of the "up" direction for input devices.
\ref PAD() DOWN LEFT RIGHT
***/
num_t BASIC_FP Basic::nup() {
  // カーソル・スクロール等の方向
  return EB_JOY_UP;
}

/***bn io DOWN
Value of the "down" direction for input devices.
\ref PAD() UP LEFT RIGHT
***/
num_t Basic::ndown() {
  return EB_JOY_DOWN;
}

/***bn io RIGHT
Value of the "right" direction for input devices.
\ref PAD() UP DOWN LEFT
***/
num_t BASIC_FP Basic::nright() {
  return EB_JOY_RIGHT;
}
/***bn io LEFT
Value of the "left" direction for input devices.
\ref PAD() UP DOWN RIGHT
***/
num_t BASIC_FP Basic::nleft() {
  return EB_JOY_LEFT;
}

/***bf io INKEY
Reads a character from the keyboard and returns its numeric value.
\usage c = INKEY
\ret Key code [`0` to `65535`]
\note
The alternative syntax `INKEY()` is supported for backwards compatibility
with earlier versions of Engine BASIC.
\ref INKEY$
***/
num_t BASIC_FP Basic::ninkey() {
  // backwards compatibility with INKEY() syntax
  if (*cip == I_OPEN && cip[1] == I_CLOSE)
    cip += 2;

  return iinkey();  // キー入力値の取得
}
