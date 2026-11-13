/* SPDX-License-Identifier: MPL-2.0
 * One sound alone, without the level control: levels KIND(b d c s) INDEX SEA  prints loudest second in dB, peak, click count */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "hadal.h"
static uint32_t tk = 77;
uint32_t plat_ticks24(void) { return tk += 999u; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int main(int c, char **v)
{
    static int32_t lr[512];
    char kind = v[1][0];
    int idx = atoi(v[2]), sea = atoi(v[3]), i, b, L, n = 44100 * 14 / 256;
    double win = 0, best = 0, peak = 0, last = 0; int wn = 0, clicks = 0;
    en_entropy(12345u + (uint32_t)idx);
    en_init();
    en_sea(sea);
    en_sea_drift(0);
    en_tide(0);
    en_output(0);
    en_shape(S_BELLS, 0);
    en_shape(S_FX, 0);
    for (L = 0; L < N_LAYER; L++)
        en_layer(L, 0);
    if (kind != 'k')
        en_layer(kind == 'b' ? L_BELL : kind == 'd' ? L_DRONE : kind == 'c' ? L_CHOIR : L_FAUNA, 1);
    if (kind == 'k')
        en_keys(2, idx, 0, 70, 50);
    if (kind == 'b') en_pick(0, idx);
    if (kind == 'd') en_pick(1, idx);
    if (kind == 'c') en_pick(2, idx);
    for (b = 0; b < 400; b++) { en_ev_t e; en_render(lr, 256, 4096); while (en_ev_get(&e)) {} }
    if (kind == 'b') en_offering(0, 0);
    if (kind == 's') en_fire(idx);
    if (kind == 'k') { en_key(0, 0, 0, 1); en_key(1, 7, 0, 1); }
    for (b = 0; b < n; b++) {
        en_ev_t e;
        en_render(lr, 256, 4096);
        while (en_ev_get(&e)) {}
        for (i = 0; i < 256; i++) {
            double x = (double)(lr[2 * i] + lr[2 * i + 1]) * .5 / 8300000.0;
            win += x * x; wn++;
            if (fabs(x) > peak) peak = fabs(x);
            if (fabs(x - last) > .12) clicks++;
            last = x;
            if (wn == 44100) { if (win > best) best = win; win = 0; wn = 0; }
        }
    }
    printf("%.1f %.1f %d\n", 10 * log10(best / 44100 + 1e-12), 20 * log10(peak + 1e-9), clicks);
    return 0;
}
