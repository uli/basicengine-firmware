#include "basic.h"

bool event_pad_enabled;
uint8_t event_pad_proc_idx[MAX_PADS];
int event_pad_last[MAX_PADS];

#include "Psx.h"
Psx psx;

void SMALL basic_init_input() {
  psx.setupPins(PSX_DATA_PIN, PSX_CMD_PIN, PSX_ATTN_PIN, PSX_CLK_PIN, PSX_DELAY);
}

uint8_t BASIC_FP process_hotkeys(uint16_t c, bool dont_dump) {
  if (c == SC_KEY_CTRL_C) {
    err = ERR_CTR_C;
  } else if (c == KEY_PRINT) {
    sc0.saveScreenshot();
  } else
    return 0;

  if (!dont_dump)
    sc0.get_ch();
  return err;
}

int32_t BASIC_FP iinkey() {
  int32_t rc = 0;

  if (c_kbhit()) {
    rc = sc0.tryGetChar();
    process_hotkeys(rc);
  }

  return rc;
}

static int BASIC_INT cursor_pad_state()
{
  // The state is kept up-to-date by the interpreter polling for Ctrl-C.
  return kb.state(PS2KEY_L_Arrow) << psxLeftShift |
         kb.state(PS2KEY_R_Arrow) << psxRightShift |
         kb.state(PS2KEY_Down_Arrow) << psxDownShift |
         kb.state(PS2KEY_Up_Arrow) << psxUpShift |
         kb.state(PS2KEY_X) << psxXShift |
         kb.state(PS2KEY_A) << psxTriShift |
         kb.state(PS2KEY_S) << psxOShift |
         kb.state(PS2KEY_Z) << psxSquShift;
}

int BASIC_INT pad_state(int num)
{
  switch (num) {
  case 0:	return (psx.read() & 0xffff) | cursor_pad_state();
  case 1:	return cursor_pad_state();
  case 2:	return psx.read() & 0xffff;
  }
  return 0;
}

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
     `2`: PSX controller
@state	`0`: current button state (default) +
        `1`: button-change events
\ret
Bit field representing the button states of the requested controller(s). The
value is the sum of any of the following bit values:
\table header
| Bit value | PSX Controller | Keyboard
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
\ref ON_PAD UP DOWN LEFT RIGHT
***/
num_t BASIC_INT npad() {
  int32_t num;
  int32_t state = 0;

  if (checkOpen()) return 0;

  if (getParam(num, 0, 2, I_NONE)) return 0;

  if (*cip == I_COMMA) {
    ++cip;
    if (getParam(state, 0, 3, I_NONE)) return 0;
  }

  if (checkClose())
    return 0;

  int ps = pad_state(num);
  if (state) {
    int cs = ps ^ event_pad_last[num];
    retval[1] = cs & ~ps;
    retval[2] = ps;
    event_pad_last[num] = ps;
    ps &= cs;
  }
  return ps;
}

void BASIC_INT event_handle_pad()
{
  for (int i = 0; i < MAX_PADS; ++i) {
    if (event_pad_proc_idx[i] == NO_PROC)
      continue;
    int new_state = pad_state(i);
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
\note
`INKEY$` is not consistent with the Engine BASIC convention of following
functions with parentheses to distinguish them from constants and variables
in order to remain compatible with other BASIC implementations.
\ref INKEY()
***/
BString BASIC_INT sinkey() {
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
num_t BASIC_FP nup() {
  // カーソル・スクロール等の方向
  return psxUp;
}

/***bn io RIGHT
Value of the "right" direction for input devices.
\ref PAD() UP DOWN LEFT
***/
num_t BASIC_FP nright() {
  return psxRight;
}
/***bn io LEFT
Value of the "left" direction for input devices.
\ref PAD() UP DOWN RIGHT
***/
num_t BASIC_FP nleft() {
  return psxLeft;
}

/***bf io INKEY
Reads a character from the keyboard and returns its numeric value.
\usage c = INKEY()
\ret Key code [`0` to `65535`]
\ref INKEY$
***/
num_t BASIC_FP ninkey() {
  if (checkOpen()||checkClose()) return 0;
  return iinkey(); // キー入力値の取得
}

