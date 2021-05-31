// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

#define COL_BG      0
#define COL_FG      1
#define COL_KEYWORD 2
#define COL_LINENUM 3
#define COL_NUM     4
#define COL_VAR     5
#define COL_LVAR    6
#define COL_OP      7
#define COL_STR     8
#define COL_PROC    9
#define COL_COMMENT 10
#define COL_BORDER  11

#define CONFIG_COLS 12

unsigned int eb_theme_color(int index);

#ifdef __cplusplus
}
#endif
