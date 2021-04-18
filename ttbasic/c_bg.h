// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

int eb_bg_set_size(int bg, int tiles_w, int tiles_h);
int eb_bg_set_pattern(int bg, int pattern_x, int pattern_y, int pattern_w);
int eb_bg_set_tile_size(int bg, int tile_size_x, int tile_size_y);
int eb_bg_set_window(int bg, int win_x, int win_y, int win_w, int win_h);
int eb_bg_set_priority(int bg, int priority);
int eb_bg_enable(int bg);
int eb_bg_disable(int bg);
void eb_bg_off(void);
int eb_bg_load(int bg, const char *file);
int eb_bg_save(int bg, const char *file);
int eb_bg_move(int bg, int x, int y);

void eb_sprite_off(void);
int eb_sprite_set_pattern(int s, int pat_x, int pat_y);
int eb_sprite_set_size(int s, int w, int h);
int eb_sprite_enable(int s);
int eb_sprite_disable(int s);
int eb_sprite_set_key(int s, ipixel_t key);
int eb_sprite_set_priority(int s, int prio);
int eb_sprite_set_frame(int s, int frame_x, int frame_y, int flip_x, int flip_y);
int eb_sprite_set_opacity(int s, int onoff);
#ifdef USE_ROTOZOOM
int eb_sprite_set_angle(int s, double angle);
int eb_sprite_set_scale_x(int s, double scale_x);
int eb_sprite_set_scale_y(int s, double scale_y);
#endif
#ifdef TRUE_COLOR
int eb_sprite_set_alpha(int s, uint8_t a);
#endif
int eb_sprite_reload(int s);
int eb_sprite_move(int s, int x, int y);

uint8_t *eb_bg_get_tiles(int bg);
int eb_bg_map_tile(int bg, int from, int to);
int eb_bg_set_tiles(int bg, int x, int y, const uint8_t *tiles, int count);
int eb_bg_set_tile(int bg, int x, int y, int t);

int eb_frameskip(int skip);

int eb_sprite_tile_collision(int s, int bg, int tile);
int eb_sprite_collision(int s1, int s2);
int eb_sprite_enabled(int s);
int eb_sprite_x(int s);
int eb_sprite_y(int s);
int eb_sprite_w(int s);
int eb_sprite_h(int s);

int eb_bg_x(int bg);
int eb_bg_y(int bg);
int eb_bg_win_x(int bg);
int eb_bg_win_y(int bg);
int eb_bg_win_width(int bg);
int eb_bg_win_height(int bg);
int eb_bg_enabled(int bg);

int eb_bg_set_win(int bg, int x, int y, int w, int h);

int eb_sprite_frame_x(int s);
int eb_sprite_frame_y(int s);
int eb_sprite_flip_x(int s);
int eb_sprite_flip_y(int s);
int eb_sprite_opaque(int s);

#ifdef __cplusplus
}
#endif
