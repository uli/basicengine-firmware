// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"

#define EBAPI __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

static bool check_param(int32_t p, int32_t min, int32_t max) {
  if (p < min || p > max) {
    E_VALUE(min, max);
    return true;
  }
  return false;
}

#ifdef __cplusplus
}
#endif
