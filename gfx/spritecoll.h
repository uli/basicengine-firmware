uint8_t GFXCLASS::spriteCollision(uint8_t collidee, uint8_t collider) {
  uint8_t dir = 0x40;  // indicates collision

  const sprite_t *us = &m_sprite[collidee];
  const rz_surface_t *us_surf = m_sprite[collidee].surf;
  const sprite_t *them = &m_sprite[collider];
  const rz_surface_t *them_surf = m_sprite[collider].surf;

  if (!us_surf || !them_surf)
    return 0;

  if (us->pos_x + us_surf->w < them->pos_x)
    return 0;
  if (them->pos_x + them_surf->w < us->pos_x)
    return 0;
  if (us->pos_y + us_surf->h < them->pos_y)
    return 0;
  if (them->pos_y + them_surf->h < us->pos_y)
    return 0;

  // sprite frame as bounding box; we may want something more flexible...
  const sprite_t *left = us, *right = them;
  rz_surface_t *left_surf, *right_surf;
  if (them->pos_x < us->pos_x) {
    dir |= EB_JOY_LEFT;
    left = them;
    right = us;
  } else if (them->pos_x + them_surf->w > us->pos_x + us_surf->w)
    dir |= EB_JOY_RIGHT;
  left_surf = left->surf;
  right_surf = right->surf;

  const sprite_t *upper = us, *lower = them;
  const rz_surface_t *upper_surf, *lower_surf;
  if (them->pos_y < us->pos_y) {
    dir |= EB_JOY_UP;
    upper = them;
    lower = us;
  } else if (them->pos_y + them_surf->h > us->pos_y + us_surf->h)
    dir |= EB_JOY_DOWN;
  upper_surf = upper->surf;
  lower_surf = lower->surf;

  // Check for pixels in overlapping area.
  for (int y = lower->pos_y;
       y < _min(lower->pos_y + lower_surf->h, upper->pos_y + upper_surf->h);
       y++) {
    int leftpy = y - left->pos_y;
    int rightpy = y - right->pos_y;

    for (int x = right->pos_x;
         x < _min(right->pos_x + right_surf->w, left->pos_x + left_surf->w);
         x++) {
      int leftpx = x - left->pos_x;
      int rightpx = x - right->pos_x;
      pixel_t leftpixel = left_surf->getPixel(leftpx, leftpy);
      pixel_t rightpixel = right_surf->getPixel(rightpx, rightpy);

      if (alphaFromColor(leftpixel) >= 0x80 && alphaFromColor(rightpixel) >= 0x80)
        return dir;
    }
  }

  // no overlapping pixels
  return 0;
}
