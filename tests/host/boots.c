/* SPDX-License-Identifier: MPL-2.0
 * Power-ons with the timer reading exactly the same every time, as a chip that boots identically would: only the count
 * kept in flash differs. boots START N */
#include <stdio.h>
#include <stdlib.h>
#include "hadal.h"
uint32_t plat_ticks24(void) { return 123456u; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn; (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int main(int c, char **v)
{
    static int32_t lr[512];
    uint32_t count = (uint32_t)atoi(v[1]);
    int useflash = c > 2 ? atoi(v[2]) : 1, i;
    en_entropy(useflash ? (count * 2654435761u) ^ 2166136261u ^ (count << 17) : 0u);
    en_init();
    for (i = 0; i < 20; i++) {
        en_ev_t e;
        en_render(lr, 256, 4096);
        while (en_ev_get(&e)) {}
    }
    printf("%u %u %u %u %u %u %.2f %.3f\n", en_st.sea, en_st.stratum, en_st.family, en_st.bell, en_st.drone, en_st.choir, en_st.root, en_st.field[2]);
    return 0;
}
