// PCX image loader. Public domain. See "unlicense" statement at the end of this file.
// dr_pcx - v0.2a - 2017-07-16
//
// David Reid - mackron@gmail.com

// USAGE
//
// dr_pcx is a single-file library. To use it, do something like the following in one .c file.
//     #define DR_PCX_IMPLEMENTATION
//     #include "dr_pcx.h"
//
// You can then #include this file in other parts of the program as you would with any other header file. Do something like
// the following to load and decode an image:
//
//     int width;
//     int height;
//     int components
//     dr_uint8* pImageData = drpcx_load_file("my_image.pcx", DR_FALSE, &width, &height, &components, 0);
//     if (pImageData == NULL) {
//         // Failed to load image.
//     }
//
//     ...
//
//     drpcx_free(pImageData);
//
// The boolean parameter (second argument in the above example) is whether or not the image should be flipped upside down.
// 
//
//
// OPTIONS
// #define these options before including this file.
//
// #define DR_PCX_NO_STDIO
//   Disable drpcx_load_file().
//
//
//
// QUICK NOTES
// - 2-bpp/4-plane and 4-bpp/1-plane formats have not been tested.

#ifndef dr_pcx_h
#define dr_pcx_h

#ifndef DR_SIZED_TYPES_DEFINED
#define DR_SIZED_TYPES_DEFINED
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    dr_int8;
typedef unsigned char    dr_uint8;
typedef   signed short   dr_int16;
typedef unsigned short   dr_uint16;
typedef   signed int     dr_int32;
typedef unsigned int     dr_uint32;
typedef   signed __int64 dr_int64;
typedef unsigned __int64 dr_uint64;
#else
#include <stdint.h>
typedef int8_t           dr_int8;
typedef uint8_t          dr_uint8;
typedef int16_t          dr_int16;
typedef uint16_t         dr_uint16;
typedef int32_t          dr_int32;
typedef uint32_t         dr_uint32;
typedef int64_t          dr_int64;
typedef uint64_t         dr_uint64;
#endif
typedef dr_uint8         dr_bool8;
typedef dr_uint32        dr_bool32;
#define DR_TRUE          1
#define DR_FALSE         0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Callback for when data is read. Return value is the number of bytes actually read.
typedef size_t (* drpcx_read_proc)(void* userData, void* bufferOut, size_t bytesToRead);


// Loads a PCX file using the given callbacks.
bool drpcx_load(drpcx_read_proc onRead, void* pUserData, dr_bool32 flipped, int* x, int* y,
                     int* internalComponents, int desiredComponents,
                     int dst_x, int dst_y, int off_x, int off_y, int w, int h, int mask = -1);

// Frees memory returned by drpcx_load() and family.
void drpcx_free(void* pReturnValueFromLoad);


#ifdef __cplusplus
}
#endif

#endif  // dr_pcx_h


///////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
///////////////////////////////////////////////////////////////////////////////
#ifdef DR_PCX_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct
{
    dr_uint8 header;
    dr_uint8 version;
    dr_uint8 encoding;
    dr_uint8 bpp;
    dr_uint16 left;
    dr_uint16 top;
    dr_uint16 right;
    dr_uint16 bottom;
    dr_uint16 hres;
    dr_uint16 vres;
    dr_uint8 palette16[48];
    dr_uint8 reserved1;
    dr_uint8 bitPlanes;
    dr_uint16 bytesPerLine;
    dr_uint16 paletteType;
    dr_uint16 screenSizeH;
    dr_uint16 screenSizeV;
    dr_uint8 reserved2[54];
} drpcx_header;

typedef struct
{
    drpcx_read_proc onRead;
    void* pUserData;
    dr_bool32 flipped;
    drpcx_header header;

    dr_uint32 width;
    dr_uint32 height;
    dr_uint32 components;    // 3 = RGB; 4 = RGBA. Only 3 and 4 are supported.
    dr_uint8* pImageData;
    int dst_x, dst_y;
    int off_x, off_y;
    int w, h;
    int mask;
} drpcx;


static dr_uint8 drpcx__read_byte(drpcx* pPCX)
{
    dr_uint8 byte = 0;
    pPCX->onRead(pPCX->pUserData, &byte, 1);

    return byte;
}

#ifdef HAVE_UNTESTED
static dr_uint8* drpcx__row_ptr(drpcx* pPCX, dr_uint32 row)
{
    dr_uint32 stride = pPCX->width * pPCX->components;

    dr_uint8* pRow = pPCX->pImageData;
    if (pPCX->flipped) {
        pRow += (pPCX->height - row - 1) * stride;
    } else {
        pRow += row * stride;
    }

    return pRow;
}
#endif

static dr_uint8 drpcx__rle(drpcx* pPCX, dr_uint8* pRLEValueOut)
{
    dr_uint8 rleCount;
    dr_uint8 rleValue;

    rleValue = drpcx__read_byte(pPCX);
    if ((rleValue & 0xC0) == 0xC0) {
        rleCount = rleValue & 0x3F;
        rleValue = drpcx__read_byte(pPCX);
    } else {
        rleCount = 1;
    }


    *pRLEValueOut = rleValue;
    return rleCount;
}


dr_bool32 drpcx__decode_1bit(drpcx* pPCX)
{
    dr_uint8 rleCount = 0;
    dr_uint8 rleValue = 0;

    dr_uint32 stride = pPCX->width;
    dr_uint32 dx = pPCX->dst_x;
    dr_uint32 dy = pPCX->dst_y;
    dr_uint32 ox = pPCX->off_x;
    dr_uint32 oy = pPCX->off_y;

    switch (pPCX->header.bitPlanes)
    {
        case 1:
        {
            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                {
                    if (rleCount == 0) {
                        rleCount = drpcx__rle(pPCX, &rleValue);
                    }
                    rleCount -= 1;

                    for (int bit = 0; (bit < 8) && ((x*8 + bit) < pPCX->width); ++bit)
                    {
                        dr_uint8 mask = (1 << (7 - bit));
                        dr_uint8 paletteIndex = (rleValue & mask) >> (7 - bit);

                        vs23.setPixel(dx + x-ox, dy + y-oy, paletteIndex * 0x0f);
                    }
                }
            }

            return DR_TRUE;

        } break;

        case 2:
        case 3:
        case 4:
        {
            dr_uint8 *pRow_ = (dr_uint8 *)calloc(stride, 1);
            if (!pRow_)
              return DR_FALSE;

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 c = 0; c < pPCX->header.bitPlanes; ++c)
                {
                    dr_uint8* pRow = pRow_;
                    for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                    {
                        if (rleCount == 0) {
                            rleCount = drpcx__rle(pPCX, &rleValue);
                        }
                        rleCount -= 1;

                        for (int bit = 0; (bit < 8) && ((x*8 + bit) < pPCX->width); ++bit)
                        {
                            dr_uint8 mask = (1 << (7 - bit));
                            dr_uint8 paletteIndex = (rleValue & mask) >> (7 - bit);

                            pRow[0] |= ((paletteIndex & 0x01) << c);
                            pRow++;
                        }
                    }
                }


                dr_uint8* pRow = pRow_;
                for (dr_uint32 x = 0; x < pPCX->width; ++x)
                {
                    dr_uint8 paletteIndex = pRow[0];
                    vs23.setPixel(dx + x-ox, dy + y-oy, vs23.colorFromRgb(
                      pPCX->header.palette16[paletteIndex*3 + 0],
                      pPCX->header.palette16[paletteIndex*3 + 1],
                      pPCX->header.palette16[paletteIndex*3 + 2]
                    ));
                    
                    pRow++;
                }
            }

            free(pRow_);
            return DR_TRUE;
        }

        default: return DR_FALSE;
    }
}

dr_bool32 drpcx__decode_2bit(drpcx* pPCX)
{
    dr_uint8 rleCount = 0;
    dr_uint8 rleValue = 0;

    dr_uint32 dx = pPCX->dst_x;
    dr_uint32 dy = pPCX->dst_y;
    dr_uint32 ox = pPCX->off_x;
    dr_uint32 oy = pPCX->off_y;

    switch (pPCX->header.bitPlanes)
    {
        case 1:
        {
            dr_uint8 paletteCGA[48];
            paletteCGA[ 0] = 0x00; paletteCGA[ 1] = 0x00; paletteCGA[ 2] = 0x00;    // #000000
            paletteCGA[ 3] = 0x00; paletteCGA[ 4] = 0x00; paletteCGA[ 5] = 0xAA;    // #0000AA
            paletteCGA[ 6] = 0x00; paletteCGA[ 7] = 0xAA; paletteCGA[ 8] = 0x00;    // #00AA00
            paletteCGA[ 9] = 0x00; paletteCGA[10] = 0xAA; paletteCGA[11] = 0xAA;    // #00AAAA
            paletteCGA[12] = 0xAA; paletteCGA[13] = 0x00; paletteCGA[14] = 0x00;    // #AA0000
            paletteCGA[15] = 0xAA; paletteCGA[16] = 0x00; paletteCGA[17] = 0xAA;    // #AA00AA
            paletteCGA[18] = 0xAA; paletteCGA[19] = 0x55; paletteCGA[20] = 0x00;    // #AA5500
            paletteCGA[21] = 0xAA; paletteCGA[22] = 0xAA; paletteCGA[23] = 0xAA;    // #AAAAAA
            paletteCGA[24] = 0x55; paletteCGA[25] = 0x55; paletteCGA[26] = 0x55;    // #555555
            paletteCGA[27] = 0x55; paletteCGA[28] = 0x55; paletteCGA[29] = 0xFF;    // #5555FF
            paletteCGA[30] = 0x55; paletteCGA[31] = 0xFF; paletteCGA[32] = 0x55;    // #55FF55
            paletteCGA[33] = 0x55; paletteCGA[34] = 0xFF; paletteCGA[35] = 0xFF;    // #55FFFF
            paletteCGA[36] = 0xFF; paletteCGA[37] = 0x55; paletteCGA[38] = 0x55;    // #FF5555
            paletteCGA[39] = 0xFF; paletteCGA[40] = 0x55; paletteCGA[41] = 0xFF;    // #FF55FF
            paletteCGA[42] = 0xFF; paletteCGA[43] = 0xFF; paletteCGA[44] = 0x55;    // #FFFF55
            paletteCGA[45] = 0xFF; paletteCGA[46] = 0xFF; paletteCGA[47] = 0xFF;    // #FFFFFF

            dr_uint8 cgaBGColor   = pPCX->header.palette16[0] >> 4;
            dr_uint8 i = (pPCX->header.palette16[3] & 0x20) >> 5;
            dr_uint8 p = (pPCX->header.palette16[3] & 0x40) >> 6;
            //dr_uint8 c = (pPCX->header.palette16[3] & 0x80) >> 7;    // Color or monochrome. How is monochrome handled?

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                {
                    if (rleCount == 0) {
                        rleCount = drpcx__rle(pPCX, &rleValue);
                    }
                    rleCount -= 1;

                    for (int bit = 0; bit < 4; ++bit)
                    {
                        if (x*4 + bit < pPCX->width)
                        {
                            dr_uint8 mask = (3 << ((3 - bit) * 2));
                            dr_uint8 paletteIndex = (rleValue & mask) >> ((3 - bit) * 2);

                            dr_uint8 cgaIndex;
                            if (paletteIndex == 0) {    // Background.
                                cgaIndex = cgaBGColor;
                            } else {                    // Foreground
                                cgaIndex = (((paletteIndex << 1) + p) + (i << 3));
                            }

                            vs23.setPixel(dx + x-ox, dy + y-oy, vs23.colorFromRgb(
                              paletteCGA[cgaIndex*3 + 0],
                              paletteCGA[cgaIndex*3 + 1],
                              paletteCGA[cgaIndex*3 + 2]
                            ));
                        }
                    }
                }
            }

            // TODO: According to http://www.fysnet.net/pcxfile.htm, we should use the palette at the end of the file
            //       instead of the standard CGA palette if the version is equal to 5. With my test files the palette
            //       at the end of the file does not exist. Research this one.
            if (pPCX->header.version == 5) {
                dr_uint8 paletteMarker = drpcx__read_byte(pPCX);
                if (paletteMarker == 0x0C) {
                    // TODO: Implement Me.
                }
            }
            
            return DR_TRUE;
        };

#ifdef HAVE_UNTESTED
        case 4:
        {
            // NOTE: This is completely untested. If anybody knows where I can get a test file please let me know or send it through to me!
            // TODO: Test Me.

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 c = 0; c < pPCX->header.bitPlanes; ++c)
                {
                    dr_uint8* pRow = drpcx__row_ptr(pPCX, y);
                    for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                    {
                        if (rleCount == 0) {
                            rleCount = drpcx__rle(pPCX, &rleValue);
                        }
                        rleCount -= 1;

                        for (int bitpair = 0; (bitpair < 4) && ((x*4 + bitpair) < pPCX->width); ++bitpair)
                        {
                            dr_uint8 mask = (4 << (3 - bitpair));
                            dr_uint8 paletteIndex = (rleValue & mask) >> (3 - bitpair);

                            pRow[0] |= ((paletteIndex & 0x03) << (c*2));
                            pRow += pPCX->components;
                        }
                    }
                }


                dr_uint8* pRow = drpcx__row_ptr(pPCX, y);
                for (dr_uint32 x = 0; x < pPCX->width; ++x)
                {
                    dr_uint8 paletteIndex = pRow[0];
                    for (dr_uint32 c = 0; c < pPCX->header.bitPlanes; ++c) {
                        pRow[c] = pPCX->header.palette16[paletteIndex*3 + c];
                    }
                    
                    pRow += pPCX->components;
                }
            }

            return DR_TRUE;
        };
#endif

        default: return DR_FALSE;
    }
}

#ifdef HAVE_UNTESTED
dr_bool32 drpcx__decode_4bit(drpcx* pPCX)
{
    // NOTE: This is completely untested. If anybody knows where I can get a test file please let me know or send it through to me!
    // TODO: Test Me.

    if (pPCX->header.bitPlanes > 1) {
        return DR_FALSE;
    }

    dr_uint8 rleCount = 0;
    dr_uint8 rleValue = 0;

    for (dr_uint32 y = 0; y < pPCX->height; ++y)
    {
        for (dr_uint32 c = 0; c < pPCX->header.bitPlanes; ++c)
        {
            dr_uint8* pRow = drpcx__row_ptr(pPCX, y);
            for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
            {
                if (rleCount == 0) {
                    rleCount = drpcx__rle(pPCX, &rleValue);
                }
                rleCount -= 1;

                for (int nibble = 0; (nibble < 2) && ((x*2 + nibble) < pPCX->width); ++nibble)
                {
                    dr_uint8 mask = (4 << (1 - nibble));
                    dr_uint8 paletteIndex = (rleValue & mask) >> (1 - nibble);

                    pRow[0] |= ((paletteIndex & 0x0F) << (c*4)); 
                    pRow += pPCX->components;
                }
            }
        }


        dr_uint8* pRow = drpcx__row_ptr(pPCX, y);
        for (dr_uint32 x = 0; x < pPCX->width; ++x)
        {
            dr_uint8 paletteIndex = pRow[0];
            for (dr_uint32 c = 0; c < pPCX->components; ++c) {
                pRow[c] = pPCX->header.palette16[paletteIndex*3 + c];
            }
                    
            pRow += pPCX->components;
        }
    }

    return DR_TRUE;
}
#endif

dr_bool32 drpcx__decode_8bit(drpcx* pPCX)
{
    dr_uint8 rleCount = 0;
    dr_uint8 rleValue = 0;
    dr_uint32 stride = pPCX->width * pPCX->components;

    dr_uint32 dx = pPCX->dst_x;
    dr_uint32 dy = pPCX->dst_y;
    dr_uint32 ox = pPCX->off_x;
    dr_uint32 oy = pPCX->off_y;
    dr_uint32 w = pPCX->w ? : pPCX->width;
    dr_uint32 h = pPCX->h ? : pPCX->height;
    
    switch (pPCX->header.bitPlanes)
    {
        case 1:
        {
            // Parse the whole file once to get to the palette.
            dr_uint8 *palette256 = (dr_uint8 *)malloc(256 * 3);
            if (!palette256)
              return DR_FALSE;

            uint32_t position = pPCX->onRead(pPCX->pUserData, (void *)-1, 0);

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                {
                    if (rleCount == 0) {
                        rleCount = drpcx__rle(pPCX, &rleValue);
                    }
                    rleCount -= 1;
                }
            }

            // At this point we can know if we are dealing with a palette or a grayscale image by checking the next byte. If ti's equal to 0x0C, we
            // need to do a simple palette lookup.
            dr_uint8 paletteMarker = drpcx__read_byte(pPCX);
            if (paletteMarker == 0x0C)
            {
                // A palette is present - we need to do a second pass.
                if (pPCX->onRead(pPCX->pUserData, palette256, 256 * 3) != 256 * 3) {
                    free(palette256);
                    return DR_FALSE;
                }
                // Convert to YUV422
                for (int i = 0; i < 256; ++i) {
                  palette256[i] = vs23.colorFromRgb(palette256[i*3], palette256[i*3+1], palette256[i*3+2]);
                }
            }

            // Go back to the image data.
            pPCX->onRead(pPCX->pUserData, (void *)-2, position);

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                {
                    if (rleCount == 0) {
                        rleCount = drpcx__rle(pPCX, &rleValue);
                    }
                    rleCount -= 1;

                    if (y >= oy && y < oy+h && x >= ox && x < ox+w && x < pPCX->width) {
                      uint8_t c;
                      if (paletteMarker == 0x0c) {
                        c = palette256[rleValue];
                      } else {
                        c = rleValue >> 4;
                      }
                      if (pPCX->mask < 0 || c != pPCX->mask)
                        vs23.setPixel(dx + x-ox, dy + y-oy, c);
                    }
                }
            }
            free(palette256);

            return DR_TRUE;
        }

        case 3:
        case 4:
        {
            dr_uint8 *pRow_ = (dr_uint8 *)malloc(stride);
            if (!pRow_)
              return DR_FALSE;

            for (dr_uint32 y = 0; y < pPCX->height; ++y)
            {
                dr_uint8* pRow;
                for (dr_uint32 c = 0; c < pPCX->components; ++c)
                {
                    pRow = pRow_;
                    for (dr_uint32 x = 0; x < pPCX->header.bytesPerLine; ++x)
                    {
                        if (rleCount == 0) {
                            rleCount = drpcx__rle(pPCX, &rleValue);
                        }
                        rleCount -= 1;

                        if (x < pPCX->width) {
                            pRow[c] = rleValue;
                            pRow += pPCX->components;
                        }
                    }
                }
                if (y >= oy && y < oy+h) {
                  dr_uint32 x;
                  pRow = pRow_ + pPCX->components - 3 + ox * pPCX->components;
                  if (pPCX->mask < 0) {
                    for (x = ox; x < ox+w && x < pPCX->header.bytesPerLine; ++x, pRow+=pPCX->components) {
                      pRow_[x-ox] = vs23.colorFromRgb(pRow[0], pRow[1], pRow[2]);
                    }
                    SpiRamWriteBytes(vs23.pixelAddr(dx, dy+y-oy), pRow_, x-ox);
                  } else {
                    for (x = 0; x < w && x < pPCX->header.bytesPerLine; ++x, pRow+=pPCX->components) {
                      uint8_t c = vs23.colorFromRgb(pRow[0], pRow[1], pRow[2]);
                      if (pPCX->mask < 0 || c != pPCX->mask)
                        vs23.setPixel(dx+x, dy+y-oy, c);
                    }
                  }
                }
            }

            free(pRow_);
            return DR_TRUE;
        }
    }

    return DR_TRUE;
}

bool drpcx_info(drpcx_read_proc onRead, void* pUserData, int* x, int* y, int* internalComponents)
{
    drpcx pcx;
    pcx.onRead    = onRead;
    pcx.pUserData = pUserData;
    if (onRead(pUserData, &pcx.header, sizeof(pcx.header)) != sizeof(pcx.header)) {
        return false;    // Failed to read the header.
    }

    if (pcx.header.header != 10) {
        return false;    // Not a PCX file.
    }

    if (pcx.header.encoding != 1) {
        return false;    // Not supporting non-RLE encoding. Would assume a value of 0 indicates raw, unencoded, but that is apparently never used.
    }

    if (pcx.header.bpp != 1 && pcx.header.bpp != 2 && pcx.header.bpp != 4 && pcx.header.bpp != 8) {
        return false;    // Unsupported pixel format.
    }

    pcx.width = pcx.header.right - pcx.header.left + 1;
    pcx.height = pcx.header.bottom - pcx.header.top + 1;
    pcx.components = (pcx.header.bpp == 8 && pcx.header.bitPlanes == 4) ? 4 : 3;

    if (x) *x = pcx.width;
    if (y) *y = pcx.height;
    if (internalComponents)	*internalComponents = pcx.components;
    
    return true;
}

bool drpcx_load(drpcx_read_proc onRead, void* pUserData, dr_bool32 flipped, int* x, int* y, int* internalComponents, int desiredComponents,
                     int dx, int dy, int ox, int oy, int w, int h, int mask)
{
    if (onRead == NULL) return false;
    if (desiredComponents > 4) return false;

    drpcx pcx;
    pcx.onRead    = onRead;
    pcx.pUserData = pUserData;
    pcx.flipped   = flipped;
    if (onRead(pUserData, &pcx.header, sizeof(pcx.header)) != sizeof(pcx.header)) {
        return false;    // Failed to read the header.
    }

    if (pcx.header.header != 10) {
        return false;    // Not a PCX file.
    }

    if (pcx.header.encoding != 1) {
        return false;    // Not supporting non-RLE encoding. Would assume a value of 0 indicates raw, unencoded, but that is apparently never used.
    }

    if (pcx.header.bpp != 1 && pcx.header.bpp != 2 && pcx.header.bpp != 4 && pcx.header.bpp != 8) {
        return false;    // Unsupported pixel format.
    }


    pcx.width = pcx.header.right - pcx.header.left + 1;
    pcx.height = pcx.header.bottom - pcx.header.top + 1;
    pcx.components = (pcx.header.bpp == 8 && pcx.header.bitPlanes == 4) ? 4 : 3;
    pcx.dst_x = dx;
    pcx.dst_y = dy;
    pcx.off_x = ox;
    pcx.off_y = oy;
    pcx.w = w;
    pcx.h = h;
    pcx.mask = mask;

    dr_bool32 result = DR_FALSE;
    switch (pcx.header.bpp)
    {
        case 1:
        {
            result = drpcx__decode_1bit(&pcx);
        } break;

        case 2:
        {
            result = drpcx__decode_2bit(&pcx);
        } break;

#ifdef HAVE_UNTESTED
        case 4:
        {
            result = drpcx__decode_4bit(&pcx);
        } break;
#endif

        case 8:
        {
            result = drpcx__decode_8bit(&pcx);
        } break;
    }

    if (!result) {
        return false;
    }

    if (x) *x = pcx.width;
    if (y) *y = pcx.height;
    if (internalComponents) *internalComponents = pcx.components;
    return true;
}

void drpcx_free(void* pReturnValueFromLoad)
{
    free(pReturnValueFromLoad);
}

#endif // DR_PCX_IMPLEMENTATION


// REVISION HISTORY
//
// v0.2a - 2017-07-16
//   - Change underlying type for booleans to unsigned.
//
// v0.2 - 2016-10-28
//   - API CHANGE: Add a parameter to drpcx_load() and family to control the number of output components.
//   - Use custom sized types rather than built-in ones to improve support for older MSVC compilers.
//
// v0.1c - 2016-10-23
//   - A minor change to dr_bool8 and dr_bool32 types.
//
// v0.1b - 2016-10-11
//   - Use dr_bool32 instead of the built-in "bool" type. The reason for this change is that it helps maintain API/ABI consistency
//     between C and C++ builds.
//
// v0.1a - 2016-09-18
//   - Change date format to ISO 8601 (YYYY-MM-DD)
//
// v0.1 - 2016-05-04
//   - Initial versioned release.


// TODO
// - Test 2-bpp/4-plane and 4-bpp/1-plane formats.


/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

