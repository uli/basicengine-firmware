// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

int eb_inkey(void);
int eb_pad_state(int num);
int eb_key_state(int scancode);

// Button Hex Representations:
#define EB_JOY_LEFT_SHIFT	0
#define EB_JOY_DOWN_SHIFT	1
#define EB_JOY_RIGHT_SHIFT	2
#define EB_JOY_UP_SHIFT	3

#define EB_JOY_FIRSTBUTTON_SHIFT	EB_JOY_START_SHIFT
#define EB_JOY_START_SHIFT	4
#define EB_JOY_SELECT_SHIFT	5

#define EB_JOY_LEFT		(1 << EB_JOY_LEFT_SHIFT)
#define EB_JOY_DOWN		(1 << EB_JOY_DOWN_SHIFT)
#define EB_JOY_RIGHT		(1 << EB_JOY_RIGHT_SHIFT)
#define EB_JOY_UP		(1 << EB_JOY_UP_SHIFT)
#define EB_JOY_START		(1 << EB_JOY_START_SHIFT)
#define EB_JOY_SELECT		(1 << EB_JOY_SELECT_SHIFT)

#define EB_JOY_SQUARE_SHIFT	8
#define EB_JOY_X_SHIFT	9
#define EB_JOY_O_SHIFT	10
#define EB_JOY_TRIANGLE_SHIFT	11
#define EB_JOY_R1_SHIFT	12
#define EB_JOY_L1_SHIFT	13
#define EB_JOY_R2_SHIFT	14
#define EB_JOY_L2_SHIFT	15
#define EB_JOY_R3_SHIFT	16
#define EB_JOY_L3_SHIFT	17

#define EB_JOY_SQUARE		(1 << EB_JOY_SQUARE_SHIFT)
#define EB_JOY_X		(1 << EB_JOY_X_SHIFT)
#define EB_JOY_O		(1 << EB_JOY_O_SHIFT)
#define EB_JOY_TRIANGLE		(1 << EB_JOY_TRIANGLE_SHIFT)
#define EB_JOY_R1		(1 << EB_JOY_R1_SHIFT)
#define EB_JOY_L1		(1 << EB_JOY_L1_SHIFT)
#define EB_JOY_R2		(1 << EB_JOY_R2_SHIFT)
#define EB_JOY_L2		(1 << EB_JOY_L2_SHIFT)
#define EB_JOY_R3		(1 << EB_JOY_R3_SHIFT)
#define EB_JOY_L3		(1 << EB_JOY_L3_SHIFT)
#ifdef __cplusplus
}
#endif
