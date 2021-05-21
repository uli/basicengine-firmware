#include "eb_sound.h"
#include "sound.h"

sts_mixer_t *eb_get_mixer(void) {
    return sound.getMixer();
}
