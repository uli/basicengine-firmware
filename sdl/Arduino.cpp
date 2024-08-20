// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include <string>

#include <video_driver.h>
#include "sdlaudio.h"
#include <eb_sys.h>

#include "Arduino.h"
#include "SPI.h"
#include "compat.h"
#ifndef __APPLE__
#include <malloc.h>
#endif

SPIClass SPI;

void loop();
void setup();

#include "sdlgfx.h"
extern SDLGFX vs23;
static void my_exit(void) {
  vs23.end();
}

#ifdef ANDROID

// Redirect stdout/stderr to the Android logging facilities.
// Adapted from
// https://stackoverflow.com/questions/10531050/redirect-stdout-to-logcat-in-android-ndk/42715692#42715692

#include <android/log.h>
#include <pthread.h>

static int pfd[2];
static pthread_t thr;
static const char *tag;

static void *thread_func(void*)
{
    ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[rdsz - 1] == '\n') --rdsz;
        buf[rdsz] = 0;  /* add null-terminator */
        __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
    }
    return 0;
}

int start_logger(const char *app_name)
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, 0, thread_func, 0) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}
#endif

#include <SDL.h>

extern "C" void init_idle();

int sdl_flags;
int sdl_user_w, sdl_user_h;
SDL_Window *sdl_window;
SDL_Renderer *sdl_renderer;

int main(int argc, char **argv) {
  int opt;

#ifdef ANDROID
  start_logger("enginebasic");
#endif

  char *path = getcwd(NULL, 0);
  if (path) {
    setenv("ENGINEBASIC_ROOT", path, 0);
    free(path);
  }

  sdl_flags = 0;

  sdl_user_w = 960;
  sdl_user_h = 540;

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
#ifdef _WIN32
      path = _fullpath(NULL, optarg, PATH_MAX);
#else
      path = realpath(optarg, NULL);
#endif
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

  vs23.init(controller_map.c_str());

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
      _exit(0);
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
#ifdef ANDROID
    case SDL_APP_WILLENTERBACKGROUND:
      vs23.suspendDisplay();
      break;
    case SDL_APP_DIDENTERFOREGROUND:
      vs23.softReset();
      break;
    case SDL_DISPLAYEVENT:
      // XXX: I'm not getting that event even when the display is changing
      // orientation, but I'm leaving it here anyway in case SDL gets saner
      // in the future.
      if (event.display.event == SDL_DISPLAYEVENT_ORIENTATION)
        vs23.softReset();
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        vs23.softReset();
      break;
    case SDL_RENDER_DEVICE_RESET:
    case SDL_RENDER_TARGETS_RESET:
      exit(1);
      break;
#endif
    default:
      //printf("SDL event %d\n", event.type);
      break;
    }
  }
}
