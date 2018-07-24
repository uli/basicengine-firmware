#ifdef ESP32

#include "esp32gfx.h"

const int lineSamples = 854;
const int syncSamples = 64;
const int burstSamples = 38;

const int burstStart = 70;

const int syncLevel = 0;
const int blankLevel = 23;
const int burstAmp = 8;//12;
const int maxLevel = 54;
const int maxUVLevel = 27;//54;
const float burstPerSample = (2 * M_PI) / (13333333 / 4433618.75);
const float burstPhase = M_PI / 4 * 3;

#include "RGB2YUV.h"

short SimplePALOutput::SIN[imageSamples];
short SimplePALOutput::COS[imageSamples];

void SimplePALOutput::init()
{
  i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
     .sample_rate = 1000000,  //not really used
     .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT, 
     .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
     .communication_format = I2S_COMM_FORMAT_I2S_MSB,
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
     // three buffers works as well, but we're playing it safe
     .dma_buf_count = 4,
     .dma_buf_len = lineSamples   //a buffer per line
  };
  
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);    //start i2s driver
  i2s_set_pin(I2S_PORT, NULL);                           //use internal DAC
  i2s_set_sample_rates(I2S_PORT, 1000000);               //dummy sample rate, since the function fails at high values

  //this is the hack that enables the highest sampling rate possible ~13MHz, have fun
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
  SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S); 
  SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);

  //untie DACs
  SET_PERI_REG_BITS(I2S_CONF_CHAN_REG(0), I2S_TX_CHAN_MOD_V, 3, I2S_TX_CHAN_MOD_S);
  SET_PERI_REG_BITS(I2S_FIFO_CONF_REG(0), I2S_TX_FIFO_MOD_V, 1, I2S_TX_FIFO_MOD_S);
}

void SimplePALOutput::setMode(const struct video_mode_t &mode)
{
  for(int i = 0; i < syncSamples; i++)
  {
    shortSync[i ^ 1] = syncLevel << 8;
    longSync[(lineSamples - syncSamples + i) ^ 1] = blankLevel  << 8;
    line[0][i ^ 1] = syncLevel  << 8;
    line[1][i ^ 1] = syncLevel  << 8;
    blank[i ^ 1] = syncLevel  << 8;
  }
  for(int i = 0; i < lineSamples - syncSamples; i++)
  {
    shortSync[(i + syncSamples) ^ 1] = blankLevel  << 8;
    longSync[(lineSamples - syncSamples + i) ^ 1] = syncLevel  << 8;
    line[0][(i + syncSamples) ^ 1] = blankLevel  << 8;
    line[1][(i + syncSamples) ^ 1] = blankLevel  << 8;
    blank[(i + syncSamples) ^ 1] = blankLevel  << 8;
  }
  for(int i = 0; i < burstSamples; i++)
  {
    int p = burstStart + i;
    unsigned short b0 = ((short)(blankLevel + sin(i * burstPerSample + burstPhase) * burstAmp)) << 8;
    unsigned short b1 = ((short)(blankLevel + sin(i * burstPerSample - burstPhase) * burstAmp)) << 8;
    line[0][p ^ 1] = b0;
    line[1][p ^ 1] = b1;
    blank[p ^ 1] = b0;
  }

  for(int i = 0; i < imageSamples; i++)
  {
    int p = mode.left + i;
    int c = p - burstStart;
    SIN[i] = round(0.436798 * sin(c * burstPerSample) * 256);
    COS[i] = round(0.614777 * cos(c * burstPerSample) * 256);     
  }

  for(int i = 0; i < 16; i++)
  {
    YLUT[i] = (blankLevel << 8) + round(i / 15. * 256 * maxLevel);
    UVLUT[i] = round((i - 8) / 7. * maxUVLevel);
  }

  setColorSpace(0);
}

void SimplePALOutput::setColorSpace(uint8_t palette)
{  
  for (int i = 0; i < 256; ++i) {
      uint8_t *rgb0 = &csp.paletteData(palette)[i*3];
      uint16_t rgb444_0 = (rgb0[0] >> 4) | (rgb0[1] & 0xf0) | ((rgb0[2]&0xf0) << 4);
      uint16_t c = RGB2YUV[rgb444_0];
      yuv2v[i] = (c >> 12) & 0xf;
      yuv2u[i] = (c >> 4) & 0xf;
      yuv2y[i] = YLUT[c & 0xf];
  }
}

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

// retrieve pixels (even/odd line)
#define PIX() \
  uint8_t px0 = *pixels0++; \
  uint8_t px1 = *pixels1++;

// calculate Y (even/odd lines separate) and UV (even/odd lines mixed)
#define YUV() \
  unsigned short p0v = yuv2v[px0]; \
  unsigned short p0u = yuv2u[px0]; \
  unsigned short p1u = yuv2u[px1]; \
  unsigned short p1v = yuv2v[px1]; \
  short y0 = yuv2y[px0]; \
  short y1 = yuv2y[px1]; \
  short u = UVLUT[(p0u+p1u)/2]; \
  short v = UVLUT[(p1v+p0v)/2];

// generate color carrier waveform
#define UWAVE(n)	short u ## n = SIN[x + (n)] * u;
#define VWAVE(n)	short v ## n = COS[x + (n)] * v;

// generate color carrier waveform using the first line's color only
// (saves cycles in borderline cases)
#define UWAVE_NOMIX(n) \
  short u0 = SIN[x + (n)] * UVLUT[p0u]; \
  short v0 = COS[x + (n)] * UVLUT[p0v];

// write samples to buffers
// (only works if each iteration of the horizontal loop starts with
// an even j)
#define STORE(off, wave) \
  line[0][j + (off)] = y0 + u ## wave + v ## wave; \
  line[1][j + (off)] = y1 + u ## wave - v ## wave;

// write single sample to buffers
// (required for modes in which j can be odd)
#define STORE_ONE() \
  line[0][j ^ 1] = y0 + u0 + v0; \
  line[1][j ^ 1] = y1 + u0 - v0;

void __attribute__((optimize("O3"))) SimplePALOutput::sendFrame(
  const struct video_mode_t *mode, uint8_t **frame)
{
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
  sendSync1(mode->top);

  //image
  for(int i = 0; i < mode->y; i += 2)
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
