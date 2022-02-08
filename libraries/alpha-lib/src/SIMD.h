#pragma once

#if defined(__ARM_NEON) && !defined(DISABLE_NEON)
#include <arm_neon.h>
#define ENABLE_NEON 1
#elif !defined(DISABLE_NEON)
#ifdef __arm__
#define ENABLE_NEON 1
#else
#define ENABLE_NEON 0
#endif
#define ENABLE_SIMD32 0
#endif
