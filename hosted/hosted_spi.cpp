#include "hosted_spi.h"
#include <stdio.h>
#include "ntsc.h"

uint8_t vs23_mem[131072];

void MoveBlockAddr(uint32_t byteaddress2, uint32_t dest_addr) {
  printf("MBA  %08X %08X\n", byteaddress2, dest_addr);
}

void SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count) {
  printf("RRBF %08X %p %d\n", address, data, count);
  for (int i = 0; i < count; ++i)
    data[i] = vs23_mem[(address+i) % 131072];
}
void SpiRamReadBytes(uint32_t address, uint8_t *data, uint32_t count) {
  printf("RRB  %08X %p %d\n", address, data, count);
  for (int i = 0; i < count; ++i)
    data[i] = vs23_mem[(address+i) % 131072];
}
uint16_t SpiRamReadByte(uint32_t address) {
  printf("RRB  %08X\n", address);
  return vs23_mem[address % 131072];
}

void SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len) {
  printf("RWBF %08X %p %d\n", address, data, len);
  for (int i = 0; i < len; ++i)
    vs23_mem[(address+i) % 131072] = data[i];
}
void SpiRamWriteWord(uint16_t waddress, uint16_t data) {
  //printf("RWW  %08X %04X\n", waddress, data);
  vs23_mem[(waddress * 2) % 131072] = data;
  vs23_mem[(waddress * 2 + 1) % 131072] = data >> 8;
}
void SpiRamWriteByte(register uint32_t address, uint8_t data) {
  printf("RWB  %08X %02X\n", address, data);
  vs23_mem[address % 131072] = data;
}
void SpiRamWriteBytes(uint32_t address, uint8_t * data, uint32_t len) {
  //printf("RWBS %08X %p %d\n", address, data, len);
  for (int i = 0; i < len; ++i)
    vs23_mem[(address+i) % 131072] = data[i];
}

void SpiRamWriteBM2Ctrl(uint16_t data1, uint16_t data2, uint16_t data3) {
  printf("RWBM2C %04X %04X %04X\n", data1, data2, data3);
}
void SpiRamWriteBMCtrl(uint16_t opcode, uint16_t data1, uint16_t data2, uint16_t data3) {
  printf("RWBMC  %04X %04X %04X %04X\n", opcode, data1, data2, data3);
}
void SpiRamWriteBMCtrlFast(uint16_t opcode, uint16_t data1, uint16_t data2) {
  printf("RWBMCF %04X %04X %04X\n", opcode, data1, data2);
}

uint16_t SpiRamReadRegister(register uint16_t opcode) {
  switch (opcode) {
  case CURLINE:
    return (micros() / 64) % 261;	// XXX: PAL? accuracy? configuration?
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
  printf("RWR  %04X %04X\n", opcode, data);
}
void SpiRamWriteByteRegister(uint16_t opcode, uint16_t data) {
  printf("RWBR %04X %04X\n", opcode, data);
}

void SpiRamWriteProgram(uint16_t opcode, uint16_t data1, uint16_t data2) {
  printf("RWP  %04X %04X %04X\n", opcode, data1, data2);
}
