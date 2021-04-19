// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

struct eb_image_spec {
  int dst_x, dst_y;
  int src_x, src_y;
  int w, h;
  unsigned int key;
};

extern struct eb_image_spec eb_image_spec_default;
#define EB_IMAGE_SPEC_DEFAULT eb_image_spec_default;

int eb_load_image(const char *filename, struct eb_image_spec *loc);
int eb_load_image_to(const char *filename, int dx, int dy, unsigned int key);

#ifdef __cplusplus
}
#endif
