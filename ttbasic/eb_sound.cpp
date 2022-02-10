// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_api.h"
#include "eb_sound.h"
#include "sound.h"

EBAPI sts_mixer_t *eb_get_mixer(void) {
    return sound.getMixer();
}
