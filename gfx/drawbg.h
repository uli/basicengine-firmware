void GFXCLASS::drawBg(bg_t *bg) {

  int tile_size_x = bg->tile_size_x;
  int tile_size_y = bg->tile_size_y;
  int ypoff = bg->scroll_y % tile_size_y;

  // start/end coordinates of the visible BG window, relative to the
  // BG's origin, in pixels
  int start_x = bg->scroll_x;
  int start_y = bg->scroll_y;

  // offset to add to BG-relative coordinates to get screen coordinates
  int offset_window_x = -start_x + bg->win_x;
  int offset_window_y = -start_y + bg->win_y;

  int tile_start_x, tile_start_y;
  int tile_end_x, tile_end_y;

  tile_start_y = bg->scroll_y / tile_size_y;
  tile_end_y = tile_start_y + (bg->win_h + ypoff) / tile_size_y + 1;
  tile_start_x = bg->scroll_x / bg->tile_size_x;
  tile_end_x = tile_start_x + (bg->win_w + tile_size_x - 1) / tile_size_x + 1;

  for (int y = tile_start_y; y < tile_end_y; ++y) {
    for (int x = tile_start_x; x < tile_end_x; ++x) {
      uint8_t tile = bg->tiles[x % bg->w + (y % bg->h) * bg->w];

      int tile_x = bg->pat_x + (tile % bg->pat_w) * tile_size_x;
      int tile_y = bg->pat_y + (tile / bg->pat_w) * tile_size_y;

      int dst_x = x * tile_size_x + offset_window_x;
      int dst_y = y * tile_size_y + offset_window_y;
      int blit_width = tile_size_x;
      int blit_height = tile_size_y;

      // clip width, height and adjust source and destination if the tile
      // crosses the BG window limits
      if (dst_y < bg->win_y) {
        tile_y -= dst_y - bg->win_y;
        blit_height += dst_y - bg->win_y;
        dst_y = bg->win_y;
      } else if (dst_y + blit_height >= bg->win_y + bg->win_h)
        blit_height = bg->win_y + bg->win_h - dst_y;

      if (dst_x < bg->win_x) {
        tile_x -= dst_x - bg->win_x;
        blit_width += dst_x - bg->win_x;
        dst_x = bg->win_x;
      } else if (dst_x + blit_width >= bg->win_x + bg->win_w)
        blit_width = bg->win_x + bg->win_w - dst_x;

      if (blit_width <= 0 || blit_height <= 0)
        continue;

      overlay_alpha_stride_div255_round_approx(
              (uint8_t *)&pixelComp(dst_x, dst_y),
              (uint8_t *)&pixelText(tile_x, tile_y),
              (uint8_t *)&pixelComp(dst_x, dst_y),
              compositePitch(), blit_height,
              blit_width, offscreenPitch());
    }
  }
}
