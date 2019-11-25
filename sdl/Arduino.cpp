// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

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

static void my_exit(void) {
  SDL_Quit();
}

#include <SDL/SDL.h>

extern "C" void init_idle();

const SDL_VideoInfo *sdl_info;
int sdl_flags;
bool sdl_keep_res = false;

int main(int argc, char **argv) {
  int opt;

  char *path = getcwd(NULL, 0);
  if (path) {
    setenv("ENGINEBASIC_ROOT", path, 0);
    free(path);
  }

  sdl_flags = SDL_HWSURFACE | SDL_DOUBLEBUF;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Cannot initialize SDL video: %s\n", SDL_GetError());
    exit(1);
  }

  sdl_info = SDL_GetVideoInfo();
  if (!sdl_info) {
    fprintf(stderr, "SDL_GetVideoInfo() failed: %s\n", SDL_GetError());
    exit(1);
  }

  if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    fprintf(stderr, "Cannot initialize SDL audio: %s\n", SDL_GetError());
  if (SDL_InitSubSystem(SDL_INIT_TIMER))
    fprintf(stderr, "Cannot initialize SDL timer: %s\n", SDL_GetError());
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
    fprintf(stderr, "Cannot initialize SDL joystick: %s\n", SDL_GetError());

  while ((opt = getopt(argc, argv, "fdr:")) != -1) {
    switch (opt) {
    case 'f': sdl_flags |= SDL_FULLSCREEN; break;
    case 'd': sdl_keep_res = true; break;
    case 'r':
      path = realpath(optarg, NULL);
      if (path) {
        setenv("ENGINEBASIC_ROOT", path, 1);
        free(path);
      }
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-f] [-d] [-r <BASIC root path>]\n",
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
