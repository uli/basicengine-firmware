// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

// 32-bpp to 32-bpp integral factor image scalers.

#include <SDL/SDL.h>

#define SRCPIXEL(x, y) ( \
	((pixel_t*)src)[ \
		(x) + (y) * src_pitch \
	] )
#define DSTPIXEL(x, y) ( \
	((pixel_t*)dst)[ \
		(x) + (y) * dst_pitch \
	] )

static void scale_1x2(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x, y*2) = p;
			DSTPIXEL(x, y*2+1) = p;
		}
	}
}

static void scale_1x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x, y*3) = p;
			DSTPIXEL(x, y*3+1) = p;
			DSTPIXEL(x, y*3+2) = p;
		}
	}
}

static void scale_2x1(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*2, y) = p;
			DSTPIXEL(x*2+1, y) = p;
		}
	}
}

static void scale_2x2(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*2, y*2) = p;
			DSTPIXEL(x*2+1, y*2) = p;
			DSTPIXEL(x*2, y*2+1) = p;
			DSTPIXEL(x*2+1, y*2+1) = p;
		}
	}
}

static void scale_2x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*2, y*3) = p;
			DSTPIXEL(x*2+1, y*3) = p;
			DSTPIXEL(x*2, y*3+1) = p;
			DSTPIXEL(x*2+1, y*3+1) = p;
			DSTPIXEL(x*2, y*3+2) = p;
			DSTPIXEL(x*2+1, y*3+2) = p;
		}
	}
}

static void scale_3x2(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*3, y*2) = p;
			DSTPIXEL(x*3+1, y*2) = p;
			DSTPIXEL(x*3+2, y*2) = p;
			DSTPIXEL(x*3, y*2+1) = p;
			DSTPIXEL(x*3+1, y*2+1) = p;
			DSTPIXEL(x*3+2, y*2+1) = p;
		}
	}
}

static void scale_3x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*3, y*3) = p;
			DSTPIXEL(x*3+1, y*3) = p;
			DSTPIXEL(x*3+2, y*3) = p;
			DSTPIXEL(x*3, y*3+1) = p;
			DSTPIXEL(x*3+1, y*3+1) = p;
			DSTPIXEL(x*3+2, y*3+1) = p;
			DSTPIXEL(x*3, y*3+2) = p;
			DSTPIXEL(x*3+1, y*3+2) = p;
			DSTPIXEL(x*3+2, y*3+2) = p;
		}
	}
}

static void scale_3x4(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*3, y*4) = p;
			DSTPIXEL(x*3+1, y*4) = p;
			DSTPIXEL(x*3+2, y*4) = p;
			DSTPIXEL(x*3, y*4+1) = p;
			DSTPIXEL(x*3+1, y*4+1) = p;
			DSTPIXEL(x*3+2, y*4+1) = p;
			DSTPIXEL(x*3, y*4+2) = p;
			DSTPIXEL(x*3+1, y*4+2) = p;
			DSTPIXEL(x*3+2, y*4+2) = p;
			DSTPIXEL(x*3, y*4+3) = p;
			DSTPIXEL(x*3+1, y*4+3) = p;
			DSTPIXEL(x*3+2, y*4+3) = p;
		}
	}
}

static void scale_4x2(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*4, y*2) = p;
			DSTPIXEL(x*4+1, y*2) = p;
			DSTPIXEL(x*4+2, y*2) = p;
			DSTPIXEL(x*4+3, y*2) = p;
			DSTPIXEL(x*4, y*2+1) = p;
			DSTPIXEL(x*4+1, y*2+1) = p;
			DSTPIXEL(x*4+2, y*2+1) = p;
			DSTPIXEL(x*4+3, y*2+1) = p;
		}
	}
}

static void scale_4x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*4, y*3) = p;
			DSTPIXEL(x*4+1, y*3) = p;
			DSTPIXEL(x*4+2, y*3) = p;
			DSTPIXEL(x*4+3, y*3) = p;
			DSTPIXEL(x*4, y*3+1) = p;
			DSTPIXEL(x*4+1, y*3+1) = p;
			DSTPIXEL(x*4+2, y*3+1) = p;
			DSTPIXEL(x*4+3, y*3+1) = p;
			DSTPIXEL(x*4, y*3+2) = p;
			DSTPIXEL(x*4+1, y*3+2) = p;
			DSTPIXEL(x*4+2, y*3+2) = p;
			DSTPIXEL(x*4+3, y*3+2) = p;
		}
	}
}

static void scale_4x4(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*4, y*4) = p;
			DSTPIXEL(x*4+1, y*4) = p;
			DSTPIXEL(x*4+2, y*4) = p;
			DSTPIXEL(x*4+3, y*4) = p;
			DSTPIXEL(x*4, y*4+1) = p;
			DSTPIXEL(x*4+1, y*4+1) = p;
			DSTPIXEL(x*4+2, y*4+1) = p;
			DSTPIXEL(x*4+3, y*4+1) = p;
			DSTPIXEL(x*4, y*4+2) = p;
			DSTPIXEL(x*4+1, y*4+2) = p;
			DSTPIXEL(x*4+2, y*4+2) = p;
			DSTPIXEL(x*4+3, y*4+2) = p;
			DSTPIXEL(x*4, y*4+3) = p;
			DSTPIXEL(x*4+1, y*4+3) = p;
			DSTPIXEL(x*4+2, y*4+3) = p;
			DSTPIXEL(x*4+3, y*4+3) = p;
		}
	}
}

static void scale_4x5(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*4, y*5) = p;
			DSTPIXEL(x*4+1, y*5) = p;
			DSTPIXEL(x*4+2, y*5) = p;
			DSTPIXEL(x*4+3, y*5) = p;
			DSTPIXEL(x*4, y*5+1) = p;
			DSTPIXEL(x*4+1, y*5+1) = p;
			DSTPIXEL(x*4+2, y*5+1) = p;
			DSTPIXEL(x*4+3, y*5+1) = p;
			DSTPIXEL(x*4, y*5+2) = p;
			DSTPIXEL(x*4+1, y*5+2) = p;
			DSTPIXEL(x*4+2, y*5+2) = p;
			DSTPIXEL(x*4+3, y*5+2) = p;
			DSTPIXEL(x*4, y*5+3) = p;
			DSTPIXEL(x*4+1, y*5+3) = p;
			DSTPIXEL(x*4+2, y*5+3) = p;
			DSTPIXEL(x*4+3, y*5+3) = p;
			DSTPIXEL(x*4, y*5+4) = p;
			DSTPIXEL(x*4+1, y*5+4) = p;
			DSTPIXEL(x*4+2, y*5+4) = p;
			DSTPIXEL(x*4+3, y*5+4) = p;
		}
	}
}

static void scale_5x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*5, y*3) = p;
			DSTPIXEL(x*5+1, y*3) = p;
			DSTPIXEL(x*5+2, y*3) = p;
			DSTPIXEL(x*5+3, y*3) = p;
			DSTPIXEL(x*5+4, y*3) = p;
			DSTPIXEL(x*5, y*3+1) = p;
			DSTPIXEL(x*5+1, y*3+1) = p;
			DSTPIXEL(x*5+2, y*3+1) = p;
			DSTPIXEL(x*5+3, y*3+1) = p;
			DSTPIXEL(x*5+4, y*3+1) = p;
			DSTPIXEL(x*5, y*3+2) = p;
			DSTPIXEL(x*5+1, y*3+2) = p;
			DSTPIXEL(x*5+2, y*3+2) = p;
			DSTPIXEL(x*5+3, y*3+2) = p;
			DSTPIXEL(x*5+4, y*3+2) = p;
		}
	}
}

static void scale_5x4(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*5, y*4) = p;
			DSTPIXEL(x*5+1, y*4) = p;
			DSTPIXEL(x*5+2, y*4) = p;
			DSTPIXEL(x*5+3, y*4) = p;
			DSTPIXEL(x*5+4, y*4) = p;
			DSTPIXEL(x*5, y*4+1) = p;
			DSTPIXEL(x*5+1, y*4+1) = p;
			DSTPIXEL(x*5+2, y*4+1) = p;
			DSTPIXEL(x*5+3, y*4+1) = p;
			DSTPIXEL(x*5+4, y*4+1) = p;
			DSTPIXEL(x*5, y*4+2) = p;
			DSTPIXEL(x*5+1, y*4+2) = p;
			DSTPIXEL(x*5+2, y*4+2) = p;
			DSTPIXEL(x*5+3, y*4+2) = p;
			DSTPIXEL(x*5+4, y*4+2) = p;
			DSTPIXEL(x*5, y*4+3) = p;
			DSTPIXEL(x*5+1, y*4+3) = p;
			DSTPIXEL(x*5+2, y*4+3) = p;
			DSTPIXEL(x*5+3, y*4+3) = p;
			DSTPIXEL(x*5+4, y*4+3) = p;
		}
	}
}

static void scale_6x3(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*6, y*3) = p;
			DSTPIXEL(x*6+1, y*3) = p;
			DSTPIXEL(x*6+2, y*3) = p;
			DSTPIXEL(x*6+3, y*3) = p;
			DSTPIXEL(x*6+4, y*3) = p;
			DSTPIXEL(x*6+5, y*3) = p;
			DSTPIXEL(x*6, y*3+1) = p;
			DSTPIXEL(x*6+1, y*3+1) = p;
			DSTPIXEL(x*6+2, y*3+1) = p;
			DSTPIXEL(x*6+3, y*3+1) = p;
			DSTPIXEL(x*6+4, y*3+1) = p;
			DSTPIXEL(x*6+5, y*3+1) = p;
			DSTPIXEL(x*6, y*3+2) = p;
			DSTPIXEL(x*6+1, y*3+2) = p;
			DSTPIXEL(x*6+2, y*3+2) = p;
			DSTPIXEL(x*6+3, y*3+2) = p;
			DSTPIXEL(x*6+4, y*3+2) = p;
			DSTPIXEL(x*6+5, y*3+2) = p;
		}
	}
}

static void scale_6x4(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*6, y*4) = p;
			DSTPIXEL(x*6+1, y*4) = p;
			DSTPIXEL(x*6+2, y*4) = p;
			DSTPIXEL(x*6+3, y*4) = p;
			DSTPIXEL(x*6+4, y*4) = p;
			DSTPIXEL(x*6+5, y*4) = p;
			DSTPIXEL(x*6, y*4+1) = p;
			DSTPIXEL(x*6+1, y*4+1) = p;
			DSTPIXEL(x*6+2, y*4+1) = p;
			DSTPIXEL(x*6+3, y*4+1) = p;
			DSTPIXEL(x*6+4, y*4+1) = p;
			DSTPIXEL(x*6+5, y*4+1) = p;
			DSTPIXEL(x*6, y*4+2) = p;
			DSTPIXEL(x*6+1, y*4+2) = p;
			DSTPIXEL(x*6+2, y*4+2) = p;
			DSTPIXEL(x*6+3, y*4+2) = p;
			DSTPIXEL(x*6+4, y*4+2) = p;
			DSTPIXEL(x*6+5, y*4+2) = p;
			DSTPIXEL(x*6, y*4+3) = p;
			DSTPIXEL(x*6+1, y*4+3) = p;
			DSTPIXEL(x*6+2, y*4+3) = p;
			DSTPIXEL(x*6+3, y*4+3) = p;
			DSTPIXEL(x*6+4, y*4+3) = p;
			DSTPIXEL(x*6+5, y*4+3) = p;
		}
	}
}

static void scale_6x5(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*6, y*5) = p;
			DSTPIXEL(x*6+1, y*5) = p;
			DSTPIXEL(x*6+2, y*5) = p;
			DSTPIXEL(x*6+3, y*5) = p;
			DSTPIXEL(x*6+4, y*5) = p;
			DSTPIXEL(x*6+5, y*5) = p;
			DSTPIXEL(x*6, y*5+1) = p;
			DSTPIXEL(x*6+1, y*5+1) = p;
			DSTPIXEL(x*6+2, y*5+1) = p;
			DSTPIXEL(x*6+3, y*5+1) = p;
			DSTPIXEL(x*6+4, y*5+1) = p;
			DSTPIXEL(x*6+5, y*5+1) = p;
			DSTPIXEL(x*6, y*5+2) = p;
			DSTPIXEL(x*6+1, y*5+2) = p;
			DSTPIXEL(x*6+2, y*5+2) = p;
			DSTPIXEL(x*6+3, y*5+2) = p;
			DSTPIXEL(x*6+4, y*5+2) = p;
			DSTPIXEL(x*6+5, y*5+2) = p;
			DSTPIXEL(x*6, y*5+3) = p;
			DSTPIXEL(x*6+1, y*5+3) = p;
			DSTPIXEL(x*6+2, y*5+3) = p;
			DSTPIXEL(x*6+3, y*5+3) = p;
			DSTPIXEL(x*6+4, y*5+3) = p;
			DSTPIXEL(x*6+5, y*5+3) = p;
			DSTPIXEL(x*6, y*5+4) = p;
			DSTPIXEL(x*6+1, y*5+4) = p;
			DSTPIXEL(x*6+2, y*5+4) = p;
			DSTPIXEL(x*6+3, y*5+4) = p;
			DSTPIXEL(x*6+4, y*5+4) = p;
			DSTPIXEL(x*6+5, y*5+4) = p;
		}
	}
}

static void scale_12x5(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			uint32_t p = SRCPIXEL(x, y);
			DSTPIXEL(x*12, y*5) = p;
			DSTPIXEL(x*12+1, y*5) = p;
			DSTPIXEL(x*12+2, y*5) = p;
			DSTPIXEL(x*12+3, y*5) = p;
			DSTPIXEL(x*12+4, y*5) = p;
			DSTPIXEL(x*12+5, y*5) = p;
			DSTPIXEL(x*12+6, y*5) = p;
			DSTPIXEL(x*12+7, y*5) = p;
			DSTPIXEL(x*12+8, y*5) = p;
			DSTPIXEL(x*12+9, y*5) = p;
			DSTPIXEL(x*12+10, y*5) = p;
			DSTPIXEL(x*12+11, y*5) = p;
			DSTPIXEL(x*12, y*5+1) = p;
			DSTPIXEL(x*12+1, y*5+1) = p;
			DSTPIXEL(x*12+2, y*5+1) = p;
			DSTPIXEL(x*12+3, y*5+1) = p;
			DSTPIXEL(x*12+4, y*5+1) = p;
			DSTPIXEL(x*12+5, y*5+1) = p;
			DSTPIXEL(x*12+6, y*5+1) = p;
			DSTPIXEL(x*12+7, y*5+1) = p;
			DSTPIXEL(x*12+8, y*5+1) = p;
			DSTPIXEL(x*12+9, y*5+1) = p;
			DSTPIXEL(x*12+10, y*5+1) = p;
			DSTPIXEL(x*12+11, y*5+1) = p;
			DSTPIXEL(x*12, y*5+2) = p;
			DSTPIXEL(x*12+1, y*5+2) = p;
			DSTPIXEL(x*12+2, y*5+2) = p;
			DSTPIXEL(x*12+3, y*5+2) = p;
			DSTPIXEL(x*12+4, y*5+2) = p;
			DSTPIXEL(x*12+5, y*5+2) = p;
			DSTPIXEL(x*12+6, y*5+2) = p;
			DSTPIXEL(x*12+7, y*5+2) = p;
			DSTPIXEL(x*12+8, y*5+2) = p;
			DSTPIXEL(x*12+9, y*5+2) = p;
			DSTPIXEL(x*12+10, y*5+2) = p;
			DSTPIXEL(x*12+11, y*5+2) = p;
			DSTPIXEL(x*12, y*5+3) = p;
			DSTPIXEL(x*12+1, y*5+3) = p;
			DSTPIXEL(x*12+2, y*5+3) = p;
			DSTPIXEL(x*12+3, y*5+3) = p;
			DSTPIXEL(x*12+4, y*5+3) = p;
			DSTPIXEL(x*12+5, y*5+3) = p;
			DSTPIXEL(x*12+6, y*5+3) = p;
			DSTPIXEL(x*12+7, y*5+3) = p;
			DSTPIXEL(x*12+8, y*5+3) = p;
			DSTPIXEL(x*12+9, y*5+3) = p;
			DSTPIXEL(x*12+10, y*5+3) = p;
			DSTPIXEL(x*12+11, y*5+3) = p;
			DSTPIXEL(x*12, y*5+4) = p;
			DSTPIXEL(x*12+1, y*5+4) = p;
			DSTPIXEL(x*12+2, y*5+4) = p;
			DSTPIXEL(x*12+3, y*5+4) = p;
			DSTPIXEL(x*12+4, y*5+4) = p;
			DSTPIXEL(x*12+5, y*5+4) = p;
			DSTPIXEL(x*12+6, y*5+4) = p;
			DSTPIXEL(x*12+7, y*5+4) = p;
			DSTPIXEL(x*12+8, y*5+4) = p;
			DSTPIXEL(x*12+9, y*5+4) = p;
			DSTPIXEL(x*12+10, y*5+4) = p;
			DSTPIXEL(x*12+11, y*5+4) = p;
		}
	}
}

static void scale_p33xp33(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h/3; ++y) {
		for (int x = 0; x < src_w/3; ++x) {
			uint32_t p = SRCPIXEL(x*3, y*3);
			DSTPIXEL(x, y) = p;
		}
	}
}

static void scale_p33xp5(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h/2; ++y) {
		for (int x = 0; x < src_w/3; ++x) {
			uint32_t p = SRCPIXEL(x*3, y*2);
			DSTPIXEL(x, y) = p;
		}
	}
}

static void scale_p5xp33(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h/3; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			uint32_t p = SRCPIXEL(x*2, y*3);
			DSTPIXEL(x, y) = p;
		}
	}
}

static void scale_p5xp5(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h/2; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			uint32_t p = SRCPIXEL(x*2, y*2);
			DSTPIXEL(x, y) = p;
		}
	}
}

static void scale_p5x1(uint32_t *src, uint32_t *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			uint32_t p = SRCPIXEL(x*2, y);
			DSTPIXEL(x, y) = p;
		}
	}
}

typedef void (*scaler_t)(uint32_t *, uint32_t *, int, int, int, int);

// Combinations that do not occur with the current set of modes are not
// implemented, but are mapped to a fallback scaler, just in case...

static scaler_t upscalers[12][6] = {
	{ NULL,		scale_1x2,	scale_1x3,	scale_1x3,	scale_1x3,	scale_1x3	},	// 1x
	{ scale_2x1,	scale_2x2,	scale_2x3,	scale_2x3,	scale_2x3,	scale_2x3	},	// 2x
	{ scale_2x1,	scale_3x2,	scale_3x3,	scale_3x4,	scale_3x4,	scale_3x4	},	// 3x
	{ scale_2x1,	scale_4x2,	scale_4x3,	scale_4x4,	scale_4x5,	scale_4x5	},	// 4x
	{ scale_2x1,	scale_4x2,	scale_5x3,	scale_5x4,	scale_5x4,	scale_5x4	},	// 5x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 6x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 7x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 8x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 9x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 10x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_6x5,	scale_6x5	},	// 11x
	{ scale_2x1,	scale_4x2,	scale_6x3,	scale_6x4,	scale_12x5,	scale_12x5	},	// 12x
};

static scaler_t downscalers[3][3] = {
	{ NULL,		scale_p5xp5,	scale_p5xp33	}, // 1x
	{ scale_p5x1,	scale_p5xp5,	scale_p5xp33	}, // .55x
	{ scale_p33xp5,	scale_p33xp5,	scale_p33xp33	}, // .33x
};

static void scale_integral(SDL_Surface *src, SDL_Surface *dst, int src_h)
{
	int upfactor_x = dst->w / src->w;
	int upfactor_y = dst->h / src_h;

	int downfactor_x = (src->w + dst->w - 1) / dst->w;
	int downfactor_y = (src_h + dst->h - 1) / dst->h;

	int src_pitch = src->pitch / sizeof(uint32_t);
	int dst_pitch = dst->pitch / sizeof(uint32_t);

	uint32_t *sp = (uint32_t *)src->pixels;
	uint32_t *dp = (uint32_t *)dst->pixels;

	// XXX: center

	if (downfactor_x > 0 && downfactor_y > 0 && downscalers[downfactor_x - 1][downfactor_y - 1])
		downscalers[downfactor_x - 1][downfactor_y - 1](sp, dp, src->w, src_h, src_pitch, dst_pitch);
	else if (upfactor_x > 0 && upfactor_y > 0 && upscalers[upfactor_x - 1][upfactor_y - 1])
		upscalers[upfactor_x - 1][upfactor_y - 1](sp, dp, src->w, src_h, src_pitch, dst_pitch);
	else {
		// 1x1
		SDL_Rect r = { 0, 0, (Uint16)src->w, (Uint16)src_h };
		SDL_BlitSurface(src, &r, dst, NULL);
	}
}

#undef SRCPIXEL
#undef DSTPIXEL
