// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

// 32-bpp to 32-bpp integral factor image scalers.

#include <SDL2/SDL.h>

#define SCALER(x)       scaler_##x##_32
#define SCALER_TYPE     scaler_32_t
#define SCALERS_UP      upscalers_32
#define SCALERS_DOWN    downscalers_32
#define SCALE_FUNC      scale_integral_32
#define SCALER_SRC_TYPE uint32_t
#define SCALER_DST_TYPE uint32_t
#define CONV(p)         ((((p) & 0x0000ff) << 16) | \
                         (((p) & 0x00ff00)) | \
                         (((p) & 0xff0000) >> 16))

#include "scalers_imp.h"

#undef SCALER
#undef SCALER_TYPE
#undef SCALERS_UP
#undef SCALERS_DOWN
#undef SCALE_FUNC
#undef SCALER_SRC_TYPE
#undef SCALER_DST_TYPE
#undef CONV

#define SCALER(x)       scaler_##x##_16
#define SCALER_TYPE     scaler_16_t
#define SCALERS_UP      upscalers_16
#define SCALERS_DOWN    downscalers_16
#define SCALE_FUNC      scale_integral_16
#define SCALER_SRC_TYPE uint16_t
#define SCALER_DST_TYPE uint16_t
#define CONV(p)         (p)

#include "scalers_imp.h"

#undef SCALER
#undef SCALER_TYPE
#undef SCALERS_UP
#undef SCALERS_DOWN
#undef SCALE_FUNC
#undef SCALER_SRC_TYPE
#undef SCALER_DST_TYPE
#undef CONV

#define SCALER(x)       scaler_##x##_8
#define SCALER_TYPE     scaler_8_t
#define SCALERS_UP      upscalers_8
#define SCALERS_DOWN    downscalers_8
#define SCALE_FUNC      scale_integral_8
#define SCALER_SRC_TYPE uint8_t
#define SCALER_DST_TYPE uint8_t
#define CONV(p)         (p)

#include "scalers_imp.h"

#undef SCALER
#undef SCALER_TYPE
#undef SCALERS_UP
#undef SCALERS_DOWN
#undef SCALE_FUNC
#undef SCALER_SRC_TYPE
#undef SCALER_DST_TYPE
#undef CONV
