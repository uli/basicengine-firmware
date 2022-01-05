#define SRCPIXEL(x, y) ( \
	((pixel_t*)src)[ \
		(x) + (y) * src_pitch \
	] )
#define DSTPIXEL(x, y) ( \
	((pixel_t*)dst)[ \
		(x) + (y) * dst_pitch \
	] )

#define PROTO(factor) static void SCALER(factor)(SCALER_SRC_TYPE *src, SCALER_DST_TYPE *dst, int src_w, int src_h, int src_pitch, int dst_pitch)

//static void scale_1x2_ ## BPP (SCALER_SRC_TYPE *src, SCALER_DST_TYPE *dst, int src_w, int src_h, int src_pitch, int dst_pitch)
PROTO(1x2)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x, y*2) = p;
			DSTPIXEL(x, y*2+1) = p;
		}
	}
}

PROTO(1x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x, y*3) = p;
			DSTPIXEL(x, y*3+1) = p;
			DSTPIXEL(x, y*3+2) = p;
		}
	}
}

PROTO(2x1)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*2, y) = p;
			DSTPIXEL(x*2+1, y) = p;
		}
	}
}

PROTO(2x2)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*2, y*2) = p;
			DSTPIXEL(x*2+1, y*2) = p;
			DSTPIXEL(x*2, y*2+1) = p;
			DSTPIXEL(x*2+1, y*2+1) = p;
		}
	}
}

PROTO(2x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*2, y*3) = p;
			DSTPIXEL(x*2+1, y*3) = p;
			DSTPIXEL(x*2, y*3+1) = p;
			DSTPIXEL(x*2+1, y*3+1) = p;
			DSTPIXEL(x*2, y*3+2) = p;
			DSTPIXEL(x*2+1, y*3+2) = p;
		}
	}
}

PROTO(3x2)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*3, y*2) = p;
			DSTPIXEL(x*3+1, y*2) = p;
			DSTPIXEL(x*3+2, y*2) = p;
			DSTPIXEL(x*3, y*2+1) = p;
			DSTPIXEL(x*3+1, y*2+1) = p;
			DSTPIXEL(x*3+2, y*2+1) = p;
		}
	}
}

PROTO(3x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(3x4)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(4x2)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(4x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(4x4)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(4x5)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(5x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(5x4)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(6x3)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(6x4)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(6x5)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(7x4)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*7, y*4) = p;
			DSTPIXEL(x*7+1, y*4) = p;
			DSTPIXEL(x*7+2, y*4) = p;
			DSTPIXEL(x*7+3, y*4) = p;
			DSTPIXEL(x*7+4, y*4) = p;
			DSTPIXEL(x*7+5, y*4) = p;
			DSTPIXEL(x*7+6, y*4) = p;
			DSTPIXEL(x*7, y*4+1) = p;
			DSTPIXEL(x*7+1, y*4+1) = p;
			DSTPIXEL(x*7+2, y*4+1) = p;
			DSTPIXEL(x*7+3, y*4+1) = p;
			DSTPIXEL(x*7+4, y*4+1) = p;
			DSTPIXEL(x*7+5, y*4+1) = p;
			DSTPIXEL(x*7+6, y*4+1) = p;
			DSTPIXEL(x*7, y*4+2) = p;
			DSTPIXEL(x*7+1, y*4+2) = p;
			DSTPIXEL(x*7+2, y*4+2) = p;
			DSTPIXEL(x*7+3, y*4+2) = p;
			DSTPIXEL(x*7+4, y*4+2) = p;
			DSTPIXEL(x*7+5, y*4+2) = p;
			DSTPIXEL(x*7+6, y*4+2) = p;
			DSTPIXEL(x*7, y*4+3) = p;
			DSTPIXEL(x*7+1, y*4+3) = p;
			DSTPIXEL(x*7+2, y*4+3) = p;
			DSTPIXEL(x*7+3, y*4+3) = p;
			DSTPIXEL(x*7+4, y*4+3) = p;
			DSTPIXEL(x*7+5, y*4+3) = p;
			DSTPIXEL(x*7+6, y*4+3) = p;
		}
	}
}

PROTO(7x5)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
			DSTPIXEL(x*7, y*5) = p;
			DSTPIXEL(x*7+1, y*5) = p;
			DSTPIXEL(x*7+2, y*5) = p;
			DSTPIXEL(x*7+3, y*5) = p;
			DSTPIXEL(x*7+4, y*5) = p;
			DSTPIXEL(x*7+5, y*5) = p;
			DSTPIXEL(x*7+6, y*5) = p;
			DSTPIXEL(x*7, y*5+1) = p;
			DSTPIXEL(x*7+1, y*5+1) = p;
			DSTPIXEL(x*7+2, y*5+1) = p;
			DSTPIXEL(x*7+3, y*5+1) = p;
			DSTPIXEL(x*7+4, y*5+1) = p;
			DSTPIXEL(x*7+5, y*5+1) = p;
			DSTPIXEL(x*7+6, y*5+1) = p;
			DSTPIXEL(x*7, y*5+2) = p;
			DSTPIXEL(x*7+1, y*5+2) = p;
			DSTPIXEL(x*7+2, y*5+2) = p;
			DSTPIXEL(x*7+3, y*5+2) = p;
			DSTPIXEL(x*7+4, y*5+2) = p;
			DSTPIXEL(x*7+5, y*5+2) = p;
			DSTPIXEL(x*7+6, y*5+2) = p;
			DSTPIXEL(x*7, y*5+3) = p;
			DSTPIXEL(x*7+1, y*5+3) = p;
			DSTPIXEL(x*7+2, y*5+3) = p;
			DSTPIXEL(x*7+3, y*5+3) = p;
			DSTPIXEL(x*7+4, y*5+3) = p;
			DSTPIXEL(x*7+5, y*5+3) = p;
			DSTPIXEL(x*7+6, y*5+3) = p;
			DSTPIXEL(x*7, y*5+4) = p;
			DSTPIXEL(x*7+1, y*5+4) = p;
			DSTPIXEL(x*7+2, y*5+4) = p;
			DSTPIXEL(x*7+3, y*5+4) = p;
			DSTPIXEL(x*7+4, y*5+4) = p;
			DSTPIXEL(x*7+5, y*5+4) = p;
			DSTPIXEL(x*7+6, y*5+4) = p;
		}
	}
}

PROTO(12x5)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w; ++x) {
			SCALER_DST_TYPE p = CONV(SRCPIXEL(x, y));
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

PROTO(p33xp33)
{
	for (int y = 0; y < src_h/3; ++y) {
		for (int x = 0; x < src_w/3; ++x) {
			SCALER_DST_TYPE p = SRCPIXEL(x*3, y*3);
			DSTPIXEL(x, y) = p;
		}
	}
}

PROTO(p33xp5)
{
	for (int y = 0; y < src_h/2; ++y) {
		for (int x = 0; x < src_w/3; ++x) {
			SCALER_DST_TYPE p = SRCPIXEL(x*3, y*2);
			DSTPIXEL(x, y) = p;
		}
	}
}

PROTO(p5xp33)
{
	for (int y = 0; y < src_h/3; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			SCALER_DST_TYPE p = SRCPIXEL(x*2, y*3);
			DSTPIXEL(x, y) = p;
		}
	}
}

PROTO(p5xp5)
{
	for (int y = 0; y < src_h/2; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			SCALER_DST_TYPE p = SRCPIXEL(x*2, y*2);
			DSTPIXEL(x, y) = p;
		}
	}
}

PROTO(p5x1)
{
	for (int y = 0; y < src_h; ++y) {
		for (int x = 0; x < src_w/2; ++x) {
			SCALER_DST_TYPE p = SRCPIXEL(x*2, y);
			DSTPIXEL(x, y) = p;
		}
	}
}

typedef void (*SCALER_TYPE)(SCALER_SRC_TYPE *, SCALER_DST_TYPE *, int, int, int, int);


// Combinations that do not occur with the current set of modes are not
// implemented, but are mapped to a fallback scaler, just in case...

// clang-format off
static SCALER_TYPE SCALERS_UP[12][6] = {
	{ NULL,		SCALER(1x2),	SCALER(1x3),	SCALER(1x3),	SCALER(1x3),	SCALER(1x3)	},	// 1x
	{ SCALER(2x1),	SCALER(2x2),	SCALER(2x3),	SCALER(2x3),	SCALER(2x3),	SCALER(2x3)	},	// 2x
	{ SCALER(2x1),	SCALER(3x2),	SCALER(3x3),	SCALER(3x4),	SCALER(3x4),	SCALER(3x4)	},	// 3x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(4x3),	SCALER(4x4),	SCALER(4x5),	SCALER(4x5)	},	// 4x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(5x3),	SCALER(5x4),	SCALER(5x4),	SCALER(5x4)	},	// 5x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(6x4),	SCALER(6x5),	SCALER(6x5)	},	// 6x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(7x5),	SCALER(7x5)	},	// 7x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(7x5),	SCALER(7x5)	},	// 8x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(7x5),	SCALER(7x5)	},	// 9x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(7x5),	SCALER(7x5)	},	// 10x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(7x5),	SCALER(7x5)	},	// 11x
	{ SCALER(2x1),	SCALER(4x2),	SCALER(6x3),	SCALER(7x4),	SCALER(12x5),	SCALER(12x5)	},	// 12x
};

static SCALER_TYPE SCALERS_DOWN[3][3] = {
	{ NULL,		SCALER(p5xp5),	SCALER(p5xp33)	}, // 1x
	{ SCALER(p5x1),	SCALER(p5xp5),	SCALER(p5xp33)	}, // .55x
	{ SCALER(p33xp5),	SCALER(p33xp5),	SCALER(p33xp33)	}, // .33x
};
// clang-format on

static void SCALE_FUNC(SDL_Surface *src, SDL_Surface *dst, int src_h)
{
	int upfactor_x = dst->w / src->w;
	int upfactor_y = dst->h / src_h;

	int downfactor_x = (src->w + dst->w - 1) / dst->w;
	int downfactor_y = (src_h + dst->h - 1) / dst->h;

	int src_pitch = src->pitch / sizeof(SCALER_SRC_TYPE);
	int dst_pitch = dst->pitch / sizeof(SCALER_DST_TYPE);

	SCALER_SRC_TYPE *sp = (SCALER_SRC_TYPE *)src->pixels;
	SCALER_DST_TYPE *dp = (SCALER_DST_TYPE *)dst->pixels;

	int off_x, off_y;
	if (downfactor_x > 1)
		off_x = (dst->w - src->w / downfactor_x) / 2;
	else
		off_x = (dst->w - src->w * upfactor_x) / 2;
	if (downfactor_y > 1)
		off_y = (dst->h - src_h / downfactor_y) / 2;
	else
		off_y = (dst->h - src_h * upfactor_y) / 2;

	dp += off_x + off_y * dst_pitch;

	if (downfactor_x > 0 && downfactor_y > 0 && SCALERS_DOWN[downfactor_x - 1][downfactor_y - 1])
		SCALERS_DOWN[downfactor_x - 1][downfactor_y - 1](sp, dp, src->w, src_h, src_pitch, dst_pitch);
	else if (upfactor_x > 0 && upfactor_y > 0 && SCALERS_UP[upfactor_x - 1][upfactor_y - 1])
		SCALERS_UP[upfactor_x - 1][upfactor_y - 1](sp, dp, src->w, src_h, src_pitch, dst_pitch);
	else {
		// 1x1
		// SDL_Blit() cannot be used in the gfx thread
		memcpy(dst->pixels, src->pixels, src->pitch * src_h);
	}
}

#undef SRCPIXEL
#undef DSTPIXEL
