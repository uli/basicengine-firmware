#ifndef __BSTRING_H
#define __BSTRING_H

#include <Arduino.h>
#include "ttconfig.h"

class BString : public String {
public:
  using String::String;
  using String::operator=;

  int fromBasic(unsigned char *s) {
    len = *s++;
    if (!reserve(len)) {
      invalidate();
      return 0;
    }
    os_memcpy(buffer, s, len);
    buffer[len] = 0;
    return len + 1;
  }
};

#endif
