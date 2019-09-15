/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2018 Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include "video.h"
#include "colorspace.h"
#include "graphics.h"

void Graphics::drawRect(int x0, int y0, int w, int h, pixel_t c, int fc)
{
  if (fc == -1) {
    w--;
    h--;
    if (w == 0 && h == 0) {
       vs23.setPixel(x0,y0,c);
    } else if (w == 0 || h == 0) {
      drawLine(x0,y0,x0+w,y0+h,c);
    } else {
       // Horizontal line
       drawLine(x0,y0  , x0+w, y0  , c);
       drawLine(x0,y0+h, x0+w, y0+h, c);
       // Vertical line
       if (h>1) {  
           drawLine(x0,  y0+1,x0  ,y0+h-1,c);
           drawLine(x0+w,y0+1,x0+w,y0+h-1,c);
       }
    }
  } else {
    for (int i = y0; i < y0+h; i++) {
          drawLine(x0, i, x0+w, i, c);
    }
  }
}

void Graphics::drawCircle(int x0, int y0, int radius, pixel_t c, int fc)
{
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;
  int pyy = y,pyx = x;
  
  //there is a fill color
  if (fc != -1)
    drawLine(x0-radius, y0, x0+radius, y0, fc);
  
  vs23.setPixel(x0, y0 + radius,c);
  vs23.setPixel(x0, y0 - radius,c);
  vs23.setPixel(x0 + radius, y0,c);
  vs23.setPixel(x0 - radius, y0,c);
  
  while(x+1 < y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    
    //there is a fill color
    if (fc != -1) {
      //prevent double draws on the same rows
      if (pyy != y) {
        drawLine(x0-x, y0+y, x0+x, y0+y, fc);
        drawLine(x0-x, y0-y, x0+x, y0-y, fc);
      }

      if (pyx != x && x != y) {
        drawLine(x0-y, y0+x, x0+y, y0+x, fc);
        drawLine(x0-y, y0-x, x0+y, y0-x, fc);
      }

      pyy = y;
      pyx = x;
    }
    vs23.setPixel(x0 + x, y0 + y,c);
    vs23.setPixel(x0 - x, y0 + y,c);
    vs23.setPixel(x0 + x, y0 - y,c);
    vs23.setPixel(x0 - x, y0 - y,c);
    vs23.setPixel(x0 + y, y0 + x,c);
    vs23.setPixel(x0 - y, y0 + x,c);
    vs23.setPixel(x0 + y, y0 - x,c);
    vs23.setPixel(x0 - y, y0 - x,c);
  }
}

// Draws a line between two points (x1,y1) and (x2,y2).
void Graphics::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
			pixel_t c)
{
  int deltax = abs(x2 - x1);
  int deltay = abs(y2 - y1);

  int slopex = x1 < x2 ? 1 : -1;
  int slopey = y1 < y2 ? 1 : -1;

  int err = (deltax > deltay ? deltax : -deltay) / 2, e2;

  for (;;) {
    vs23.setPixel(x1, y1, c);

    if (x1 == x2 && y1 == y2)
      break;

    e2 = err;
    if (e2 > -deltax) {
      err -= deltay;
      x1 += slopex;
    }
    if (e2 <  deltay) {
      err += deltax;
      y1 += slopey;
    }
  }
}

void Graphics::drawLineRgb(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
			   uint8_t r, uint8_t g, uint8_t b)
{
	pixel_t c = csp.colorFromRgb(r, g, b);

	drawLine(x1, y1, x2, y2, c);
}

