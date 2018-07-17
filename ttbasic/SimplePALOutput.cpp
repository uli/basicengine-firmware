#ifdef ESP32

#include "SimplePALOutput.h"

inline void SimplePALOutput::sendLine(unsigned short *l)
{
  i2s_write_bytes(I2S_PORT, (char*)l, lineSamples * sizeof(unsigned short), portMAX_DELAY);
}

void SimplePALOutput::sendSync1() {
  //long sync
  for(int i = 0; i < 3; i++)
    sendLine(longSync);
  //short sync
  for(int i = 0; i < 4; i++)
    sendLine(shortSync);
  //blank lines
  for(int i = 0; i < 37; i++)
    sendLine(blank);
}

void SimplePALOutput::sendSync2() {
  for(int i = 0; i < 25; i++)
    sendLine(blank);
  for(int i = 0; i < 3; i++)
    sendLine(shortSync);
}

void __attribute__((optimize("O3"))) SimplePALOutput::sendFrame(uint8_t **frame)
{
  int l = 0;
  sendSync1();

  //image
  for(int i = 0; i < 240; i += 2)
  {
    uint8_t *pixels0 = frame[i];
    uint8_t *pixels1 = frame[i + 1];
    int j = frameStart;
    for(int x = 0; x < imageSamples; x += 2)
    {
      uint8_t px0 = *pixels0++;
      uint8_t px1 = *pixels1++;
      unsigned short p0v = yuv2v[px0];
      unsigned short p0u = yuv2u[px0];
      unsigned short p1u = yuv2u[px1];
      unsigned short p1v = yuv2v[px1];
      
      short y0 = yuv2y[px0];
      short y1 = yuv2y[px1];
      short u = UVLUT[(p0u+p1u)/2];
      short v = UVLUT[(p1v+p0v)/2];
      short u0 = (SIN[x] * u);
      short u1 = (SIN[x + 1] * u);
      short v0 = (COS[x] * v);
      short v1 = (COS[x + 1] * v);
      //word order is swapped for I2S packing (j + 1) comes first then j
      line[0][j] = y0 + u1 + v1;
      line[1][j] = y1 + u1 - v1;
      line[0][j + 1] = y0 + u0 + v0;
      line[1][j + 1] = y1 + u0 - v0;
      j += 2;
    }
    sendLine(line[0]);
    sendLine(line[1]);
  }
  
  sendSync2();
}

#endif	// ESP32
