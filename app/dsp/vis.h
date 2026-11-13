/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1: the picture. An 8-bit 120x120 feedback field, one warp and one set of marks for each sea. */
#pragma once
#include <stdint.h>
#define VIS_NMODE 40
extern const char *const VIS_MODE[VIS_NMODE];
extern const char *const VIS_PAL[15];
void vis_init(void);
void vis_set_mode(int m);
void vis_bright(int b);   /* 0 normal, 1 bright, 2 brightest: the picture, not the backlight */             /* -1: the sea's own */
int vis_mode(void);
void vis_title(int on);               /* the power-on sonar: rings rising out of the dark */
void vis_step(float dt);              /* one frame of the field, from en_st */
/* h rows of the panel from row y0 into px (240 wide, byte-swapped RGB565); rows in dim0..dim1 at a quarter */
void vis_scan(uint16_t *px, int y0, int h, int dim0, int dim1);
void app_midi_note(int kind, int note, int vel);   /* the app's: a note the world played (vis_step hands them on) */
