#include "ntsc.h"
#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "vs23s010.h"

const struct vs23_mode_t VS23S010::modes[] = {
  // maximum usable without overscan, 76 6-pixel chars/line, 57 8-pixel chars
  { 456, 224, 9, 10, 3, 1 },
  // a bit smaller, may fit better on some TVs
  { 432, 216, 13, 14, 3, 1 },
  { 320, 216, 11, 15, 4, 1 },	// VS23 NTSC demo
  { 320, 200, 20, 15, 4, 1 },	// (M)CGA, Commodore et al.
  { 256, 224, 9, 15, 5, 0 },	// SNES
  { 256, 192, 24, 15, 5, 0 },	// MSX, Spectrum, NDS
  { 160, 200, 20, 15, 8, 0 },	// Commodore/PCjr/CPC multi-color
  // overscan modes
  { 352, 240, 0, 8, 4, 1 },	// PCE overscan (is this useful?)
  { 282, 240, 0, 8, 5, 0 },	// PCE overscan (is this useful?)
};

// Common modes not included:
// GBA (240x160): Lines do not scale evenly.
// Apple ][ hires: Does not fill the screen well (too narrow).

// Two-vclocks-per-pixel modes (>456 pixels wide are not included because of
// two issues:
// 1. At 8bpp, filling the entire screen would take more than 128k.
// 2. Only two microprogram instructions can be executed per pixel, making
//    it impossible to support the full color gamut.

#define NUM_MODES (sizeof(VS23S010::modes)/sizeof(vs23_mode_t))
const uint8_t VS23S010::numModes = NUM_MODES;

const struct vs23_mode_t *vs23_current_mode = &VS23S010::modes[0];

uint16_t SpiRamReadByte(uint32_t address)
{
  uint16_t result;
  digitalWrite(15, LOW);
  SPI.transfer(3);
  SPI.transfer(address >> 16);
  SPI.transfer((uint8_t)(address >> 8));
  SPI.transfer((uint8_t)address);
  result = SPI.transfer(0);
  digitalWrite(15, HIGH);
  return result;  
}

struct bw {
  uint32_t address;
   
};

void SpiRamWriteByte(register uint32_t address, uint8_t data) {
  uint8_t req[5];
  digitalWrite(15, LOW);
  //SPI.transfer(2);
  req[0] = 2;
  //SPI.transfer(address >> 16);
  req[1] = address >> 16;
  //SPI.transfer((uint8_t)(address >> 8));
  req[2] = address >> 8;
  //SPI.transfer((uint8_t)address);
  req[3] = address;
  //SPI.transfer(data);
  req[4] = data;
  SPI.writeBytes(req, 5);
  digitalWrite(15, HIGH);
  //if (SpiRamReadByte(address) != data)
  //  Serial.printf("mismatch: %02X should be %02X\n", SpiRamReadByte(address), data);
}

void SpiRamWriteBytes(uint32_t address, uint8_t *data, uint32_t len)
{
  uint8_t req[4];
  digitalWrite(15, LOW);
  req[0] = 2;
  req[1] = address >> 16;
  req[2] = address >> 8;
  req[3] = address;
  SPI.writeBytes(req, 4);
  SPI.writeBytes(data, len);
  digitalWrite(15, HIGH);
}

void SpiRamWriteRegister(uint16_t opcode, uint16_t data)
{
  digitalWrite(15, LOW);
  SPI.transfer(opcode);
  SPI.transfer(data >> 8);
  SPI.transfer((uint8_t)data);
  digitalWrite(15, HIGH);
}

void SpiRamWriteByteRegister(uint16_t opcode, uint16_t data)
{
  digitalWrite(15, LOW);
  SPI.transfer(opcode);
  SPI.transfer((uint8_t)data);
  digitalWrite(15, HIGH);
}

void SpiRamWriteWord(uint16_t waddress, uint16_t data)
{
  uint8_t req[6];
  uint32_t address = (uint32_t)waddress * 2;
  digitalWrite(15, LOW);
#if 0
  SPI.transfer(2);
  SPI.transfer(address >> 16);
  SPI.transfer((uint8_t)(address >> 8));
  SPI.transfer((uint8_t)address);
  SPI.transfer(data >> 8);
  SPI.transfer((uint8_t)data);
#else
  req[0] = 2;
  req[1] = address >> 16;
  req[2] = address >> 8;
  req[3] = address;
  req[4] = data >> 8;
  req[5] = data;
  SPI.writeBytes(req, 6);
#endif
  digitalWrite(15, HIGH);
}

void SpiRamWriteProgram(register uint16_t opcode, register uint16_t data1, uint16_t data2)
{
  digitalWrite(15, LOW);
  SPI.transfer(opcode);
  SPI.transfer(data1 >> 8);
  SPI.transfer((uint8_t)data1);
  SPI.transfer(data2 >> 8);
  SPI.transfer((uint8_t)data2);
  digitalWrite(15, HIGH);
}

uint16_t SpiRamReadRegister(uint16_t opcode)
{
  uint16_t result;
  digitalWrite(15, LOW);
  SPI.transfer(opcode);
  result = SPI.transfer(0) << 8;
  result |= SPI.transfer(0);
  digitalWrite(15, HIGH);
  return result;
}

uint8_t SpiRamReadRegister8(uint16_t opcode)
{
  uint16_t result;
  digitalWrite(15, LOW);
  SPI.transfer(opcode);
  result = SPI.transfer(0);
  digitalWrite(15, HIGH);
  return result;
}

void SpiRamWriteBMCtrl(uint16_t opcode, uint16_t data1, uint16_t data2, uint16_t data3) {
	Serial.printf("%02x <= %04x%04x%02xh\n",opcode,data1,data2,data3);
	digitalWrite(15, LOW);
	SPI.transfer(opcode);
	SPI.transfer(data1>>8);
	SPI.transfer(data1);
	SPI.transfer(data2>>8);
	SPI.transfer(data2);
	SPI.transfer(data3);
	digitalWrite(15, HIGH);
}

void SpiRamWriteBM2Ctrl(uint16_t opcode, uint16_t data1, uint16_t data2, uint16_t data3) {
	Serial.printf("%02x <= %04x%02x%02xh\n",opcode,data1,data2,data3);
	digitalWrite(15, LOW);
	SPI.transfer(opcode);
	SPI.transfer(data1>>8);
	SPI.transfer(data1);
	SPI.transfer(data2);
	SPI.transfer(data3);
	digitalWrite(15, HIGH);
}

void SpiRamWriteBM3Ctrl(uint16_t opcode) {
	Serial.printf("%02x\n",opcode);
	digitalWrite(15, LOW);
	SPI.transfer(opcode);
	digitalWrite(15, HIGH);
}

/// Set proto type picture line indexes	
void SetLineIndex(uint16_t line, uint16_t wordAddress) {
	uint32_t indexAddr = INDEX_START_BYTES + line*3;
	SpiRamWriteByte(indexAddr++,0); // Byteaddress and bits to 0, proto to 0
	SpiRamWriteByte(indexAddr++,wordAddress); // Actually it's wordaddress
	SpiRamWriteByte(indexAddr,wordAddress >> 8);
}

/// Set picture type line indexes		
void SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress) {
	uint32_t indexAddr = INDEX_START_BYTES + line*3;
	SpiRamWriteByte(indexAddr++,(uint16_t)((byteAddress << 7) & 0x80) | (protoAddress & 0xf)); // Byteaddress LSB, bits to 0, proto to given value
	SpiRamWriteByte(indexAddr++,(uint16_t)(byteAddress >> 1)); //This is wordaddress
	SpiRamWriteByte(indexAddr,(uint16_t)(byteAddress >> 9));
}
	
/// Set picture pixel to a yuv value
void SetPixyuv(uint16_t xpos, uint16_t ypos, uint16_t yuv) {
#ifndef BYTEPIC
        {
                uint16_t wordaddress;
                wordaddress = PICLINE_WORD_ADDRESS(ypos)+xpos;
                SpiRamWriteWord(wordaddress,yuv);
        }
#else
        {
                uint32_t byteaddress;
                byteaddress = PICLINE_BYTE_ADDRESS(ypos)+xpos;
                SpiRamWriteByte(byteaddress,yuv);
        }
#endif
}

	
/// Set picture pixel to a RGB value 
void SetPixel(uint16_t xpos, uint16_t ypos, uint16_t r, uint16_t g, uint16_t b) {
	uint16_t pixdata;
	// this is for 4 bits U, 4 bits V and 8 bits Y
	// Y/U/VRGB give out 8 bit values, Y positive, U&V signed integers
	pixdata = 	((URGB(r,g,b)>>(8-UBITS))<<USHIFT) |
				((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) |
				((YRGB(r,g,b)>>(8-YBITS)));
#ifndef BYTEPIC
	{	
		uint16_t wordaddress;
		wordaddress = PICLINE_WORD_ADDRESS(ypos)+xpos;
		SpiRamWriteWord(wordaddress,pixdata);
	}
#else
	{
		uint32_t byteaddress;
		byteaddress = PICLINE_BYTE_ADDRESS(ypos)+xpos;
		SpiRamWriteByte(byteaddress,pixdata);
	}
#endif	
}

/// Draws a line between two points (x1,y1) and (x2,y2), y2 must be higher than or equal to y1
void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, uint16_t g, uint16_t b) {
	int deltax, deltay, offset;
	uint16_t i,j,ystart;
	offset = 0;
	deltax = x2-x1;
	deltay = y2-y1;

	if (deltax != 0 && deltay != 0){
		offset = x1-deltax*y1/deltay;
		for (i=0;i<deltay;i++){
			SetPixel(deltax*(y1+i)/deltay+offset,y1+i,r,g,b);
		}
	} else if (deltax==0) {
		for (i=0;i<deltay;i++){
			SetPixel(x1,y1+i,r,g,b);
		}
	} else {
		for (i=0;i<deltax;i++){
			SetPixel(x1+i,y1,r,g,b);
		}
	}
}

/// Fills a rectangle. Volor is given in RGB 565 format
void FillRect565(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t rgb) {
	uint16_t re;
	uint16_t gr;
	uint16_t bl;
	uint16_t i;
	int deltax;
			
	deltax = x2-x1;
	re = (rgb>>8)&0xf8;
	gr = (rgb>>3)&0xfc;
	bl = (rgb<<3)&0xf8;
			
	for (i=0;i<deltax;i++) {
		DrawLine(x1+i,y1,x1+i,y2,re,gr, bl);
	}
}
	
/// Writes a 16-bit pixel stripe. This is used by VSOS display driver.
uint16_t *SpiRamWriteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels) {
#ifndef BYTEPIC
	uint32_t address = ((uint32_t)(PICLINE_WORD_ADDRESS(y)) + x) * 2;
#else	
	uint32_t address = ((uint32_t)(PICLINE_WORD_ADDRESS(y))* 2) + x;
#endif
	static uint16_t buf[2];
#if 0
	spi1.Ioctl(&spi1,IOCTL_START_FRAME,0);
	buf[0] = (0x02 << 8) | (uint16_t)(address >> 16);
	buf[1] = ((uint16_t)address & 0xffff);
	spi1.Write(&spi1,buf,0,4);

	spi1.Write(&spi1,pixels,0,width*2);
	spi1.Ioctl(&spi1,IOCTL_END_FRAME,0);
#endif
	return pixels+width;
}

/// Writes a 8-bit pixel stripe. This is used by VSOS display driver.
uint16_t *SpiRamWriteByteStripe(uint16_t x, uint16_t y, uint16_t width, uint16_t *pixels) {
	uint32_t address = PICLINE_BYTE_ADDRESS(y) + x;
	static uint16_t buf[2];
#if 0
	spi1.Ioctl(&spi1,IOCTL_START_FRAME,0);
	buf[0] = (0x02 << 8) | (uint16_t)(address >> 16);
	buf[1] = ((uint16_t)address & 0xffff);
	spi1.Write(&spi1,buf,0,4);
	
	spi1.Write(&spi1,pixels,0,width+1);
	spi1.Ioctl(&spi1,IOCTL_END_FRAME,0);
#endif
	return pixels+width;
}
				
/// Handler for VSOS standard LcdFilledRectangle calling convention
void TvFilledRectangle (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *texture, uint16_t color) {
	uint16_t w;
	static uint16_t buf[400]; //WAARALLISTA! 
	uint16_t r,g,b;
	
#if 0
	if (x1 >= PICX) return;
	if (x2 >= PICX) x2=PICX-1;
#endif		
	if (y1 >= PICY) return;
	if (y2 >= PICY) y2=PICY-1;

	w = (x2-x1)+1;
	if (w>400) w=400;
	if (!texture) {	
		r = (color>>8)&0xf8;
		g = (color>>3)&0xfc;
		b = (color<<3)&0xf8;		
		color = ((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS));
#ifdef	BYTEPIC
		color |= (color << 8);
#endif
		memset(buf,color,w);
		while (y1 <= y2) {
#ifndef BYTEPIC				
			SpiRamWriteStripe(x1,y1,w,buf);
#else
			SpiRamWriteByteStripe(x1,y1,w,buf);
#endif	
			y1++;
		}
	} else {
		while (y1 <= y2) {
			uint16_t i;
			uint16_t *t = texture;
			uint16_t *out = texture;
			for (i=0; i<w; i++)  {
				r = (*t>>8)&0xf8;
				g = (*t>>3)&0xfc;
				b = (*t<<3)&0xf8;	
#ifndef BYTEPIC
				*t++ = ((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS));
#else
				*t ++;
				if (i%2==0) {
					*out = (((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS))) << 8;
				} else {
					*out++ |= (((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS)));
				}
				if (i==w-1) {
					*out = (((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS))) << 8;
					*out |= (((URGB(r,g,b)>>(8-UBITS))<<USHIFT) | ((VRGB(r,g,b)>>(8-VBITS))<<VSHIFT) | (YRGB(r,g,b)>>(8-YBITS)));
				}
#endif
			}
#ifndef BYTEPIC			
			texture = SpiRamWriteStripe(x1,y1,w,texture);
#else
			texture = SpiRamWriteByteStripe(x1,y1,w,texture);
#endif			
			y1++;
		}	
	}
}



void VS23S010::SpiRamVideoInit() {
	uint16_t i,j,wi;
	uint32_t w;
	uint16_t linelen = PLLCLKS_PER_LINE;
	
	// Init SPI of VS1005 and check VD23 id	
	//SpiRamInit();
			
	Serial.printf("Linelen: %d PLL clks\n",linelen);
	printf("Picture line area is %d x %d\n",PICX,PICY);
	printf("Upper left corner is point (0,0) and lower right corner (%d,%d)\n",PICX-1,PICY-1);
	printf("Memory space available for picture bytes %ld\n", 131072-PICLINE_START);
	printf("Free bytes %ld\n", 131072-PICLINE_START-(PICX+BEXTRA)*PICY*PICBITS/8);
	printf("Picture line %d bytes\n",PICLINE_LENGTH_BYTES+BEXTRA);
	printf("Picture length, %d color cycles\n",PICLENGTH);
	printf("Picture pixel bits %d\n",PICBITS);
	printf("Start pixel %x\n",(STARTPIX-1));
	printf("End pixel %x\n",(ENDPIX-1));
	printf("Index start address %x\n",INDEX_START_BYTES);
	printf("Picture line 0 address %lx\n",PICLINE_BYTE_ADDRESS(0));

	// 1. Select the first VS23 for following commands in case there
	// are several VS23 ICs connected to same SPI bus.
	SpiRamWriteByteRegister(WRITE_MULTIIC, 0xe);
	// 2. Set SPI memory address autoincrement
	SpiRamWriteByteRegister(WRITE_STATUS, 0x40);
	
	// 3. XReset & XCSPar pins of VS23 to high on the testcard
/*	PERIP(GPIO2_MODE) &= ~(0x3);
	PERIP(GPIO2_DDR) |= 0x3;
	PERIP(GPIO2_SET_MASK) = 0x3; */
		
	// 4. Write picture start and end
	SpiRamWriteRegister(PICSTART, (STARTPIX-1));
	SpiRamWriteRegister(PICEND, (ENDPIX-1));
				
	// 5. Enable PLL clock
	SpiRamWriteRegister(VDCTRL1, (VDCTRL1_PLL_ENABLE) | (VDCTRL1_SELECT_PLL_CLOCK));
	// 6. Clear the video memory
	for (w=0; w<65536; w++) SpiRamWriteWord((uint16_t)w,0x0000); //Clear memory
	// 7. Set length of one complete line (unit: PLL clocks)
	SpiRamWriteRegister(LINELEN, (PLLCLKS_PER_LINE));
	// 8. Set microcode program for picture lines
	SpiRamWriteProgram(PROGRAM,
	(OP4 << 8) | OP3,
	(OP2 << 8) | OP1);
	// 9. Define where Line Indexes are stored in memory
	SpiRamWriteRegister(INDEXSTART, INDEX_START_LONGWORDS);
	// 10. Enable the PAL Y lowpass filter, not used in NTSC
	//SpiRamWriteBMCtrl(BLOCKMVC1,0,0,BLOCKMVC1_PYF);
	// 11. Set all line indexes to point to protoline 0 (which by definition is in the beginning of the SRAM)
	for (i=0; i<TOTAL_LINES; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(0));
	}
	
	// At this time, the chip would continuously output the proto line 0.
	// This protoline will become our most "normal" horizontal line.
	// For TV-Out, fill the line with black level,
	// and insert a few pixels of sync level (0) and color burst to the beginning.
	// Note that the chip hardware adds black level to all nonproto areas so
	// protolines and normal picture have different meaning for the same Y value.
	// In protolines, Y=0 is at sync level and in normal picture Y=0 is at black level (offset +102).
			
	// In protolines, each pixel is 8 PLLCLKs, which in TV-out modes means one color
	// subcarrier cycle. Each pixel has 16 bits (one word): VVVVUUUUYYYYYYYY.
			
#ifdef INTERLACE
	// Construct protoline 0	and 1. Protoline 0 is used for most of the picture. Protoline 1
	// has a similar first half than 0, but the end has a short sync pulse. This is used for
	// line 623. Protoline 2 has a normal sync and color burst and nothing else. It is used 
	// between vertical sync lines and visible lines, but is not mandatory always.
	for (j=0; j<=2; j++) {
		wi = PROTOLINE_WORD_ADDRESS(j);
		// Set all to blank level.
		for (i=0; i<=COLORCLKS_PER_LINE; i++) {
			SpiRamWriteWord(wi++, BLANK_LEVEL);
		}
		//Set the color level to black
		wi = PROTOLINE_WORD_ADDRESS(j)+BLANKEND; 
		for (i=BLANKEND; i<FRPORCH; i++) {
			SpiRamWriteWord(wi++, BLACK_LEVEL);
		}
		// Set HSYNC
		wi = PROTOLINE_WORD_ADDRESS(j);
		for (i=0; i<SYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL);
		// Set color burst
		wi = PROTOLINE_WORD_ADDRESS(j)+BURST;
		for (i=0; i<BURSTDUR; i++) SpiRamWriteWord(wi++,BURST_LEVEL);
			
		// For testing purposes, make some interesting pattern to protos 0 and 1.
		// Protoline 2 is blank from color burst to line end.
		if (j<2) {
			wi = PROTOLINE_WORD_ADDRESS(j)+BLANKEND+55;
			for (i=BLANKEND+55; i<FRPORCH; i++) {
				SpiRamWriteWord(wi++, BLACK_LEVEL+0x20);
			}
			wi = PROTOLINE_WORD_ADDRESS(j)+BLANKEND;
			SpiRamWriteWord(wi++, 0x7F);
			for (i=1; i<=50; i++) {
				SpiRamWriteWord(wi++, 0x797F+i*0x1300); //"Proto-maximum" green level + color carrier wave
			}
			// To make red+blue strip
			#define RED_BIT 0x0400
			#define BLUE_BIT 0x0800
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 126, (0x7300 | BLACK_LEVEL) + RED_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 130, (0x7300 | BLACK_LEVEL) + RED_BIT + BLUE_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 134, (0x7300 | BLACK_LEVEL) + BLUE_BIT);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 138, (0x7300 | BLACK_LEVEL));
					
			// Max V and min U levels
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 146, 0x79c1);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 147, 0x79c1);
			SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 148, 0x79c1);
			
			// Orangish color to the end of proto line
			w = PROTOLINE_WORD_ADDRESS(0)+FRPORCH-1;
			for (i=0; i<=90; i++) {
				SpiRamWriteWord((uint16_t)w--, (WHITE_LEVEL-0x30)|0xc400);
			}
		}
	}
		
	// Add to the second half of protoline 1 a short sync
	wi = PROTOLINE_WORD_ADDRESS(1)+COLORCLKS_LINE_HALF;
	for (i=0; i<SHORTSYNCM; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the middle of line
	for (i=COLORCLKS_LINE_HALF+SHORTSYNCM; i<COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL); // To the end of the line to blank level
	}
					
	// Now let's construct protoline 3, this will become our short+short VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(3);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(3);
	for (i=0; i<SHORTSYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the beginning of line
	wi = PROTOLINE_WORD_ADDRESS(3)+COLORCLKS_LINE_HALF;
	for (i=0; i<SHORTSYNCM; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the middle of line
		
			
	// Now let's construct protoline 4, this will become our long+long VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(4);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(4);
	for (i=0; i<LONGSYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Long sync at the beginning of line
	wi = PROTOLINE_WORD_ADDRESS(4)+COLORCLKS_LINE_HALF;
	for (i=0; i<LONGSYNCM; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Long sync at the middle of line
		
	// Now let's construct protoline 5, this will become our long+short VSYNC line
	wi = PROTOLINE_WORD_ADDRESS(5);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(5);
	for (i=0; i<LONGSYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the beginning of line
	wi = PROTOLINE_WORD_ADDRESS(5)+COLORCLKS_LINE_HALF;
	for (i=0; i<SHORTSYNCM; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Long sync at the middle of line
		
	// And yet a short+long sync line, protoline 6
	wi = PROTOLINE_WORD_ADDRESS(6);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(6);
	for (i=0; i<SHORTSYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the beginning of line
	wi = PROTOLINE_WORD_ADDRESS(6)+COLORCLKS_LINE_HALF;
	for (i=0; i<LONGSYNCM; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Long sync at the middle of line
			
	// Just short sync line, the last one, protoline 7
	wi = PROTOLINE_WORD_ADDRESS(7);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord(wi++, BLANK_LEVEL);
	}
	wi = PROTOLINE_WORD_ADDRESS(7);
	for (i=0; i<SHORTSYNC; i++) SpiRamWriteWord(wi++,SYNC_LEVEL); // Short sync at the beginning of line
			
#else
			
	// Protolines for progressive NTSC, here is not created a protoline corresponding to interlace protoline 2.
	// Construct protoline 0
	w = PROTOLINE_WORD_ADDRESS(0); // Could be w=0 because proto 0 always starts at address 0
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	// Set the color level to black
	w = PROTOLINE_WORD_ADDRESS(0)+BLANKEND; 
	for (i=BLANKEND; i<FRPORCH; i++) {
		SpiRamWriteWord((uint16_t)w++, BLACK_LEVEL);
	}
	// Set HSYNC
	w = PROTOLINE_WORD_ADDRESS(0);
	for (i=0; i<SYNC; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL);
	// Set color burst
	w = PROTOLINE_WORD_ADDRESS(0)+BURST;
	for (i=0; i<BURSTDUR; i++) SpiRamWriteWord((uint16_t)w++,BURST_LEVEL);
	// Makes a black&white picture
	//for (i=0; i<BURSTDUR; i++) SpiRamWriteWord(w++,BLANK_LEVEL);
		
	// For testing purposes, make some interesting pattern to proto 0
#if 0	
	w = PROTOLINE_WORD_ADDRESS(0)+BLANKEND;
	SpiRamWriteWord(w++, 0x7F);
	for (i=1; i<=50; i++) {
		SpiRamWriteWord(w++, 0x797F+i*0x1300); //"Proto-maximum" green level + color carrier wave
	}
#endif	

#if 0	
	// To make red+blue strip
	#define RED_BIT 0x0400
	#define BLUE_BIT 0x0800
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 126, 0x7380 + RED_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 130, 0x7380 + RED_BIT + BLUE_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 134, 0x7380 + BLUE_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 138, 0x7380);
	// Max V and min U levels
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 146, 0x79c1);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 147, 0x79c1);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 148, 0x79c1);
	
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 156, 0xf54f+BLANK_LEVEL);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 157, 0xf54f+BLANK_LEVEL);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(0) + 158, 0xf54f+BLANK_LEVEL);

	// Orangish color to the end of proto line
	
	w = PROTOLINE_WORD_ADDRESS(0)+FRPORCH-1; 
	for (i=0; i<=90; i++) {
		SpiRamWriteWord((uint16_t)w--, (WHITE_LEVEL-0x30)|0xc400);
	}
#endif	

	// Now let's construct protoline 1, this will become our short+short VSYNC line
	w = PROTOLINE_WORD_ADDRESS(1);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(1);
	for (i=0; i<SHORTSYNC; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Short sync at the beginning of line
	w = PROTOLINE_WORD_ADDRESS(1)+COLORCLKS_LINE_HALF;
	for (i=0; i<SHORTSYNCM; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Short sync at the middle of line
		
			
	// Now let's construct protoline 2, this will become our long+long VSYNC line
	w = PROTOLINE_WORD_ADDRESS(2);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(2);
	for (i=0; i<LONGSYNC; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Long sync at the beginning of line
	w = PROTOLINE_WORD_ADDRESS(2)+COLORCLKS_LINE_HALF;
	for (i=0; i<LONGSYNCM; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Long sync at the middle of line
		
	// Now let's construct protoline 3, this will become our long+short VSYNC line
	w = PROTOLINE_WORD_ADDRESS(3);
	for (i=0; i<=COLORCLKS_PER_LINE; i++) {
		SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
	}
	w = PROTOLINE_WORD_ADDRESS(3);
	for (i=0; i<LONGSYNC; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Short sync at the beginning of line
	w = PROTOLINE_WORD_ADDRESS(3)+COLORCLKS_LINE_HALF;
	for (i=0; i<SHORTSYNCM; i++) SpiRamWriteWord((uint16_t)w++,SYNC_LEVEL); // Long sync at the middle of line	
#endif
			
	// 12. Now set first eight lines of frame to point to NTSC sync lines
	// Here the frame starts, lines 1 and 2
#ifdef INTERLACE
	for (i=0; i<3; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
	}
	// Lines 4 to 6
	for (i=3; i<6; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
	}
	// Lines 7 to 9
	for (i=6; i<9; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
	}
	// At this point, lines 0 to 8 are VSYNC lines and all other lines are proto0.
		
	// Another batch of VSYNC lines is made between the two fields of interlaced NTSC picture.
	// The lines start from 261.
	SetLineIndex(FIELD1START, PROTOLINE_WORD_ADDRESS(1));
	for (i=FIELD1START+1; i<FIELD1START+3; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
	}
	SetLineIndex(FIELD1START+3, PROTOLINE_WORD_ADDRESS(6));
	for (i=FIELD1START+4; i<FIELD1START+6; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
	}
	SetLineIndex(FIELD1START+6, PROTOLINE_WORD_ADDRESS(5));
	for (i=FIELD1START+7; i<FIELD1START+9; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
	}
	SetLineIndex(FIELD1START+9, PROTOLINE_WORD_ADDRESS(7));
	
	// Set empty lines with sync and color burst to beginning of both frames.
	for (i=9; i<FRONT_PORCH_LINES; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
	}
	for (i=FIELD1START+10; i<FIELD1START+FRONT_PORCH_LINES; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
	}
			
#else
	
	// For progressive NTSC case, this is quite a minimum case.		
	// Here the frame starts, lines 1 to 3
	for (i=0; i<3; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
	}
	// Lines 4 to 6
	for (i=3; i<6; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
	}
	// Lines 7 to 9
	for (i=6; i<9; i++) {
		SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
	}
#endif
	
	// 13. Set pic line indexes to point to protoline 0 and their individual picture line.
	for (i=0; i<ENDLINE-STARTLINE; i++) {
		SetPicIndex(i+STARTLINE, PICLINE_BYTE_ADDRESS(i),0);
		// All lines use picture line 0
		//SetPicIndex(i+STARTLINE, PICLINE_BYTE_ADDRESS(0),0);
#ifdef INTERLACE
		// In interlaced case in both fields the same area is picture box area.
		SetPicIndex(i+STARTLINE+FIELD1START, PICLINE_BYTE_ADDRESS(i),0);
#endif
	}
	
	// 14. Set number of lines, length of pixel and enable video generation
	SpiRamWriteRegister(VDCTRL2, (VDCTRL2_LINECOUNT*(TOTAL_LINES-1))
	| (VDCTRL2_PIXEL_WIDTH * (PLLCLKS_PER_PIXEL-1))
	| (VDCTRL2_ENABLE_VIDEO));
		
#if 0
	// Set some random background color to picture area
	for (i=0; i<ENDLINE-STARTLINE; i++) {
		for (j=0; j<=PICLINE_LENGTH_BYTES;j++) {
#ifndef BYTEPIC		
			if (j%2==0){
				SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)+j,0);
			} else {
				SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)+j,0x20); // Some random level, grey
			}
#else
			SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)+j,0x2); // Some random level, grey
#endif			
		}
	}
#endif

	// Draw some color bars
	{
		uint16_t re=0;
		uint16_t gr=0;
		uint16_t bl=0;
		uint16_t rgb565=0;
		uint16_t k;
		uint16_t xp=0;

		for (i=0;i<256;i++){
	
			for (j=0;j<16;j++){
				xp=10+(i%16)*18+j;
				if (xp > XPIXELS)
					continue;
				for (k=0;k<10;k++){
					SetPixyuv(xp,6+(i/16)*12+k,i);				
				}
			}
		}

		
#if 0
		DrawLine(50, 50, 100, 100, re, gr, bl);
		DrawLine(50, 50, 50, 100, re, gr, bl);
		DrawLine(50, 50, 100, 50, re, gr, bl);
				
		for (i=0;i<10;i++){
			DrawLine(150+i, 50, 100+i, 100, re, gr, bl);
			DrawLine(150, 50+i, 100, 100+i, re, gr, bl);
		}
#endif
		
		// Frame around the picture box
		re=0;
		gr=255;
		bl=0;
		DrawLine(0, 0, 0, PICY-1, re, gr, bl);
		DrawLine(0, 0, PICX-1, 0, re, gr, bl);		
		DrawLine(PICX-1, 0, PICX-1, PICY-1, re, gr, bl);
		DrawLine(0, PICY-1, PICX-1, PICY-1, re, gr, bl);
#if 0
		// Some rectangles
		// 255 0 34  f8, 0 7
		rgb565=0xf807;
		FillRect565(100, 100, 215, 177, rgb565);
		// 166 125 60 a0 3e 1
		rgb565=0xa401;
		FillRect565(150, 150, 216, 180, rgb565);
#endif
	}
	
	// Read video logic line pointer register
 for(int i=0; i < 10; ++i) {
	Serial.printf("Current line: %04x\n",SpiRamReadRegister(CURLINE));
  delayMicroseconds(42);
 }
 Serial.printf("ID%04X\n", SpiRamReadRegister(0x9f));
	
	// Init NTSC display as VSOS3 monitor
#if 0
	InitVODisplay();
#endif
		
	// Fixes the picture to proto area border artifacts if BEXTRA > 0.
	if (BEXTRA>0) {
		for (i=0; i<ENDLINE-STARTLINE; i++) {
			for (j=0; j<BEXTRA;j++) {
#ifdef BYTEPIC				
				if (j%2==1){
#else
					if (j%2==0){
#endif
						SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)+PICLINE_LENGTH_BYTES+j,0x00); // V and U of the first proto pixel after picture.
					} else {
						SpiRamWriteByte(PICLINE_BYTE_ADDRESS(i)+PICLINE_LENGTH_BYTES+j,0x37); // Y of the first proto pixel after picture.
					}
				}
			}
		}
//	SpiRamWriteBMCtrl(0x34, 0, 0, 0x10);
	SpiRamWriteBMCtrl(0x34, 0, 0, 0x00);
	}

void MoveBlock (uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir)
{
  static uint8_t last_dir = 0;
  uint32_t byteaddress1 = PICLINE_BYTE_ADDRESS(y_dst)+x_dst;
  uint32_t byteaddress2 = PICLINE_BYTE_ADDRESS(y_src)+x_src;
  // If the last move was a reverse one, we have to wait until it's finished
  // before we can set the new addresses.
  if (last_dir)
    while (!blockFinished()) {}
  SpiRamWriteBMCtrl(0x34, byteaddress2 >> 1, byteaddress1 >> 1, ((byteaddress1 & 1) << 1) | ((byteaddress2 & 1) << 2) | dir);
  if (!last_dir)
    while (!blockFinished()) {}
  SpiRamWriteBM2Ctrl(0x35, PICLINE_LENGTH_BYTES+BEXTRA+1-width-1, width, height-1);
  SpiRamWriteBM3Ctrl(0x36);
  last_dir = dir;
}
