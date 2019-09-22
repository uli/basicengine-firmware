#include <video_driver.h>
#include "sdlaudio.h"

#include "Arduino.h"
#include "FS.h"
#include "SPI.h"
#include <malloc.h>

void digitalWrite(uint8_t pin, uint8_t val) {
  printf("DW %d <- %02X\n", pin, val);
}

int digitalRead(uint8_t pin) {
  return 0;
}

void pinMode(uint8_t pin, uint8_t mode) {
}

fs::FS SPIFFS;
SPIClass SPI;

void loop();
void setup();

struct palette {
  uint8_t r, g, b;
};
#include <N-0C-B62-A63-Y33-N10.h>
#include <P-EE-A22-B22-Y44-N10.h>

static void my_exit(void)
{
  fprintf(stderr, "my_exit\n");
}

#include <SDL/SDL.h>

extern "C" void init_idle();

#define SDL_X_SIZE 800
#define SDL_Y_SIZE 600

int sdl_w, sdl_h, sdl_flags;

int main(int argc, char **argv)
{
  int opt;
  sdl_flags = SDL_HWSURFACE;
  sdl_w = SDL_X_SIZE;
  sdl_h = SDL_Y_SIZE;

  SDL_Init(SDL_INIT_EVERYTHING);
  const SDL_VideoInfo *info = SDL_GetVideoInfo();

  while ((opt = getopt(argc, argv, "f")) != -1) {
    switch (opt) {
    case 'f':
      sdl_flags |= SDL_FULLSCREEN;
      sdl_w = info->current_w;
      sdl_h = info->current_h;
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-f]\n",
              argv[0]);
      exit(1);
    }
  }

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

  atexit(my_exit);

  setup();
  for (;;)
    loop();
}

#include "border_pal.h"

uint64_t total_frames = 0;
extern uint64_t total_samples;
extern int sound_reinit_rate;

void platform_process_events() {
  SDL_Event event;
  
  audio.pumpEvents();
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
}

#include "Wire.h"
TwoWire Wire;
