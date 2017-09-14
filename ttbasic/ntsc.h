#ifndef NTSC_H
#define NTSC_H

//#include <vsos.h>
//#include "devHwSpi.h"
#include <stdint.h>

/// Selects interlaced NTSC picture. When commented progressive NTSC is used.		
//#define INTERLACE
/// Selects byte wide pixel. When commented word wide pixel is used.
#define BYTEPIC	
		
/// Crystal frequency in MHZ (float, observe accuracy)
#define XTAL_MHZ 3.579545
/// Line length in microseconds (float, observe accuracy)
#define LINE_LENGTH_US 63.5555
/// Frame length in lines (visible lines + nonvisible lines)
/// Amount has to be odd for NTSC and RGB colors
#ifdef INTERLACE
#define TOTAL_LINES 525
#define FIELD1START 261
#else
#define TOTAL_LINES 263
#endif
		
/// Number of lines used after the VSYNC but before visible area.
#define FRONT_PORCH_LINES 20
		
/// Width, in PLL clocks, of each pixel
/// Used 4 to 8 for 160x120 pics
#define PLLCLKS_PER_PIXEL 4

/// Extra bytes can be added to end of picture lines to prevent pic-to-proto 
/// border artifacts. 8 is a good value. 0 can be tried to test, if there is 
/// no need for extra bytes.		
#define BEXTRA 8

/// Definitions for picture lines
/// On which line the picture area begins, the Y direction.
//#define STARTLINE ((uint16_t)(TOTAL_LINES/4))
#define STARTLINE (FRONT_PORCH_LINES+11)
//#define YPIXELS 120
#define YPIXELS 215
/// The last picture area line
#define ENDLINE STARTLINE+YPIXELS
/// The first pixel of the picture area, the X direction.
#define STARTPIX (BLANKEND+15)
/// The last pixel of the picture area. Set PIXELS to wanted value and suitable 
/// ENDPIX value is calculated.
//#define XPIXELS 161
#define XPIXELS 319
#define ENDPIX ((uint16_t)(STARTPIX+PLLCLKS_PER_PIXEL*XPIXELS/8))
//#define ENDPIX 195		

/// Reserve memory for this number of different prototype lines
/// (prototype lines are used for sync timing, porch and border area)
#ifdef INTERLACE
#define PROTOLINES 8
#else
#define PROTOLINES 4
#endif

/// Define USE_SLOTTED_PROTOS if you want to use fixed length prototype "slots"
/// (allows patterned border area by defining different protolines
/// also for visible lines in addition to nonvisible lines)
//#define USE_SLOTTED_PROTOS
		
#ifdef USE_SLOTTED_PROTOS
/// Protoline lenght is one slot fixed in VS23S010 hardware: 512 bytes
/// (if your real protoline lenght is longer than one slot, you must 
/// use several slots per proto and there are total 16 slots)
#define PROTOLINE_LENGTH_WORDS 512
#else
/// Protoline lenght is the real lenght of protoline (optimal memory 
/// layout, but visible lines' prototype must always be proto 0)
#define PROTOLINE_LENGTH_WORDS ((uint16_t)(LINE_LENGTH_US*XTAL_MHZ+0.5))
#endif
		
/// PLL frequency
#define PLL_MHZ (XTAL_MHZ * 8.0)
/// 10 first pllclks, which are not in the counters are decremented here
#define PLLCLKS_PER_LINE ((uint16_t)((LINE_LENGTH_US * PLL_MHZ)+0.5-10))
/// 10 first pllclks, which are not in the counters are decremented here
#define COLORCLKS_PER_LINE ((uint16_t)((LINE_LENGTH_US * XTAL_MHZ)+0.5-10.0/8.0))
#define COLORCLKS_LINE_HALF ((uint16_t)((LINE_LENGTH_US * XTAL_MHZ)/2+0.5-10.0/8.0))
	
#define PROTO_AREA_WORDS (PROTOLINE_LENGTH_WORDS * PROTOLINES)
#define INDEX_START_LONGWORDS ((PROTO_AREA_WORDS+1)/2)
#define INDEX_START_WORDS (INDEX_START_LONGWORDS * 2)
#define INDEX_START_BYTES (INDEX_START_WORDS * 2)
		
/// Define NTSC video timing constants
/// NTSC short sync duration is 2.35 us
#define SHORT_SYNC_US 2.542
/// For the start of the line, the first 10 extra PLLCLK sync (0) cycles
/// are subtracted.
#define SHORTSYNC ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ-10.0/8.0))
/// For the middle of the line the whole duration of sync pulse is used.
#define SHORTSYNCM ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ))
/// NTSC long sync duration is 27.3 us
#define LONG_SYNC_US 27.33275
#define LONGSYNC ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))
#define LONGSYNCM ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))
/// Normal visible picture line sync length is 4.7 us
#define SYNC_US 4.7
#define SYNC ((uint16_t)(SYNC_US*XTAL_MHZ-10.0/8.0))
/// Color burst starts at 5.6 us
#define BURST_US 5.3
#define BURST ((uint16_t)(BURST_US*XTAL_MHZ-10.0/8.0))
/// Color burst duration is 2.25 us
#define BURST_DUR_US 2.67
#define BURSTDUR ((uint16_t)(BURST_DUR_US*XTAL_MHZ))
/// NTSC sync to blanking end time is 10.5 us
#define BLANK_END_US 9.155
#define BLANKEND ((uint16_t)(BLANK_END_US*XTAL_MHZ-10.0/8.0))
/// Front porch starts at the end of the line, at 62.5us 
#define FRPORCH_US 61.8105
#define FRPORCH ((uint16_t)(FRPORCH_US*XTAL_MHZ-10.0/8.0))

/// Select U, V and Y bit widths for 16-bit or 8-bit wide pixels.
#ifndef BYTEPIC
#define UBITS 4
#define VBITS 4
#define YBITS 8
#else
#define UBITS 2
#define VBITS 2
#define YBITS 4
#endif

/// Protoline 0 starts always at address 0
#define PROTOLINE_BYTE_ADDRESS(n) (PROTOLINE_LENGTH_WORDS)*2*(n))
#define PROTOLINE_WORD_ADDRESS(n) (PROTOLINE_LENGTH_WORDS*(n))

/// Calculate picture lengths in pixels and bytes, coordinate areas for picture area
#define PICLENGTH (ENDPIX-STARTPIX)
#define PICX ((uint16_t)(PICLENGTH*8/PLLCLKS_PER_PIXEL))
#define PICY (ENDLINE-STARTLINE)
#define PICBITS (UBITS+VBITS+YBITS)
#ifndef BYTEPIC
#define PICLINE_LENGTH_BYTES ((uint16_t)(PICX*PICBITS/8+0.5))
#else
#define PICLINE_LENGTH_BYTES ((uint16_t)(PICX*PICBITS/8+0.5+1))
#endif
/// Picture area memory start point
#define PICLINE_START (INDEX_START_BYTES+TOTAL_LINES*3+1)
/// Picture area line start addresses
#define PICLINE_WORD_ADDRESS(n) (PICLINE_START/2+(PICLINE_LENGTH_BYTES/2+BEXTRA/2)*(n))
#define PICLINE_BYTE_ADDRESS(n) ((uint32_t)(PICLINE_START+((uint32_t)(PICLINE_LENGTH_BYTES)+BEXTRA)*(n)))

/// 8-bit RGB to 8-bit YUV444 conversion
#define YRGB(r,g,b) ((76*r+150*g+29*b)>>8)
#define URGB(r,g,b) (((r<<7)-107*g-20*b)>>8)
#define VRGB(r,g,b) ((-43*r-84*g+(b<<7))>>8)

/// Pattern generator microcode
/// ---------------------------
/// Bits 7:6  a=00|b=01|y=10|-=11
/// Bits 5:3  n pick bits 1..8
/// bits 2:0  shift 0..6
#define PICK_A (0<<6)
#define PICK_B (1<<6)
#define PICK_Y (2<<6)
#define PICK_NOTHING (3<<6)
#define PICK_BITS(a)(((a)-1)<<3)
#define SHIFT_BITS(a)(a)

#ifndef BYTEPIC
/// 16 bits per pixel, U4 V4 Y8
/// PICK_B is U
#define OP1 (PICK_B + PICK_BITS(4) + SHIFT_BITS(4))
/// PICK_A is V
#define OP2 (PICK_A + PICK_BITS(4) + SHIFT_BITS(4))
#define OP3 (PICK_Y + PICK_BITS(8) + SHIFT_BITS(6))
#define OP4 (PICK_NOTHING + SHIFT_BITS(2))
/// U & V data shift values in a 16-bit pixel are defined.
#define USHIFT 12
#define VSHIFT 8
#else
/// 8 bits per pixel, U2 V2 Y4
#define OP1 (PICK_B + PICK_BITS(2) + SHIFT_BITS(2))
#define OP2 (PICK_A + PICK_BITS(2) + SHIFT_BITS(2))
#define OP3 (PICK_Y + PICK_BITS(4) + SHIFT_BITS(4))
#define OP4 (PICK_NOTHING)
/// U & V data shift values in a 8-bit pixel are defined.
#define USHIFT 6
#define VSHIFT 4
#endif
	
/// General VS23 commands
#define WRITE_STATUS 0x01
#define WRITE_MULTIIC 0xb8

/// Bit definitions
#define VDCTRL1 0x2B
#define VDCTRL1_UVSKIP (1<<0)
#define VDCTRL1_DACDIV (1<<3)
#define VDCTRL1_PLL_ENABLE (1<<12)
#define VDCTRL1_SELECT_PLL_CLOCK (1<<13)
#define VDCTRL1_USE_UVTABLE (1<<14)
#define VDCTRL1_DIRECT_DAC (1<<15)

#define VDCTRL2 0x2D
#define VDCTRL2_LINECOUNT (1<<0)
#define VDCTRL2_PIXEL_WIDTH (1<<10)
#define VDCTRL2_NTSC (0<<14)
#define VDCTRL2_PAL (1<<14)
#define VDCTRL2_ENABLE_VIDEO (1<<15)

#define BLOCKMVC1_PYF (1<<4)

/// VS23 video commands
#define PROGRAM 0x30
#define PICSTART 0x28
#define PICEND 0x29
#define LINELEN 0x2a
#define LINELEN_VGP_OUTPUT (1<<15)
#define YUVBITS 0x2b
#define INDEXSTART 0x2c
#define LINECFG 0x2d
#define VTABLE 0x2e
#define UTABLE 0x2f
#define BLOCKMVC1 0x34
#define CURLINE 0x53

/// Sync, blank, burst and white level definitions, here are several options
/// These are for proto lines and so format is VVVVUUUUYYYYYYYY 
/// Sync is always 0
#define SYNC_LEVEL  0x0000
/// one LSB is 5.1724137mV
#define SYNC_LEVEL  0x0000
/// 285 mV to 75 ohm load
#define BLANK_LEVEL 0x0066
/// 339 mV to 75 ohm load
#define BLACK_LEVEL 0x0066
/// 285 mV burst
#define BURST_LEVEL 0x0d66
#define WHITE_LEVEL 0x00ff


//extern DEVICE spi1;

void SpiRamInit();
void SpiRamWriteByte(register uint32_t address, uint8_t data);
void SpiRamWriteBytes(uint32_t address, uint8_t *data, uint32_t len);
uint16_t SpiRamReadByte(register uint32_t address);
uint16_t SpiRamReadRegister(register uint16_t opcode);
void SpiRamWriteWord(uint16_t waddress, uint16_t data);
void SpiRamWriteRegister(register uint16_t opcode, register uint16_t data);
void SpiRamVideoInit();
void SpiRamWrite7Words(uint16_t waddress, uint16_t *data);
void SpiRamWrite8Words(uint16_t waddress, uint16_t *data);
void MoveBlock (uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t x2, uint16_t y2);
void SpiRamWriteProgram(register uint16_t opcode, register uint16_t data1, uint16_t data2);
void SpiRamWriteBMCtrl(register uint16_t opcode, register uint16_t data1, uint16_t data2, uint16_t data3);
void SpiRamWriteByteRegister(register uint16_t opcode, register uint16_t data);
	
#endif
