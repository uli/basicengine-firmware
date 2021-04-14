#pragma once

#if defined(__ARM_NEON) && !defined(DISABLE_NEON)
#include <arm_neon.h>
#define ENABLE_NEON 1
#elif defined(__x86_64__) && !defined(DISABLE_NEON)
#include <NEON_2_SSE.h>
#define ENABLE_NEON 1
#else
#define ENABLE_NEON 0
#define ENABLE_SIMD32 0
#endif
