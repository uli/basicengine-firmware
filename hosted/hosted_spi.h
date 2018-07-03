#include <stdint.h>

void MoveBlockAddr(uint32_t byteaddress2, uint32_t dest_addr);
void SpiRamReadBytesFast(uint32_t address, uint8_t *data, uint32_t count);
void SpiRamWriteBytesFast(uint32_t address, uint8_t *data, uint32_t len);
