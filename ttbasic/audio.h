// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include "ttconfig.h"

#include <stdint.h>
#ifdef AUDIO_16BIT
typedef int16_t sample_t;
#else
typedef uint8_t sample_t;
#endif

#ifdef ESP32
#include "AudioOutput.h"
#elif defined(H3)
#include "h3audio.h"
#elif defined(__DJGPP__)
#include <dosaudio.h>
#elif defined(SDL)
#include <sdlaudio.h>
#elif defined(ESP8266)
#include "esp8266audio.h"
#else
#define SOUND_BUFLEN 0
#endif
