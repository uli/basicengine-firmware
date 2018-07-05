#include "Arduino.h"
#include "SdFat.h"
#include "FS.h"
#include "SPI.h"
#include <vs23s010.h>

void digitalWrite(uint8_t pin, uint8_t val) {
  printf("DW %d <- %02X\n", pin, val);
}

void pinMode(uint8_t pin, uint8_t mode) {
}

uint32_t timer0_read() {
  return 0;
}
void timer0_write(uint32_t count) {
}

void timer0_isr_init() {
}
void timer0_attachInterrupt(timercallback userFunc) {
}
void timer0_detachInterrupt(void) {
}

fs::FS SPIFFS;
SPIClass SPI;

void loop();
void setup();

SDL_Surface *screen;
SDL_Color palette[2][256];

static bool m_pal = false;

struct palette {
  uint8_t r, g, b;
};
#include <N-0C-B62-A63-Y33-N10.h>
#include <P-EE-A22-B22-Y44-N10.h>

int main(int argc, char **argv)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  screen = SDL_SetVideoMode(1400, 1050, 8, SDL_HWSURFACE);
  if (!screen) {
    fprintf(stderr, "SDL set mode failed: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);
  for (int i = 0; i < 256; ++i) {
    palette[0][i].r = n_0c_b62_a63_y33_n10[i].r;
    palette[0][i].g = n_0c_b62_a63_y33_n10[i].g;
    palette[0][i].b = n_0c_b62_a63_y33_n10[i].b;
    palette[1][i].r = p_ee_a22_b22_y44_n10[i].r;
    palette[1][i].g = p_ee_a22_b22_y44_n10[i].g;
    palette[1][i].b = p_ee_a22_b22_y44_n10[i].b;
  }
  setup();
  for (;;)
    loop();
}

void hosted_pump_events() {
  static int last_line = 0;
  SDL_Event event;
  SDL_PumpEvents();
  while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_ALLEVENTS ^ (SDL_KEYUPMASK|SDL_KEYDOWNMASK)) == 1) {
    switch (event.type) {
      case SDL_QUIT:
        exit(0);
        break;
      default:
        //printf("SDL event %d\n", event.type);
        break;
    }
    SDL_PumpEvents();
  }

  int new_line = SpiRamReadRegister(CURLINE);

  if (new_line > last_line) {
    for (int i = last_line; i < vs23.height(); ++i) {
#define STRETCH_Y 4.4	// empirically determined
#define m_current_mode vs23.m_current_mode	// needed by STARTLINE...
      for (int j = 0; j < STRETCH_Y; ++j) {
        // one SDL pixel is one VS23 PLL clock cycle wide
        // picstart is defined in terms of color clocks (PLL/8)
        uint8_t *scr = (uint8_t *)screen->pixels +
                       (int(i*STRETCH_Y + j + STARTLINE))*screen->pitch +
                       vs23_int.picstart * 8 - 308;
        uint8_t *vdc = vs23_mem + vs23.piclineByteAddress(i);
        for (int x = 0; x < vs23.width(); ++x) {
          // pixel width determined by VS23 program length
          for (int p = 0; p < vs23_int.plen; ++p) {
            *scr++ = vdc[x];
          }
        }
      }
    }
  } else if (new_line < last_line) {
    int pal = vs23_mem[PROTOLINE_BYTE_ADDRESS(0) + BURST*2];
    m_pal = vs23.isPal();
    if (pal == 0x0c || pal == 0xdd) {
      SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, palette[0], 0, 256);
    } else {
      SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, palette[1], 0, 256);
    }
    SDL_Flip(screen);
  }
  last_line = new_line;
  vs23.setFrame(micros() / vs23_int.line_us / vs23_int.line_count);
}

#include "Wire.h"
TwoWire Wire;
