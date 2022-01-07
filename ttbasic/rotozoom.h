
/*

SDL_rotozoom - rotozoomer

LGPL (c) A. Schiffler

*/

/*
 * Adapted for Engine BASIC
 * Copyright (c) 2021 Ulrich Hecht
 */

#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

#include <math.h>
#include <stdint.h>
#include "ttconfig.h"

// Stand-in for SDL_Surface
struct rz_surface_t {
	rz_surface_t(int width, int height,
		     pixel_t *in_pixels = NULL, int in_pitch = 0,
		     pixel_t ckey = (pixel_t)-1) {
		if (in_pixels) {
			pixels = in_pixels;
			pitch = in_pitch;
			free_pixels = false;
		} else {
			pixels = new pixel_t[width * height];
			pitch = width * sizeof(pixel_t);
			free_pixels = true;
		}
		w = width;
		h = height;
		colorkey = ckey;
	}

	~rz_surface_t() {
		if (free_pixels)
			delete[] pixels;
	}

	void fill(pixel_t color) {
		for (int i = 0; i < w * h; ++i)
			pixels[i] = color;
	}

	pixel_t getPixel(int x, int y) {
		return pixels[y * pitch / 4 + x];
	}

	pixel_t *pixels;
	int w, h;
	int pitch;
	pixel_t colorkey;
	bool free_pixels;
};

#ifndef M_PI
#define M_PI	3.141592654
#endif

	/* ---- Defines */

	/*!
	\brief Disable anti-aliasing (no smoothing).
	*/
#define SMOOTHING_OFF		0

	/*!
	\brief Enable anti-aliasing (smoothing).
	*/
#define SMOOTHING_ON		1

	/* ---- Function Prototypes */

#ifndef SDL_ROTOZOOM_SCOPE
#  define SDL_ROTOZOOM_SCOPE extern
#endif

	/*

	Rotozoom functions

	*/

	SDL_ROTOZOOM_SCOPE rz_surface_t *rotozoomSurface(rz_surface_t * src, double angle, double zoom, int smooth);

	SDL_ROTOZOOM_SCOPE rz_surface_t *rotozoomSurfaceXY
		(rz_surface_t * src, double angle, double zoomx, double zoomy, int smooth);


	SDL_ROTOZOOM_SCOPE void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth,
		int *dstheight);

	SDL_ROTOZOOM_SCOPE void rotozoomSurfaceSizeXY
		(int width, int height, double angle, double zoomx, double zoomy,
		int *dstwidth, int *dstheight);

	/*

	Zooming functions

	*/

	SDL_ROTOZOOM_SCOPE rz_surface_t *zoomSurface(rz_surface_t * src, double zoomx, double zoomy, int smooth);

	SDL_ROTOZOOM_SCOPE void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight);

	/*

	Shrinking functions

	*/

	SDL_ROTOZOOM_SCOPE rz_surface_t *shrinkSurface(rz_surface_t * src, int factorx, int factory);

	/*

	Specialized rotation functions

	*/

	SDL_ROTOZOOM_SCOPE rz_surface_t* rotateSurface90Degrees(rz_surface_t* src, int numClockwiseTurns);

#endif				/* _SDL_rotozoom_h */
