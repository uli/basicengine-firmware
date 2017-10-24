#include <algorithm>
using namespace std;

#include <stdint.h>
#include <stdio.h>
#define PROGMEM
#include "ttbasic/palette_rgb.h"

static void rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b, int &h, int &s, int &v)
{
  v = max(std::max(r, g), b);
  if (v == 0) {
    s = 0;
    h = 0;
    return;
  }
  int m = min(min(r, g), b);
  s = 255 * (v - m) / v;
  if (v == m) {
    h = 0;
  } else {
    int d = v - m;
    if (v == r)
      h = 60 * (g - b) / d;
    else if (v == g)
      h = 120 + 60 * (b-r) / d;
    else
      h = 240 + 60 * (r-g) / d;
  }
  if (h < 0)
    h += 360;
}

#define H_WEIGHT 3
#define S_WEIGHT 1
#define V_WEIGHT 2

#define BITS 4
#define MUL 17

#define COLV (256>>shdown)

int main()
{
  int shdown = 8-BITS;
  printf("/*GIMP Palette\n"
"Name: P-EE-A22-B22-Y44-N10\n"
"Columns: 3\n"
"#*/\n"
"static const uint8_t rgb444_to_yuv422[] PROGMEM = {\n");
  long totdiff = 0;
  int cr,cg,cb;
  for (cr = 0; cr < COLV; ++cr) {
  for (cg = 0; cg < COLV; ++cg) {
  for (cb = 0; cb < COLV; ++cb) {
    int ch,cs,cv;
    rgb_to_hsv(cr*MUL, cg*MUL, cb*MUL, ch, cs, cv);
    int mindiff2 = 100000000;
    int best2, best_h2, best_s2, best_v2;
    for (int i=0; i<256; ++i) {
        int r = yuv_palette_rgb[i*3];
        int g = yuv_palette_rgb[i*3+1];
        int b = yuv_palette_rgb[i*3+2];
        int h,s,v;
        rgb_to_hsv(r, g, b, h, s, v);
        int diff = ( H_WEIGHT*abs(ch-h)+S_WEIGHT*abs(cs-s)+V_WEIGHT*abs(cv-v) );
        if (diff < mindiff2) {
          mindiff2 = diff;
          best2 = i;
          best_h2 = h;
          best_s2 = s;
          best_v2 = v;
        }
    }
    printf("%3d,\n", best2);
  }
  }
  }
  printf("};\n");
}
