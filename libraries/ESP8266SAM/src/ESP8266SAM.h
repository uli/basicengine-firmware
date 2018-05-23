/*
  ESP8266SAM
  Port of SAM to the ESP8266
  
  Copyright (C) 2017  Earle F. Philhower, III
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _ESP8266SAM_H
#define _ESP8266SAM_H

#include <Arduino.h>
#include <stdint.h>

#define SAMPLE_BUF_SIZE 16

#ifdef PC_HOSTED
#include <SDL/SDL.h>
extern void fill_audio(void *udata, Uint8 *stream, int len);
#endif

class ESP8266SAM {

public:
  ESP8266SAM()
  {
    singmode = false;
    phonetic = false;
    pitch = 64;
    mouth = 128;
    throat = 128;
    speed = 72;
    oldtimetableindex = 0;
    mem59 = 0;
    bufferpos = 0;
    memset(amplitude1, 0, 256);
    memset(amplitude2, 0, 256);
    memset(amplitude3, 0, 256);
    prepo_reset_xy = false;
    render_state = RENDER_IDLE;
    bufptr_read = bufptr_write = 0;
  #ifdef PC_HOSTED
    SDL_AudioSpec wanted;
    wanted.freq = 22050;
    wanted.format = AUDIO_U8;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = fill_audio;
    wanted.userdata = this;
    if (SDL_OpenAudio(&wanted, NULL) < 0) {
      fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
      exit(1);
    }
    SDL_PauseAudio(0);
  #else
  #endif
  };
  
  ~ESP8266SAM()
  {
#ifdef PC_HOSTED
    SDL_CloseAudio();
#endif
  }

  enum SAMVoice { VOICE_SAM, VOICE_ELF, VOICE_ROBOT, VOICE_STUFFY, VOICE_OLDLADY, VOICE_ET };
  void SetVoice(enum SAMVoice voice);

  void SetSingMode(bool val) { singmode = val; }
  void SetPhonetic(bool val) { phonetic = val; }
  void SetPitch(uint8_t val) { pitch = val; }
  void SetMouth(uint8_t val) { mouth = val; }
  void SetThroat(uint8_t val) { throat = val; }
  void SetSpeed(uint8_t val) { speed = val; }

  void Say(const char *str);
  void Say_P(const char *str) {
    char ram[256];
    strncpy_P(ram, str, 256);
    ram[255] = 0;
    Say(ram);
  };

  bool moreSamples();
  uint8_t getSample();
  void wait() {
    while (render_state != RENDER_IDLE) {}
  }
  bool finished() {
    return render_state == RENDER_IDLE;
  }

private:
  static void OutputByteCallback(void *cbdata, unsigned char b);
  void OutputByte(unsigned char b);
  bool singmode;
  bool phonetic;
  int pitch;
  int speed;
  int mouth;
  int throat;

  // render
  void Output8BitAry(int index, unsigned char ary[5]);
  void Output8Bit(int index, unsigned char A);
  unsigned char Read(unsigned char p, unsigned char Y);
  void Write(unsigned char p, unsigned char Y, unsigned char value);
  void RenderSample();
  void Render();
  void RenderLoop();
  void AddInflection(unsigned char mem48, unsigned char phase1);
  void SetMouthThroat(unsigned char mouth, unsigned char throat);
  unsigned char trans(unsigned char mem39212, unsigned char mem39213);

  int GetBufferLength(){return bufferpos;};
  void SetInput(char *_input);
  void Init();
  int SAMMain( void (*cb)(void *, unsigned char), void *cbd );
  int SAMPrepare();
  void PrepareOutput();
  void InsertBreath();
  void CopyStress();
  void Insert(unsigned char position/*var57*/, unsigned char mem60, unsigned char mem59, unsigned char mem58);
  int Parser1();
  void SetPhonemeLength();

  void Code41240();
  void Parser2();

  void AdjustLengths();

  void Code47503(unsigned char mem52);

  // reciter
  void Code37055(unsigned char mem59);
  void Code37066(unsigned char mem58);
  int TextToPhonemes(char *input); // Code36484

  unsigned char A, X, Y;
  unsigned char mem44;
  unsigned char mem47;
  unsigned char mem49;
  unsigned char mem39;
  unsigned char mem50;
  unsigned char mem51;
  unsigned char mem53;
  unsigned char mem56;
  unsigned char mem59;

  unsigned char mem48;
  unsigned char phase1;  //mem43
  unsigned char phase2;
  unsigned char phase3;
  unsigned char mem38;
  unsigned char speedcounter; //mem45

  unsigned char mem66;

  unsigned char freq1data[80];
  unsigned char freq2data[80];
  unsigned char freq3data[80];

  unsigned char pitches[256]; // tab43008

  unsigned char frequency1[256];
  unsigned char frequency2[256];
  unsigned char frequency3[256];

  unsigned char amplitude1[256];
  unsigned char amplitude2[256];
  unsigned char amplitude3[256];

  unsigned char sampledConsonantFlag[256]; // tab44800

  unsigned char inputtemp[256];   // secure copy of input tab36096

  unsigned char oldtimetableindex;
  unsigned char lastAry[5];

  unsigned char stress[256]; //numbers from 0 to 8
  unsigned char phonemeLength[256]; //tab40160
  unsigned char phonemeindex[256];

  unsigned char phonemeIndexOutput[60]; //tab47296
  unsigned char stressOutput[60]; //tab47365
  unsigned char phonemeLengthOutput[60]; //tab47416

  char input[256];

  // contains the final soundbuffer
  int bufferpos;
  
  enum {
    RENDER_IDLE,
    RENDER_PREP,
    RENDER_PREP_RESTORE,
    RENDER_REND,
    RENDER_LOOP,
    RENDER_SAMPLE_1,
    RENDER_SAMPLE_2,
    RENDER_SAMPLE_3,
    RENDER_SAMPLE_END,
  } render_state;
  bool last_loop;
  int8_t last_render_sample_call;
  bool prepo_reset_xy;
  unsigned char prepo_reset_x_value;

  uint8_t buffer[SAMPLE_BUF_SIZE];
  int8_t bufptr_read;
  int8_t bufptr_write;
};

#endif
