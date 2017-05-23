/*---------------------------------------------------------------------------------------------------------------------------------------------------
   @file hexedit.c - full screen hex editor

   Copyright (c) 2014 Frank Meyer - frank(at)fli4l.de

   Revision History:
   V1.0 2015 xx xx Frank Meyer, original version
   V1.1 2017 01 14 ChrisMicro, converted to Arduino example

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   Possible keys:
    LEFT                back one byte
    RIGHT               forward one byte
    DOWN                one line down
    UP                  one line up
    TAB                 toggle between hex and ascii columns
    any other           input as hex or ascii value
    2 x ESCAPE          exit
  ---------------------------------------------------------------------------------------------------------------------------------------------------
*/

//
// 修正 2017/03/14 by たま吉さん, 参照領域アドレスを32ビット対応、行数を16行に修正
//

#include <mcurses.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void _itox(uint8_t val);
void _itoxx(unsigned char i);
uint8_t _xtoi(uint8_t ch);
void _print_hex_line(uint8_t line, uint32_t off);
void hexedit2(uint32_t offset, uint8_t flgEdit);

//End of Auto generated function prototypes by Atmel Studio

#define FIRST_LINE      1
//#define LAST_LINE       (LINES - 1)
#define LAST_LINE       (FIRST_LINE + 16)

#define BYTES_PER_ROW   16
#define FIRST_HEX_COL   (7+2)
#define FIRST_ASCII_COL 57
#define LAST_BYTE_COL   (FIRST_HEX_COL + 3 * BYTES_PER_ROW)
#define IS_PRINT(ch)    (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0))

#define MODE_HEX        0
#define MODE_ASCII      1

#define MAXADDRESS      0x0801FFFF
#define IS_HEX(ch)      (((ch) >= 'A' && (ch) <= 'F') || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= '0' && (ch) <= '9'))

#ifdef unix
#define PEEK(x)         memory[x]
#define POKE(x,v)       memory[x] = (v)
unsigned char           memory[65536];
#else // SDCC
#define PEEK(x)         *((unsigned char *) (x))
#define POKE(x,v)       *((unsigned char *) (x)) = (v)
#endif

/*---------------------------------------------------------------------------------------------------------------------------------------------------
   itox: convert a decimal value 0-15 into hexadecimal digit
  ---------------------------------------------------------------------------------------------------------------------------------------------------
*/
void _itox (uint8_t val) {
  uint8_t ch;

  val &= 0x0F;

  if (val <= 9)  {
    ch = val + '0';
  } else  {
    ch = val - 10 + 'A';
  }
  addch (ch);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
   itoxx: convert a decimal value 0-255 into 2 hexadecimal digits
  ---------------------------------------------------------------------------------------------------------------------------------------------------
*/

void _itoxx (unsigned char i) {
  _itox (i >> 4);
  _itox (i & 0x0F);
}

uint8_t _xtoi (uint8_t ch) {
  uint8_t val;

  if (ch >= 'A' && ch <= 'F') {
    val = (ch - 'A') + 10;
  } else if (ch >= 'a' && ch <= 'f') {
    val = (ch - 'a') + 10;
  } else {
    val = (ch - '0');
  }
  _itox (val);     // print value in hex
  return val;
}

void _ito8x(uint32_t i) {
  _itox ( i >> 24);
  _itox ((i >> 16) & 0xff );
  _itox ((i >>  8) & 0xff );
  _itox (i & 0xff );
}

uint32_t _8xtoi(uint8_t* str) {
   uint32_t rc = 0;
   uint8_t i;
   for (i=0; i <8; i++)
      rc |= ((uint32_t)_xtoi(str[i]))<<(28-i*4);
   return rc;
}

void _print_hex_line (uint8_t line, uint32_t off) {
  uint8_t  col;
  uint8_t  ch;
  uint32_t adr;

  move (line, 0);
  _itoxx ( (off >> 24) & 0xff);
  _itoxx ( (off >> 16) & 0xff);
  _itoxx ( (off >>  8) & 0xff);
  _itoxx ( off & 0xFF);
  addch ('|');
  move (line, FIRST_HEX_COL);
  for (col = 0; col < BYTES_PER_ROW; col++) {
    _itoxx (PEEK(off));
    addch (' ');
    off++;
  }

  off -= BYTES_PER_ROW;
  move (line, FIRST_ASCII_COL - 1);
  addch ('|');
  for (col = 0; col < BYTES_PER_ROW; col++) {
    ch = PEEK(off);

    if (IS_PRINT(ch)) {
      addch (ch);
    } else {
      addch ('.');
    }
    off++;
  }
}

void _print_header(uint8_t l) {
  uint8_t         col;
  uint8_t         byte;

  move (l, 0);
  attrset (A_REVERSE);

  for (col = 0; col < FIRST_HEX_COL - 1; col++) {
    addch (' ');
  }
  addch ('|');

  for (byte = 0; byte < BYTES_PER_ROW - 1; byte++)  {
    _itoxx (byte);
    addch (' ');
    col += 3;
  }
  _itoxx (byte);
  addch ('|');
  /*
      for ( ; col < FIRST_ASCII_COL; col++) {
          addch (' ');
      }
  */
  for (byte = 0; byte < BYTES_PER_ROW; byte++)  {
    itox (byte);
  }

  attrset (A_NORMAL);

}

void _print_key_info(uint8_t ln) {
  move (ln, 0);
  addstr("keys cursor:[LEFT][RIGHT][DOWN][UP][PageUP][PageDown][TAB] adr:[HOME]");
}

uint32_t _input_address(uint32_t adr, uint8_t ln) {
  uint8_t  ch;
  uint8_t  str[16];
  uint32_t rc = 0;
  uint8_t i;
  uint8_t pos = 9;
  uint8_t flgEnd = 0;
    
  sprintf(str, "%08X", adr);
  move (ln, 0);
  addstr("Address [");
  addstr(str);
  addstr("]");
  
  do {
    move (ln, pos);
    ch = getch();
    switch(ch) {
      case KEY_LEFT:
        if (pos > 9) {
           pos--;
           move (ln, pos);
        }
        break;
      case KEY_RIGHT:
        if (pos < 16) {
          pos++;
          move (ln, pos);
        }
        break;
      case KEY_HOME:
        rc = 0xFFFFFFFF;
        flgEnd = 1;
        break;
      case KEY_CR: 
        move (ln, 9);
        rc = _8xtoi(str);
        flgEnd = 1;
        break;
      default:
        if (IS_HEX(ch)) {
           if (ch>='a' && ch <='f') {
               ch = ch-'a'+'A';
           }
           addch(ch);
           str[pos-9]=ch;
           if (pos < 16) {
              pos++;
           } else {
             move (ln, pos);      
            }
        }
        break;
    }    
  } while(!flgEnd);
  return rc;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
   hexdit: hex editor
  ---------------------------------------------------------------------------------------------------------------------------------------------------
*/
void hexedit2 (uint32_t offset, uint8_t flgEdit) {
  uint8_t         ch;
  uint8_t         line;
  uint8_t         col;
  uint32_t        off, tmp_off;
  uint8_t         byte;
  uint8_t         mode = MODE_HEX;
  uint8_t         tmp_line;
  uint8_t         i;
  uint32_t        rc;
  
  clear ();
  setscrreg (FIRST_LINE, LAST_LINE);

  off = offset;
  _print_header(0);
  _print_header(LAST_LINE);
  _print_key_info(LAST_LINE + 1);
  for (line = FIRST_LINE; line < LAST_LINE; line++) {
    _print_hex_line (line, off);
    off += BYTES_PER_ROW;
  }

  off = offset;
  line = FIRST_LINE;
  col = FIRST_HEX_COL;
  byte = 0;

  do {
    move (line, col);
    ch = getch ();

    switch (ch) {
      case KEY_HOME: {
          tmp_off = off - (line - 1) * 16;
          rc = _input_address(tmp_off, LAST_LINE + 2);
          move (line, col);
          if ( (rc != 0xFFFFFFFF) && (rc >= offset) && (rc < MAXADDRESS -256)) {
            tmp_off = rc;
            off = rc + (line-1) * BYTES_PER_ROW;
            for (i = FIRST_LINE; i < LAST_LINE; i++) {
              _print_hex_line (i, tmp_off);
              tmp_off += BYTES_PER_ROW;
            }            
          }
          move (line, col);
          break;
       }
      case KEY_RIGHT: {
          if (byte < BYTES_PER_ROW - 1) {
            byte++;
            col += (mode == MODE_HEX) ? 3 : 1;
          }
          break;
        }
      case KEY_LEFT: {
          if (byte > 0) {
            byte--;
            col -= (mode == MODE_HEX) ? 3 : 1;
          }
          break;
        }
      case KEY_NPAGE: {
          if (off + 256 < MAXADDRESS) {
            tmp_off = off + 256 - (line - 1) * 16;
            off += 256;
            for (i = FIRST_LINE; i < LAST_LINE; i++) {
              _print_hex_line (i, tmp_off);
              tmp_off += BYTES_PER_ROW;
            }
          }
          break;
        }
      case KEY_DOWN: {
          //              if (off < 65520) {
          if (off < MAXADDRESS - 15) {
            off += BYTES_PER_ROW;

            if (line == LAST_LINE - 1) {
              scroll ();
              _print_hex_line (line, off);
              _print_header(LAST_LINE);
            } else {
              line++;
            }
          }
          break;
        }
      case KEY_PPAGE: {
          if (off - 256 - BYTES_PER_ROW * line >= offset) {
            tmp_off = off - 256 - (line - 1) * 16;
            off -= 256;
          } else {
            tmp_off = offset;
            off = offset + (line - 1) * BYTES_PER_ROW;
          }
          for (i = FIRST_LINE; i < LAST_LINE; i++) {
            _print_hex_line (i, tmp_off);
            tmp_off += BYTES_PER_ROW;
          }
          break;
        }
      case KEY_UP: {
          if (off - BYTES_PER_ROW >= offset) {
            off -= BYTES_PER_ROW;
            if (line == FIRST_LINE) {
              move(LAST_LINE - 1, col);
              deleteln ();
              move (line, col);
              insertln ();
              _print_hex_line (line, off);
            } else {
              line--;
            }
          }
          break;
        }
      case KEY_TAB: {
          if (mode == MODE_HEX) {
            mode = MODE_ASCII;
            col = FIRST_ASCII_COL + byte;
          } else {
            mode = MODE_HEX;
            col = FIRST_HEX_COL + 3 * byte;
          }
        }
      default: {
          if (flgEdit) {
            if (mode == MODE_HEX) {
              if (IS_HEX(ch)) {
                uint32_t    addr  = off + byte;
                uint8_t     value = xtoi (ch) << 4;

                ch = getch ();

                if (IS_HEX(ch)) {
                  value |= xtoi (ch);
                  POKE(addr, value);
                  move (line, FIRST_ASCII_COL + byte);

                  if (IS_PRINT(value)) {
                    addch (value);
                  } else {
                    addch ('.');
                  }
                }
              }
            } else  {// MODE_ASCII
              if (IS_PRINT(ch))  {
                uint32_t addr = off + byte;
                POKE(addr, ch);
                addch (ch);
                move (line, FIRST_HEX_COL + 3 * byte);
                _itoxx (ch);
              }
            }
          }
        }
    }
  } while (ch != KEY_ESCAPE);
}

