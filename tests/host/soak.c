/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1's soak: hours of the real engine at full speed, every sound fired, every sea visited, every control used.
 *   cc -O2 -ffp-contract=off -DPLAT_HOST ... tests/host/soak.c app/dsp/hadal.c -lm -o soak; ./soak HOURS SEED */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "hadal.h"
uint32_t plat_ticks24(void) { static uint32_t t = 12345; return t += 24000u; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int main(int c, char **v)
{
    double hours = c > 1 ? atof(v[1]) : 8.0, sec = 0, sum = 0, mins = 0;
    static int32_t lr[512];
    unsigned long blocks = (unsigned long)(hours * 3600.0 * 44100.0 / 256.0), b, n = 0, seen_sfx = 0;
    uint32_t r = c > 2 ? (uint32_t)atoi(v[2]) : 7u;
    double lo = 0, hi = -200; int gotone = 0;
    clock_t t0;
    en_seed(r); en_init(); en_seed(r);
    t0 = clock();
    for (b = 0; b < blocks; b++) {
        int i; en_ev_t e;
        en_render(lr, 256, 4096);
        while (en_ev_get(&e)) {}
        for (i = 0; i < 512; i++) { double x = lr[i] / 8300000.0; sum += x * x; }
        n += 512;
        sec = b * 256.0 / 44100.0;
        if (b % 172 == 0) {
            int s = (int)sec;
            if (s % 397 == 0) { en_fire((int)(seen_sfx % EN_NSFX)); seen_sfx++; }
            if (s % 1800 == 900) en_freeze(1);
            if (s % 1800 == 990) { en_freeze(0); en_offering(s % 12, 0); }
            if (s % 2311 == 0) { en_pick(0, (s / 2311) % (EN_NBELL + 1) - 1); en_pick(1, (s / 2311) % (EN_NDRONE + 1) - 1); en_pick(2, (s / 2311) % (EN_NCHOIR + 1) - 1); }
            if (s % 3607 == 0) en_upheaval();
            if (s % 601 == 0) { en_macro(s % 4, (s % 101) * .01f); en_mutate(); }
            if (s % 1500 == 0) { en_keep(0); }
            if (s % 700 == 0) en_output((s / 700) & 1);
            if (s % 1103 == 0) { en_hold((s / 1103) % EN_NSFX, 1); en_hold((s / 1103 + EN_NSFX - 3) % EN_NSFX, 0); }
            if (s % 2777 == 0) en_evolve((s / 2777) % 3);
            if (s % 3301 == 0) en_favorites((s / 3301) & 1 ? 0x7000u : 0u);
            if (s % 4001 == 0) en_fade(30.0f, (s / 4001) & 1 ? 0.0f : 1.0f);
        }
        if ((b + 1) % (10336) == 0) {
            double db = 10 * log10(sum / n + 1e-12);
            if (db > hi) hi = db;
            if (!gotone || db < lo) lo = db;
            gotone = 1; sum = 0; n = 0; mins++;
        }
    }
    {
        double cpu = (double)(clock() - t0) / CLOCKS_PER_SEC;
        printf("{\"hours\":%.1f,\"speed\":%.0f,\"bad\":%u,\"peak_db\":%.2f,\"rms_lo\":%.1f,\"rms_hi\":%.1f,\"bells\":%u,\"born\":%u,\"strata\":%u,\"era\":%u,\"seas\":%d}\n",
               hours, hours * 3600 / cpu, en_st.bad, 20 * log10(en_st.peak + 1e-9), lo, hi, en_st.bells, en_st.born, en_st.strata, en_st.era, __builtin_popcount(en_st.seas_seen));
    }
    return 0;
}
