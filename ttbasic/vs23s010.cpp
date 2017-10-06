#include "ttconfig.h"

#if USE_VS23 == 1

#include "vs23s010.h"
#include "ntsc.h"

void VS23S010::setPixel(uint16_t x, uint16_t y, uint8_t c)
{
  uint32_t byteaddress = PICLINE_BYTE_ADDRESS(y) + x;
  SpiRamWriteByte(byteaddress, c);
}

void VS23S010::adjust(int16_t cnt)
{
  // XXX: Huh?
}

void VS23S010::begin(uint8_t mode, uint8_t spino, uint8_t *extram)
{
}

void VS23S010::end()
{
}

VS23S010 vs23;
#endif
