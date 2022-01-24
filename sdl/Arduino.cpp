// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include <string>

#include <video_driver.h>
#include "sdlaudio.h"
#include <eb_sys.h>

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

#include <SDL2/SDL.h>

extern "C" void init_idle();

int sdl_flags;
int sdl_user_w, sdl_user_h;
SDL_Window *sdl_window;
SDL_Renderer *sdl_renderer;

int main(int argc, char **argv) {
  int opt;

  char *path = getcwd(NULL, 0);
  if (path) {
    setenv("ENGINEBASIC_ROOT", path, 0);
    free(path);
  }

  sdl_flags = 0;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "Cannot initialize SDL video: %s\n", SDL_GetError());
    exit(1);
  }

  sdl_user_w = 640;
  sdl_user_h = 480;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    fprintf(stderr, "Cannot initialize SDL audio: %s\n", SDL_GetError());
  if (SDL_InitSubSystem(SDL_INIT_TIMER))
    fprintf(stderr, "Cannot initialize SDL timer: %s\n", SDL_GetError());
  if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER))
    fprintf(stderr, "Cannot initialize SDL game controller: %s\n", SDL_GetError());

  while ((opt = getopt(argc, argv, "fdr:s:")) != -1) {
    switch (opt) {
    case 'f': sdl_flags |= SDL_WINDOW_FULLSCREEN; break;
    case 'd':
      sdl_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
      sdl_user_w = 0; sdl_user_h = 0;
      break;
    case 's':
      sscanf(optarg, "%dx%d", &sdl_user_w, &sdl_user_h);
      break;
    case 'r':
      path = realpath(optarg, NULL);
      if (path) {
        setenv("ENGINEBASIC_ROOT", path, 1);
        free(path);
      }
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-f] [-d] [-s <w>x<h>] [-r <BASIC root path>]\n",
              argv[0]);
      exit(1);
    }
  }

  std::string controller_map = std::string(getenv("ENGINEBASIC_ROOT")) +
                               std::string("/sys/gamecontrollerdb.txt");
  SDL_GameControllerAddMappingsFromFile(controller_map.c_str());
  SDL_GameControllerEventState(SDL_ENABLE);

  atexit(my_exit);

  eb_set_cpu_speed(75);

  setup();
  for (;;)
    loop();
}

#include "border_pal.h"

uint64_t total_frames = 0;
extern uint64_t total_samples;
extern int sound_reinit_rate;

#include <mouse.h>
Mouse mouse;

extern std::queue<SDL_Event> kbd_events;
extern std::queue<SDL_Event> controller_events;

void platform_process_events() {
  SDL_Event event;

  audio.pumpEvents();
  SDL_PumpEvents();
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      exit(0);
      break;
    case SDL_MOUSEMOTION:
      mouse.move(event.motion.xrel, event.motion.yrel);
      mouse.warp(event.motion.x, event.motion.y);
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      int button_bit = 1 << (event.button.button - 1);
      int buttons = mouse.buttons() & ~button_bit;
      if (event.button.state)
        buttons |= button_bit;
      mouse.setButtons(buttons);
      break;
    }
    case SDL_KEYUP:
    case SDL_KEYDOWN:
      kbd_events.push(event);
      break;
    case SDL_CONTROLLERAXISMOTION:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
      controller_events.push(event);
      break;
    default:
      //printf("SDL event %d\n", event.type);
      break;
    }
  }
}

#include "Wire.h"
TwoWire Wire;
