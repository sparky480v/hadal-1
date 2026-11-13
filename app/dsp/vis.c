/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1: the picture (vis.h). A mesh of 13 x 13 points moves the field in fixed point, each sea draws its own marks
 * into it, the bells and creatures theirs, and a 256-entry palette colours it. -O2. */
#include "vis.h"
#include "hadal.h"
#include "fastmath.h"

#if defined EN_HOST || defined PLAT_HOST
#define POOLD
#else
#define POOLD __attribute__((section(".pool")))
#endif
#define VW 120
#define GRID 12
#define GN (GRID + 1)
static uint8_t fld[2][VW * VW] POOLD;
static uint8_t *A, *Bf;
static int32_t mx[GN * GN], my[GN * GN];
static uint16_t lut[256];
static float cur[18];
static int pal_now = -1, pal_tgt, pal_tick, bright;
static int mode_auto = 1, mode;
static float vt;
static float flash;
static int title_on;
const char *const VIS_MODE[VIS_NMODE] = {"TRENCH", "ROSE WINDOW", "MANDALA", "SQUARE MAW", "AURORA", "INK", "OMBAK",
                                         "SCOPE", "EVENT HORIZON", "SOLAR BARQUE", "BLACK RIVER", "CUNEIFORM RAIN", "HEART TRACE", "SPARKS", "BRANCHES",
                                         "ANGLER", "VAULT", "PRAYER WHEEL", "OBSIDIAN SUN", "ICE SHARDS", "TORII", "GONG", "RADAR",
                                         "STAR RUSH", "EYE OF RA", "REEDS", "GATES", "BRAINWAVES", "MOLTEN", "EYES",
                                         "SPIRAL", "TUNNEL", "CANDLES", "WEB", "BLACK MOON", "COMETS",
                                         "MELT", "BLEED", "MORPH", "VEINS"};
const char *const VIS_PAL[15] = {"ICHOR", "LUMEN", "EMBER", "BONE", "VERDIGRIS", "SAFFRON", "OBSIDIAN", "FROST", "INDIGO", "PHOSPHOR", "SANDSTONE", "PEWTER", "SICKROOM", "RUST", "MOSS"};
#define NPAL 15
static const uint8_t PALS[NPAL][18] = {
    {0,0,0, 8,10,36, 26,28,110, 76,42,196, 132,120,255, 236,230,255},
    {0,0,0, 2,18,26, 0,64,78, 0,146,140, 84,232,196, 230,255,236},
    {0,0,0, 24,3,8, 84,8,16, 176,28,18, 248,110,40, 255,228,180},
    {0,0,0, 14,12,12, 52,44,40, 116,102,88, 190,174,148, 252,244,224},
    {0,0,0, 4,16,14, 20,56,44, 50,118,88, 142,184,124, 244,238,188},
    {0,0,0, 28,8,2, 108,34,4, 204,92,8, 255,176,40, 255,240,196},
    {0,0,0, 4,20,16, 8,72,58, 150,22,34, 232,72,56, 250,232,200},
    {0,0,0, 6,16,32, 26,62,104, 66,136,180, 168,218,236, 250,254,255},
    {0,0,0, 8,8,30, 28,30,88, 72,78,140, 176,170,190, 250,240,228},
    {0,0,0, 4,18,6, 10,66,22, 36,150,52, 120,236,110, 224,255,214},
    {0,0,0, 24,14,4, 92,58,18, 176,122,44, 236,190,92, 255,244,206},
    {0,0,0, 10,14,18, 40,50,60, 92,108,120, 170,186,194, 240,246,248},
    {0,0,0, 6,18,14, 30,60,48, 98,136,108, 190,214,160, 246,250,214},
    {0,0,0, 26,8,2, 92,30,8, 186,78,14, 255,152,36, 255,238,170},
    {0,0,0, 6,16,6, 24,48,20, 66,96,36, 140,158,66, 232,226,150}};

static struct { float x, y, r, v, a; uint8_t on; } rings[10];
static struct { float x, y, v; } snow[26];
static float trail[EN_NFA][16][2];
static uint8_t trail_n[EN_NFA];
static uint8_t dabs[3][81];
static const int DAB_D[3] = {3, 5, 9};
static uint32_t vrng = 0x1234567u;
static float vr(void) { vrng ^= vrng << 13; vrng ^= vrng >> 17; vrng ^= vrng << 5; return (float)(vrng >> 8) * (1.0f / 16777216.0f); }

static void palette_build(void)
{
    static const int X[6] = {0, 34, 82, 138, 198, 255};
    int v, k = 0, hue = title_on ? 0 : (int)(150.0f * fm_sinf(vt * .045f));
    for (v = 0; v < 256; v++) {
        float u;
        int r, g, b;
        uint32_t c;
        while (k < 4 && v > X[k + 1])
            k++;
        u = (float)(v - X[k]) / (float)(X[k + 1] - X[k]);
        r = (int)(cur[k * 3] + (cur[k * 3 + 3] - cur[k * 3]) * u);
        g = (int)(cur[k * 3 + 1] + (cur[k * 3 + 4] - cur[k * 3 + 1]) * u);
        b = (int)(cur[k * 3 + 2] + (cur[k * 3 + 5] - cur[k * 3 + 2]) * u);
        {                                /* and all the time a slow turn of the colours, every few minutes */
            int rr = r, gg = g, bb = b, hk = hue < 0 ? -hue : hue;
            r = (rr * (256 - hk) + (hue > 0 ? gg : bb) * hk) >> 8;
            g = (gg * (256 - hk) + (hue > 0 ? bb : rr) * hk) >> 8;
            b = (bb * (256 - hk) + (hue > 0 ? rr : gg) * hk) >> 8;
        }
        if (bright) {                    /* lift the dark half, so it can be seen in daylight */
            int lift = bright * 34, mul = 256 + bright * 70;
            r = (r * mul >> 8) + (lift * (255 - r) >> 8) * v / 255;
            g = (g * mul >> 8) + (lift * (255 - g) >> 8) * v / 255;
            b = (b * mul >> 8) + (lift * (255 - b) >> 8) * v / 255;
            r = r > 255 ? 255 : r;
            g = g > 255 ? 255 : g;
            b = b > 255 ? 255 : b;
        }
        c = (uint32_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
        lut[v] = (uint16_t)((c >> 8) | (c & 255u) << 8);
    }
}
static void palette_step(void)
{
    int i;
    if (pal_now < 0) {
        for (i = 0; i < 18; i++)
            cur[i] = PALS[pal_tgt][i];
        pal_now = pal_tgt;
        palette_build();
        return;
    }
    if (++pal_tick < 4)
        return;
    pal_tick = 0;
    for (i = 0; i < 18; i++)
        cur[i] += ((float)PALS[pal_tgt][i] - cur[i]) * (title_on ? .12f : .04f);   /* about 8 seconds between two palettes */
    palette_build();
}

void vis_init(void)
{
    int s, i, x, y;
    A = fld[0];
    Bf = fld[1];
    for (s = 0; s < 3; s++) {
        int d = DAB_D[s];
        float c = (float)(d - 1) * .5f;
        for (y = 0; y < d; y++)
            for (x = 0; x < d; x++) {
                float dx = (float)x - c, dy = (float)y - c, r = fm_sqrtf(dx * dx + dy * dy) / (c + .6f);
                dabs[s][y * d + x] = r < 1.0f ? (uint8_t)(255.0f * (1.0f - r) * (1.0f - r)) : 0;
            }
    }
    for (i = 0; i < 26; i++) {
        snow[i].x = vr() * VW;
        snow[i].y = vr() * VW;
        snow[i].v = .08f + vr() * .32f;
    }
}
void vis_bright(int b) { bright = b; pal_tick = 99; }
static void transition_start(void);
static int m_from_set(int m);
void vis_set_mode(int m)
{
    mode_auto = m < 0;
    if (m >= 0 && m < VIS_NMODE && m != mode)
        m_from_set(m);
}
int vis_mode(void) { return mode; }
void vis_title(int on) { title_on = on; }
static void title_marks(float t)
{
    static float next;
    int i;
    if (t >= next) {                     /* a ping, from the deep */
        next = t + .55f;
        for (i = 0; i < 10; i++)
            if (!rings[i].on)
                break;
        if (i < 10) {
            rings[i].on = 1;
            rings[i].x = 60.0f;
            rings[i].y = 60.0f;
            rings[i].r = 3.0f;
            rings[i].v = 2.4f;
            rings[i].a = 1.0f;
        }
    }
}

static inline float sn(float x) { return fm_sinf(x); }
static void field(int m, float u, float v, float r, float t, float en, float un, float *ox, float *oy)
{
    float dx = 0.0f, dy = 0.0f;
    switch (m) {
    case 0:
        dx = 1.1f * sn(v * 3.1f + t * .17f) + .75f * sn(v * 7.3f - t * .23f + u * 2.0f);
        dy = -1.0f - .4f * un + .8f * sn(u * 2.9f + t * .11f) + .4f * sn(u * 8.0f + t * .3f);
        break;
    case 1: {
        float rot = .009f + .004f * sn(t * .07f);
        dx = -v * 60.0f * rot - u * (1.1f + en);
        dy = u * 60.0f * rot - v * (1.1f + en);
        break;
    }
    case 2: {
        float br = .9f * sn(t * .23f) + .5f * sn(r * 9.0f - t * .4f);
        dx = u * br - v * 1.1f;
        dy = v * br + u * 1.1f;
        break;
    }
    case 3: {
        float au = fm_fabsf(u), av = fm_fabsf(v), c = (au > av ? au : av) + .02f, z = 3.2f + 1.6f * en;
        float s0 = fm_floorf(c * 6.0f) / 6.0f;
        dx = -(u / c) * s0 * z + (v > 0 ? .9f : -.9f) * sn(t * .2f);
        dy = -(v / c) * s0 * z - (u > 0 ? .9f : -.9f) * sn(t * .2f);
        break;
    }
    case 4:
        dx = .5f * sn(v * 3.0f + t * .1f);
        dy = 1.3f + 1.1f * sn(u * 3.1f + t * .2f + sn(u * 7.0f - t * .1f) * 1.4f);
        break;
    case 5:
        dx = .7f * sn(v * 5.0f + t * .09f) + .3f * sn(u * 9.0f + v * 4.0f + t * .13f);
        dy = .7f * sn(u * 5.0f - t * .08f) + .3f * sn(v * 9.0f - u * 4.0f + t * .11f);
        break;
    case 6: {
        float ua = u - .45f, ub = u + .45f, a1 = fm_sqrtf(ua * ua + v * v), a2 = fm_sqrtf(ub * ub + v * v);
        float w1 = sn(a1 * 15.0f - t * 1.3f), w2 = sn(a2 * 15.6f - t * 1.34f);
        dx = 1.2f * (w1 * ua / (a1 + .1f) + w2 * ub / (a2 + .1f));
        dy = 1.2f * (w1 * v / (a1 + .1f) + w2 * v / (a2 + .1f));
        break;
    }
    case 7: {
        float tear = sn(v * 20.0f + t * 3.0f) > .93f ? 5.0f * sn(t * 9.0f) : 0.0f;
        dx = tear + .5f * sn(v * 3.0f + t * .3f);
        dy = -.65f;
        break;
    }
    case 9:
        dx = .35f * sn(v * 6.0f + t * .15f);
        dy = -.9f + .25f * sn(u * 5.0f + t * .2f);
        break;
    case 10:
        dx = 1.3f + .4f * sn(v * 9.0f + t * .3f);
        dy = .45f * sn(u * 6.0f - t * .25f + v * 3.0f);
        break;
    case 11:
        dx = .4f * sn(v * 4.0f + t * .17f);
        dy = -1.25f - .3f * un;
        break;
    case 12:                  /* whole pixels: the trace stays sharp */
        dx = 2.0f;
        dy = 0.0f;
        break;
    case 13:
        dx = .6f * sn(v * 7.0f + t * .4f) + .3f * sn(u * 11.0f - t * .5f);
        dy = 1.1f + .4f * en;
        break;
    case 14:
        dx = .45f * sn(v * 4.0f + t * .07f) + .2f * sn(u * 13.0f + t * .1f);
        dy = .35f * sn(u * 3.0f - t * .06f);
        break;
    case 15:                  /* ANGLER: slow black water */
        dx = .5f * sn(v * 2.3f + t * .09f);
        dy = .55f + .3f * sn(u * 3.0f + t * .13f);
        break;
    case 16:                  /* VAULT: everything rises into the roof */
        dx = -u * .35f;
        dy = -.9f - .5f * en + .2f * sn(u * 6.0f + t * .2f);
        break;
    case 17: {                /* PRAYER WHEEL: a steady turn */
        float q = 1.0f + .5f * en;
        dx = -v * q * 1.6f;
        dy = u * q * 1.6f;
        break;
    }
    case 18:                  /* OBSIDIAN SUN: outward in steps */
        dx = u / r * (fm_floorf(r * 5.0f) * .32f + .3f) * (1.0f + en);
        dy = v / r * (fm_floorf(r * 5.0f) * .32f + .3f) * (1.0f + en);
        break;
    case 19:                  /* ICE SHARDS: a hard slant */
        dx = .9f + .3f * sn(v * 8.0f + t * .2f);
        dy = 1.1f;
        break;
    case 20:                  /* TORII: fog going sideways */
        dx = .75f + .4f * sn(v * 5.0f + t * .12f);
        dy = .2f * sn(u * 4.0f + t * .1f);
        break;
    case 21: {                /* GONG: the skin of the metal */
        float w = sn(r * 12.0f - t * 1.6f) * (.8f + 1.4f * en);
        dx = u / r * w;
        dy = v / r * w;
        break;
    }
    case 22:                  /* RADAR: phosphor that barely moves */
        dx = u * -.05f;
        dy = v * -.05f;
        break;
    case 23:                  /* STAR RUSH: out from the middle */
        dx = -u * (2.4f + 2.0f * en);
        dy = -v * (2.4f + 2.0f * en);
        break;
    case 24:                  /* EYE OF RA: heat rising off the sand */
        dx = .4f * sn(v * 9.0f + t * .5f);
        dy = -.7f;
        break;
    case 25:                  /* REEDS: bending together */
        dx = .9f * sn(t * .3f + v * 1.5f);
        dy = -.25f;
        break;
    case 26:                  /* GATES: drawn into the door */
        dx = -u * (1.5f + en);
        dy = -v * (1.5f + en);
        break;
    case 27:                  /* BRAINWAVES: whole pixels */
        dx = 2.0f;
        dy = 0.0f;
        break;
    case 28:                  /* MOLTEN: slow fat convection */
        dx = .9f * sn(v * 3.0f + t * .2f + sn(u * 3.0f + t * .1f));
        dy = .9f + .6f * sn(u * 4.0f - t * .15f);
        break;
    case 30: {                /* SPIRAL: everything turning into itself */
        float pull = -.5f - .5f * en;
        dx = u * pull - v * 1.5f;
        dy = v * pull + u * 1.5f;
        break;
    }
    case 31:                  /* TUNNEL: falling forward */
        dx = -u * (2.8f + 1.6f * en) + .4f * sn(t * .2f);
        dy = -v * (2.8f + 1.6f * en) + .4f * sn(t * .17f);
        break;
    case 32:                  /* CANDLES: heat going up */
        dx = .35f * sn(v * 8.0f + t * 1.1f + u * 2.0f);
        dy = 1.5f + .5f * en;
        break;
    case 33:                  /* WEB: barely stirring */
        dx = .3f * sn(r * 8.0f - t * .4f) * u;
        dy = .3f * sn(r * 8.0f - t * .4f) * v;
        break;
    case 34:                  /* BLACK MOON: light running down the water */
        dx = .35f * sn(v * 13.0f + t * .5f);
        dy = -.7f;
        break;
    case 35:                  /* COMETS: a slow wheel of dust */
        dx = v * .7f;
        dy = -u * .7f;
        break;
    case 36:                  /* MELT: everything sags and runs down, each column at its own pace */
        dx = .35f * sn(v * 5.0f + t * .2f) - u * .12f;
        dy = .8f + .55f * sn(u * 6.5f + t * .27f) + .35f * sn(u * 15.0f - t * .4f) + .5f * en;
        break;
    case 37: {                /* BLEED: ink soaking outward from three slow wounds */
        int k;
        for (k = 0; k < 3; k++) {
            float cx = .55f * sn(t * (.05f + (float)k * .02f) + (float)k * 2.1f), cy = .5f * sn(t * (.04f + (float)k * .017f) + (float)k * 1.3f);
            float qx = u - cx, qy = v - cy, d2 = qx * qx + qy * qy + .12f, w = (.22f + .25f * en) / d2;
            dx += qx * w * (1.0f + .3f * sn(d2 * 30.0f - t));
            dy += qy * w * (1.0f + .3f * sn(d2 * 30.0f - t));
        }
        dx = fm_clampf(dx, -2.2f, 2.2f);
        dy = fm_clampf(dy, -2.2f, 2.2f);
        break;
    }
    case 38: {                /* MORPH: breathing in and out while the whole thing slowly turns one way, then the other */
        float br = .8f * sn(t * .3f), tw = .55f * sn(t * .11f);
        dx = u * br - v * tw + .2f * sn(v * 7.0f + t * .5f);
        dy = v * br + u * tw + .2f * sn(u * 7.0f - t * .5f);
        break;
    }
    case 39:                  /* VEINS: barely moving, a slow outward creep */
        dx = .22f * sn(v * 9.0f + t * .2f) + u * .14f;
        dy = .22f * sn(u * 9.0f - t * .17f) + v * .14f;
        break;
    case 29:                  /* EYES: a slow turn of the dark */
        dx = -v * .35f + .2f * sn(v * 6.0f + t * .1f);
        dy = u * .35f + .2f * sn(u * 6.0f - t * .1f);
        break;
    default: {
        float pull = -.7f / (r * r + .12f), swirl = 1.3f / (r + .2f);
        dx = (u * pull - v * swirl * .55f) * 1.25f;
        dy = (v * pull + u * swirl * .55f) * 1.25f;
        break;
    }
    }
    *ox = dx;
    *oy = dy;
}
static int m_from = -1, mut_kind = -1;
static float m_w = 1.0f, mut_w, mut_t, mut_len, next_mode = 20.0f, next_mut = 12.0f, next_pal = 30.0f;
static int pal_own = -1;
/* the change from one picture to the next: a few seconds of melting, bleeding, swirling or draining, the colours
 * running while it happens, chosen at random each time */
static int tr_kind = -1;
static float tr_t, tr_len, tr_w;
static int m_from_set(int m)            /* a picture chosen by hand melts in the same way */
{
    m_from = mode;
    mode = m;
    m_w = 0.0f;
    transition_start();
    return m;
}
static void transition_start(void)
{
    tr_kind = (int)(vr() * 6.0f) % 6;
    tr_t = 0.0f;
    tr_len = 4.5f + vr() * 3.5f;
    if (vr() < .5f)                      /* half the time the colours bleed into new ones too */
        pal_own = (int)(vr() * NPAL) % NPAL;
}
static void mesh(float en, float un, int frozen)
{
    float t = vt, fz = frozen ? .05f : 1.0f, ease = m_w * m_w * (3.0f - 2.0f * m_w);
    int gx, gy;
    for (gy = 0; gy < GN; gy++)
        for (gx = 0; gx < GN; gx++) {
            float u = (float)gx / GRID * 2.0f - 1.0f, v = (float)gy / GRID * 2.0f - 1.0f, r = fm_sqrtf(u * u + v * v) + 1e-4f;
            float dx, dy;
            int j = gy * GN + gx;
            field(mode, u, v, r, t, en, un, &dx, &dy);
            if (m_from >= 0 && ease < 1.0f) {   /* the morph: one pattern's motion melting into the next */
                float ax, ay;
                field(m_from, u, v, r, t, en, un, &ax, &ay);
                dx = ax + (dx - ax) * ease;
                dy = ay + (dy - ay) * ease;
            }
            if (mut_w > 0.0f) {          /* a mutation, swelling in and out */
                if (mut_kind == 0) {     /* zoom in */
                    dx -= u * 2.2f * mut_w;
                    dy -= v * 2.2f * mut_w;
                } else if (mut_kind == 1) {   /* twist */
                    dx += -v * 2.0f * mut_w;
                    dy += u * 2.0f * mut_w;
                } else if (mut_kind == 2) {   /* ripple */
                    float w = sn(r * 14.0f - t * 2.0f) * 1.4f * mut_w;
                    dx += u / r * w;
                    dy += v / r * w;
                } else if (mut_kind == 3) {   /* melt downward */
                    dy += 1.6f * mut_w * (1.0f + .5f * sn(u * 5.0f + t));
                }
            }
            if (tr_w > 0.0f) {           /* the change itself */
                float w = tr_w;
                if (tr_kind == 0) {          /* melt: it all runs down, unevenly */
                    dy += 2.4f * w * (1.0f + .6f * sn(u * 7.0f + t * 1.3f));
                    dx += .7f * w * sn(v * 9.0f + t * 2.0f);
                } else if (tr_kind == 1) {   /* bleed: it soaks outward in waves */
                    float q = 1.9f * w * (1.0f + .5f * sn(r * 10.0f - t * 3.0f));
                    dx += u * q;
                    dy += v * q;
                } else if (tr_kind == 2) {   /* swirl: a whirlpool that winds up and lets go */
                    dx += -v * 2.8f * w * (1.2f - r * .4f);
                    dy += u * 2.8f * w * (1.2f - r * .4f);
                } else if (tr_kind == 3) {   /* drain: pulled into the middle and gone */
                    dx -= u * 2.6f * w;
                    dy -= v * 2.6f * w;
                } else if (tr_kind == 4) {   /* liquid: rippling and sliding sideways */
                    float rw = sn(r * 16.0f - t * 4.0f) * 1.6f * w;
                    dx += u / r * rw + 1.2f * w * sn(v * 5.0f + t);
                    dy += v / r * rw;
                } else {                     /* boil: churning in every direction */
                    dx += 1.8f * w * sn(v * 11.0f + t * 3.1f) * sn(u * 4.0f - t);
                    dy += 1.8f * w * sn(u * 11.0f - t * 2.7f) * sn(v * 4.0f + t);
                }
            }
            mx[j] = (int32_t)(((float)gx * VW / GRID + dx * fz) * 256.0f);
            my[j] = (int32_t)(((float)gy * VW / GRID + dy * fz) * 256.0f);
        }
}

static void warp(int decay)
{
    int cy, cx, W1 = VW - 1;
    uint8_t *S = A, *D = Bf, *t;
    for (cy = 0; cy < GRID; cy++) {
        int y0 = cy * VW / GRID, y1 = (cy + 1) * VW / GRID, H = y1 - y0;
        for (cx = 0; cx < GRID; cx++) {
            int x0 = cx * VW / GRID, x1 = (cx + 1) * VW / GRID, Wd = x1 - x0, j = cy * GN + cx, y;
            int32_t lx = mx[j], ly = my[j], rx = mx[j + 1], ry = my[j + 1];
            int32_t dlx = (mx[j + GN] - lx) / H, dly = (my[j + GN] - ly) / H;
            int32_t drx = (mx[j + GN + 1] - rx) / H, dry = (my[j + GN + 1] - ry) / H;
            for (y = y0; y < y1; y++) {
                int32_t sx = lx, sy = ly, ix = (rx - lx) / Wd, iy = (ry - ly) / Wd;
                uint8_t *o = D + y * VW + x0;
                int x;
                for (x = 0; x < Wd; x++) {
                    int px = sx >> 8, py = sy >> 8, fx = (sx & 255) >> 4, fy = (sy & 255) >> 4, q, top, bot, val;
                    px = px < 0 ? 0 : px >= W1 ? W1 - 1 : px;
                    py = py < 0 ? 0 : py >= W1 ? W1 - 1 : py;
                    q = py * VW + px;
                    top = S[q] * (16 - fx) + S[q + 1] * fx;
                    bot = S[q + VW] * (16 - fx) + S[q + VW + 1] * fx;
                    val = (top * (16 - fy) + bot * fy) >> 8;
                    val = (val * decay >> 8) - 1;
                    *o++ = (uint8_t)(val < 0 ? 0 : val);
                    sx += ix;
                    sy += iy;
                }
                lx += dlx; ly += dly; rx += drx; ry += dry;
            }
        }
    }
    t = A; A = D; Bf = t;
}
static void mirror(void)
{
    int H = VW / 2, x, y;
    for (y = 0; y < H; y++) {
        int top = y * VW, bot = (VW - 1 - y) * VW;
        for (x = 0; x < H; x++) {
            int a = A[top + x], b = A[x * VW + y], v = a > b ? a : b;
            A[top + x] = (uint8_t)v;
            A[top + VW - 1 - x] = (uint8_t)v;
            A[bot + x] = (uint8_t)v;
            A[bot + VW - 1 - x] = (uint8_t)v;
        }
    }
}
static void reflect(void)
{
    int y, x;
    for (y = 0; y < VW / 2; y++) {
        const uint8_t *a = A + y * VW;
        uint8_t *b = A + (VW - 1 - y) * VW;
        for (x = 0; x < VW; x++)
            b[x] = (uint8_t)(a[x] * 5 >> 3);
    }
}
static inline void add(float x, float y, int v)
{
    int ix = (int)x, iy = (int)y, o, z;
    if (x < 0.0f || y < 0.0f || ix >= VW || iy >= VW)
        return;
    o = iy * VW + ix;
    z = A[o] + v;
    A[o] = (uint8_t)(z > 255 ? 255 : z);
}
static void dab(float x, float y, int size, int gain)
{
    int d = DAB_D[size], r = (d - 1) >> 1, x0 = (int)x - r, y0 = (int)y - r, i, j;
    for (j = 0; j < d; j++)
        for (i = 0; i < d; i++)
            add((float)(x0 + i), (float)(y0 + j), dabs[size][j * d + i] * gain >> 8);
}
static void line(float x0, float y0, float x1, float y1, int v)
{
    float ax = fm_fabsf(x1 - x0), ay = fm_fabsf(y1 - y0), m = ax > ay ? ax : ay;
    int n = (int)m, i;
    if (n < 1)
        n = 1;
    if (n > 170)
        n = 170;
    for (i = 0; i <= n; i++)
        add(x0 + (x1 - x0) * (float)i / (float)n, y0 + (y1 - y0) * (float)i / (float)n, v);
}
static void ribbon(float base, float amp, int k, float t, int v)
{
    float py = 0.0f;
    int x;
    for (x = 0; x < VW; x += 2) {
        float fx = (float)x, yy = base + amp * sn(fx * .05f * (1.0f + (float)k * .6f) + t * (.4f + (float)k * .23f)) * sn(fx * .017f + t * .13f) + 2.0f * sn(fx * .31f + t);
        if (x)
            line(fx - 2.0f, py, fx, yy, v);
        py = yy;
    }
}
static void ring(float cx, float cy, float r, int sides, float spin, int v, float wob, float t)
{
    int n = sides * 6, i;
    float px = 0.0f, py = 0.0f;
    if (n > 72)
        n = 72;
    for (i = 0; i <= n; i++) {
        float a = (float)i / (float)n * 6.28318f + spin, rr = r * (1.0f + wob * fm_cosf(a * (float)sides + t));
        float x = cx + fm_cosf(a) * rr, y = cy + sn(a) * rr;
        if (i)
            line(px, py, x, y, v);
        px = x;
        py = y;
    }
}
static float fmodf1(float x, float m) { return x - fm_floorf(x / m) * m; }

static void motif2(int m, float en, float d, const float *ch, float t)
{
    int i, k, x, v = 26 + (int)(70.0f * d);
    float a, b, r;
    switch (m) {
    case 15:                              /* a lure on a stalk, and things drifting up to it */
        a = 60.0f + 26.0f * sn(t * .23f);
        b = 44.0f + 12.0f * sn(t * .31f);
        line(a - 18.0f * sn(t * .23f), 2, a, b, 18 + (int)(30 * d));
        dab(a, b, 2, 150 + (int)(100 * en));
        for (i = 0; i < 4; i++)
            if (ch[i] > .03f)
                dab(a + 24.0f * sn(t * .5f + (float)i * 1.6f), b + 18.0f * fm_cosf(t * .4f + (float)i), 0, (int)(180 * ch[i]));
        break;
    case 16:                              /* ribs of an arch */
        for (k = 0; k < 3; k++) {
            float h = 36.0f + (float)k * 24.0f + 8.0f * en, w = 16.0f + (float)k * 15.0f, px = 60.0f - w, py = 118.0f;
            for (i = 1; i <= 14; i++) {
                float s0 = (float)i / 14.0f, x1 = 60.0f - w * (1.0f - s0 * s0), y1 = 118.0f - h * sn(s0 * 1.5708f);
                line(px, py, x1, y1, v >> 1);
                px = x1;
                py = y1;
            }
        }
        break;
    case 17:                              /* spokes around a hub */
        a = t * .4f;
        for (i = 0; i < 8; i++) {
            float q = a + (float)i * .7854f;
            line(60 + 12 * fm_cosf(q), 60 + 12 * sn(q), 60 + (46 + 8 * en) * fm_cosf(q), 60 + (46 + 8 * en) * sn(q), i & 1 ? v >> 1 : v);
        }
        ring(60, 60, 12, 8, a, v, 0, t);
        break;
    case 18:                              /* a black disc with stepped teeth */
        ring(60, 60, 26 + 6 * en, 12, t * .05f, v, .1f, t);
        ring(60, 60, 42 + 3 * en, 8, -t * .04f, v >> 1, .05f, t);
        break;
    case 19:                              /* splinters */
        for (i = 0; i < 6; i++) {
            float fx = fmodf1((float)i * 37.0f + t * (5.0f + (float)i), 120.0f), fy = fmodf1((float)i * 23.0f + t * (9.0f + (float)i * 2.0f), 120.0f);
            float l = 5.0f + 7.0f * d + (float)(i & 3);
            line(fx, fy, fx + l, fy + l * 1.3f, v);
            line(fx, fy, fx + l * .4f, fy - l * .3f, v >> 1);
        }
        break;
    case 20:                              /* a gate standing in water */
        x = (int)(8.0f * sn(t * .05f));
        for (k = -1; k <= 1; k += 2) {
            float px = 60.0f + (float)k * 26.0f + (float)x;
            line(px, 34, px, 104, v);
            line(px + 1, 34, px + 1, 104, v);
        }
        line(22 + x, 30, 98 + x, 26, v);
        line(22 + x, 31, 98 + x, 27, v);
        line(30 + x, 44, 90 + x, 44, v >> 1);
        break;
    case 21:                              /* a struck disc */
        r = 16.0f + 28.0f * en;
        ring(60, 60, r, 12, 0, v, .02f, t);
        ring(60, 60, r * .55f, 12, 0, v >> 1, .03f, t);
        dab(60, 60, 2, 60 + (int)(170 * en));
        break;
    case 22:                              /* a sweep and what it finds */
        a = t * .9f;
        line(60, 60, 60 + 56 * fm_cosf(a), 60 + 56 * sn(a), 110);
        ring(60, 60, 56, 12, 0, 10, 0, t);
        ring(60, 60, 30, 12, 0, 8, 0, t);
        for (i = 0; i < EN_NFA; i++)
            if (en_st.fa_on[i])
                dab(60 + (en_st.fa_x[i] - .5f) * 96, 60 + (en_st.fa_y[i] - .5f) * 96, 1, 120);
        break;
    case 23:                              /* points thrown from the middle */
        for (i = 0; i < 8; i++) {
            float q = (float)i * 2.399f + t * .13f, s0 = 4.0f + 8.0f * fmodf1((float)i * .37f + t * .4f, 1.0f);
            dab(60 + fm_cosf(q) * s0, 60 + sn(q) * s0, 0, 190 + (int)(60 * en));
        }
        break;
    case 24:                              /* an eye, and it opens */
        b = 8.0f + 16.0f * (.5f + .5f * sn(t * .17f));
        for (x = 0; x < 16; x++) {
            float s0 = (float)x / 16.0f, s1 = (float)(x + 1) / 16.0f, y0 = sn(s0 * 3.1416f) * b, y1 = sn(s1 * 3.1416f) * b;
            line(20 + s0 * 80, 60 - y0, 20 + s1 * 80, 60 - y1, v);
            line(20 + s0 * 80, 60 + y0, 20 + s1 * 80, 60 + y1, v);
        }
        ring(60, 60, 5 + b * .4f, 6, t * .3f, v, 0, t);
        dab(60, 60, 1, 120 + (int)(100 * en));
        break;
    case 25:                              /* reeds above the water */
        for (i = 0; i < 9; i++) {
            float bx = 8.0f + (float)i * 13.0f, tip = 18.0f + 24.0f * sn((float)i * 2.1f) * sn((float)i * 2.1f);
            line(bx, 60, bx + 7.0f * sn(t * .5f + (float)i), tip, v >> 1);
        }
        break;
    case 26:                              /* doors inside doors */
        r = 8.0f + 46.0f * fmodf1(t * .06f, 1.0f);
        for (k = 0; k < 2; k++) {
            float q = k ? r * .5f : r;
            line(60 - q, 110, 60 - q, 60 - q, v);
            line(60 + q, 110, 60 + q, 60 - q, v);
            line(60 - q, 60 - q, 60 + q, 60 - q, v);
        }
        break;
    case 27:                              /* four traces drawn at the right edge */
        for (i = 0; i < 4; i++) {
            float base = 20.0f + (float)i * 27.0f, g = i == 0 ? en_st.low : i == 1 ? en_st.mid : i == 2 ? en_st.hi : ch[0] * .2f;
            float y = base + 9.0f * sn(t * (1.5f + (float)i) + (float)i) * (.25f + fm_minf(1.0f, g * 8.0f));
            dab(117, y, 0, 255);
        }
        break;
    case 28:                              /* metal rising in fat drops */
        for (i = 0; i < 5; i++)
            dab(14.0f + (float)i * 23.0f + 7.0f * sn(t * .3f + (float)i), 118.0f - 8.0f * en * sn(t + (float)i), 2, 90 + (int)(130 * en));
        break;
    case 30:                              /* two arms */
        for (k = 0; k < 2; k++)
            for (i = 0; i < 26; i++) {
                float q = (float)i * .24f + t * .3f + (float)k * 3.1416f, rr = 3.0f + (float)i * 1.9f;
                add(60 + fm_cosf(q) * rr, 60 + sn(q) * rr, v);
            }
        break;
    case 31:                              /* a frame far off, coming closer */
        r = 5.0f + 3.0f * en;
        line(60 - r, 60 - r, 60 + r, 60 - r, v * 2);
        line(60 + r, 60 - r, 60 + r, 60 + r, v * 2);
        line(60 + r, 60 + r, 60 - r, 60 + r, v * 2);
        line(60 - r, 60 + r, 60 - r, 60 - r, v * 2);
        break;
    case 32:                              /* a row of flames */
        for (i = 0; i < 5; i++) {
            float fx = 16.0f + (float)i * 22.0f, fl = sn(t * (2.3f + (float)i * .7f) + (float)i) * 2.0f;
            line(fx, 118, fx, 96, 14);
            dab(fx + fl, 90.0f + fl, 1, 150 + (int)(100 * en));
        }
        break;
    case 33:                              /* spokes and a thread winding out */
        for (i = 0; i < 8; i++) {
            float q = (float)i * .7854f + .2f * sn(t * .1f);
            line(60, 60, 60 + 58 * fm_cosf(q), 60 + 58 * sn(q), v >> 2);
        }
        for (k = 1; k < 5; k++)
            ring(60, 60, (float)k * 11.0f + 2.0f * en, 8, .2f * sn(t * .1f), v >> 1, .05f, t * .3f);
        if (en_st.fa_on[0])
            dab(60 + (en_st.fa_x[0] - .5f) * 90, 60 + (en_st.fa_y[0] - .5f) * 90, 1, 200);
        break;
    case 34:                              /* a dark disc with a burning rim */
        ring(60, 42, 25 + 2 * en, 12, 0, v, .01f, t);
        for (i = 0; i < 12; i++)
            add(60 + 25 * sn((float)i * .52f + t * .2f), 42 + 25 * fm_cosf((float)i * .52f + t * .2f), 150);
        line(0, 96, 119, 96, v >> 2);
        break;
    case 35:                              /* three bright things, going round */
        for (i = 0; i < 3; i++) {
            float q = t * (.31f + (float)i * .09f) + (float)i * 2.1f, rr = 24.0f + (float)i * 13.0f + 5.0f * sn(t * .13f + (float)i);
            dab(60 + fm_cosf(q) * rr, 60 + sn(q) * rr, 1, 170 + (int)(85 * en));
        }
        break;
    case 36:                              /* wax: a running edge at the top, drops falling from it */
        ribbon(3, 2 + 2 * en, 0, t * .5f, v + 40);
        ribbon(6, 3 + 3 * en, 1, t * .4f, v);
        for (i = 0; i < 6; i++) {
            float fx = 10.0f + (float)i * 20.0f + 4.0f * sn(t * .2f + (float)i), fy = fmodf1(t * (5.0f + (float)i * 1.7f) + (float)i * 29.0f, 132.0f) - 6.0f;
            dab(fx, fy, 1, 200 + (int)(55 * en));
            dab(fx, fy - 3.0f, 0, 160);
        }
        break;
    case 37:                              /* the wounds the ink comes from */
        for (k = 0; k < 3; k++) {
            float cx = 60.0f + 33.0f * sn(t * (.05f + (float)k * .02f) + (float)k * 2.1f), cy = 60.0f + 30.0f * sn(t * (.04f + (float)k * .017f) + (float)k * 1.3f);
            dab(cx, cy, 2, 170 + (int)(85 * en));
            ring(cx, cy, 6.0f + 4.0f * sn(t * (1.1f + (float)k * .3f)), 4, 0, 70 + (int)(60 * en), .2f, t);
        }
        break;
    case 38: {                            /* one shape becoming the next: ring, star, eye, door, and round again */
        float s0 = t * .07f, f = s0 - fm_floorf(s0), px = 0, py = 0;
        int sh = (int)fm_floorf(s0) & 3, nx = (sh + 1) & 3;
        f = f * f * (3.0f - 2.0f * f);
        for (i = 0; i <= 72; i++) {
            float q = (float)i / 72.0f * 6.28318f + t * .05f, c = fm_cosf(q), s1 = sn(q), ra = 0, rb = 0, rr, xx, yy;
            for (k = 0; k < 2; k++) {
                int which = k ? nx : sh;
                float rv;
                if (which == 0) rv = 30.0f;                                            /* ring */
                else if (which == 1) rv = 30.0f * (1.0f + .38f * fm_cosf(q * 5.0f));     /* star */
                else if (which == 2) rv = 1.0f / fm_sqrtf(c * c / 1296.0f + s1 * s1 / 196.0f);   /* eye */
                else { float a4 = c * c * c * c + s1 * s1 * s1 * s1; rv = 30.0f / fm_sqrtf(fm_sqrtf(a4)); }   /* door */
                if (k) rb = rv; else ra = rv;
            }
            rr = (ra + (rb - ra) * f) * (1.0f + .12f * en);
            xx = 60.0f + c * rr;
            yy = 60.0f + s1 * rr;
            if (i)
                line(px, py, xx, yy, v);
            px = xx;
            py = yy;
        }
        if (sh == 2 || nx == 2)
            dab(60, 60, 1, 80 + (int)(150 * en));
        break;
    }
    case 39: {                            /* cracks creeping out, branching, pulsing with the sound */
        static float vx[6], vy[6], va[6];
        static int vinit;
        if (!vinit) {
            vinit = 1;
            for (i = 0; i < 6; i++) { vx[i] = 60; vy[i] = 60; va[i] = (float)i * 1.047f; }
        }
        for (i = 0; i < 6; i++) {
            float ox = vx[i], oy = vy[i];
            va[i] += (vr() - .5f) * .3f;
            vx[i] += fm_cosf(va[i]) * 1.4f;
            vy[i] += sn(va[i]) * 1.4f;
            line(ox, oy, vx[i], vy[i], 220 + (int)(35 * en));
            line(ox + 1, oy, vx[i] + 1, vy[i], 60 + (int)(60 * en));
            if (vx[i] < 1 || vy[i] < 1 || vx[i] > 118 || vy[i] > 118 || vr() < .004f) {   /* a new crack, from an old one */
                int o = (int)(vr() * 6.0f) % 6;
                vx[i] = o == i ? 60.0f + (vr() - .5f) * 30.0f : vx[o];
                vy[i] = o == i ? 60.0f + (vr() - .5f) * 30.0f : vy[o];
                va[i] = vr() * 6.283f;
            }
        }
        break;
    }
    default:                              /* something looking back */
        for (i = 0; i < 3; i++) {
            float ph = t * (.11f + (float)i * .03f) + (float)i * 2.0f, ex = 30.0f + (float)i * 30.0f + 12.0f * sn(ph * .7f), ey = 34.0f + (float)i * 26.0f + 8.0f * sn(ph * .5f);
            if (sn(ph) > -.5f && sn(ph * 7.0f) > -.92f) {
                dab(ex - 6.0f, ey, 1, 150 + (int)(100 * en));
                dab(ex + 6.0f, ey, 1, 150 + (int)(100 * en));
            }
        }
        break;
    }
}
static void motif(float en, float t)
{
    float d = en_st.dr * .7f;
    int m = mode, i, x, k;
    float ch[EN_NCH];
    for (i = 0; i < EN_NCH; i++)
        ch[i] = en_st.ch[i];
    d = d > 1.0f ? 1.0f : d < .42f ? .42f : d;
    if (m >= 15) {
        motif2(m, en, d, ch, t);
        return;
    }
    if (m == 0) {
        int dv = 20 + (int)(62.0f * d);
        ribbon(106, 5 + 13 * en, 0, t, dv);
        ribbon(106, 5 + 13 * en, 1, t, dv);
        for (i = 0; i < EN_NCH; i++) {
            float a = ch[i], fx, h;
            int y;
            if (a < .02f)
                continue;
            fx = 18.0f + (float)i * 28.0f + 8.0f * sn(t * .1f + (float)i * 2.0f);
            h = 54.0f + 42.0f * a;
            for (y = 0; y < (int)h; y += 2) {
                int lim = (int)((1.0f - (float)y / h) * a * 54.0f);
                float jit = sn((float)y * .23f + t * .4f + (float)i) * 2.5f;
                add(fx + jit, (float)y, lim);
                add(fx + jit, (float)y + 1.0f, lim);
                add(fx + jit + 1.0f, (float)y, lim >> 1);
            }
        }
    } else if (m == 1) {
        ring(60, 60, 50, 12, t * .03f, 16 + (int)(40 * d), .04f, t * .4f);
        ring(60, 60, 22 + 6 * en, 8, -t * .05f, 22 + (int)(44 * d), .16f, t * .3f);
        for (i = 0; i < 4; i++)
            if (ch[i] > .03f) {
                float a = (float)i * .39f + .2f + t * .05f;
                line(60, 60, 60 + fm_cosf(a) * 58, 60 + sn(a) * 58, (int)(70 * ch[i]));
            }
    } else if (m == 2) {
        ring(30, 30, 9 + 5 * sn(t * .4f), 6, t * .1f, 26 + (int)(40 * d), .2f, t);
        ring(60, 60, 44 + 6 * en, 8, -t * .06f, 18 + (int)(40 * d), .08f, t * .5f);
        for (i = 0; i < 4; i++)
            if (ch[i] > .03f)
                dab(12.0f + (float)i * 13.0f, 12.0f + (float)i * 5.0f, 1, (int)(110 * ch[i]));
    } else if (m == 3) {
        float r = 12 + 44 * fmodf1(t * .07f, 1.0f), q = fm_floorf(r / 4.0f + .5f) * 4.0f;
        int v = 46 + (int)(90 * d);
        line(60 - q, 60 - q, 60 + q, 60 - q, v);
        line(60 + q, 60 - q, 60 + q, 60 + q, v);
        line(60 + q, 60 + q, 60 - q, 60 + q, v);
        line(60 - q, 60 + q, 60 - q, 60 - q, v);
        if (sn(t * .8f) > .95f)
            dab(60, 60, 2, 120);
    } else if (m == 4) {
        for (k = 0; k < 2; k++) {
            float yb = 14.0f + (float)k * 16.0f;
            for (x = k; x < VW; x += 2) {
                float fx = (float)x;
                add(fx, yb + 7 * sn(fx * .07f + t * .22f + (float)k) + 4 * sn(fx * .19f - t * .3f), 22 + (int)(66 * d));
            }
        }
        if (sn(t * .13f) > .9f) {
            float fx = 60 + 50 * sn(t * 1.7f);
            line(fx, 92, fx + 18 * sn(t * 3.0f), 116, 120);
        }
    } else if (m == 5) {
        float a0 = t * .11f;
        for (i = 0; i < 44; i++) {
            float g = (float)i / 44.0f, a = a0 + g * 5.5f, r = 36 + 3 * sn(a * 3 + t * .1f);
            dab(60 + fm_cosf(a) * r, 58 + sn(a) * r, i > 34 ? 0 : 1, (int)((34 + 76 * d) * g));
        }
        for (i = 0; i < 2; i++)
            if (ch[i] > .05f)
                dab(30.0f + (float)i * 60.0f, 16, 2, (int)(60 * ch[i]));
    } else if (m == 6) {
        int v = 18 + (int)(46 * d);
        ring(33, 60, 7 + 3 * sn(t * 1.3f), 4, 0, v, 0, 0);
        ring(87, 60, 7 + 3 * sn(t * 1.34f), 4, 0, v, 0, 0);
        ring(33, 60, 10 + fmodf1(t * 9.0f, 58.0f), 4, 0, v >> 1, 0, 0);
        ring(87, 60, 10 + fmodf1(t * 9.4f, 58.0f), 4, 0, v >> 1, 0, 0);
    } else if (m == 7) {
        float y0 = 60 + 34 * sn(t * .17f), amp = 6 + 26 * en, py = y0;
        for (x = 0; x < VW; x += 2) {
            float fx = (float)x, y = y0 + amp * sn(fx * (.09f + en_st.mid * .6f) + t * 2.1f) * sn(fx * .021f + t * .4f) + sn(fx * 9.1f + t * 13.0f) * 36.0f * en_st.hi;
            if (x)
                line(fx - 2.0f, py, fx, y, 60 + (int)(90 * d));
            py = y;
        }
    } else if (m == 9) {         /* a sun sinking through the twelve gates of the night */
        float y = fmodf1(t * 3.2f, 136.0f) - 8.0f;
        int v = 26 + (int)(70 * d);
        ring(60, y, 10 + 2 * en, 8, t * .2f, v + 20, 0, 0);
        line(14, 0, 14, 119, v >> 1);
        line(106, 0, 106, 119, v >> 1);
        for (i = 0; i < 6; i++) {
            float q = 10.0f + (float)i * 20.0f;
            line(14, q, 28, q, v >> 1);
            line(92, q, 106, q, v >> 1);
        }
    } else if (m == 10) {        /* a still horizon, and one swan going the other way */
        float x0 = 124 - fmodf1(t * 2.6f, 136.0f);
        ribbon(58, 1 + 2 * en, 0, t, 24 + (int)(46 * d));
        for (i = 0; i < 10; i++) {
            float q = (float)i;
            dab(x0 + q * 1.2f, 54 - 9 * sn(q * .34f), i > 7 ? 1 : 0, 110);
        }
        dab(x0 + 13, 56, 1, 150);
    } else if (m == 11) {        /* wedges, falling like dust */
        int v = 40 + (int)(90 * d);
        for (i = 0; i < 2; i++) {
            float x0 = vr() * VW, y0 = vr() * 110.0f;
            line(x0, y0, x0 + 4, y0, v);
            line(x0, y0, x0 + 2, y0 + 5, v);
            line(x0 + 4, y0, x0 + 2, y0 + 5, v);
        }
    } else if (m == 12) {        /* a heart on a monitor, slowing and racing with the water */
        static float ph, py = 60;
        float rate = .75f + en_st.m[M_UNREST] * .9f, y = 60;
        int v = 100 + (int)(120 * d);
        ph = fmodf1(ph + rate * .033f, 1.0f);
        if (ph < .06f) y = 60 + 10 * sn(ph * 52.0f);
        else if (ph < .12f) y = 60 - (30 + 14 * en) * sn((ph - .06f) * 52.0f);
        else if (ph < .19f) y = 60 + 13 * sn((ph - .12f) * 45.0f);
        else if (ph > .32f && ph < .46f) y = 60 - 7 * sn((ph - .32f) * 22.4f);
        line(108, py, 110, y, v);
        line(108, py + 1, 110, y + 1, v >> 1);
        line(110, y, 112, y, v);
        py = y;
    } else if (m == 13) {        /* a turning gear, and what flies off it */
        int v = 30 + (int)(80 * d);
        ring(60, 112, 26 + 3 * en, 12, t * .3f, v, .12f, 0);
        ring(60, 112, 9, 6, -t * .3f, v, 0, 0);
        for (i = 0; i < 7; i++)
            dab(60 + (vr() - .5f) * 66, 100 - vr() * 12, 0, 120 + (int)(100 * en));
    } else if (m == 14) {        /* three bare trees, and eyes that open */
        int tree;
        int v = 12 + (int)(26 * d);
        for (tree = 0; tree < 3; tree++) {
            float x0 = 22.0f + (float)tree * 38.0f, h = 40.0f + (float)(tree & 1) * 16.0f;
            float sway = sn(t * .3f + (float)tree) * .08f;
            line(x0, 119, x0, 119 - h, v + 6);
            for (i = 0; i < 4; i++) {
                float y = 119 - h * (.45f + (float)i * .18f), l = 24.0f - (float)i * 5.0f;
                float a = .75f + sway;
                float xa = x0 + sn(a) * l, xb = x0 - sn(a - sway * 2) * l, yt = y - fm_cosf(a) * l;
                line(x0, y, xa, yt, v);
                line(x0, y, xb, yt, v);
                line(xa, yt, xa + 5, yt - 6, v >> 1);
                line(xb, yt, xb - 5, yt - 6, v >> 1);
            }
        }
        if (sn(t * .7f) > .8f) {
            dab(52, 62, 0, 180);
            dab(58, 62, 0, 180);
        }
    } else {
        ring(60, 60, 19 + 2 * en, 12, t * .1f, 30 + (int)(58 * d), .05f, t * .7f);
        for (i = 0; i < 3; i++) {
            float a = fmodf1(t * .13f + (float)i * 2.1f, 6.283f), r = 57 - fmodf1(t * 6.0f + (float)i * 21.0f, 40.0f);
            add(60 + fm_cosf(a) * r, 60 + sn(a) * r, 150);
        }
    }
}

static void creatures(float t)
{
    int i, j;
    for (i = 0; i < EN_NFA; i++) {
        float x = en_st.fa_x[i] * VW, y = en_st.fa_y[i] * VW, a0 = en_st.fa_a[i], fi = (float)i;
        int k = en_st.fa_v[i], v = 90 + (int)(165.0f * a0);
        if (!en_st.fa_on[i]) {
            trail_n[i] = 0;
            continue;
        }
        if (k == 0) {
            int n = trail_n[i];
            if (n < 16)
                n = ++trail_n[i];
            for (j = 0; j + 1 < n; j++) {
                trail[i][j][0] = trail[i][j + 1][0];
                trail[i][j][1] = trail[i][j + 1][1];
            }
            trail[i][n - 1][0] = x + 7 * sn(t * 1.2f + fi);
            trail[i][n - 1][1] = y + 5 * fm_cosf(t * .9f + fi);
            for (j = 0; j < n; j++)
                dab(trail[i][j][0] + sn((float)j * .7f + t * 2.0f) * 2.4f, trail[i][j][1], j > 11 ? 1 : 0, v * (j + 2) / 18);
        } else if (k == 1) {
            for (j = 0; j < 3; j++) {
                float a = fi + (float)j * 2.1f + sn(t * 5.0f + (float)j) * .3f, l = 10 + 22 * a0 * fm_fabsf(sn(t * 7.0f + (float)j));
                line(x, y, x + fm_cosf(a) * l, y + sn(a) * l, v >> 1);
            }
        } else if (k == 2) {
            if (sn(t * 16.0f + fi) > .55f) {
                dab(x, y, 1, v);
                for (j = 0; j < 22; j++) {
                    float a = (float)j / 22.0f * 6.283f;
                    add(x + fm_cosf(a) * 8, y + sn(a) * 8, v >> 2);
                }
            }
        } else if (k == 3) {
            float ph = fmodf1(t * .6f + fi, 1.0f);
            for (j = 0; j < 12; j++) {
                float a = (float)j * 2.399f + t * .2f, r = (1 - ph) * (34 + (float)j * 2) * (1 - a0 * .25f);
                dab(x + fm_cosf(a) * r, y + sn(a) * r, 0, v >> 1);
            }
        } else if (k == 5) {
            for (j = 0; j < 7; j++) {
                float yy = fmodf1(y + (float)j * 6 + sn(t * 3.0f + (float)j) * 9 + VW, VW), ln = 16 + 30 * fm_fabsf(sn(t * 4.0f + (float)j * 2));
                line(x - ln, yy, x + ln, yy, v >> 2);
            }
        } else if (k == 6) {
            for (j = 0; j < 9; j++) {
                float a = (float)j * 2.4f + t * (.6f + (float)j * .03f), r = 7 + (float)j * 2.2f + 3 * sn(t * 2.0f + (float)j);
                dab(x + fm_cosf(a) * r, y + sn(a) * r * .6f, 0, v >> 1);
            }
        } else {
            float r = 6 + 12 * a0;
            for (j = 0; j < 36; j++) {
                float a = (float)j / 36.0f * 6.283f;
                add(x + fm_cosf(a) * r * (1 + .25f * sn(a * 2 + t)), y + sn(a) * r * .55f, v >> 1);
            }
            dab(x, y, 1, v >> 1);
        }
    }
}

void vis_step(float dt)
{
    static const uint8_t DEC[VIS_NMODE] = {246, 244, 247, 242, 244, 250, 240, 236, 246, 244, 246, 250, 253, 238, 243,
                                           248, 242, 240, 244, 240, 247, 238, 251, 246, 244, 246, 240, 253, 250, 250,
                                           242, 240, 244, 246, 246, 248, 246, 252, 244, 253};
    en_ev_t e;
    int i, frozen = 0;                   /* HOLD freezes the sound; the picture keeps living */
    float en, t;
    vt += dt;
    if (vt > 20000.0f)
        vt -= 19998.0f;
    t = vt;
    while (en_ev_get(&e)) {
        if (e.type == EV_BELL) {
            for (i = 0; i < 10; i++)
                if (!rings[i].on)
                    break;
            if (i == 10)
                i = 0;
            rings[i].on = 1;
            rings[i].x = (float)e.x * (VW / 255.0f);
            rings[i].y = (float)e.y * (VW / 255.0f);
            rings[i].r = 1;
            rings[i].v = 1.3f + vr();
            rings[i].a = (float)e.a / 255.0f;
            dab(rings[i].x, rings[i].y, 2, 255);
        } else if (e.type == EV_NOTE) {
            app_midi_note(e.y, e.a, e.x);
        } else if (e.type == EV_OMEN || e.type == EV_UPHEAVAL) {
            flash = 1.0f;
        }
    }
    {   /* the picture keeps changing by itself: a new pattern every half minute or so, melting from the last; colours of
         * its own; and mutations that swell in and out. The SCREEN knob holds the pattern but not the rest. */
        float pace = .7f + en_st.m[M_UNREST] * .8f;
        if (m_w < 1.0f)
            m_w = fm_minf(1.0f, m_w + dt / 7.0f);
        next_mode -= dt * pace;
        if (mode_auto && !title_on && next_mode <= 0.0f) {
            int pick = (int)(vr() * 3.0f), nm;
            nm = vr() < .55f ? (pick == 0 ? en_st.sea : pick == 1 ? en_st.sea + 15 : 30 + en_st.sea % 6) : (int)(vr() * VIS_NMODE) % VIS_NMODE;
            if (nm != mode) {
                m_from = mode;
                mode = nm;
                m_w = 0.0f;
                transition_start();
            }
            next_mode = 22.0f + vr() * 40.0f;
        }
        if (mode < 0 || mode >= VIS_NMODE)
            mode = en_st.sea;
        next_mut -= dt * pace;
        if (!title_on && next_mut <= 0.0f && mut_kind < 0) {
            mut_kind = (int)(vr() * 6.0f) % 6;
            mut_t = 0.0f;
            mut_len = 6.0f + vr() * 10.0f;
            next_mut = 14.0f + vr() * 26.0f;
        }
        if (tr_kind >= 0) {
            tr_t += dt;
            tr_w = sn(fm_minf(1.0f, tr_t / tr_len) * 3.1416f);
            if (tr_t >= tr_len) {
                tr_kind = -1;
                tr_w = 0.0f;
            }
        }
        if (mut_kind >= 0) {
            mut_t += dt;
            mut_w = sn(fm_minf(1.0f, mut_t / mut_len) * 3.1416f);
            if (mut_t >= mut_len) {
                mut_kind = -1;
                mut_w = 0.0f;
            }
        }
        next_pal -= dt * pace;
        if (next_pal <= 0.0f) {
            pal_own = vr() < .5f ? -1 : (int)(vr() * NPAL) % NPAL;
            next_pal = 25.0f + vr() * 35.0f;
        }
    }
    pal_tgt = title_on ? 1 : pal_own >= 0 ? pal_own : en_st.palette;
    if (title_on) {
        mode = 8;
        title_marks(t);
    }
    palette_step();
    en = fm_minf(1.0f, en_st.low * 7.0f + en_st.swell);
    mesh(en, en_st.m[M_UNREST], frozen);
    {
        int dec = (int)DEC[mode] - (int)(en_st.m[M_ROT] * 7.0f) + (int)(tr_w * 7.0f);   /* the change leaves longer trails */
        warp(frozen ? 255 : dec > 254 ? 254 : dec);
    }
    if (mode == 1 || mode == 2 || mode == 16 || mode == 17 || (mut_kind == 4 && mut_w > .3f))
        mirror();
    if (mode == 10 || mode == 25 || (mut_kind == 5 && mut_w > .3f))
        reflect();
    if (!frozen) {
        if (!title_on)
            motif(en, t);
        for (i = 0; i < 10; i++) {
            float a, q = rings[i].r;
            int steps, v, k;
            if (!rings[i].on)
                continue;
            rings[i].r = q + rings[i].v * dt * 18.0f;
            rings[i].a *= 1.0f - dt * 1.5f;
            steps = 12 + (int)(q * 4.0f);
            steps = steps > 90 ? 90 : steps;
            v = (int)(150.0f * rings[i].a);
            if (v > 8)
                for (k = 0; k < steps; k++) {
                    a = (float)k / (float)steps * 6.283f;
                    add(rings[i].x + fm_cosf(a) * q, rings[i].y + sn(a) * q, v);
                }
            if (rings[i].a < .05f || q > 92.0f)
                rings[i].on = 0;
        }
        if (!title_on)
            creatures(t);
        for (i = 0; i < 26; i++) {
            snow[i].y += snow[i].v * (1.0f + fm_minf(1.0f, en_st.hi * 18.0f) * 2.0f);
            snow[i].x += .08f * sn(t * .4f + snow[i].y * .1f);
            if (snow[i].y >= VW) {
                snow[i].y = 0;
                snow[i].x = vr() * VW;
            }
            add(snow[i].x, snow[i].y, 34 + (int)(40.0f * snow[i].v));
        }
    }
    if (flash > 0.0f)
        flash -= dt * .6f;
}

void vis_scan(uint16_t *px, int y0, int h, int dim0, int dim1)
{
    static const int8_t BAYER[4] = {-3, 1, 3, -1};
    int y, x, bump = flash > 0.0f ? (int)(flash * 90.0f) : 0;
    int cyc = tr_w > .05f ? (int)(tr_w * 140.0f * (.5f + .5f * sn(vt * 1.7f))) : 0;   /* during a change: the colours run */
    for (y = y0; y < y0 + h; y++) {
        const uint8_t *row = A + (y >> 1) * VW;
        int by = (y & 1) << 1, dim = y >= dim0 && y < dim1;
        for (x = 0; x < 240; x++) {
            int v = row[x >> 1];
            if (v)
                v += BAYER[by | (x & 1)];
            if (cyc && v > 14) {
                v += cyc;
                if (v > 255)
                    v -= 242;
            }
            v += bump;
            v = v < 0 ? 0 : v > 255 ? 255 : v;
            *px++ = lut[dim ? v >> 2 : v];
        }
    }
}
