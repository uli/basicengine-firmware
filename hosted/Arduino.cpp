#include "Arduino.h"
#include "SdFat.h"
#include "FS.h"
#include "SPI.h"
#include <vs23s010.h>
#include <malloc.h>

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

struct palette {
  uint8_t r, g, b;
};
#include <N-0C-B62-A63-Y33-N10.h>
#include <P-EE-A22-B22-Y44-N10.h>

#define SDL_X_SIZE 1400
#define SDL_Y_SIZE 1050
#define STRETCH_Y 4.4	// empircally determined
#define YUV_Y_SIZE (int(SDL_Y_SIZE/STRETCH_Y))

#define SDL_X_OFF ((screen->w - SDL_X_SIZE) / 2 * screen->format->BytesPerPixel)
#define SDL_Y_OFF ((screen->h - SDL_Y_SIZE) / 2 * screen->pitch)

#define VIEWPORT_X 308
#define VIEWPORT_Y 22

uint8_t y_screen[SDL_X_SIZE*YUV_Y_SIZE];
uint8_t u_screen[SDL_X_SIZE*YUV_Y_SIZE];
uint8_t v_screen[SDL_X_SIZE*YUV_Y_SIZE];

FILE *vid_fp = NULL;
FILE *aud_fp = NULL;
#define VIDEO_FIFO "video.y4m"
#define AUDIO_FIFO "audio.8u1"

//#define USE_MENCODER

void dump_yuv()
{
  fprintf(vid_fp, "FRAME\n");
  fwrite(y_screen, sizeof(y_screen), 1, vid_fp);
#ifdef USE_MENCODER
  // wrong U/V order
  fwrite(v_screen, sizeof(y_screen), 1, vid_fp);
  fwrite(u_screen, sizeof(y_screen), 1, vid_fp);
#else
  fwrite(u_screen, sizeof(y_screen), 1, vid_fp);
  fwrite(v_screen, sizeof(y_screen), 1, vid_fp);
#endif
}

static SDL_Thread *scrthr;
static int screen_thread(void *p);
static bool screen_quit = false;

static void my_exit(void)
{
  screen_quit = true;
  SDL_WaitThread(scrthr, NULL);
  if (vid_fp)
    pclose(vid_fp);
  if (aud_fp)
    pclose(aud_fp);
  unlink(VIDEO_FIFO);
  unlink(AUDIO_FIFO);
  SDL_Quit();
}

#ifdef USE_MENCODER
#define ENCODER_CMD "mencoder"
#else
#define ENCODER_CMD "ffmpeg"
#endif

int hosting_mem_allocated;

extern int sound_reinit_rate;
void reinit_sound();

int main(int argc, char **argv)
{
  int opt;
  int sdl_flags = SDL_HWSURFACE;
  int sdl_w = SDL_X_SIZE;
  int sdl_h = SDL_Y_SIZE;

  SDL_Init(SDL_INIT_EVERYTHING);
  const SDL_VideoInfo *info = SDL_GetVideoInfo();

  const char *video_file = NULL;
  while ((opt = getopt(argc, argv, "fr::")) != -1) {
    switch (opt) {
    case 'r':
      if (optarg)
	video_file = optarg;
      else
	video_file = "video.mp4";
      break;
    case 'f':
      sdl_flags |= SDL_FULLSCREEN;
      sdl_w = info->current_w;
      sdl_h = info->current_h;
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-r video_file]\n",
              argv[0]);
      exit(1);
    }
  }

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  screen = SDL_SetVideoMode(sdl_w, sdl_h, 32, sdl_flags);
  if (!screen) {
    fprintf(stderr, "SDL set mode failed: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(my_exit);
  for (int i = 0; i < 256; ++i) {
    palette[0][i].r = n_0c_b62_a63_y33_n10[i].r;
    palette[0][i].g = n_0c_b62_a63_y33_n10[i].g;
    palette[0][i].b = n_0c_b62_a63_y33_n10[i].b;
    palette[1][i].r = p_ee_a22_b22_y44_n10[i].r;
    palette[1][i].g = p_ee_a22_b22_y44_n10[i].g;
    palette[1][i].b = p_ee_a22_b22_y44_n10[i].b;
  }

  if (video_file) {
    memset(u_screen, 128, sizeof(u_screen));
    memset(v_screen, 128, sizeof(v_screen));

    mkfifo(VIDEO_FIFO, 0600);
    mkfifo(AUDIO_FIFO, 0600);

    int ret = system(ENCODER_CMD " 2>/dev/null");
    if (WEXITSTATUS(ret) == 127) {
      fprintf(stderr, ENCODER_CMD " command not found\n");
      exit(1);
    }
    ret = system("mbuffer -V 2>/dev/null");
    if (WEXITSTATUS(ret) == 127) {
      fprintf(stderr, "mbuffer command not found\n");
      exit(1);
    }

    char *cmd;
#ifdef USE_MENCODER
    ret = asprintf(&cmd, ENCODER_CMD " -o '%s' "
           "-ovc x264 "
           "-oac mp3lame "
           "-audiofile " AUDIO_FIFO " -audio-demuxer rawaudio "
           "-rawaudio channels=1:samplesize=1:rate=16000 "
           "-noskip " // "-audio-delay -2 "
           "-really-quiet "
           VIDEO_FIFO " &", video_file);
#else
    ret = asprintf(&cmd, ENCODER_CMD " "
           "-f u8 -c:a pcm_u8 -ac 1 -ar 16000 -i " AUDIO_FIFO " "
           "-f yuv4mpegpipe -i " VIDEO_FIFO " "
           "-y '%s' &", video_file);
#endif
    if (ret < 0 || system(cmd)) {
      fprintf(stderr, "failed to start " ENCODER_CMD);
      exit(1);
    }

    vid_fp = popen("mbuffer -q -m200M >" VIDEO_FIFO, "w");
    if (vid_fp)
      fprintf(vid_fp, "YUV4MPEG2 W%d H%d F60:1 Ip A10:%1.1lf C444\n", SDL_X_SIZE, YUV_Y_SIZE, STRETCH_Y*10);
    else {
      fprintf(stderr, "failed to start video buffer");
      exit(1);
    }

    aud_fp = popen("mbuffer -q -m2M >" AUDIO_FIFO, "w");
    if (!aud_fp) {
      fprintf(stderr, "failed to start audio buffer");
      exit(1);
    }
  }

  SDL_CreateThread(screen_thread, NULL);

  hosting_mem_allocated = mallinfo().uordblks;  

  // HACK: We can't change the sample rate once we have chroot'ed, so we
  // need to do it here.
  sound_reinit_rate = 16000;
  reinit_sound();

  // chroot to virtual file system and drop root privileges
  char *sudo_uid = secure_getenv("SUDO_UID");
  if (getuid() != 0 || sudo_uid == NULL) {
    fprintf(stderr, "needs to run as root using sudo (sorry)\n");
    exit(1);
  }

  if (chroot("vfs") < 0) {
    fprintf(stderr, "failed to chroot to vfs directory\n");
    exit(1);
  }

  if (setuid((uid_t)strtoll(sudo_uid, NULL, 10)) < 0) {
    fprintf(stderr, "WARNING: failed to drop root privileges\n");
  }

  setup();
  for (;;)
    loop();
}

#include "border_pal.h"

uint64_t total_frames = 0;
extern uint64_t total_samples;

void platform_process_events() {
  SDL_Event event;

//  if (sound_reinit_rate)
//    reinit_sound();

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

int screen_thread(void *p)
{
  static int last_line = 0;
  static SDL_Color *pl = palette[0];

  while (!screen_quit) {
  if (!vs23_int.enabled) {
    SDL_Delay(10);
    continue;
  }
  int new_line = SpiRamReadRegister(CURLINE);

  if (new_line > last_line) {
    // draw picture lines
    for (int i = last_line; i < _min(new_line,vs23.height()); ++i) {
#define m_current_mode vs23.m_current_mode	// needed by STARTLINE...
#define m_pal vs23_int.pal

      // pointer to SDL screen line
      uint8_t *scr_line = (uint8_t *)screen->pixels + SDL_X_OFF +
                          (int((i + STARTLINE - VIEWPORT_Y ) * STRETCH_Y)*screen->pitch) + SDL_Y_OFF;
      // X offset of start of picture (pixels)
      int xoff = vs23_int.picstart * 8 - VIEWPORT_X;
      // offset to YUV screen line (bytes, from start of component
      int yuv_line_off = int(i+STARTLINE-VIEWPORT_Y)*SDL_X_SIZE;

      // draw SDL screen border
      for (int j = 0; j < STRETCH_Y; ++j) {
        memcpy(scr_line + j * screen->pitch, screen->pixels + SDL_X_OFF + SDL_Y_OFF, xoff * screen->format->BytesPerPixel);
        memcpy(scr_line + j * screen->pitch + (xoff + vs23.width()) * screen->format->BytesPerPixel,
               (uint8_t *)screen->pixels + SDL_X_OFF + SDL_Y_OFF + (xoff + vs23.width()) * screen->format->BytesPerPixel,
               (SDL_X_SIZE - vs23.width() - xoff) * screen->format->BytesPerPixel);
      }

      if (vid_fp) {
        // draw YUV border
        int yuv_right_off = yuv_line_off + xoff + vs23.width();
        int yuv_right_width = SDL_X_SIZE - xoff - vs23.width();
        memcpy(&y_screen[yuv_line_off], y_screen, xoff);
        memcpy(&y_screen[yuv_right_off], y_screen + xoff + vs23.width(), yuv_right_width);
        memcpy(&u_screen[yuv_line_off], u_screen, SDL_X_SIZE);
        memcpy(&u_screen[yuv_right_off], u_screen + xoff + vs23.width(), yuv_right_width);
        memcpy(&v_screen[yuv_line_off], v_screen, SDL_X_SIZE);
        memcpy(&v_screen[yuv_right_off], v_screen + xoff + vs23.width(), yuv_right_width);
      }

      // one SDL pixel is one VS23 PLL clock cycle wide
      // picstart is defined in terms of color clocks (PLL/8)
      uint8_t *scr = scr_line + xoff * screen->format->BytesPerPixel;

      yuv_line_off += vs23_int.picstart * 8 - VIEWPORT_X;
      uint8_t *y_scr = y_screen + yuv_line_off;
      uint8_t *u_scr = u_screen + yuv_line_off;
      uint8_t *v_scr = v_screen + yuv_line_off;

      uint8_t *vdc = vs23_mem + vs23.piclineByteAddress(i);
      uint8_t *sscr = scr;
      for (int x = 0; x < vs23.width(); ++x) {
        // pixel width determined by VS23 program length
        for (int p = 0; p < vs23_int.plen; ++p) {
          *scr++ = pl[vdc[x]].b;
          *scr++ = pl[vdc[x]].g;
          *scr++ = pl[vdc[x]].r;
          *scr++ = 0;
          int y = 0.299 * scr[-2] + 0.587* scr[-3] + 0.114 * scr[-4];
          int u = -0.147 * scr[-2] - 0.289 * scr[-3] + 0.436 * scr[-4];
          int v = 0.615 * scr[-2] - 0.515 * scr[-3] - 0.100 * scr[-4];
          *y_scr++ = y;
          *u_scr++ = u + 128;
          *v_scr++ = v + 128;
        }
      }
      scr = sscr + screen->pitch;
      for (int j = 1; j < STRETCH_Y; ++j, scr += screen->pitch) {
        memcpy(scr, sscr,
               vs23.width() * screen->format->BytesPerPixel * vs23_int.plen);
      }
    }
  } else if (new_line < last_line) {
    // frame complete, blit it

    // update palette
    int pal = vs23_mem[PROTOLINE_BYTE_ADDRESS(0) + BURST*2];
    if (pal == 0x0c || pal == 0xdd)
      pl = palette[0];
    else
      pl = palette[1];

    SDL_Flip(screen);

    if (vid_fp)
      dump_yuv();
    total_frames++;

    // audio starts ~2s ahead for some reason; fill in frames
    // until synchronized
    while (total_samples >= total_frames * 16000/60) {
      if (vid_fp)
        dump_yuv();
      total_frames++;
    }
    uint8_t *p = (uint8_t*)screen->pixels + SDL_X_OFF + SDL_Y_OFF;
    for (int x = 0; x < SDL_X_SIZE; ++x) {
      int w = PROTOLINE_WORD_ADDRESS(0) + BLANKEND + x/8;
      int y = (vs23_mem[w*2+1] - 0x66) * 255 / 0x99;
      int uv = vs23_mem[w*2];

      // couldn't find a working transformation, using a palette
      // generated by VS23Palette06.exe
      y /= 4;	// palette uses 6-bit luminance
      int v = 15 - (uv >> 4);
      int u = 15 - (uv & 0xf);
      int idx = ((u&0xf) << 6) | ((v& 0xf) << 10) | y;
      int val = border_pal[idx];
      int r = val & 0xff;
      int g = (val >> 8) & 0xff;
      int b = (val >> 16) & 0xff;

      *p++ = b;
      *p++ = g;
      *p++ = r;
      *p++ = 0;

      y_screen[x] = 0.299 * r + 0.587* g + 0.114 * b;
      u_screen[x] = 128 + (-0.147 * r - 0.289 * g + 0.436 * b);
      v_screen[x] = 128 + (0.615 * r - 0.515 * g - 0.100 * b);
    }

    // Left/right border is drawn at the time the respective
    // picture line is; here, we only draw protolines.

    // draw SDL screen border top (before start of picture)
    p = (uint8_t*)screen->pixels + SDL_X_OFF + SDL_Y_OFF + screen->pitch;
    for (int i = 1; i < int((STARTLINE - VIEWPORT_Y)*STRETCH_Y); ++i) {
      memcpy(p, screen->pixels + SDL_X_OFF + SDL_Y_OFF, SDL_X_SIZE * screen->format->BytesPerPixel);
      p += screen->pitch;
    }
    // draw SDL screen border bottom (after end of picture)
    for (int i = (STARTLINE - VIEWPORT_Y+vs23.height()) * STRETCH_Y; i < SDL_Y_SIZE; ++i) {
      memcpy((uint8_t *)screen->pixels + SDL_X_OFF + SDL_Y_OFF + i * screen->pitch,
             screen->pixels + SDL_X_OFF + SDL_Y_OFF,
             SDL_X_SIZE * screen->format->BytesPerPixel);
    }
    if (vid_fp) {
      // draw YUV border top (before start of picture)
      for (int i = 1; i < (STARTLINE - VIEWPORT_Y); ++i) {
        memcpy(&y_screen[i*SDL_X_SIZE], y_screen, SDL_X_SIZE);
        memcpy(&u_screen[i*SDL_X_SIZE], u_screen, SDL_X_SIZE);
        memcpy(&v_screen[i*SDL_X_SIZE], v_screen, SDL_X_SIZE);
      }
      // draw YUV border bottom (after end of picture)
      for (int i = STARTLINE-VIEWPORT_Y+vs23.height(); i < YUV_Y_SIZE; ++i) {
        memcpy(&y_screen[i*SDL_X_SIZE], y_screen, SDL_X_SIZE);
        memcpy(&u_screen[i*SDL_X_SIZE], u_screen, SDL_X_SIZE);
        memcpy(&v_screen[i*SDL_X_SIZE], v_screen, SDL_X_SIZE);
      }
    }
  }
  last_line = new_line;
  usleep(100);
  }
  return 0;
}

uint32_t Video::frame()
{
  // The hosted environment is not real-time enough to provide an asynchronous
  // update of m_frame with reasonable accuracy; we therefore calculate it on
  // demand.
  m_frame = (micros() / vs23_int.line_us - vs23.m_sync_line) / vs23_int.line_count;
  return m_frame;
}

extern "C" size_t umm_free_heap_size( void )
{
  return 70000 - (mallinfo().uordblks - hosting_mem_allocated);
}

#include "Wire.h"
TwoWire Wire;
