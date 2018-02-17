//
// file: TKeyboard.h
// PS/2 keyboard control for Arduino STM32 by Tamakichi-san (たま吉さん)
//

#include <TKeyboard.h>

static TPS2 pb; // PS/2 interface object
volatile static uint8_t _flgLED; // LED control use flag (true: use  false: don't)
volatile static uint8_t _flgUS;  // US keyboard mode
static const uint8_t (*key_ascii)[2];

// Scan code analysis state transition codes
#define STS_SYOKI          0  // initial
#define STS_1KEY           1   // 1 byte (END) [1]
#define STS_1KEY_BREAK     2   // BREAK [2]
#define STS_1KEY_BREAK_1   3   // BREAK + 1 byte (END) [2-1]
#define STS_MKEY_1         4   // multi-byte 1st byte [3]
#define STS_MKEY_2         5   // multi-byte 2nd byte (END)[3-1]
#define STS_MKEY_BREAK     6   // multi-byte 1st byte + BREAK [3-2]
#define STS_MKEY_BREAK_1   7   // multi-byte 1st byte + BREAK + 2nd byte (END)[3-2-1]
#define STS_MKEY_BREAK_SC2 8   // multi-byte 1st byte + BREAK + prnScrn 2nd byte [3-2-2]
#define STS_MKEY_SC2       9   // multi-byte 1st byte + prnScrn 2nd byte [3-3]
#define STS_MKEY_SC3      10   // multi-byte 1st byte + prnScrn 3rd byte [3-3-1]
#define STS_MKEY_PS       11   // pause key 1st byte [4]

// Lock key status
// * ON/OFF state of LOCK is indicated by bit 0
#define LOCK_Start    B00 // initial
#define LOCK_ON_Make  B01 // Pressing and holding the LOCK ON key
#define LOCK_ON_Break B11 // LOCK ON key released
#define LOCK_OFF_Make B10 // Pressing and holding the LOCK OFF key


// Scan code 0x00 - 0x83 => Key code conversion table
// 132 bytes worth
static const uint8_t keycode1[] PROGMEM = {
  0,		PS2KEY_F9,	0,		PS2KEY_F5,	PS2KEY_F3,	PS2KEY_F1,	PS2KEY_F2,	PS2KEY_F12,	// 0x00-0x07
  PS2KEY_F13,	PS2KEY_F10,	PS2KEY_F8,	PS2KEY_F6,	PS2KEY_F4,	PS2KEY_Tab,	PS2KEY_HanZen,	PS2KEY_PAD_Equal,// 0x08-0x0F
  PS2KEY_F14,	PS2KEY_L_Alt,	PS2KEY_L_Shift,	PS2KEY_Romaji,	PS2KEY_L_Ctrl,	PS2KEY_Q,	PS2KEY_1,	0,		// 0x10-0x17
  PS2KEY_F15,	0,		PS2KEY_Z,	PS2KEY_S,	PS2KEY_A,	PS2KEY_W,	PS2KEY_2,	0,		// 0x18-0x1F
  PS2KEY_F16,	PS2KEY_C,	PS2KEY_X,	PS2KEY_D,	PS2KEY_E,	PS2KEY_4,	PS2KEY_3,	0,		// 0x20-0x27
  PS2KEY_F17,	PS2KEY_Space,	PS2KEY_V,	PS2KEY_F,	PS2KEY_T,	PS2KEY_R,	PS2KEY_5,	0,		// 0x28-0x2F
  PS2KEY_F18,	PS2KEY_N,	PS2KEY_B,	PS2KEY_H,	PS2KEY_G,	PS2KEY_Y,	PS2KEY_6,	0,		// 0x30-0x37
  PS2KEY_F19,	0,		PS2KEY_M,	PS2KEY_J,	PS2KEY_U,	PS2KEY_7,	PS2KEY_8,	0,		// 0x38-0x3F
  PS2KEY_F20,	PS2KEY_Kamma,	PS2KEY_K,	PS2KEY_I,	PS2KEY_O,	PS2KEY_0,	PS2KEY_9,	0,		// 0x40-0x47
  PS2KEY_F21,	PS2KEY_Dot,	PS2KEY_Question,PS2KEY_L,	PS2KEY_Semicolon,PS2KEY_P,	PS2KEY_minus,	0,		// 0x48-0x4F
  PS2KEY_F22,	PS2KEY_Ro,	PS2KEY_Colon,	0,		PS2KEY_AT,	PS2KEY_Hat,	0,		PS2KEY_F23,	// 0x50-0x57
  PS2KEY_CapsLock,PS2KEY_R_Shift,PS2KEY_Enter,	PS2KEY_L_brackets,0,		PS2KEY_R_brackets,0,		0,		// 0x58-0x5F
  0,		PS2KEY_Pipe2,	0,		0,		PS2KEY_Henkan,	0,		PS2KEY_Backspace,PS2KEY_Muhenkan,// 0x60-0x67
  0,		PS2KEY_PAD_1,	PS2KEY_Pipe,	PS2KEY_PAD_4,	PS2KEY_PAD_7,	PS2KEY_PAD_Kamma,0,		0,		// 0x68-0x6F
  PS2KEY_PAD_0,	PS2KEY_PAD_DOT,	PS2KEY_PAD_2,	PS2KEY_PAD_5,	PS2KEY_PAD_6,	PS2KEY_PAD_8,	PS2KEY_ESC,	PS2KEY_NumLock,	// 0x70-0x77
  PS2KEY_F11,	PS2KEY_PAD_Plus,PS2KEY_PAD_3,	PS2KEY_PAD_Minus,PS2KEY_PAD_Multi,PS2KEY_PAD_9,	PS2KEY_ScrollLock,0,		// 0x78-0x7F
  0,		0,		0,		PS2KEY_F7,									// 0x80-0x83
};

// Scan code 0xE010-0xE07D => Key code conversion table
// 2 bytes (scan code lower byte, key code) * 38
static const uint8_t keycode2[][2] PROGMEM = {
  { 0x10, PS2KEY_WWW_Search },    { 0x11, PS2KEY_R_Alt },      { 0x14, PS2KEY_R_Ctrl },      { 0x15, PS2KEY_PrevTrack },
  { 0x18, PS2KEY_WWW_Favorites }, { 0x1F, PS2KEY_L_GUI },      { 0x20, PS2KEY_WWW_Refresh }, { 0x21, PS2KEY_VolumeDown },
  { 0x23, PS2KEY_Mute },          { 0x27, PS2KEY_R_GUI },      { 0x28, PS2KEY_WWW_Stop },    { 0x2B, PS2KEY_Calc },
  { 0x2F, PS2KEY_APP },           { 0x30, PS2KEY_WWW_Forward },{ 0x32, PS2KEY_VolumeUp },    { 0x34, PS2KEY_PLAY },
  { 0x37, PS2KEY_POWER },         { 0x38, PS2KEY_WWW_Back },   { 0x3A, PS2KEY_WWW_Home },    { 0x3B, PS2KEY_Stop },
  { 0x3F, PS2KEY_Sleep },         { 0x40, PS2KEY_Mycomputer }, { 0x48, PS2KEY_Mail },        { 0x4A, PS2KEY_PAD_Slash },
  { 0x4D, PS2KEY_NextTrack },     { 0x50, PS2KEY_MEdiaSelect },{ 0x5A, PS2KEY_PAD_Enter },   { 0x5E, PS2KEY_Wake },
  { 0x69, PS2KEY_End },           { 0x6B, PS2KEY_L_Arrow },    { 0x6C, PS2KEY_Home },        { 0x70, PS2KEY_Insert },
  { 0x71, PS2KEY_Delete },        { 0x72, PS2KEY_Down_Arrow }, { 0x74, PS2KEY_R_Arrow },     { 0x75, PS2KEY_Up_Arrow },
  { 0x7A, PS2KEY_PageDown },      { 0x7D, PS2KEY_PageUp },
};

// Pause Key scan codes
static const uint8_t pausescode[] PROGMEM = {
  0xE1,0x14,0x77,0xE1,0xF0,0x14,0xF0,0x77
};

// PrintScreen Key scan codes (break);
static const uint8_t prnScrncode2[] __FLASH__ = {
  0xE0,0xF0,0x7C,0xE0,0xF0,0x12
};


// Key code / ASCII code conversion table
// { not shifted, shifted }

// US keyboard
static const uint8_t key_ascii_us[][2] PROGMEM = {
  /*{0x1B,0x1B},{0x09,0x09},{0x0D,0x0D},{0x08,0x08},{0x7F,0x7F},*/
  { ' ', ' '},{ ',','\"'},{ ';', ':'},{ ',', '<'},{ '-', '_'},{ '.', '>'},{ '/', '?'},{ '[', '{'},
  { ']', '}'},{ '\\','|'},{'\\', '|'},{ '=', '+'},{ '\\','_'},{ '0', ')'},{ '1', '!'},{ '2', '@'},
  { '3', '#'},{ '4', '$'},{ '5', '%'},{ '6', '^'},{ '7', '&'},{ '8', '*'},{ '9', '('},{'\\', '|'},
  {   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{ 'a', 'A'},{ 'b', 'B'},
  { 'c', 'C'},{ 'd', 'D'},{ 'e', 'E'},{ 'f', 'F'},{ 'g', 'G'},{ 'h', 'H'},{ 'i', 'I'},{ 'j', 'J'},
  { 'k', 'K'},{ 'l', 'L'},{ 'm', 'M'},{ 'n', 'N'},{ 'o', 'O'},{ 'p', 'P'},{ 'q', 'Q'},{ 'r', 'R'},
  { 's', 'S'},{ 't', 'T'},{ 'u', 'U'},{ 'v', 'V'},{ 'w', 'W'},{ 'x', 'X'},{ 'y', 'Y'},{ 'z', 'Z'},
};

// Japanese keyboard
static const uint8_t key_ascii_jp[][2] PROGMEM = {
  /*{0x1B,0x1B},{0x09,0x09},{0x0D,0x0D},{0x08,0x08},{0x7F,0x7F},*/
  { ' ', ' '},{ ':', '*'},{ ';', '+'},{ ',', '<'},{ '-', '='},{ '.', '>'},{ '/', '?'},{ '@', '`'},
  { '[', '{'},{ '\\','|'},{ ']', '}'},{ '^', '~'},{ '\\','_'},{ '0', 0  },{ '1', '!'},{ '2','\"'},
  { '3', '#'},{ '4', '$'},{ '5', '%'},{ '6', '&'},{ '7','\''},{ '8', '('},{ '9', ')'},{   0,  0 },
  {   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{   0,  0 },{ 'a', 'A'},{ 'b', 'B'},
  { 'c', 'C'},{ 'd', 'D'},{ 'e', 'E'},{ 'f', 'F'},{ 'g', 'G'},{ 'h', 'H'},{ 'i', 'I'},{ 'j', 'J'},
  { 'k', 'K'},{ 'l', 'L'},{ 'm', 'M'},{ 'n', 'N'},{ 'o', 'O'},{ 'p', 'P'},{ 'q', 'Q'},{ 'r', 'R'},
  { 's', 'S'},{ 't', 'T'},{ 'u', 'U'},{ 'v', 'V'},{ 'w', 'W'},{ 'x', 'X'},{ 'y', 'Y'},{ 'z', 'Z'},
};

// Conversion table for numeric pad keys 94-111
// { Normal code, NumLock/Shift code, 1: key code  0: ASCII code }
static const uint8_t tenkey[][3] PROGMEM = {
  { '=',  '=', 0 },
  { 0x0D, 0x0D, 0 },
  { '0',  PS2KEY_Insert, 1 },
  { '1',  PS2KEY_End, 1 },
  { '2',  PS2KEY_Down_Arrow, 1 },
  { '3',  PS2KEY_PageDown, 1 },
  { '4',  PS2KEY_L_Arrow, 1 },
  { '5',  '5', 0 },
  { '6',  PS2KEY_R_Arrow, 1 },
  { '7',  PS2KEY_Home, 1 },
  { '8',  PS2KEY_Up_Arrow, 1 },
  { '9',  PS2KEY_PageUp, 1 },
  { '*',  '*', 0 },
  { '+',  '+', 0 },
  { ',',  ',', 1 },
  { '-',  PS2KEY_PAD_Minus, 1 },
  { '.',  0x7f, 0 },
  { '/',  '/', 0 },
};

// Start of use (initialization)
// Arguments
//   clk:	PS/2 CLK
//   dat:	PS/2 DATA
//   flgLED:	false: Do not control the LED, true: do LED control
// Return value
//   0: Normal termination      Other than 0: Abnormal termination
uint8_t TKeyboard::begin(uint8_t clk, uint8_t dat, uint8_t flgLED, uint8_t flgUS)
{
  uint8_t rc;
  _flgUS = flgUS;
  if (_flgUS)
    key_ascii = key_ascii_us;
  else
    key_ascii = key_ascii_jp;

  memset(m_key_state, 0, sizeof(m_key_state));

  _flgLED = flgLED;    // Availability of LED control
  pb.begin(clk, dat);  // Initialization of PS/2 interface
  rc = init();         // Initializing the keyboard
  return rc;
}

// End of use
void TKeyboard::end()
{
  pb.end();
}

// Keyboard initialization
// Return value:	0: Normal completion	not 0: Abnormal termination
uint8_t TKeyboard::init()
{
  uint8_t c,err;

  pb.disableInterrupts();
  pb.clear_queue();

  // Reset command: FF	response is FA, AA
  if ( (err = pb.hostSend(0xff)) )
    goto ERROR;

  err = pb.response(&c);
  if (err || (c != 0xFA))
    goto ERROR;

  err = pb.response(&c);
  if (err || (c != 0xAA))
    goto ERROR;

  // Identification information check: F2
  // Response is FA, AB, 83 if keyboard identification OK
  if ( (err = pb.hostSend(0xf2)) )
    goto ERROR;

  err = pb.response(&c);
  if (err || (c != 0xFA))
    goto ERROR;

  err = pb.response(&c);
  if (err || (c != 0xAB))
    goto ERROR;

  err = pb.response(&c);
  if (err || (c != 0x83))
    goto ERROR;

ERROR:
  // USB/PS2 "DeLUX" keyboard works despite the error, but if we don't wait a
  // little bit here we will lose the first key press.
  delay(1);
  pb.mode_idole(TPS2::D_IN);
  pb.enableInterrupts();
  return err;
}

// Enable CLK change interrupt
void TKeyboard::enableInterrupts()
{
  pb.enableInterrupts();
}

// Disable CLK change interrupt
void TKeyboard::disableInterrupts()
{
  pb.disableInterrupts();
}

// Interrupt priority setting
void TKeyboard::setPriority(uint8_t n)
{
  pb.setPriority(n);
}

// Set PS/2 line to idle state
void TKeyboard::mode_idole()
{
  pb.mode_idole(TPS2::D_IN);
}

// Prohibit communication on PS/2 line
void TKeyboard::mode_stop()
{
  pb.mode_stop();
}

// Set PS/2 line to host transmission mode
void TKeyboard::mode_send()
{
  pb.mode_send();
}

// Scan code search
uint8_t TKeyboard::findcode(uint8_t c)
{
  int t_p = 0; // Search range upper limit
  int e_p = sizeof(keycode2)/sizeof(keycode2[0])-1; // Search range lower limit
  int pos;
  uint16_t d = 0;
  int flg_stop = 0;

  while(true) {
    pos = t_p + ((e_p - t_p+1)>>1);
    d = pgm_read_byte(&keycode2[pos][0]);
    if (d == c) {
      flg_stop = 1;
      break;
    } else if (c > d) {
      t_p = pos + 1;
      if (t_p > e_p) {
	break;
      }
    } else {
      e_p = pos -1;
      if (e_p < t_p)
	break;
    }
  }
  if (!flg_stop)
    return 0;
  return pgm_read_byte(&keycode2[pos][1]);
}

//
// Convert scan code to key code
// Return value
//   Normal
//     Lower 8 bits: Key code
//     Upper 1 bit: 0: MAKE 1: BREAK
//   No key input
//     0
//   Error
//     255
//
uint16_t TKeyboard::scanToKeycode()
{
  static uint8_t state = STS_SYOKI;
  static uint8_t scIndex = 0;
  uint16_t c, code = 0;

  while(pb.available()) {
    c = pb.dequeue();     // Retrieve 1 byte from queue
    //Serial.print("S=");
    //Serial.println(c,HEX);
    switch(state) {
    case STS_SYOKI: // [0]->
      if (c <= 0x83) {
	// [0]->[1] 1 byte (END)
	code = pgm_read_byte(keycode1+c);
	goto DONE;
      } else {
	switch(c) {
	case 0xF0:  state = STS_1KEY_BREAK; continue; // ->[2]
	case 0xE0:  state = STS_MKEY_1; continue;     // ->[3]
	case 0xE1:  state = STS_MKEY_PS; scIndex = 0; continue;  // ->[4]
	default:    goto STS_ERROR;  // -> ERROR
	}
      }
      break;

    case STS_1KEY_BREAK: // [2]->
      if (c <= 0x83) {
	// [2]->[2-1] BREAK + 1 byte (END)
	code = pgm_read_byte(keycode1+c) | BREAK_CODE;
	goto DONE;
      } else {
	goto STS_ERROR; // -> ERROR
      }
      break;

    case STS_MKEY_1: // [3]->
      switch(c) {
      case 0xF0:  state = STS_MKEY_BREAK; continue; // ->[3-2]
#if 0
      // Doing this causes the shift key to become stuck when doing Shift-CursorDown
      // because the shift code (0x12) comes before the break code (0xf0), at least
      // with the DeLUX USB keyboard.
      // This may break key combos with PrintScreen.
      case 0x12:  state = STS_MKEY_SC2; scIndex=1; continue; // ->[3-3]
#endif
      default:
	code = findcode(c);
	if (code) {
	  // [3]->[3-1] multi-byte 2nd byte (END)
	  goto DONE;
	} else {
	  goto STS_ERROR; // -> ERROR
	}
      }
      break;

    case STS_MKEY_BREAK: // [3-2]->
      if (c == 0x7C) {
	state = STS_MKEY_BREAK_SC2; scIndex=2; continue; // ->[3-2-2]
      } else {
	code = findcode(c);
	if (code) {
	  // [3-2]->[3-2-1] BREAK + multi-byte 2nd byte (END)
	  code |= BREAK_CODE;
	  goto DONE;
	} else {
	  goto STS_ERROR; // -> ERROR
	}
      }
      break;

    case STS_MKEY_BREAK_SC2: // [3-2-2]->
      scIndex++;
      if (scIndex >= sizeof(prnScrncode2)) {
	goto STS_ERROR; // -> ERROR
      }
      if (c == prnScrncode2[scIndex]) {
	if (scIndex == sizeof(prnScrncode2)-1) {
	  // ->[3-2-2-1-1-1](END)
	  code = PS2KEY_PrintScreen | BREAK_CODE; // BREAK+PrintScreen
	  goto DONE;
	} else {
	  continue;
	}
      }
      break;

    case STS_MKEY_SC2:  // [3-3]->
      if ( c == 0xe0 ) {
	state  = STS_MKEY_SC3;
	continue;
      }
      break;

    case STS_MKEY_SC3:
      switch(c) {
      case 0x70: code = PS2KEY_Insert;      goto DONE; break; // [INS]
      case 0x71: code = PS2KEY_Delete;      goto DONE; break; // [DEL]
      case 0x6B: code = PS2KEY_L_Arrow;     goto DONE; break; // [←]
      case 0x6C: code = PS2KEY_Home;        goto DONE; break; // [HOME]
      case 0x69: code = PS2KEY_End;         goto DONE; break; // [END]
      case 0x75: code = PS2KEY_Up_Arrow;    goto DONE; break; // [↑]
      case 0x72: code = PS2KEY_Down_Arrow;  goto DONE; break; // [↓]
      case 0x7d: code = PS2KEY_PageUp;      goto DONE; break; // [PageUp]
      case 0x7a: code = PS2KEY_PageDown;    goto DONE; break; // [PageDown]
      case 0x74: code = PS2KEY_R_Arrow;     goto DONE; break; // [→]
      case 0x7C: code = PS2KEY_PrintScreen; goto DONE; break; // [PrintScreen]
      default:  goto STS_ERROR; break;  // -> ERROR
      }

      break;

    case STS_MKEY_PS:  // [4]->
      scIndex++;
      if (scIndex >= sizeof(pausescode)) {
	goto STS_ERROR; // -> ERROR
      }
      if (c == pgm_read_byte(&pausescode[scIndex])) {
	if (scIndex == sizeof(pausescode)-1) {
	  // ->[4-1-1-1-1-1-1-1](END)
	  code = PS2KEY_Pause; // Pause key
	  goto DONE;
	} else {
	  continue;
	}
      } else {
	goto STS_ERROR; // -> ERROR
      }
      break;

    default:
      goto STS_ERROR; // -> ERROR
      break;
    }
  }
  return PS2KEY_NONE;
STS_ERROR:
  code = PS2KEY_ERROR;
DONE:
  state = STS_SYOKI;
  scIndex = 0;
  return code;
}

uint8_t TKeyboard::m_key_state[256/8];

//
// Acquire input key information (consider CapsLock, NumLock, ScrollLock)
// Specification
//   - Return the ASCII code character or key code and associated information
//     corresponding to the entered key.
//     => keyEvent structure (2 bytes)
//     (ASCII code if there is one, key code otherwise.)
//     - Lower 8 bits ASCII code or key code
//     - The following information is set in the upper 8 bits
//       B00000001: Make/Break type  0: Make (pressed)  1: Break (released)
//       B00000010: ASCII code/key code type  0: ASCII  1: key code
//       B00000100: Shift held (CapsLock considered)  0: no  1: yes
//       B00001000: Ctrl held  0: no  1: yes
//       B00010000: Alt held  0: no 1: yes
//       B00100000: GUI held  0: no 1: yes
//
//     - In case of error or no input, set the following values in the lower 8 bits
//       No input: 0x00, error: 0xFF
//
// Returned value: input information
//
keyEvent TKeyboard::read()
{
  //static uint16_t sts_state     = 0;           // キーボード状態
  static keyinfo sts_state = {.value = 0};
  static uint8_t sts_numlock   = LOCK_Start; // NumLock state
  static uint8_t sts_CapsLock  = LOCK_Start; // CapsLock state
  static uint8_t sts_ScrolLock = LOCK_Start; // ScrollLock state

  keyinfo c;
  uint16_t code;
  uint16_t bk;

  // Obtain key code
  c.value = 0;
  if (pb.available())
    code = scanToKeycode();
  else
    code = 0;
  bk = code & BREAK_CODE;
  code &= 0xff;

  if (code == 0 || code == 0xff)
    goto DONE;  // No key input or error

  if (bk)
    m_key_state[code/8] &= ~(1 << code % 8);
  else
    m_key_state[code/8] |= 1 << code % 8;

  // Normal key
  if (code >= PS2KEY_Space && code <= PS2KEY_Z) {
    // Keys affected by the state of CapsLock (A-Z)
    if (code >= PS2KEY_A && code <= PS2KEY_Z)
      c.value = pgm_read_byte(&key_ascii[code-PS2KEY_Space][((sts_CapsLock&1)&&sts_state.kevt.SHIFT)||(!(sts_CapsLock&1)&&!sts_state.kevt.SHIFT) ? 0 : 1]);
    else
      c.value = pgm_read_byte(&key_ascii[code-PS2KEY_Space][sts_state.kevt.SHIFT ? 1 : 0]);
    goto DONE;
  } else if (code >= PS2KEY_PAD_Equal && code <= PS2KEY_PAD_Slash) {
    // Numeric keypad
    if ( (sts_numlock & 1) &&  !sts_state.kevt.SHIFT ) {
      // NumLock is enabled and Shift is not held
      c.value = pgm_read_byte(&tenkey[code-PS2KEY_PAD_Equal][0]);
      //Serial.println("[DEBUG:NumLock]");
    } else {
      c.value = pgm_read_byte(&tenkey[code-PS2KEY_PAD_Equal][1]);
      if (pgm_read_byte(&tenkey[code-PS2KEY_PAD_Equal][2]))
	c.value |= PS2KEY_CODE;
    }
    goto DONE;
  } else if (code >=PS2KEY_L_Alt && code <= PS2KEY_CapsLock) {
    // Input control key

    // Save the state of Lock keys before operation
    uint8_t prv_numlock = sts_numlock & 1;
    uint8_t prv_CapsLock = sts_CapsLock & 1;
    uint8_t prv_ScrolLock = sts_ScrolLock & 1;

    switch(code) {
    case PS2KEY_L_Shift:
    case PS2KEY_R_Shift:  sts_state.kevt.SHIFT = bk ? 0 : 1; break;
    case PS2KEY_L_Ctrl:
    case PS2KEY_R_Ctrl:   sts_state.kevt.CTRL = bk ? 0 : 1;  break;
    case PS2KEY_L_Alt:
    case PS2KEY_R_Alt:    sts_state.kevt.ALT = bk ? 0 : 1; break;
    case PS2KEY_L_GUI:    // Windows keys
    case PS2KEY_R_GUI:    sts_state.kevt.GUI = bk ? 0 : 1;  break;

    case PS2KEY_CapsLock: // Toggle action
      switch (sts_CapsLock) {
      case LOCK_Start:				// initial
        if (!bk)
	  sts_CapsLock = LOCK_ON_Make;
	break;

      case LOCK_ON_Make:			// Pressing and holding the LOCK ON key
        if (bk)
	  sts_CapsLock = LOCK_ON_Break;
        break;

      case LOCK_ON_Break:			// LOCK ON key released
        if (!bk)
	  sts_CapsLock = LOCK_OFF_Make;
        break;

      case LOCK_OFF_Make:			// Pressing and holding the LOCK OFF key
        if (bk)
	  sts_CapsLock = LOCK_Start;
        break;

      default:
        sts_CapsLock = LOCK_Start;
        break;
      }
      break;

    case PS2KEY_ScrollLock: // Toggle action
      switch (sts_ScrolLock) {
      case LOCK_Start:				// initial
        if (!bk)
	  sts_ScrolLock = LOCK_ON_Make;
	break;

      case LOCK_ON_Make:			// Pressing and holding the LOCK ON key
        if (bk)
	  sts_ScrolLock = LOCK_ON_Break;
        break;

      case LOCK_ON_Break:			// LOCK ON key released
        if (!bk)
	  sts_ScrolLock = LOCK_OFF_Make;
        break;

      case LOCK_OFF_Make:			// Pressing and holding the LOCK OFF key
        if (bk)
	  sts_ScrolLock = LOCK_Start;
        break;

      default:
        sts_ScrolLock = LOCK_Start;
        break;
      }
      break;

    case PS2KEY_NumLock: // Toggle action
      switch (sts_numlock) {
      case LOCK_Start:				// initial
        if (!bk)
	  sts_numlock = LOCK_ON_Make;
        break;

      case LOCK_ON_Make:			// Pressing and holding the LOCK ON key
        if (bk)
	  sts_numlock = LOCK_ON_Break;
        break;

      case LOCK_ON_Break:			// LOCK ON key released
        if (!bk)
	  sts_numlock = LOCK_OFF_Make;
        break;

      case LOCK_OFF_Make:			// Pressing and holding the LOCK OFF key
        if (bk)
	  sts_numlock = LOCK_Start;
	break;

      default:
        sts_numlock = LOCK_Start;
        break;
      }
      break;

    default:
      goto ERROR;
    }

    c.value = PS2KEY_NONE; // The input control key has no input

    // Control of LED (Control of LED when status of each Lock key is changed)
    if ((prv_numlock != (sts_numlock & 1)) ||
        (prv_CapsLock != (sts_CapsLock & 1)) ||
        (prv_ScrolLock != (sts_ScrolLock & 1)))
      ctrl_LED(sts_CapsLock & 1, sts_numlock & 1, sts_ScrolLock & 1);
    goto DONE;
  } else if (code == PS2KEY_HanZen && _flgUS) {
    if (sts_state.kevt.SHIFT)
      c.value = '`';
    else
      c.value = '~';
    goto DONE;
  } else {
    // Other characters (key codes are assumed to be character codes)
    c.value = code|PS2KEY_CODE;
    goto DONE;
  }

ERROR:
  c.value = PS2KEY_ERROR;

DONE:
  c.value |= sts_state.value|bk;
  return c.kevt;
}

// keyboard LED lighting setting
// Arguments
//   swCaps:	CapsLock LED control	0: off 1: on
//   swNum:	NumLock LED control	0: off 1: on
//   swScrol:	ScrollLock LED control	0: off 1: on
// Return value
//   0: normal	1: abnormal
//
uint8_t TKeyboard::ctrl_LED(uint8_t swCaps, uint8_t swNum, uint8_t swScrol)
{

  if(!_flgLED)
    return 0;

  uint8_t c = 0, err, tmp;
  //pb.disableInterrupts();

  if(swCaps)
    c |= 0x04;  // CapsLock LED
  if(swNum)
    c |= 0x02;  // NumLock LED
  if(swScrol)
    c |= 0x01;  // ScrollLock LED

  if ((err = pb.send(0xed)))
    goto ERROR;
  if ((err = pb.rcev(&tmp)))
    goto ERROR;
  if (tmp != 0xFA) {
    //Serial.println("Send Error 1");
    goto ERROR;
  }
  if ((err = pb.send(c)))
    goto ERROR;
  if ((err = pb.rcev(&tmp)))
    goto ERROR;
  if (tmp != 0xFA) {
    //Serial.println("Send Error 2");
    goto ERROR;
  }
  goto DONE;

ERROR:
  // Restoration by keyboard initialization
  init();
DONE:
  return err;
}
