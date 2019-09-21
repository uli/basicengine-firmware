#include "ttconfig.h"

#ifdef USE_VS23
#include "vs23s010.h"
#endif
#ifdef USE_ESP32GFX
#include "esp32gfx.h"
#endif
#ifdef USE_H3GFX
#include "h3gfx.h"
#endif
#ifdef USE_DOSGFX
#include <dosgfx.h>
#endif
#ifdef USE_SDLGFX
#include <sdlgfx.h>
#endif
