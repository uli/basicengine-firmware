#include "ttconfig.h"

#if defined(ESP8266)
#include "esp8266audio.h"
#else
#define SOUND_BUFLEN 0
#endif
