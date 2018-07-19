#ifdef ESP32

#include "esp32gfx.h"

inline void SimplePALOutput::sendLine(unsigned short *l)
{
  i2s_write_bytes(I2S_PORT, (char*)l, lineSamples * sizeof(unsigned short), portMAX_DELAY);
}

void SimplePALOutput::sendSync1(int blank_lines) {
  //long sync
  for(int i = 0; i < 3; i++)
    sendLine(longSync);
  //short sync
  for(int i = 0; i < 4; i++)
    sendLine(shortSync);
  //blank lines
  for(int i = 0; i < blank_lines; i++)
    sendLine(blank);
}

void SimplePALOutput::sendSync2(int blank_lines) {
  for(int i = 0; i < blank_lines; i++)
    sendLine(blank);
  for(int i = 0; i < 3; i++)
    sendLine(shortSync);
}

const struct video_mode_t ESP32GFX::modes_pal[SPO_NUM_MODES] PROGMEM = {
        // Much smaller than the screen, but have to be compatible with NTSC.
	{460, 224, 45, 256, 1},
	// This could work with a pixel clock divider of 4, but it would be
	// extremely wide (near overscan).
	{436, 216, 49, 268, 1},
	{320, 216, 49, 168, 2},	// VS23 NTSC demo
	{320, 200, 57, 168, 2},	// (M)CGA, Commodore et al.
	{256, 224, 45, 232, 2},	// SNES
	{256, 192, 61, 232, 2},	// MSX, Spectrum, NDS
	{160, 200, 57, 168, 4},	// Commodore/PCjr/CPC
						// multi-color
	// "Overscan modes" are actually underscan on PAL.
	{352, 240, 37, 108, 2},	// PCE overscan (barely)
	{282, 240, 37, 200, 2},	// PCE overscan (underscan on PAL)
	// Maximum PAL (the timing would allow more, but we would run out of memory)
	// NB: This does not line up with any font sizes; the width is chosen
	// to avoid artifacts.
	{508, 240, 37, 234, 1},
	{320, 256, 29, 168, 2},	// maximum PAL at 2 clocks per pixel
	{320, 240, 37, 168, 2},	// DawnOfAV demo, Mode X
	// maximum the software renderer can handle with 4 DMA buffers
	// more is possible with more buffers, but the memory usage is
	// prohibitive
	{512, 256, 29, 232, 1},
};

void __attribute__((optimize("O3"))) SimplePALOutput::sendFrame(
  const struct video_mode_t *mode, uint8_t **frame)
{
  int l = 0;
  sendSync1(mode->top);

  //image
  for(int i = 0; i < mode->y; i += 2)
  {
    uint8_t *pixels0 = frame[i];
    uint8_t *pixels1 = frame[i + 1];
    int j = mode->left;
    for(int x = 0; x < mode->x * 2; x += 2)
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
  
  sendSync2(302 - mode->y - mode->top);
}

void __attribute__((optimize("O3"))) SimplePALOutput::sendFrame1ppc(
  const struct video_mode_t *mode, uint8_t **frame)
{
  int l = 0;
  sendSync1(mode->top);

  //image
  for(int i = 0; i < mode->y; i += 2)
  {
    uint8_t *pixels0 = frame[i];
    uint8_t *pixels1 = frame[i + 1];
    int j = mode->left;
    for(int x = 0; x < mode->x; x++)
    {
      uint8_t px0 = *pixels0++;
      uint8_t px1 = *pixels1++;
      unsigned short p0v = yuv2v[px0];
      unsigned short p0u = yuv2u[px0];
      unsigned short p1u = yuv2u[px1];
      unsigned short p1v = yuv2v[px1];
      
      short y0 = yuv2y[px0];
      short y1 = yuv2y[px1];

      // we don't mix the hues in 1ppc modes because it takes too much time
      short u0 = (SIN[x] * UVLUT[p0u]);
      short v0 = (COS[x] * UVLUT[p0v]);

      //word order is swapped for I2S packing (j + 1) comes first then j
      line[0][j^1] = y0 + u0 + v0;
      line[1][j^1] = y1 + u0 - v0;
      j++;
    }
    sendLine(line[0]);
    sendLine(line[1]);
  }
  
  sendSync2(302 - mode->y - mode->top);
}

void __attribute__((optimize("O3"))) SimplePALOutput::sendFrame4ppc(
  const struct video_mode_t *mode, uint8_t **frame)
{
  int l = 0;
  sendSync1(mode->top);

  //image
  for(int i = 0; i < mode->y; i += 4)
  {
    uint8_t *pixels0 = frame[i];
    uint8_t *pixels1 = frame[i + 1];
    int j = mode->left;
    for(int x = 0; x < mode->x * 4; x += 4)
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
      short u2 = (SIN[x + 2] * u);
      short u3 = (SIN[x + 3] * u);
      short v0 = (COS[x] * v);
      short v1 = (COS[x + 1] * v);
      short v2 = (COS[x + 2] * v);
      short v3 = (COS[x + 3] * v);
      //word order is swapped for I2S packing (j + 1) comes first then j
      line[0][j] = y0 + u1 + v1;
      line[1][j] = y1 + u1 - v1;
      line[0][j + 1] = y0 + u0 + v0;
      line[1][j + 1] = y1 + u0 - v0;
      line[0][j + 2] = y0 + u3 + v3;
      line[1][j + 2] = y1 + u3 - v3;
      line[0][j + 3] = y0 + u2 + v2;
      line[1][j + 3] = y1 + u2 - v2;
      j += 4;
    }
    sendLine(line[0]);
    sendLine(line[1]);
  }
  
  sendSync2(302 - mode->y - mode->top);
}

#endif	// ESP32
