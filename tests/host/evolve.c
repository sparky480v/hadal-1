/* SPDX-License-Identifier: MPL-2.0
 * Hands off: one sea with nobody touching it. Writes the sound, mono at 5512 Hz in int16, to stdout and the engine's
 * state every ten seconds to a log: evolve SEA SEED SECONDS DRIFT LOG [DEPTH]   SEA -1: let the engine choose */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hadal.h"
static uint32_t tk = 1;
uint32_t plat_ticks24(void) { return tk += 999u; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int main(int c, char **v)
{
    static int32_t lr[512];
    int sea = atoi(v[1]), seed = atoi(v[2]), drift = atoi(v[4]), i;
    long b, n = (long)(atof(v[3]) * 44100.0 / 256.0), next = 0;
    FILE *f = fopen(v[5], "w");
    float acc = 0.0f; int na = 0;
    tk = (uint32_t)seed * 7919u + 3u;
    en_entropy(((uint32_t)seed * 2654435761u) ^ ((uint32_t)seed << 17) ^ 2166136261u);
    en_init();
    en_seed(tk);
    if (sea >= 0)
        en_sea(sea);
    en_sea_drift(drift);
    en_tide(.65f);
    en_output(1);
    if (getenv("HADAL_USB"))
        en_usb_split(1);
    if (getenv("HADAL_PHONE"))
        en_usb_tone(1);
    if (c > 6)
        en_macro(M_DEPTH, (float)atof(v[6]));
    {   /* HADAL_SHAPE=i:v,i:v   the SHAPE controls */
        const char *q = getenv("HADAL_SHAPE");
        while (q && *q) {
            int a_ = atoi(q), b_;
            q = strchr(q, ':') + 1;
            b_ = atoi(q);
            en_shape(a_, b_);
            q = strchr(q, ',');
            q = q ? q + 1 : 0;
        }
    }
    for (b = 0; b < n; b++) {
        en_ev_t e;
        if (getenv("HADAL_SWELL")) {
            if (b == 10336) en_swell(1);   /* one minute in, for 15 s */
            if (b == 12920) en_swell(0);
        }
        en_render(lr, 256, 4096);
        while (en_ev_get(&e)) {}
        for (i = 0; i < 256; i++) {
            acc += (float)(lr[2 * i] + lr[2 * i + 1]) * .5f;
            if (++na == 8) {
                short o = (short)(acc / 8.0f / 8300000.0f * 32767.0f);
                fwrite(&o, 2, 1, stdout);
                acc = 0.0f; na = 0;
            }
        }
        if (b >= next) {
            next = b + 1723;
            fprintf(f, "%ld %d %d %d %d %d %d %d %.3f %.3f %.3f %.3f %.3f %.3f %.2f %.3f %.3f %.3f %.3f %u %u %u\n", b * 256 / 44100,
                en_st.sea, en_st.stratum, en_st.family, en_st.bell, en_st.drone, en_st.choir, en_st.era,
                en_st.field[0], en_st.field[1], en_st.field[2], en_st.field[3], en_st.field[4], en_st.field[5], en_st.root,
                en_st.m[0], en_st.m[1], en_st.m[2], en_st.m[3], en_st.bells, en_st.born, en_st.strata);
        }
    }
    fclose(f);
    return 0;
}
