// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "eb_img.h"

extern sdfiles bfs;

struct eb_image_spec eb_image_spec_default = {
  .dst_x = -1,
  .dst_y = -1,
  .src_x = 0,
  .src_y = 0,
  .w = -1,
  .h = -1,
  .scale_x = 1.0,
  .scale_y = 1.0,
  .key = 0,
};

int eb_load_image(const char *filename, struct eb_image_spec *loc) {
  if (!loc)
    return -1;

  int32_t dx = loc->dst_x;
  int32_t dy = loc->dst_y;
  int32_t w = loc->w;
  int32_t h = loc->h;

  int err = bfs.loadBitmap((char *)filename, dx, dy, loc->src_x, loc->src_y, w,
                           h, loc->scale_x, loc->scale_y, loc->key);

  if (err == 0) {
    loc->dst_x = dx;
    loc->dst_y = dy;
    loc->w = w;
    loc->h = h;
  }

  return err;
}

int eb_load_image_to(const char *filename, int dx, int dy, unsigned int key) {
  struct eb_image_spec loc = EB_IMAGE_SPEC_DEFAULT;
  loc.key = key;
  loc.dst_x = dx;
  loc.dst_y = dy;

  return eb_load_image(filename, &loc);
}
