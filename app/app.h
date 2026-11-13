/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1 app: the app's half of the contract with fm1-base (device/unity.c, host/plat_host.c). */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "hadal.h"
#include "vis.h"

#define APP_NAME "HADAL-1"
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_CANVAS_PX (240u * 24u)       /* the picture goes out in strips */
#define APP_SPLASH_RGB RGB(84, 232, 196)
#ifdef APP_VERSION
#define EN_VERSION APP_VERSION
#else
#define EN_VERSION "DEV"
#endif

enum { OBJ_PROJ, OBJ_PANEL, OBJ_NOBJ };
#define FM1B_UAC_DEEP 1                  /* the render can take over 40 % of its time: USB needs the deeper ring, or it crackles */
#define APP_AFTER_TAP
void app_after_tap(int32_t *lr, uint32_t n);
#define APP_STORE_MAP {0xFC000u, 0xDC000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t gain_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
