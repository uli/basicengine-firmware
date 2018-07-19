#include "ttconfig.h"

#ifdef ESP32
#include "AudioOutput.h"
#elif defined(ESP8266)
#include "esp8266audio.h"
#else
#define SOUND_BUFLEN 0
#endif
