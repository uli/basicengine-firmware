#include "hosted_spi.h"
#include <stdio.h>
#include "ntsc.h"
#include "SPI.h"

uint8_t vs23_mem[131072];
struct vs23_int vs23_int;

void SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count) {
  //printf("RRBF %08X %p %d\n", address, data, count);
  for (int i = 0; i < count; ++i)
    data[i] = vs23_mem[(address+i) % 131072];
}
void SpiRamReadBytes(uint32_t address, uint8_t *data, uint32_t count) {
  //printf("RRBS %08X %p %d\n", address, data, count);
  for (int i = 0; i < count; ++i)
    data[i] = vs23_mem[(address+i) % 131072];
}
uint16_t SpiRamReadByte(uint32_t address) {
  printf("RRB  %08X\n", address);
  return vs23_mem[address % 131072];
}

void SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len) {
  //printf("RWBF %08X %p %d\n", address, data, len);
  for (int i = 0; i < len; ++i)
    vs23_mem[(address+i) % 131072] = data[i];
}
void SpiRamWriteWord(uint16_t waddress, uint16_t data) {
  //printf("RWW  %08X %04X\n", waddress, data);
  vs23_mem[(waddress * 2) % 131072] = data >> 8;
  vs23_mem[(waddress * 2 + 1) % 131072] = data;
}
void SpiRamWriteByte(register uint32_t address, uint8_t data) {
  //printf("RWB  %08X %02X\n", address, data);
  vs23_mem[address % 131072] = data;
}
void SpiRamWriteBytes(uint32_t address, uint8_t * data, uint32_t len) {
  //printf("RWBS %08X %p %d\n", address, data, len);
  for (int i = 0; i < len; ++i)
    vs23_mem[(address+i) % 131072] = data[i];
}

uint32_t mvsrc, mvtgt;
uint16_t mvskp;
uint8_t mvlen, mvlin;
int mvdir;

void SpiRamWriteBM2Ctrl(uint16_t data1, uint16_t data2, uint16_t data3) {
  //printf("RWBM2C %04X %04X %04X\n", data1, data2, data3);
  mvskp = data1;
  mvlen = data2;
  mvlin = data3;
}
void SpiRamWriteBMCtrl(uint16_t opcode, uint16_t data1, uint16_t data2, uint16_t data3) {
  //printf("RWBMC  %04X %04X %04X %04X\n", opcode, data1, data2, data3);
  mvsrc = (data1 << 1) | ((data3 >> 2) & 1);
  mvtgt = (data2 << 1) | ((data3 >> 1) & 1);
  mvdir = data3 & 1 ? -1 : 1;
}
void SpiRamWriteBMCtrlFast(uint16_t opcode, uint16_t data1, uint16_t data2) {
  printf("RWBMCF %04X %04X %04X\n", opcode, data1, data2);
  mvsrc = (mvsrc & 1) | (data1 << 1);
  mvtgt = (mvtgt & 1) | (data2 << 1);
}

void MoveBlockAddr(uint32_t byteaddress2, uint32_t dest_addr) {
  //printf("MBA  %08X %08X\n", byteaddress2, dest_addr);
  mvsrc = byteaddress2;
  mvtgt = dest_addr;
  SPI.write(0x36);
}

uint16_t SpiRamReadRegister(register uint16_t opcode) {
  switch (opcode) {
  case CURLINE:
    return int(micros() / vs23_int.line_us) % vs23_int.line_count;
  default:
    printf("RRR  %04X\n", opcode);
    return 0xffff;
  }
}

uint8_t SpiRamReadRegister8(uint16_t opcode) {
  printf("RRR8 %04X\n", opcode);
  return 0xff;
}

void SpiRamWriteRegister(uint16_t opcode, uint16_t data) {
  switch (opcode) {
  case VDCTRL1:
    vs23_int.vdctrl1 = data;
    break;
  case VDCTRL2:
    vs23_int.vdctrl2 = data;
    vs23_int.line_count = (data & 0x3ff) - 1;
    vs23_int.enabled = !!(data & 0x8000);
    vs23_int.pal = !!(data & 0x4000);
    vs23_int.line_us = vs23_int.pal ? LINE_LENGTH_US_PAL : LINE_LENGTH_US_NTSC;
    vs23_int.plen = ((data >> 10) & 0xf) + 1;
    break;
  case WRITE_STATUS:	vs23_int.write_status = data; break;
  case PICSTART:	vs23_int.picstart = data; break;
  case PICEND:	vs23_int.picend = data; break;
  case LINELEN:	vs23_int.linelen = data; break;
  case INDEXSTART:	vs23_int.indexstart = data; break;
  default:
    printf("RWR  %04X %04X\n", opcode, data);
    break;
  }
}
void SpiRamWriteByteRegister(uint16_t opcode, uint16_t data) {
  printf("RWBR %04X %04X\n", opcode, data);
}

void SpiRamWriteProgram(uint16_t opcode, uint16_t data1, uint16_t data2) {
  printf("RWP  %04X %04X %04X\n", opcode, data1, data2);
}

void SPIClass::write(uint8_t data) {
  if (data == 0x36) {
    //printf("mov src %05X tgt %05X dir %d lin %d len %d skp %d\n",
    //  mvsrc, mvtgt, mvdir, mvlin, mvlen, mvskp);
    int vs, vt, x, y;
    for (vs = mvsrc, vt = mvtgt, x = 0, y = 0;; ) {
      vs23_mem[vt] = vs23_mem[vs];
      x++; vs += mvdir; vt += mvdir;
      if (x == mvlen) {
        vt += mvdir * mvskp;
        vs += mvdir * mvskp;
        x = 0;
        y++;
        if (y == mvlin + 1)
          break;
      }
    }
  } else
    printf("SPIwrite %04X\n",data);
}
