//
// 豊四季Tiny BASIC for Arduino STM32 Graphic drawing device
// 2017/07/19 by たま吉さん
//

#ifndef __tGraphicDev_h__
#define __tGraphicDev_h__

#include "../../ttbasic/ttconfig.h"

#include <Arduino.h>

const uint8_t* tv_getFontAdr() ;

class tGraphicDev {
  protected:
    uint16_t gwidth;             // Screen graphic horizontal size
    uint16_t gheight;            // Screen graphic vertical size

  public:
     void  init();
    inline const uint8_t *getfontadr() { return tv_getFontAdr()+3; };   // Font address pointer
    virtual uint16_t getGWidth();       // Graphics screen width acquisition
    virtual uint16_t getGHeight();      // Graphics screen vertical width acquisition

    // Graphic drawing
    virtual void pset(int16_t x, int16_t y, uint8_t c);
    virtual void line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c);
    virtual void circle(int16_t x, int16_t y, int16_t r, uint8_t c, int f);
    virtual void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int f);
    //virtual void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);
    virtual void gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode);
    virtual void set_gcursor(uint16_t, uint16_t);
    virtual void gputch(uint8_t c);

};

#endif
