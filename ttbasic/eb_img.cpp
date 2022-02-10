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

EBAPI int eb_load_image(const char *filename, struct eb_image_spec *loc) {
  if (!loc)
    return -1;

  int err = bfs.loadBitmap((char *)filename, loc->dst_x, loc->dst_y, loc->src_x,
                           loc->src_y, loc->w, loc->h, loc->scale_x,
                           loc->scale_y, loc->key);

  return err;
}

EBAPI int eb_load_image_to(const char *filename, int dx, int dy, unsigned int key) {
  struct eb_image_spec loc = EB_IMAGE_SPEC_DEFAULT;
  loc.key = key;
  loc.dst_x = dx;
  loc.dst_y = dy;

  return eb_load_image(filename, &loc);
}
