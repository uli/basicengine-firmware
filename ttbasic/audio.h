// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include "ttconfig.h"

#include <stdint.h>
#include "BString.h"

#ifdef AUDIO_16BIT
typedef int16_t sample_t;
#else
typedef uint8_t sample_t;
#endif

#if defined(H3)
#include "h3audio.h"
#elif defined(__DJGPP__)
#include <dosaudio.h>
#elif defined(SDL)
#include <sdlaudio.h>
#else
#error unknown platform
#endif
