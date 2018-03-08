#include "ntsc.h"
#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "vs23s010.h"
#include "lock.h"
#include "ttconfig.h"

// #define DEBUG

// Remember to update VS23_MAX_X/Y!
struct vs23_mode_t VS23S010::modes[] = {
	// maximum usable without overscan, 76 6-pixel chars/line, 57 8-pixel
	// chars
	{456, 224, 9, 10, 3, 9, 11000000},
	// a bit smaller, may fit better on some TVs
	{432, 216, 13, 14, 3, 9, 11000000},
	{320, 216, 11, 15, 4, 9, 11000000},	// VS23 NTSC demo
	{320, 200, 20, 15, 4, 9, 14000000},	// (M)CGA, Commodore et al.
	{256, 224, 9, 15, 5, 9, 15000000},	// SNES
	{256, 192, 24, 15, 5, 8, 11000000},	// MSX, Spectrum, NDS XXX: has 
						// artifacts
	{160, 200, 20, 15, 8, 8, 11000000},	// Commodore/PCjr/CPC
						// multi-color
	// overscan modes
	{352, 240, 0, 8, 4, 9, 11000000},	// PCE overscan (is this
						// useful?)
	{282, 240, 0, 8, 5, 8, 11000000},	// PCE overscan (is this
						// useful?)
};

// Common modes not included:
// GBA (240x160): Lines do not scale evenly.
// Apple ][ hires: Does not fill the screen well (too narrow).

// Two-vclocks-per-pixel modes (>456 pixels wide are not included because of
// two issues:
// 1. At 8bpp, filling the entire screen would take more than 128k.
// 2. Only two microprogram instructions can be executed per pixel, making
// it impossible to support the full color gamut.

#define NUM_MODES (sizeof(VS23S010::modes)/sizeof(vs23_mode_t))
const uint8_t VS23S010::numModes = NUM_MODES;

const struct vs23_mode_t *vs23_current_mode = &VS23S010::modes[0];

static inline void vs23Select()
{
	SpiLock();
	VS23_SELECT;
}

static inline void vs23Deselect()
{
	VS23_DESELECT;
	SpiUnlock();
}

uint16_t SpiRamReadByte(uint32_t address)
{
	uint16_t result;

	vs23Select();
	SPI.transfer(3);
	SPI.transfer(address >> 16);
	SPI.transfer((uint8_t) (address >> 8));
	SPI.transfer((uint8_t) address);
	result = SPI.transfer(0);
	vs23Deselect();
	return result;
}

void ICACHE_RAM_ATTR SpiRamReadBytes(uint32_t address, uint8_t * data,
				     uint32_t count)
{
	uint8_t req[4];

	req[0] = 3;
	req[1] = address >> 16;
	req[2] = address >> 8;
	req[3] = address;
	vs23Select();
	SPI.writeBytes(req, 4);
	SPI.transferBytes(NULL, data, count);
	vs23Deselect();
}

void SpiRamWriteByte(register uint32_t address, uint8_t data)
{
	uint8_t req[5];

	vs23Select();
	req[0] = 2;
	req[1] = address >> 16;
	req[2] = address >> 8;
	req[3] = address;
	req[4] = data;
	SPI.writeBytes(req, 5);
	vs23Deselect();
	// if (SpiRamReadByte(address) != data)
	// Serial.printf("mismatch: %02X should be %02X\n",
	// SpiRamReadByte(address), data);
}

void ICACHE_RAM_ATTR SpiRamWriteBytes(uint32_t address, uint8_t * data, uint32_t len)
{
	uint8_t req[4];

	vs23Select();
	req[0] = 2;
	req[1] = address >> 16;
	req[2] = address >> 8;
	req[3] = address;
	SPI.writeBytes(req, 4);
	SPI.writeBytes(data, len);
	vs23Deselect();
}

void SpiRamWriteRegister(uint16_t opcode, uint16_t data)
{
	// Serial.printf("%02x <= %04xh\n",opcode,data);
	vs23Select();
	SPI.transfer(opcode);
	SPI.transfer(data >> 8);
	SPI.transfer((uint8_t) data);
	vs23Deselect();
}

void SpiRamWriteByteRegister(uint16_t opcode, uint16_t data)
{
	// Serial.printf("%02x <= %02xh\n",opcode,data);
	vs23Select();
	SPI.transfer(opcode);
	SPI.transfer((uint8_t) data);
	vs23Deselect();
}

void SpiRamWriteWord(uint16_t waddress, uint16_t data)
{
	uint8_t req[6];
	uint32_t address = (uint32_t) waddress * 2;

	vs23Select();
	req[0] = 2;
	req[1] = address >> 16;
	req[2] = address >> 8;
	req[3] = address;
	req[4] = data >> 8;
	req[5] = data;
	SPI.writeBytes(req, 6);
	vs23Deselect();
}

void SpiRamWriteProgram(register uint16_t opcode, register uint16_t data1,
			uint16_t data2)
{
	// Serial.printf("PROG:%04x%04xh\n",data1,data2);
	vs23Select();
	SPI.transfer(opcode);
	SPI.transfer(data1 >> 8);
	SPI.transfer((uint8_t) data1);
	SPI.transfer(data2 >> 8);
	SPI.transfer((uint8_t) data2);
	vs23Deselect();
}

uint16_t ICACHE_RAM_ATTR SpiRamReadRegister(uint16_t opcode)
{
	uint16_t result;

	vs23Select();
	SPI.transfer(opcode);
	result = SPI.transfer(0) << 8;
	result |= SPI.transfer(0);
	vs23Deselect();
	return result;
}

uint8_t SpiRamReadRegister8(uint16_t opcode)
{
	uint16_t result;

	vs23Select();
	SPI.transfer(opcode);
	result = SPI.transfer(0);
	vs23Deselect();
	return result;
}

void ICACHE_RAM_ATTR SpiRamWriteBMCtrl(uint16_t opcode, uint16_t data1,
				       uint16_t data2, uint16_t data3)
{
	uint8_t req[6] = { (uint8_t)opcode, (uint8_t)(data1 >> 8),
	                   (uint8_t)data1, (uint8_t)(data2 >> 8),
	                   (uint8_t)data2, (uint8_t)data3 };
	// Serial.printf("%02x <= %04x%04x%02xh\n",opcode,data1,data2,data3);
	vs23Select();
	SPI.writeBytes(req, 6);
	vs23Deselect();
}

void SpiRamWriteBMCtrlFast(uint16_t opcode, uint16_t data1, uint16_t data2)
{
	uint8_t req[5] = { (uint8_t)opcode, (uint8_t)(data1 >> 8),
	                   (uint8_t)data1, (uint8_t)(data2 >> 8),
	                   (uint8_t)data2 };
	VS23_SELECT;
	SPI.writeBytes(req, 5);
	VS23_DESELECT;
}

void ICACHE_RAM_ATTR SpiRamWriteBM2Ctrl(uint16_t data1, uint16_t data2,
					uint16_t data3)
{
	uint8_t req[5] = { 0x35, (uint8_t)(data1 >> 8), (uint8_t)data1,
	                   (uint8_t)data2, (uint8_t)data3 };
	// Serial.printf("%02x <= %04x%02x%02xh\n",opcode,data1,data2,data3);
	vs23Select();
	SPI.writeBytes(req, 5);
	vs23Deselect();
}

// / Set proto type picture line indexes 
void VS23S010::SetLineIndex(uint16_t line, uint16_t wordAddress)
{
	uint32_t indexAddr = INDEX_START_BYTES + line * 3;

	SpiRamWriteByte(indexAddr++, 0);	// Byteaddress and bits to 0,
						// proto to 0
	SpiRamWriteByte(indexAddr++, wordAddress);	// Actually it's
							// wordaddress
	SpiRamWriteByte(indexAddr, wordAddress >> 8);
}

// / Set picture type line indexes 
void VS23S010::SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress)
{
	uint32_t indexAddr = INDEX_START_BYTES + line * 3;

	// Byteaddress LSB, bits to 0, proto to given value
	SpiRamWriteByte(indexAddr++,
			(uint16_t)((byteAddress << 7) & 0x80) | (protoAddress &
								  0xf));
	// This is wordaddress
	SpiRamWriteByte(indexAddr++, (uint16_t)(byteAddress >> 1));
	SpiRamWriteByte(indexAddr, (uint16_t)(byteAddress >> 9));
}

// / Set picture pixel to a RGB value 
void VS23S010::setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g,
			   uint8_t b)
{
	uint16_t pixdata = colorFromRgb(r, g, b);

#ifndef BYTEPIC
	{
		uint16_t wordaddress;

		wordaddress = PICLINE_WORD_ADDRESS(ypos) + xpos;
		SpiRamWriteWord(wordaddress, pixdata);
	}
#else
	{
		uint32_t byteaddress;

		byteaddress = pixelAddr(xpos, ypos);
		SpiRamWriteByte(byteaddress, pixdata);
	}
#endif
}

// Draws a line between two points (x1,y1) and (x2,y2), y2 must be higher
// than or equal to y1
void VS23S010::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
			uint8_t c)
{
	int deltax, deltay, offset;
	uint16_t i;

	offset = 0;
	deltax = x2 - x1;
	deltay = y2 - y1;

	if (deltax != 0 && deltay != 0) {
		offset = x1 - deltax * y1 / deltay;
		for (i = 0; i < deltay; i++) {
			setPixel(deltax * (y1 + i) / deltay + offset, y1 + i,
				 c);
		}
	} else if (deltax == 0) {
		for (i = 0; i < deltay; i++) {
			setPixel(x1, y1 + i, c);
		}
	} else {
		for (i = 0; i < deltax; i++) {
			setPixel(x1 + i, y1, c);
		}
	}
}

void VS23S010::drawLineRgb(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
			   uint8_t r, uint8_t g, uint8_t b)
{
	int c = colorFromRgb(r, g, b);

	drawLine(x1, y1, x2, y2, c);
}

// / Fills a rectangle. Volor is given in RGB 565 format
void VS23S010::FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
			   uint16_t rgb)
{
	uint16_t re;
	uint16_t gr;
	uint16_t bl;
	uint16_t i;
	int deltax;

	deltax = x2 - x1;
	re = (rgb >> 8) & 0xf8;
	gr = (rgb >> 3) & 0xfc;
	bl = (rgb << 3) & 0xf8;

	for (i = 0; i < deltax; i++) {
		drawLineRgb(x1 + i, y1, x1 + i, y2, re, gr, bl);
	}
}

void VS23S010::setColorSpace(uint8_t palette)
{
	// 8. Set microcode program for picture lines
	// Use HROP1/HROP2/OP4/OP4 for 2 PLL clocks per pixel modes
	SpiRamWriteProgram(PROGRAM,
          (vs23_ops[palette][3] << 8) | vs23_ops[palette][2],
          (vs23_ops[palette][1] << 8) | vs23_ops[palette][0]
        );
	// Set color burst
	uint32_t w = PROTOLINE_WORD_ADDRESS(0) + BURST;
	for (int i = 0; i < BURSTDUR; i++)
		SpiRamWriteWord((uint16_t)w++, BURST_LEVEL | (vs23_ops[palette][4] << 8));

        switch (palette) {
        case 0: setColorConversion(0, 7, 3, 6, true); break;
        case 1: setColorConversion(1, 7, 4, 7, true); break;
        default: break;
        }
        m_colorspace = palette;
}

void VS23S010::setBorder(uint8_t y, uint8_t uv)
{
	uint32_t w = PROTOLINE_WORD_ADDRESS(0) + BLANKEND;
	for (int i = BLANKEND; i < FRPORCH; i++) {
		SpiRamWriteWord((uint16_t)w++, (uv << 8) | (y + 0x66));
	}
}

void SMALL VS23S010::SpiRamVideoInit()
{
	uint16_t i, j;
	uint32_t w;
	int original_spi_div = SPI1CLK;

	SPI1CLK = m_min_spi_div;
#ifdef DEBUG
	uint16_t linelen = PLLCLKS_PER_LINE;
	Serial.printf("Linelen: %d PLL clks\n", linelen);
	printf("Picture line area is %d x %d\n", PICX, PICY);
	printf("Upper left corner is point (0,0) and lower right corner (%d,%d)\n",
	       PICX - 1, PICY - 1);
	printf("Memory space available for picture bytes %ld\n",
	       131072 - PICLINE_START);
	printf("Free bytes %ld\n",
	       131072 - PICLINE_START - (PICX + BEXTRA) * PICY * PICBITS / 8);
	printf("Picture line %d bytes\n", PICLINE_LENGTH_BYTES + BEXTRA);
	printf("Picture length, %d color cycles\n", PICLENGTH);
	printf("Picture pixel bits %d\n", PICBITS);
	printf("Start pixel %x\n", (STARTPIX - 1));
	printf("End pixel %x\n", (ENDPIX - 1));
	printf("Index start address %x\n", INDEX_START_BYTES);
	printf("Picture line 0 address %lx\n", piclineByteAddress(0));
	printf("Last line %d\n", PICLINE_MAX);
#endif

	// Disable video generation
	SpiRamWriteRegister(VDCTRL2, 0);

	// 1. Select the first VS23 for following commands in case there
	// are several VS23 ICs connected to same SPI bus.
	SpiRamWriteByteRegister(WRITE_MULTIIC, 0xe);
	// 2. Set SPI memory address autoincrement
	SpiRamWriteByteRegister(WRITE_STATUS, 0x40);

	// 4. Write picture start and end
	SpiRamWriteRegister(PICSTART, (STARTPIX - 1));
	SpiRamWriteRegister(PICEND, (ENDPIX - 1));

	// 5. Enable PLL clock
	SpiRamWriteRegister(VDCTRL1,
			    (VDCTRL1_PLL_ENABLE) | (VDCTRL1_SELECT_PLL_CLOCK));
	// 6. Clear the video memory
	for (w = 0; w < 65536; w++)
		SpiRamWriteWord((uint16_t)w, 0x0000);	// Clear memory
	// 7. Set length of one complete line (unit: PLL clocks)
	SpiRamWriteRegister(LINELEN, (PLLCLKS_PER_LINE));
	// 9. Define where Line Indexes are stored in memory
	SpiRamWriteRegister(INDEXSTART, INDEX_START_LONGWORDS);
	// 10. Enable the PAL Y lowpass filter, not used in NTSC
	// SpiRamWriteBMCtrl(BLOCKMVC1,0,0,BLOCKMVC1_PYF);
	// 11. Set all line indexes to point to protoline 0 (which by
	// definition is in the beginning of the SRAM)
	for (i = 0; i < TOTAL_LINES; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(0));
	}

	// At this time, the chip would continuously output the proto line 0.
	// This protoline will become our most "normal" horizontal line.
	// For TV-Out, fill the line with black level, and insert a few
	// pixels of sync level (0) and color burst to the beginning.
	// Note that the chip hardware adds black level to all nonproto
	// areas so protolines and normal picture have different meaning for
	// the same Y value.
	// In protolines, Y=0 is at sync level and in normal picture Y=0 is at 
	// black level (offset +102).

	// In protolines, each pixel is 8 PLLCLKs, which in TV-out modes
	// means one color subcarrier cycle.  Each pixel has 16 bits (one
	// word):
	// VVVVUUUUYYYYYYYY.

if (m_interlace) {
	uint16_t wi;
	// Construct protoline 0 and 1. Protoline 0 is used for most of the
	// picture.  Protoline 1 has a similar first half than 0, but the
	// end has a short sync pulse.  This is used for line 623. 
	// Protoline 2 has a normal sync and color burst and nothing else. 
	// It is used between vertical sync lines and visible lines, but is
	// not mandatory always.
	for (j = 0; j <= 2; j++) {
		wi = PROTOLINE_WORD_ADDRESS(j);
		// Set all to blank level.
		for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
			SpiRamWriteWord(wi++, BLANK_LEVEL);
		}
		// Set the color level to black
		wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND;
		for (i = BLANKEND; i < FRPORCH; i++) {
			SpiRamWriteWord(wi++, BLACK_LEVEL);
		}
		// Set HSYNC
		wi = PROTOLINE_WORD_ADDRESS(j);
		for (i = 0; i < SYNC; i++)
			SpiRamWriteWord(wi++, SYNC_LEVEL);
		// Set color burst
		wi = PROTOLINE_WORD_ADDRESS(j) + BURST;
		for (i = 0; i < BURSTDUR; i++)
			SpiRamWriteWord(wi++, BURST_LEVEL);

#if 0
		// For testing purposes, make some interesting pattern to
		// protos 0 and 1.
		// Protoline 2 is blank from color burst to line end.
		if (j < 2) {
			wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND + 55;
			for (i = BLANKEND + 55; i < FRPORCH; i++) {
				SpiRamWriteWord(wi++, BLACK_LEVEL + 0x20);
			}
			wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND;
			SpiRamWriteWord(wi++, 0x7F);
			for (i = 1; i <= 50; i++) {
				// "Proto-maximum" green level + color
				// carrier wave
				SpiRamWriteWord(wi++, 0x797F + i * 0x1300);
			}
			// To make red+blue strip
#define RED_BIT 0x0400
#define BLUE_BIT 0x0800
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 126,
					(0x7300 | BLACK_LEVEL) + RED_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 130,
					(0x7300 | BLACK_LEVEL) + RED_BIT +
					BLUE_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 134,
					(0x7300 | BLACK_LEVEL) + BLUE_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 138,
					(0x7300 | BLACK_LEVEL));

			// Max V and min U levels
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 146,
					0x79c1);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 147,
					0x79c1);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 148,
					0x79c1);

			// Orangish color to the end of proto line
			w = PROTOLINE_WORD_ADDRESS(0) + FRPORCH - 1;
			for (i = 0; i <= 90; i++) {
				SpiRamWriteWord((uint16_t)w--,
						(WHITE_LEVEL - 0x30) | 0xc400);
			}
		}
#endif
	}

	// Add to the second half of protoline 1 a short sync
	wi = PROTOLINE_WORD_ADDRESS(1) + COLORCLKS_LINE_HALF;
	for (i = 0; i < SHORTSYNCM; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// middle of line
	for (i = COLORCLKS_LINE_HALF + SHORTSYNCM; i < COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);	// To the end of the
							// line to blank level
	}

	// Now let's construct protoline 3, this will become our short+short
	// VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(3);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(3);
	for (i = 0; i < SHORTSYNC; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// beginning of line
	wi = PROTOLINE_WORD_ADDRESS(3) + COLORCLKS_LINE_HALF;
	for (i = 0; i < SHORTSYNCM; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// middle of line

	// Now let's construct protoline 4, this will become our long+long
	// VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(4);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(4);
	for (i = 0; i < LONGSYNC; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
							// beginning of line
	wi = PROTOLINE_WORD_ADDRESS(4) + COLORCLKS_LINE_HALF;
	for (i = 0; i < LONGSYNCM; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
							// middle of line

	// Now let's construct protoline 5, this will become our long+short
	// VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(5);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(5);
	for (i = 0; i < LONGSYNC; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// beginning of line
	wi = PROTOLINE_WORD_ADDRESS(5) + COLORCLKS_LINE_HALF;
	for (i = 0; i < SHORTSYNCM; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
							// middle of line

	// And yet a short+long sync line, protoline 6
	wi = PROTOLINE_WORD_ADDRESS(6);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(6);
	for (i = 0; i < SHORTSYNC; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// beginning of line
	wi = PROTOLINE_WORD_ADDRESS(6) + COLORCLKS_LINE_HALF;
	for (i = 0; i < LONGSYNCM; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
							// middle of line

	// Just short sync line, the last one, protoline 7
	wi = PROTOLINE_WORD_ADDRESS(7);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(7);
	for (i = 0; i < SHORTSYNC; i++)
		SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
							// beginning of line

} else {	// interlace

	// Protolines for progressive NTSC, here is not created a protoline
	// corresponding to interlace protoline 2.
	// Construct protoline 0
	w = PROTOLINE_WORD_ADDRESS(0);	// Could be w=0 because proto 0 always 
					// starts at address 0
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}

	// Set the color level to black
	setBorder(0, 0);

	// Set HSYNC
	w = PROTOLINE_WORD_ADDRESS(0);
	for (i = 0; i < SYNC; i++)
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	// Makes a black&white picture
	// for (i=0; i<BURSTDUR; i++) SpiRamWriteWord(w++,BLANK_LEVEL);

	// Now let's construct protoline 1, this will become our short+short
	// VSYNC line
	w = PROTOLINE_WORD_ADDRESS(1);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(1);
	for (i = 0; i < SHORTSYNC; i++) {
		// Short sync at the beginning of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(1) + COLORCLKS_LINE_HALF;
	for (i = 0; i < SHORTSYNCM; i++) {
		// Short sync at the middle of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}

	// Now let's construct protoline 2, this will become our long+long
	// VSYNC line
	w = PROTOLINE_WORD_ADDRESS(2);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(2);
	for (i = 0; i < LONGSYNC; i++) {
		// Long sync at the beginning of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(2) + COLORCLKS_LINE_HALF;
	for (i = 0; i < LONGSYNCM; i++) {
		// Long sync at the middle of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}

	// Now let's construct protoline 3, this will become our long+short
	// VSYNC line
	w = PROTOLINE_WORD_ADDRESS(3);
	for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(3);
	for (i = 0; i < LONGSYNC; i++) {
		// Short sync at the beginning of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(3) + COLORCLKS_LINE_HALF;
	for (i = 0; i < SHORTSYNCM; i++) {
		// Long sync at the middle of line
		SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
	}
}

	setColorSpace(0);

	// 12. Now set first eight lines of frame to point to NTSC sync lines
	// Here the frame starts, lines 1 and 2
	if (m_interlace) {
		if (m_pal) {
			for (i=0; i<2; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
			}
			// Line 3
			SetLineIndex(2, PROTOLINE_WORD_ADDRESS(5));
			// Lines 4 and 5
			for (i=3; i<5; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			// At this point, lines 0 to 4 are VSYNC lines and all other lines are proto0.

			// Another batch of VSYNC lines is made between the two fields of interlaced PAL picture.
			// The lines start from 310.
			for (i=FIELD1START; i<FIELD1START+2; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			SetLineIndex(FIELD1START+2, PROTOLINE_WORD_ADDRESS(6));
			for (i=FIELD1START+3; i<FIELD1START+5; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
			}
			for (i=FIELD1START+5; i<FIELD1START+7; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			SetLineIndex(FIELD1START+7, PROTOLINE_WORD_ADDRESS(7));

			// Set empty lines with sync and color burst to beginning of both frames.
			for (i=5; i<FRONT_PORCH_LINES; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}
			for (i=FIELD1START+8; i<FIELD1START+FRONT_PORCH_LINES+2; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}

			// These are the three last lines of the frame, lines 623-625
			SetLineIndex(TOTAL_LINES-1-3, PROTOLINE_WORD_ADDRESS(1));
			for (i=TOTAL_LINES-1-2; i<=TOTAL_LINES-1; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
		} else {
			for (i = 0; i < 3; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			// Lines 4 to 6
			for (i = 3; i < 6; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
			}
			// Lines 7 to 9
			for (i = 6; i < 9; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			// At this point, lines 0 to 8 are VSYNC lines and all other lines are 
			// proto0.

			// Another batch of VSYNC lines is made between the two fields of
			// interlaced NTSC picture.
			// The lines start from 261.
			SetLineIndex(FIELD1START, PROTOLINE_WORD_ADDRESS(1));
			for (i = FIELD1START + 1; i < FIELD1START + 3; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			SetLineIndex(FIELD1START + 3, PROTOLINE_WORD_ADDRESS(6));
			for (i = FIELD1START + 4; i < FIELD1START + 6; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
			}
			SetLineIndex(FIELD1START + 6, PROTOLINE_WORD_ADDRESS(5));
			for (i = FIELD1START + 7; i < FIELD1START + 9; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
			}
			SetLineIndex(FIELD1START + 9, PROTOLINE_WORD_ADDRESS(7));

			// Set empty lines with sync and color burst to beginning of both
			// frames.
			for (i = 9; i < FRONT_PORCH_LINES; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}
			for (i = FIELD1START + 10; i < FIELD1START + FRONT_PORCH_LINES; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}
		}
	} else {	// interlace
		if (m_pal) {
			// For progressive PAL case, this is quite a minimum case.		
			// Here the frame starts, lines 1 and 2
			for (i=0; i<2; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}
			// Line 3
			SetLineIndex(2, PROTOLINE_WORD_ADDRESS(3));
			// Lines 4 and 5
			for (i=3; i<5; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
			}
			// These are three last lines of the frame, lines 310-312
			for (i=TOTAL_LINES-3; i<TOTAL_LINES; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
			}
		} else {
			// For progressive NTSC case, this is quite a minimum case.  
			// Here the frame starts, lines 1 to 3
			for (i = 0; i < 3; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
			}
			// Lines 4 to 6
			for (i = 3; i < 6; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
			}
			// Lines 7 to 9
			for (i = 6; i < 9; i++) {
				SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
			}
		}
	}

	// 13. Set pic line indexes to point to protoline 0 and their
	// individual picture line.
	for (i = 0; i < ENDLINE - STARTLINE; i++) {
		SetPicIndex(i + STARTLINE, piclineByteAddress(i), 0);
		// All lines use picture line 0
		// SetPicIndex(i+STARTLINE, PICLINE_BYTE_ADDRESS(0),0);
if (m_interlace) {
		// In interlaced case in both fields the same area is picture
		// box area.
		// XXX: In PAL example, it says "TOTAL_LINES/2" instead of FIELD1START
		SetPicIndex(i + STARTLINE + FIELD1START, piclineByteAddress(i),
			    0);
}
	}

	// Draw some color bars
	{
		uint16_t re = 0;
		uint16_t gr = 0;
		uint16_t bl = 0;
		uint16_t xp = 0;

		for (i = 0; i < 256; i++) {

			for (j = 0; j < 16; j++) {
				xp = 10 + (i % 16) * 18 + j;
				if (xp > XPIXELS - 10)
					continue;
				drawLine(xp, 6 + (i / 16) * 12, xp,
					 16 + (i / 16) * 12, i);
			}
		}

		// Frame around the picture box
		re = 0;
		gr = 255;
		bl = 0;
		drawLineRgb(0, 0, 0, PICY - 1, re, gr, bl);
		drawLineRgb(0, 0, PICX - 1, 0, re, gr, bl);
		drawLineRgb(PICX - 1, 0, PICX - 1, PICY - 1, re, gr, bl);
		drawLineRgb(0, PICY - 1, PICX - 1, PICY - 1, re, gr, bl);
	}

	// Fixes the picture to proto area border artifacts if BEXTRA > 0.
	if (BEXTRA > 0) {
		for (i = 0; i < ENDLINE - STARTLINE; i++) {
			for (j = 0; j < BEXTRA; j++) {
#ifdef BYTEPIC
				if (j % 2 == 1) {
#else
				if (j % 2 == 0) {
#endif
					// V and U of the first proto pixel
					// after picture.
					SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)
							+ PICLINE_LENGTH_BYTES +
							j, 0x00);
				} else {
					// Y of the first proto pixel after
					// picture.
					SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)
							+ PICLINE_LENGTH_BYTES +
							j, 0x37);
				}
			}
		}
	}
	SPI1CLK = original_spi_div;

	// 14. Set number of lines, length of pixel and enable video
	// generation
	// The line count is TOTAL_LINES - 1 in the example, but the only
	// value that doesn't produce a crap picture is 263 (even interlaced!),
	// so we hard-code it here.
	SpiRamWriteRegister(VDCTRL2, (VDCTRL2_LINECOUNT * 263)
			    | (VDCTRL2_PIXEL_WIDTH * (PLLCLKS_PER_PIXEL - 1))
			    | (m_pal ? VDCTRL2_PAL : 0)
			    | (VDCTRL2_ENABLE_VIDEO));

}

void ICACHE_RAM_ATTR VS23S010::MoveBlock(uint16_t x_src, uint16_t y_src,
					 uint16_t x_dst, uint16_t y_dst,
					 uint8_t width, uint8_t height,
					 uint8_t dir)
{
	static uint8_t last_dir = 0;

#ifdef DEBUG
	if (x_src < 0 || x_dst < 0 || x_src + width > m_current_mode->x ||
	    x_dst + width > m_current_mode->x || y_src < 0 || y_dst < 0 ||
	    y_src + height > lastLine() || y_dst + height > lastLine()) {
		Serial.printf("BADMOV %dx%d %d,%d -> %d,%d\n", width, height,
			      x_src, y_src, x_dst, y_dst);
	}
#endif
	uint32_t byteaddress1 = pixelAddr(x_dst, y_dst);
	uint32_t byteaddress2 = pixelAddr(x_src, y_src);

	// If the last move was a reverse one, we have to wait until it's
	// finished before we can set the new addresses.
	if (last_dir)
		while (!blockFinished()) {
		}
	SpiRamWriteBMCtrl(BLOCKMVC1, byteaddress2 >> 1, byteaddress1 >> 1,
			  ((byteaddress1 & 1) << 1) | ((byteaddress2 & 1) << 2)
			  | dir | lowpass());
	if (!last_dir)
		while (!blockFinished()) {
		}
	SpiRamWriteBM2Ctrl(m_pitch - width, width, height - 1);
	startBlockMove();
	last_dir = dir;
}
