// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

// 32-bpp to 32-bpp integral factor image scalers.

#include <SDL/SDL.h>

#define SCALER(x) scaler_ ## x ## _32
#define SCALER_TYPE scaler_32_t
#define SCALERS_UP upscalers_32
#define SCALERS_DOWN downscalers_32
#define SCALE_FUNC scale_integral_32
#define SCALER_SRC_TYPE uint32_t
#define SCALER_DST_TYPE uint32_t
#define CONV(p) (p)

#include "scalers_imp.h"

#undef SCALER
#undef SCALER_TYPE
#undef SCALERS_UP
#undef SCALERS_DOWN
#undef SCALE_FUNC
#undef SCALER_SRC_TYPE
#undef SCALER_DST_TYPE
#undef CONV




