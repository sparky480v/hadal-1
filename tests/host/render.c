/* SPDX-License-Identifier: MPL-2.0
 * One sea at full speed into a raw stereo file, for the sound checks: render SEA SPEAKER SECONDS FILE [SEED] */
#include <stdio.h>
#include <stdlib.h>
#include "hadal.h"
static uint32_t tk = 1;
uint32_t plat_ticks24(void) { return tk += 999u; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int main(int c, char **v)
{
    static int32_t lr[512];
    static float o[512];
    int sea = atoi(v[1]), spk = atoi(v[2]), sec = atoi(v[3]), i;
    long b, warm = (long)(25.0 * 44100.0 / 256.0), n = (long)((double)sec * 44100.0 / 256.0);
    FILE *f = fopen(v[4], "wb");
    tk = c > 5 ? (uint32_t)atoi(v[5]) * 7919u + 3u : 3u;
    en_init();
    en_seed(tk);
    en_sea(sea);
    en_sea_drift(0);
    en_output(spk);
    for (b = 0; b < warm + n; b++) {
        en_ev_t e;
        en_render(lr, 256, 4096);
        while (en_ev_get(&e)) {}
        if (b >= warm) {
            for (i = 0; i < 512; i++)
                o[i] = (float)lr[i] / 8300000.0f;
            fwrite(o, 4, 512, f);
        }
    }
    fclose(f);
    return 0;
}
