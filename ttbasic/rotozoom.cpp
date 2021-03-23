/*

SDL_rotozoom.c - rotozoomer, zoomer and shrinker for 32bit or 8bit surfaces

LGPL (c) A. Schiffler

*/

/*
 * Adapted for Engine BASIC
 * Copyright (c) 2021 Ulrich Hecht
 */

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <video.h>

#include "rotozoom.h"

/* ---- Internally used structures */

/*!
\brief A 32 bit RGBA pixel.
*/
typedef struct tColorRGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} tColorRGBA;

/*!
\brief A 8bit Y/palette pixel.
*/
typedef struct tColorY {
	uint8_t y;
} tColorY;

/*!
\brief Returns maximum of two numbers a and b.
*/
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))

/*!
\brief Number of guard rows added to destination surfaces.

This is a simple but effective workaround for observed issues.
These rows allocate extra memory and are then hidden from the surface.
Rows are added to the end of destination surfaces when they are allocated.
This catches any potential overflows which seem to happen with
just the right src image dimensions and scale/rotation and can lead
to a situation where the program can segfault.
*/
#define GUARD_ROWS (2)

/*!
\brief Lower limit of absolute zoom factor or rotation degrees.
*/
#define VALUE_LIMIT	0.001

/*!
\brief Returns colorkey info for a surface
*/
uint32_t _colorkey(rz_surface_t *src)
{
	uint32_t key = 0;
 	if (src)
 	{
 		key = src->colorkey;
	}
 	return key;
}


/*!
\brief Internal 32 bit integer-factor averaging Shrinker.

Shrinks 32 bit RGBA/ABGR 'src' surface to 'dst' surface.
Averages color and alpha values values of src pixels to calculate dst pixels.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to shrink (input).
\param dst The shrunken surface (output).
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return 0 for success or -1 for error.
*/
int _shrinkSurfaceRGBA(rz_surface_t * src, rz_surface_t * dst, int factorx, int factory)
{
	int x, y, dx, dy, sgap, dgap, ra, ga, ba, aa;
	int n_average;
	tColorRGBA *sp, *osp, *oosp;
	tColorRGBA *dp;

	/*
	* Averaging integer shrink
	*/

	/* Precalculate division factor */
	n_average = factorx*factory;

	/*
	* Scan destination
	*/
	sp = (tColorRGBA *) src->pixels;
	sgap = src->pitch - src->w * 4;

	dp = (tColorRGBA *) dst->pixels;
	dgap = dst->pitch - dst->w * 4;

	for (y = 0; y < dst->h; y++) {

		osp=sp;
		for (x = 0; x < dst->w; x++) {

			/* Trace out source box and accumulate */
			oosp=sp;
			ra=ga=ba=aa=0;
			for (dy=0; dy < factory; dy++) {
				for (dx=0; dx < factorx; dx++) {
					ra += sp->r;
					ga += sp->g;
					ba += sp->b;
					aa += sp->a;

					sp++;
				}
				/* src dx loop */
				sp = (tColorRGBA *)((uint8_t*)sp + (src->pitch - 4*factorx)); // next y
			}
			/* src dy loop */

			/* next box-x */
			sp = (tColorRGBA *)((uint8_t*)oosp + 4*factorx);

			/* Store result in destination */
			dp->r = ra/n_average;
			dp->g = ga/n_average;
			dp->b = ba/n_average;
			dp->a = aa/n_average;

			/*
			* Advance destination pointer
			*/
			dp++;
		}
		/* dst x loop */

		/* next box-y */
		sp = (tColorRGBA *)((uint8_t*)osp + src->pitch*factory);

		/*
		* Advance destination pointers
		*/
		dp = (tColorRGBA *) ((uint8_t *) dp + dgap);
	}
	/* dst y loop */

	return (0);
}

/*!
\brief Internal 8 bit integer-factor averaging shrinker.

Shrinks 8bit Y 'src' surface to 'dst' surface.
Averages color (brightness) values values of src pixels to calculate dst pixels.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to shrink (input).
\param dst The shrunken surface (output).
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return 0 for success or -1 for error.
*/
int _shrinkSurfaceY(rz_surface_t * src, rz_surface_t * dst, int factorx, int factory)
{
	int x, y, dx, dy, sgap, dgap, a;
	int n_average;
	uint8_t *sp, *osp, *oosp;
	uint8_t *dp;

	/*
	* Averaging integer shrink
	*/

	/* Precalculate division factor */
	n_average = factorx*factory;

	/*
	* Scan destination
	*/
	sp = (uint8_t *) src->pixels;
	sgap = src->pitch - src->w;

	dp = (uint8_t *) dst->pixels;
	dgap = dst->pitch - dst->w;

	for (y = 0; y < dst->h; y++) {

		osp=sp;
		for (x = 0; x < dst->w; x++) {

			/* Trace out source box and accumulate */
			oosp=sp;
			a=0;
			for (dy=0; dy < factory; dy++) {
				for (dx=0; dx < factorx; dx++) {
					a += (*sp);
					/* next x */
					sp++;
				}
				/* end src dx loop */
				/* next y */
				sp = (uint8_t *)((uint8_t*)sp + (src->pitch - factorx));
			}
			/* end src dy loop */

			/* next box-x */
			sp = (uint8_t *)((uint8_t*)oosp + factorx);

			/* Store result in destination */
			*dp = a/n_average;

			/*
			* Advance destination pointer
			*/
			dp++;
		}
		/* end dst x loop */

		/* next box-y */
		sp = (uint8_t *)((uint8_t*)osp + src->pitch*factory);

		/*
		* Advance destination pointers
		*/
		dp = (uint8_t *)((uint8_t *)dp + dgap);
	}
	/* end dst y loop */

	return (0);
}

/*!
\brief Internal 32 bit Zoomer with optional anti-aliasing by bilinear interpolation.

Zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to zoom (input).
\param dst The zoomed surface (output).
\param flipx Flag indicating if the image should be horizontally flipped.
\param flipy Flag indicating if the image should be vertically flipped.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return 0 for success or -1 for error.
*/
int _zoomSurfaceRGBA(rz_surface_t * src, rz_surface_t * dst, int flipx, int flipy, int smooth)
{
	int x, y, sx, sy, *sax, *say, *csax, *csay, csx, csy, ex, ey, t1, t2, sstep, lx, ly;
	tColorRGBA *c00, *c01, *c10, *c11, *cswap;
	tColorRGBA *sp, *csp, *dp;
	int dgap;

	/*
	* Variable setup
	*/
	if (smooth) {
		/*
		* For interpolation: assume source dimension is one pixel
		* smaller to avoid overflow on right and bottom edge.
		*/
		sx = (int) (65536.0 * (float) (src->w - 1) / (float) dst->w);
		sy = (int) (65536.0 * (float) (src->h - 1) / (float) dst->h);
	} else {
		sx = (int) (65536.0 * (float) src->w / (float) dst->w);
		sy = (int) (65536.0 * (float) src->h / (float) dst->h);
	}

	/*
	* Allocate memory for row increments
	*/
	if ((sax = (int *) malloc((dst->w + 1) * sizeof(uint32_t))) == NULL) {
		return (-1);
	}
	if ((say = (int *) malloc((dst->h + 1) * sizeof(uint32_t))) == NULL) {
		free(sax);
		return (-1);
	}

	/*
	* Precalculate row increments
	*/
	sp = csp = (tColorRGBA *) src->pixels;
	dp = (tColorRGBA *) dst->pixels;

	if (flipx) csp += (src->w-1);
	if (flipy) csp += ((src->pitch/4)*(src->h-1));

	csx = 0;
	csax = sax;
	for (x = 0; x <= dst->w; x++) {
		*csax = csx;
		csax++;
		csx &= 0xffff;
		csx += sx;
	}
	csy = 0;
	csay = say;
	for (y = 0; y <= dst->h; y++) {
		*csay = csy;
		csay++;
		csy &= 0xffff;
		csy += sy;
	}

	dgap = dst->pitch - dst->w * 4;

	/*
	* Switch between interpolating and non-interpolating code
	*/
	if (smooth) {

		/*
		* Interpolating Zoom
		*/

		/*
		* Scan destination
		*/
		csay = say;
		ly = 0;
		for (y = 0; y < dst->h; y++) {
			/*
			* Setup color source pointers
			*/
			c00 = csp;
			c01 = csp;
			c01++;
			c10 = csp;
			c10 += src->pitch/4;
			c11 = c10;
			c11++;
			if (flipx) {
				cswap = c00; c00=c01; c01=cswap;
				cswap = c10; c10=c11; c11=cswap;
			}
			if (flipy) {
				cswap = c00; c00=c10; c10=cswap;
				cswap = c01; c01=c11; c11=cswap;
			}

			csax = sax;
			lx = 0;
			for (x = 0; x < dst->w; x++) {
				/*
				* Draw and interpolate colors
				*/
				ex = (*csax & 0xffff);
				ey = (*csay & 0xffff);
				t1 = ((((c01->r - c00->r) * ex) >> 16) + c00->r) & 0xff;
				t2 = ((((c11->r - c10->r) * ex) >> 16) + c10->r) & 0xff;
				dp->r = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->g - c00->g) * ex) >> 16) + c00->g) & 0xff;
				t2 = ((((c11->g - c10->g) * ex) >> 16) + c10->g) & 0xff;
				dp->g = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->b - c00->b) * ex) >> 16) + c00->b) & 0xff;
				t2 = ((((c11->b - c10->b) * ex) >> 16) + c10->b) & 0xff;
				dp->b = (((t2 - t1) * ey) >> 16) + t1;
				t1 = ((((c01->a - c00->a) * ex) >> 16) + c00->a) & 0xff;
				t2 = ((((c11->a - c10->a) * ex) >> 16) + c10->a) & 0xff;
				dp->a = (((t2 - t1) * ey) >> 16) + t1;

				/*
				* Advance source pointers
				*/
				csax++;
				if (*csax > 0)
				{
					sstep = (*csax >> 16);
					lx += sstep;
					if (flipx) sstep = -sstep;
					if (lx <= src->w)
					{
						c00 += sstep;
						c01 += sstep;
						c10 += sstep;
						c11 += sstep;
					}
				}

				/*
				* Advance destination pointer
				*/
				dp++;
			}

			/*
			* Advance source pointer
			*/
			csay++;
			if (*csay > 0)
			{
				sstep = (*csay >> 16);
				ly += sstep;
				if (flipy) sstep = -sstep;
				if (ly < src->h)
				{
					csp += (sstep * (src->pitch/4));
				}
			}

			/*
			* Advance destination pointers
			*/
			dp = (tColorRGBA *) ((uint8_t *) dp + dgap);
		}
	} else {

		/*
		* Non-Interpolating Zoom
		*/
		csay = say;
		for (y = 0; y < dst->h; y++) {
			sp = csp;
			csax = sax;
			for (x = 0; x < dst->w; x++) {
				/*
				* Draw
				*/
				*dp = *sp;
				/*
				* Advance source pointers
				*/
				csax++;
				if (*csax > 0)
				{
					sstep = (*csax >> 16);
					if (flipx) sstep = -sstep;
					sp += sstep;
				}
				/*
				* Advance destination pointer
				*/
				dp++;
			}
			/*
			* Advance source pointer
			*/
			csay++;
			if (*csay > 0)
			{
				sstep = (*csay >> 16) * (src->pitch/4);
				if (flipy) sstep = -sstep;
				csp += sstep;
			}

			/*
			* Advance destination pointers
			*/
			dp = (tColorRGBA *) ((uint8_t *) dp + dgap);
		}
	}

	/*
	* Remove temp arrays
	*/
	free(sax);
	free(say);

	return (0);
}

/*!

\brief Internal 8 bit Zoomer without smoothing.

Zooms 8bit palette/Y 'src' surface to 'dst' surface.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to zoom (input).
\param dst The zoomed surface (output).
\param flipx Flag indicating if the image should be horizontally flipped.
\param flipy Flag indicating if the image should be vertically flipped.

\return 0 for success or -1 for error.
*/
int _zoomSurfaceY(rz_surface_t * src, rz_surface_t * dst, int flipx, int flipy)
{
	int x, y;
	uint32_t *sax, *say, *csax, *csay;
	int csx, csy;
	uint8_t *sp, *dp, *csp;
	int dgap;

	/*
	* Allocate memory for row increments
	*/
	if ((sax = (uint32_t *) malloc((dst->w + 1) * sizeof(uint32_t))) == NULL) {
		return (-1);
	}
	if ((say = (uint32_t *) malloc((dst->h + 1) * sizeof(uint32_t))) == NULL) {
		free(sax);
		return (-1);
	}

	/*
	* Pointer setup
	*/
	sp = csp = (uint8_t *) src->pixels;
	dp = (uint8_t *) dst->pixels;
	dgap = dst->pitch - dst->w;

	if (flipx) csp += (src->w-1);
	if (flipy) csp  = ( (uint8_t*)csp + src->pitch*(src->h-1) );

	/*
	* Precalculate row increments
	*/
	csx = 0;
	csax = sax;
	for (x = 0; x < dst->w; x++) {
		csx += src->w;
		*csax = 0;
		while (csx >= dst->w) {
			csx -= dst->w;
			(*csax)++;
		}
		csax++;
	}
	csy = 0;
	csay = say;
	for (y = 0; y < dst->h; y++) {
		csy += src->h;
		*csay = 0;
		while (csy >= dst->h) {
			csy -= dst->h;
			(*csay)++;
		}
		csay++;
	}

	/*
	* Draw
	*/
	csay = say;
	for (y = 0; y < dst->h; y++) {
		csax = sax;
		sp = csp;
		for (x = 0; x < dst->w; x++) {
			/*
			* Draw
			*/
			*dp = *sp;
			/*
			* Advance source pointers
			*/
			sp += (*csax) * (flipx ? -1 : 1);
			csax++;
			/*
			* Advance destination pointer
			*/
			dp++;
		}
		/*
		* Advance source pointer (for row)
		*/
		csp += ((*csay) * src->pitch) * (flipy ? -1 : 1);
		csay++;

		/*
		* Advance destination pointers
		*/
		dp += dgap;
	}

	/*
	* Remove temp arrays
	*/
	free(sax);
	free(say);

	return (0);
}

/*!
\brief Internal 32 bit rotozoomer with optional anti-aliasing.

Rotates and zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface based on the control
parameters by scanning the destination surface and applying optionally anti-aliasing
by bilinear interpolation.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src Source surface.
\param dst Destination surface.
\param cx Horizontal center coordinate.
\param cy Vertical center coordinate.
\param isin Integer version of sine of angle.
\param icos Integer version of cosine of angle.
\param flipx Flag indicating horizontal mirroring should be applied.
\param flipy Flag indicating vertical mirroring should be applied.
\param smooth Flag indicating anti-aliasing should be used.
*/
void _transformSurfaceRGBA(rz_surface_t * src, rz_surface_t * dst, int cx, int cy, int isin, int icos, int flipx, int flipy, int smooth)
{
	int x, y, t1, t2, dx, dy, xd, yd, sdx, sdy, ax, ay, ex, ey, sw, sh;
	tColorRGBA c00, c01, c10, c11, cswap;
	tColorRGBA *pc, *sp;
	int gap;

	/*
	* Variable setup
	*/
	xd = ((src->w - dst->w) << 15);
	yd = ((src->h - dst->h) << 15);
	ax = (cx << 16) - (icos * cx);
	ay = (cy << 16) - (isin * cx);
	sw = src->w - 1;
	sh = src->h - 1;
	pc = (tColorRGBA*) dst->pixels;
	gap = dst->pitch - dst->w * 4;

	/*
	* Switch between interpolating and non-interpolating code
	*/
	if (smooth) {
		for (y = 0; y < dst->h; y++) {
			dy = cy - y;
			sdx = (ax + (isin * dy)) + xd;
			sdy = (ay - (icos * dy)) + yd;
			for (x = 0; x < dst->w; x++) {
				dx = (sdx >> 16);
				dy = (sdy >> 16);
				if (flipx) dx = sw - dx;
				if (flipy) dy = sh - dy;
				if ((dx > -1) && (dy > -1) && (dx < (src->w-1)) && (dy < (src->h-1))) {
					sp = (tColorRGBA *)src->pixels;;
					sp += ((src->pitch/4) * dy);
					sp += dx;
					c00 = *sp;
					sp += 1;
					c01 = *sp;
					sp += (src->pitch/4);
					c11 = *sp;
					sp -= 1;
					c10 = *sp;
					if (flipx) {
						cswap = c00; c00=c01; c01=cswap;
						cswap = c10; c10=c11; c11=cswap;
					}
					if (flipy) {
						cswap = c00; c00=c10; c10=cswap;
						cswap = c01; c01=c11; c11=cswap;
					}
					/*
					* Interpolate colors
					*/
					ex = (sdx & 0xffff);
					ey = (sdy & 0xffff);
					t1 = ((((c01.r - c00.r) * ex) >> 16) + c00.r) & 0xff;
					t2 = ((((c11.r - c10.r) * ex) >> 16) + c10.r) & 0xff;
					pc->r = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.g - c00.g) * ex) >> 16) + c00.g) & 0xff;
					t2 = ((((c11.g - c10.g) * ex) >> 16) + c10.g) & 0xff;
					pc->g = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.b - c00.b) * ex) >> 16) + c00.b) & 0xff;
					t2 = ((((c11.b - c10.b) * ex) >> 16) + c10.b) & 0xff;
					pc->b = (((t2 - t1) * ey) >> 16) + t1;
					t1 = ((((c01.a - c00.a) * ex) >> 16) + c00.a) & 0xff;
					t2 = ((((c11.a - c10.a) * ex) >> 16) + c10.a) & 0xff;
					pc->a = (((t2 - t1) * ey) >> 16) + t1;
				}
				sdx += icos;
				sdy += isin;
				pc++;
			}
			pc = (tColorRGBA *) ((uint8_t *) pc + gap);
		}
	} else {
		for (y = 0; y < dst->h; y++) {
			dy = cy - y;
			sdx = (ax + (isin * dy)) + xd;
			sdy = (ay - (icos * dy)) + yd;
			for (x = 0; x < dst->w; x++) {
				dx = (short) (sdx >> 16);
				dy = (short) (sdy >> 16);
				if (flipx) dx = (src->w-1)-dx;
				if (flipy) dy = (src->h-1)-dy;
				if ((dx >= 0) && (dy >= 0) && (dx < src->w) && (dy < src->h)) {
					sp = (tColorRGBA *) ((uint8_t *) src->pixels + src->pitch * dy);
					sp += dx;
					*pc = *sp;
				}
				sdx += icos;
				sdy += isin;
				pc++;
			}
			pc = (tColorRGBA *) ((uint8_t *) pc + gap);
		}
	}
}

/*!

\brief Rotates and zooms 8 bit palette/Y 'src' surface to 'dst' surface without smoothing.

Rotates and zooms 8 bit RGBA/ABGR 'src' surface to 'dst' surface based on the control
parameters by scanning the destination surface.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src Source surface.
\param dst Destination surface.
\param cx Horizontal center coordinate.
\param cy Vertical center coordinate.
\param isin Integer version of sine of angle.
\param icos Integer version of cosine of angle.
\param flipx Flag indicating horizontal mirroring should be applied.
\param flipy Flag indicating vertical mirroring should be applied.
*/
void transformSurfaceY(rz_surface_t * src, rz_surface_t * dst, int cx, int cy, int isin, int icos, int flipx, int flipy)
{
	int x, y, dx, dy, xd, yd, sdx, sdy, ax, ay, sw, sh;
	tColorY *pc, *sp;
	int gap;

	/*
	* Variable setup
	*/
	xd = ((src->w - dst->w) << 15);
	yd = ((src->h - dst->h) << 15);
	ax = (cx << 16) - (icos * cx);
	ay = (cy << 16) - (isin * cx);
	sw = src->w - 1;
	sh = src->h - 1;
	pc = (tColorY*) dst->pixels;
	gap = dst->pitch - dst->w;
	/*
	* Clear surface to colorkey
	*/
	memset(pc, (int)(_colorkey(src) & 0xff), dst->pitch * dst->h);
	/*
	* Iterate through destination surface
	*/
	for (y = 0; y < dst->h; y++) {
		dy = cy - y;
		sdx = (ax + (isin * dy)) + xd;
		sdy = (ay - (icos * dy)) + yd;
		for (x = 0; x < dst->w; x++) {
			dx = (short) (sdx >> 16);
			dy = (short) (sdy >> 16);
			if (flipx) dx = (src->w-1)-dx;
			if (flipy) dy = (src->h-1)-dy;
			if ((dx >= 0) && (dy >= 0) && (dx < src->w) && (dy < src->h)) {
				sp = (tColorY *) (src->pixels);
				sp += (src->pitch * dy + dx);
				*pc = *sp;
			}
			sdx += icos;
			sdy += isin;
			pc++;
		}
		pc += gap;
	}
}

/*!
\brief Rotates a 32 bit surface in increments of 90 degrees.

Specialized 90 degree rotator which rotates a 'src' surface in 90 degree
increments clockwise returning a new surface. Faster than rotozoomer since
not scanning or interpolation takes place. Input surface must be 32 bit.
(code contributed by J. Schiller, improved by C. Allport and A. Schiffler)

\param src Source surface to rotate.
\param numClockwiseTurns Number of clockwise 90 degree turns to apply to the source.

\returns The new, rotated surface; or NULL for surfaces with incorrect input format.
*/
rz_surface_t* rotateSurface90Degrees(rz_surface_t* src, int numClockwiseTurns)
{
	int row, col, newWidth, newHeight;
	int bpp, src_ipr, dst_ipr;
	rz_surface_t* dst;
	uint32_t* srcBuf;
	uint32_t* dstBuf;

	/* Has to be a valid surface pointer and only 32-bit surfaces (for now) */
	if (!src || 32 != 32) { return NULL; }

	/* normalize numClockwiseTurns */
	while(numClockwiseTurns < 0) { numClockwiseTurns += 4; }
	numClockwiseTurns = (numClockwiseTurns % 4);

	/* if it's even, our new width will be the same as the source surface */
	newWidth = (numClockwiseTurns % 2) ? (src->h) : (src->w);
	newHeight = (numClockwiseTurns % 2) ? (src->w) : (src->h);
	dst = new rz_surface_t(newWidth, newHeight);
	if (!dst)
		return NULL;

	/* Calculate int-per-row */
	bpp = 4;//32 / 8;
	src_ipr = src->pitch / bpp;
	dst_ipr = dst->pitch / bpp;

	switch(numClockwiseTurns) {
		case 0: /* Make a copy of the surface */
			{
				/* Unfortunately SDL_BlitSurface cannot be used to make a copy of the surface
				   since it does not preserve alpha. */

				if (src->pitch == dst->pitch) {
					/* If the pitch is the same for both surfaces, the memory can be copied all at once. */
					memcpy(dst->pixels, src->pixels, (src->h * src->pitch));
				}
				else
				{
					/* If the pitch differs, copy each row separately */
					srcBuf = (uint32_t*)(src->pixels);
					dstBuf = (uint32_t*)(dst->pixels);
					for (row = 0; row < src->h; row++) {
						memcpy(dstBuf, srcBuf, dst->w * bpp);
						srcBuf += src_ipr;
						dstBuf += dst_ipr;
					} /* end for(col) */
				} /* end for(row) */
			}
			break;

			/* rotate clockwise */
		case 1: /* rotated 90 degrees clockwise */
		 {
			 for (row = 0; row < src->h; ++row) {
				 srcBuf = (uint32_t*)(src->pixels) + (row * src_ipr);
				 dstBuf = (uint32_t*)(dst->pixels) + (dst->w - row - 1);
				 for (col = 0; col < src->w; ++col) {
					 *dstBuf = *srcBuf;
					 ++srcBuf;
					 dstBuf += dst_ipr;
				 }
				 /* end for(col) */
			 }
			 /* end for(row) */
		 }
		 break;

		case 2: /* rotated 180 degrees clockwise */
		 {
			 for (row = 0; row < src->h; ++row) {
				 srcBuf = (uint32_t*)(src->pixels) + (row * src_ipr);
				 dstBuf = (uint32_t*)(dst->pixels) + ((dst->h - row - 1) * dst_ipr) + (dst->w - 1);
				 for (col = 0; col < src->w; ++col) {
					 *dstBuf = *srcBuf;
					 ++srcBuf;
					 --dstBuf;
				 }
			 }
		 }
		 break;

		case 3:
		 {
			 for (row = 0; row < src->h; ++row) {
				 srcBuf = (uint32_t*)(src->pixels) + (row * src_ipr);
				 dstBuf = (uint32_t*)(dst->pixels) + row + ((dst->h - 1) * dst_ipr);
				 for (col = 0; col < src->w; ++col) {
					 *dstBuf = *srcBuf;
					 ++srcBuf;
					 dstBuf -= dst_ipr;
				 }
			 }
		 }
		 break;
	}
	/* end switch */

	return dst;
}


/*!
\brief Internal target surface sizing function for rotozooms with trig result return.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the destination surface.
\param dstheight The calculated height of the destination surface.
\param canglezoom The sine of the angle adjusted by the zoom factor.
\param sanglezoom The cosine of the angle adjusted by the zoom factor.

*/
void _rotozoomSurfaceSizeTrig(int width, int height, double angle, double zoomx, double zoomy,
							  int *dstwidth, int *dstheight,
							  double *canglezoom, double *sanglezoom)
{
	double x, y, cx, cy, sx, sy;
	double radangle;
	int dstwidthhalf, dstheighthalf;

	/*
	* Determine destination width and height by rotating a centered source box
	*/
	radangle = angle * (M_PI / 180.0);
	*sanglezoom = sin(radangle);
	*canglezoom = cos(radangle);
	*sanglezoom *= zoomx;
	*canglezoom *= zoomx;
	x = (double)(width / 2);
	y = (double)(height / 2);
	cx = *canglezoom * x;
	cy = *canglezoom * y;
	sx = *sanglezoom * x;
	sy = *sanglezoom * y;

	dstwidthhalf = MAX((int)
		ceil(MAX(MAX(MAX(fabs(cx + sy), fabs(cx - sy)), fabs(-cx + sy)), fabs(-cx - sy))), 1);
	dstheighthalf = MAX((int)
		ceil(MAX(MAX(MAX(fabs(sx + cy), fabs(sx - cy)), fabs(-sx + cy)), fabs(-sx - cy))), 1);
	*dstwidth = 2 * dstwidthhalf;
	*dstheight = 2 * dstheighthalf;
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurfaceXY() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSizeXY(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight)
{
	double dummy_sanglezoom, dummy_canglezoom;

	_rotozoomSurfaceSizeTrig(width, height, angle, zoomx, zoomy, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurface() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight)
{
	double dummy_sanglezoom, dummy_canglezoom;

	_rotozoomSurfaceSizeTrig(width, height, angle, zoom, zoom, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Rotates and zooms a surface and optional anti-aliasing.

Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees and 'zoom' a scaling factor. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
rz_surface_t *rotozoomSurface(rz_surface_t * src, double angle, double zoom, int smooth)
{
	return rotozoomSurfaceXY(src, angle, zoom, zoom, smooth);
}

/*!
\brief Rotates and zooms a surface with different horizontal and vertival scaling factors and optional anti-aliasing.

Rotates and zooms a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees, 'zoomx and 'zoomy' scaling factors. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
rz_surface_t *rotozoomSurfaceXY(rz_surface_t * src, double angle, double zoomx, double zoomy, int smooth)
{
	rz_surface_t *rz_src;
	rz_surface_t *rz_dst;
	double zoominv;
	double sanglezoom, canglezoom, sanglezoominv, canglezoominv;
	int dstwidthhalf, dstwidth, dstheighthalf, dstheight;
	int i;
	int flipx,flipy;
	uint8_t r,g,b,a;
	uint32_t colorkey = 0;
	int colorKeyAvailable = 0;

	/*
	* Sanity check
	*/
	if (src == NULL)
		return (NULL);

	if (src->colorkey != (pixel_t)-1)
	{
		colorkey = _colorkey(src);
		vs23.rgbaFromColor(colorkey, r, g, b, a);
		colorKeyAvailable = 1;
	}
	/*
	* Determine if source surface is 32bit or 8bit
	*/

	/*
	* Use source surface 'as is'
	*/
	rz_src = src;

	/*
	* Sanity check zoom factor
	*/
	flipx = (zoomx<0.0);
	if (flipx) zoomx=-zoomx;
	flipy = (zoomy<0.0);
	if (flipy) zoomy=-zoomy;
	if (zoomx < VALUE_LIMIT) zoomx = VALUE_LIMIT;
	if (zoomy < VALUE_LIMIT) zoomy = VALUE_LIMIT;
	zoominv = 65536.0 / (zoomx * zoomx);

	/*
	* Check if we have a rotozoom or just a zoom
	*/
	if (fabs(angle) > VALUE_LIMIT) {

		/*
		* Angle!=0: full rotozoom
		*/
		/*
		* -----------------------
		*/

		/* Determine target size */
		_rotozoomSurfaceSizeTrig(rz_src->w, rz_src->h, angle, zoomx, zoomy, &dstwidth, &dstheight, &canglezoom, &sanglezoom);

		/*
		* Calculate target factors from sin/cos and zoom
		*/
		sanglezoominv = sanglezoom;
		canglezoominv = canglezoom;
		sanglezoominv *= zoominv;
		canglezoominv *= zoominv;

		/* Calculate half size */
		dstwidthhalf = dstwidth / 2;
		dstheighthalf = dstheight / 2;

		/*
		* Alloc space to completely contain the rotated surface
		* Target surface is 32bit with source RGBA/ABGR ordering
		*/
		rz_dst = new rz_surface_t(dstwidth, dstheight + GUARD_ROWS);

		/* Check target */
		if (rz_dst == NULL)
			return NULL;

		/* Adjust for guard rows */
		rz_dst->h = dstheight;

		if (colorKeyAvailable == 1){
			colorkey = vs23.colorFromRgb(r, g, b);
			rz_dst->fill(colorkey);
		}

		/*
		* Check which kind of surface we have
		*/

		/*
		* Call the 32bit transformation routine to do the rotation (using alpha)
		*/
		_transformSurfaceRGBA(rz_src, rz_dst, dstwidthhalf, dstheighthalf,
			(int) (sanglezoominv), (int) (canglezoominv),
			flipx, flipy,
			smooth);
		/*
		* Turn on source-alpha support
		*/
		// XXX	SDL_SetAlpha(rz_dst, SDL_SRCALPHA, 255);
		// XXX	SDL_SetColorKey(rz_dst, SDL_SRCCOLORKEY | SDL_RLEACCEL, _colorkey(rz_src));

	} else {

		/*
		* Angle=0: Just a zoom
		*/
		/*
		* --------------------
		*/

		/*
		* Calculate target size
		*/
		zoomSurfaceSize(rz_src->w, rz_src->h, zoomx, zoomy, &dstwidth, &dstheight);

		/*
		* Alloc space to completely contain the zoomed surface
		* Target surface is 32bit with source RGBA/ABGR ordering
		*/
		rz_dst = new rz_surface_t(dstwidth, dstheight);

		/* Check target */
		if (rz_dst == NULL)
			return NULL;

		/* Adjust for guard rows */
		rz_dst->h = dstheight;

		if (colorKeyAvailable == 1){
			colorkey = vs23.colorFromRgb(r, g, b);
			rz_dst->fill(colorkey);
		}

		/*
		* Call the 32bit transformation routine to do the zooming (using alpha)
		*/
		_zoomSurfaceRGBA(rz_src, rz_dst, flipx, flipy, smooth);

		/*
		* Turn on source-alpha support
		*/
		// XXX SDL_SetAlpha(rz_dst, SDL_SRCALPHA, 255);
		// XXX SDL_SetColorKey(rz_dst, SDL_SRCCOLORKEY | SDL_RLEACCEL, _colorkey(rz_src));
	}

	/*
	* Return destination surface
	*/
	return (rz_dst);
}

/*!
\brief Calculates the size of the target surface for a zoomSurface() call.

The minimum size of the target surface is 1. The input factors can be positive or negative.

\param width The width of the source surface to zoom.
\param height The height of the source surface to zoom.
\param zoomx The horizontal zoom factor.
\param zoomy The vertical zoom factor.
\param dstwidth Pointer to an integer to store the calculated width of the zoomed target surface.
\param dstheight Pointer to an integer to store the calculated height of the zoomed target surface.
*/
void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight)
{
	/*
	* Make zoom factors positive
	*/
	int flipx, flipy;
	flipx = (zoomx<0.0);
	if (flipx) zoomx = -zoomx;
	flipy = (zoomy<0.0);
	if (flipy) zoomy = -zoomy;

	/*
	* Sanity check zoom factors
	*/
	if (zoomx < VALUE_LIMIT) {
		zoomx = VALUE_LIMIT;
	}
	if (zoomy < VALUE_LIMIT) {
		zoomy = VALUE_LIMIT;
	}

	/*
	* Calculate target size
	*/
	*dstwidth = (int) ((double) width * zoomx);
	*dstheight = (int) ((double) height * zoomy);
	if (*dstwidth < 1) {
		*dstwidth = 1;
	}
	if (*dstheight < 1) {
		*dstheight = 1;
	}
}

/*!
\brief Zoom a surface by independent horizontal and vertical factors with optional smoothing.

Zooms a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is on
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.
If zoom factors are negative, the image is flipped on the axes.

\param src The surface to zoom.
\param zoomx The horizontal zoom factor.
\param zoomy The vertical zoom factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new, zoomed surface.
*/
rz_surface_t *zoomSurface(rz_surface_t * src, double zoomx, double zoomy, int smooth)
{
	rz_surface_t *rz_src;
	rz_surface_t *rz_dst;
	int dstwidth, dstheight;
	int i;
	int flipx, flipy;

	/*
	* Sanity check
	*/
	if (src == NULL)
		return (NULL);

	/*
	* Use source surface 'as is'
	*/
	rz_src = src;

	flipx = (zoomx<0.0);
	if (flipx) zoomx = -zoomx;
	flipy = (zoomy<0.0);
	if (flipy) zoomy = -zoomy;

	/* Get size if target */
	zoomSurfaceSize(rz_src->w, rz_src->h, zoomx, zoomy, &dstwidth, &dstheight);

	/*
	* Alloc space to completely contain the zoomed surface
	* Target surface is 32bit with source RGBA/ABGR ordering
	*/
	rz_dst = new rz_surface_t(dstwidth, dstheight + GUARD_ROWS);

	/* Check target */
	if (rz_dst == NULL)
		return NULL;

	/* Adjust for guard rows */
	rz_dst->h = dstheight;

	/*
	* Call the 32bit transformation routine to do the zooming (using alpha)
	*/
	_zoomSurfaceRGBA(rz_src, rz_dst, flipx, flipy, smooth);
	/*
	* Turn on source-alpha support
	*/
	// XXX SDL_SetAlpha(rz_dst, SDL_SRCALPHA, 255);

	/*
	* Return destination surface
	*/
	return (rz_dst);
}

/*!
\brief Shrink a surface by an integer ratio using averaging.

Shrinks a 32bit or 8bit 'src' surface to a newly created 'dst' surface.
'factorx' and 'factory' are the shrinking ratios (i.e. 2=1/2 the size,
3=1/3 the size, etc.) The destination surface is antialiased by averaging
the source box RGBA or Y information. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.
The input surface is not modified. The output surface is newly allocated.

\param src The surface to shrink.
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return The new, shrunken surface.
*/
/*@null@*/
rz_surface_t *shrinkSurface(rz_surface_t *src, int factorx, int factory)
{
	int result;
	rz_surface_t *rz_src;
	rz_surface_t *rz_dst = NULL;
	int dstwidth, dstheight;
	int i;
	int haveError = 0;

	/*
	* Sanity check
	*/
	if (src == NULL) {
		return (NULL);
	}

	/*
	* Use source surface 'as is'
	*/
	rz_src = src;

	/* Get size for target */
	dstwidth=rz_src->w/factorx;
	while (dstwidth*factorx>rz_src->w) { dstwidth--; }
	dstheight=rz_src->h/factory;
	while (dstheight*factory>rz_src->h) { dstheight--; }

	/*
	* Alloc space to completely contain the shrunken surface
	* (with added guard rows)
	*/

	/*
	* Target surface is 32bit with source RGBA/ABGR ordering
	*/
	rz_dst = new rz_surface_t(dstwidth, dstheight + GUARD_ROWS);

	/* Check target */
	if (rz_dst == NULL) {
		haveError = 1;
		goto exitShrinkSurface;
	}

	/* Adjust for guard rows */
	rz_dst->h = dstheight;

	/*
	* Check which kind of surface we have
	*/

	/*
	* Call the 32bit transformation routine to do the shrinking (using alpha)
	*/
	result = _shrinkSurfaceRGBA(rz_src, rz_dst, factorx, factory);
	if ((result!=0) || (rz_dst==NULL)) {
		haveError = 1;
		goto exitShrinkSurface;
	}

	/*
	* Turn on source-alpha support
	*/
	result = 0; // XXX SDL_SetAlpha(rz_dst, SDL_SRCALPHA, 255);
	if (result!=0) {
		haveError = 1;
		goto exitShrinkSurface;
	}

exitShrinkSurface:
	/* Check error state; maybe need to cleanup destination */
	if (haveError==1) {
		if (rz_dst!=NULL) {
			delete rz_dst;
		}
		rz_dst=NULL;
	}

	/*
	* Return destination surface
	*/
	return (rz_dst);
}
