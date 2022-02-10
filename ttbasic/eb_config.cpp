// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_api.h"
#include "eb_config.h"
#include "basic.h"

EBAPI unsigned int eb_theme_color(int index) {
    return csp.colorFromRgb(CONFIG.color_scheme[index]);
}
