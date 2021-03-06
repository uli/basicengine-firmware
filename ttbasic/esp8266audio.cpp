#ifdef ESP8266

#include "esp8266audio.h"

// Array with 32-bit values which have one bit more set to '1' in every
// consecutive array index value
// Taken from Espressif MP3 decoder demo.
#ifndef HOSTED
const uint32_t BASIC_DAT fakePwm[] = {
  0x00000010, 0x00000410, 0x00400410, 0x00400C10, 0x00500C10, 0x00D00C10,
  0x20D00C10, 0x21D00C10, 0x21D80C10, 0xA1D80C10, 0xA1D80D10, 0xA1D80D30,
  0xA1DC0D30, 0xA1DC8D30, 0xB1DC8D30, 0xB9DC8D30, 0xB9FC8D30, 0xBDFC8D30,
  0xBDFE8D30, 0xBDFE8D32, 0xBDFE8D33, 0xBDFECD33, 0xFDFECD33, 0xFDFECD73,
  0xFDFEDD73, 0xFFFEDD73, 0xFFFEDD7B, 0xFFFEFD7B, 0xFFFFFD7B, 0xFFFFFDFB,
  0xFFFFFFFB, 0xFFFFFFFF
};
#endif

ESP8266Audio audio;

#endif  // ESP8266
