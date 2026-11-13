/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1: the engine (hadal.h). One compilation unit, -O2. Built on the shape of Charles Vestal's AMB-1 engine:
 * a command queue, a block render, xorshift random numbers, and the Bjorklund-free slow motion of a garden left alone. */
#include "hadal.h"
#include "fastmath.h"
#include "plat.h"

#if defined EN_HOST || defined PLAT_HOST
#define EN_POOL
#else
#define EN_POOL __attribute__((section(".pool")))
#endif
#define BARRIER() __asm__ volatile("" ::: "memory")
#define TWO_PI 6.2831853f
#define BLK 256
#define CTL_DT ((float)BLK / EN_SR)

typedef struct { const char *name; float r[7], d[7], g[7], att, lo, hi, bend; } bell_t;
typedef struct { const char *name; int wt; float fm, lp, f1, q1, a1, f2, q2, a2, sw; int rel; float g; } drone_t;
typedef struct { const char *name; int wt; float rt[3], lo, hi, cut, f1, q1, f2, q2, vib, vr, br, glide; int res; float g; } choir_t;
typedef struct {
    const char *name; int v, custom; float trim, life0, life1; int wave;
    float ss, n, imp, tw, ts, f0, f1, to0, to1; int arch;
    float r1f, r1q, r2f, r2q, r1to, rw, am, amd, rate0, rate1, rto, jit; int dbl;
    float bdec, syll0, syll1, fld0, fld1, c00, c01, c10, c11, chk; int perc; float gain;
} sfx_t;
typedef struct { const char *name, *from; uint8_t fam[2], bell[4], drone[3], choir[4], sfx[10], pal[2]; float w[6]; } sea_t;
#include "world_tables.h"
#include "fx_banks.h"

/* Every sea is its own instrument: its own composer, its own balance, its own room. */
enum { LEAD_NONE = -1, LEAD_HORN, LEAD_CLAY, LEAD_FLUTE, LEAD_SYNTH, LEAD_REED, LEAD_PIPE };
enum { ST_SWELL, ST_CHORALE, ST_CHANT, ST_RITUAL, ST_SHIMMER, ST_MA, ST_COLOTOMIC, ST_SEQUENCE, ST_PULSAR, ST_PROCESSION,
       ST_RIVER, ST_DIRGE, ST_MONITOR, ST_MACHINE, ST_ROOTS };
typedef struct {
    int style;
    float bpm, drone, noise, bell, choir, fx, lead, drum;
    float t60, damp, wet, tape_fb, tape_mix, ghost, fx_min, strata, cut;
    int lead_voice, root;
} world_t;
static const world_t WORLD[15] = {
    /* style          bpm drone noise bell choir fx   lead drum t60   damp  wet  tapefb tape ghost fx/m strata cut lead      root */
    {ST_SWELL,        0,  1.25f,.25f, .65f, .6f, .6f, 0,   0,   1.3f, 1400, 1.05f,.9f, .6f, .9f, 1.4f, 1.5f, .8f, LEAD_NONE,  -3},
    {ST_CHORALE,      0,  1.05f,.08f, .95f, .9f, .4f, 0,   0,   1.7f, 2000, 1.3f, .6f, .35f,.4f, .8f,  1.3f, 1.1f,LEAD_NONE,  0},
    {ST_CHANT,        0,  1.2f, .08f, .95f, .8f, .4f, .55f,0,   1.3f, 1700, 1.1f, .6f, .35f,.35f,.7f,  1.5f, .95f,LEAD_HORN,  -2},
    {ST_RITUAL,       0,  1.0f, .2f,  .6f,  .4f, .6f, .35f,.45f,.9f,  1800, .9f,  .9f, .6f, .7f, 1.5f, 1.2f, .85f,LEAD_CLAY,  -1},
    {ST_SHIMMER,      0,  .8f,  .14f, 1.0f, .6f, .5f, .22f,0,   1.8f, 4600, 1.25f,.8f, .6f, .5f, 1.1f, 1.4f, .7f, LEAD_FLUTE, 5},
    {ST_MA,           0,  .75f, .04f, 1.0f, .5f, .3f, .35f,0,   1.6f, 2200, 1.1f, .5f, .25f,.25f,.5f,  1.7f, .6f, LEAD_FLUTE, 2},
    {ST_COLOTOMIC,    0,  .95f, .08f, 1.0f, .4f, .3f, 0,   .2f, 1.0f, 2400, .9f,  .5f, .35f,.3f, .7f,  1.3f, .9f, LEAD_NONE,  -1},
    {ST_SEQUENCE,     0,  1.0f, .08f, .55f, .4f, .6f, .4f, .2f, .8f,  2800, .75f, 1.1f,.8f, .5f, 1.8f, 1.1f, 1.1f,LEAD_SYNTH, 0},
    {ST_PULSAR,       0,  .95f, .03f, .85f, .5f, .5f, .3f, .3f, 1.9f, 1500, 1.3f, .7f, .55f,.7f, .9f,  1.6f, .55f,LEAD_SYNTH, -4},
    {ST_PROCESSION,   0,  1.05f,.08f, .75f, .6f, .35f,.45f,.4f, 1.1f, 1900, 1.0f, .6f, .35f,.45f,.9f,  1.3f, .85f,LEAD_REED,  -2},
    {ST_RIVER,        0,  .95f, .08f, 1.0f, .6f, .35f,.3f, 0,   1.3f, 2600, 1.05f,.6f, .35f,.5f, .9f,  1.3f, .8f, LEAD_FLUTE, 3},
    {ST_DIRGE,        0,  1.3f, .1f,  .7f,  .7f, .45f,.2f, .6f, 1.0f, 1100, .9f,  .7f, .45f,.85f,1.0f, 1.5f, .7f, LEAD_REED,  -5},
    {ST_MONITOR,      0,  .9f,  .04f, .9f,  .4f, .55f,.2f, .25f,.8f,  3000, .8f,  .5f, .45f,.55f,1.4f, 1.2f, 1.0f,LEAD_SYNTH, 2},
    {ST_MACHINE,      0,  1.2f, .12f, .75f, .4f, .6f, .15f,.5f, .9f,  2000, .8f,  .9f, .7f, .7f, 1.8f, 1.1f, 1.15f,LEAD_SYNTH,-3},
    {ST_ROOTS,        0,  .9f,  .14f, .7f,  .5f, .6f, .35f,.45f,1.1f, 1700, 1.0f, .6f, .45f,.7f, 1.4f, 1.3f, .8f, LEAD_PIPE,  -1},
};
/* what no other world has: its own two-note drone chord, the two resonances of the body the whole sound passes through,
 * how open that body is, and how the drone breathes. The rest of a world wanders; this is what stays. */
static const struct { float r[2], f1, g1, f2, g2, tilt, br, bd; } SIG[15] = {
    {{1.0f, 1.06f},  95, 2.2f,  240, 1.5f, 1300, .05f, .35f},    /* HADAL: a grinding second, the pressure of the deep */
    {{1.0f, 1.5f},  340, 2.0f,  880, 1.6f, 3000, .03f, .15f},    /* CATHEDRAL: an open fifth in stone */
    {{1.0f, 3.0f},  210, 2.4f,  560, 2.0f, 2000, .18f, .45f},    /* BARDO: root and twelfth, breathed */
    {{1.0f, 1.414f},175, 1.5f, 2050, 2.6f, 3800, 1.1f, .35f},     /* MICTLAN: the tritone, trembling */
    {{2.0f, 4.24f}, 1150,2.6f, 2500, 2.0f, 7000, 3.0f, .2f},     /* NIFLHEIM: thin, high, shivering */
    {{2.0f, 3.0f},  470, 2.8f, 1050, 1.0f, 1700, .07f, .5f},     /* YOMI: hollow and far off */
    {{1.0f, 1.31f}, 225, 1.8f, 1400, 2.8f, 4200, 2.1f, .45f},     /* PATALA: a wide pelog fourth, beating like a gong */
    {{3.0f, 3.03f}, 430, 1.0f, 1900, 3.0f, 5000, 6.0f, .25f},    /* SIGNAL: two tones not quite agreeing, a radio's throat */
    {{.5f, 5.03f},   75, 2.6f, 3100, 2.0f, 8000, .02f, .4f},     /* HELIOPAUSE: nothing between the bottom and the top */
    {{1.5f, 2.25f}, 320, 2.2f, 1250, 2.2f, 3500, .4f,  .35f},    /* DUAT: stacked fifths in a reed */
    {{1.0f, 1.2f},  200, 2.4f,  820, 1.4f, 2500, .25f, .3f},     /* TUONELA: a minor third, under water */
    {{.5f, .75f},    62, 3.0f,  330, 2.0f,  900, .12f, .4f},     /* KURNUGI: all of it in the ground */
    {{4.0f, 4.76f}, 760, 2.6f, 1500, 1.0f, 2600, 1.0f, .3f},     /* WARD 9: a sterile third, at the pace of a pulse */
    {{1.0f, 2.83f}, 135, 2.4f, 2900, 3.0f, 9000, 4.0f, .35f},     /* FOUNDRY: an iron hum and a shriek above it */
    {{1.33f, 2.0f}, 105, 2.4f,  520, 2.8f, 1500, .6f,  .4f},    /* HOLLOW: wood and air */
};
#define W (&WORLD[sea])
static const char *const STRATA_NAME[EN_NSTRATA] = {"ABYSS", "PROCESSION", "REQUIEM", "SWARM", "WRECK", "VOID"};
static const float STRATA[EN_NSTRATA][5] = {
    {1.00f, .14f, .12f, .00f, .20f}, {.72f, 1.00f, .18f, .22f, .30f}, {.62f, .28f, .14f, 1.00f, .40f},
    {.55f, .14f, 1.00f, .18f, .55f}, {.72f, .42f, .72f, .22f, 1.00f}, {.26f, .10f, .07f, .00f, .75f}};
static const float MR[6] = {1.0f, .5f, 2.0f, 1.5f, 1.414f, 2.76f};

const char *en_sea_name(int i) { return SEAS[(unsigned)i % NSEA].name; }
const char *en_sea_from(int i) { return SEAS[(unsigned)i % NSEA].from; }
const char *en_bell_name(int i) { return BELLS[(unsigned)i % NBELL].name; }
const char *en_drone_name(int i) { return DRONES[(unsigned)i % NDRONE].name; }
const char *en_choir_name(int i) { return CHOIRS[(unsigned)i % NCHOIR].name; }
const char *en_sfx_name(int i) { return SFX[(unsigned)i % NSFX].name; }
const char *en_stratum_name(int i) { return STRATA_NAME[(unsigned)i % EN_NSTRATA]; }
const char *en_family_name(int i) { return FAM_NAME[(unsigned)i % NFAM]; }
/* ------------------------------------------------------------ commands --- */
enum { C_MACRO, C_LAYER, C_TIDE, C_SEA, C_DRIFT, C_PICK, C_CAST, C_CASTW, C_FIRE, C_PLAY, C_FREEZE, C_OMEN, C_MUTATE,
       C_UPHEAVAL, C_OFFER, C_KEEP, C_REVIVE, C_PANIC, C_SEED, C_OUTPUT, C_HOLD, C_HELDW, C_EVOLVE, C_FAV, C_FADE, C_SHAPE, C_SWELL, C_DUCK, C_KEY, C_KEYS, C_LETGO, C_SCENE };
#define QN 64u
typedef struct { uint16_t c; int16_t a; int32_t b; } cmd_t;
static cmd_t q[QN];
static volatile uint32_t q_w, q_r;
static void post(int c, int a, int32_t b)
{
    uint32_t w = q_w;
    if (w - q_r >= QN)
        return;
    q[w % QN].c = (uint16_t)c;
    q[w % QN].a = (int16_t)a;
    q[w % QN].b = b;
    BARRIER();
    q_w = w + 1;
}
void en_macro(int m, float v) { post(C_MACRO, m, (int32_t)(v * 4096.0f)); }
void en_layer(int l, float v) { post(C_LAYER, l, (int32_t)(v * 4096.0f)); }
void en_tide(float v) { post(C_TIDE, 0, (int32_t)(v * 4096.0f)); }
void en_sea(int i) { post(C_SEA, i, 0); }
void en_sea_drift(int on) { post(C_DRIFT, on, 0); }
void en_pick(int w, int i) { post(C_PICK, w, i); }
void en_cast_set(int i, int on) { post(C_CAST, i, on); }
void en_cast_all(const uint32_t *m) { int i; for (i = 0; i < EN_MASK; i++) post(C_CASTW, i, (int32_t)m[i]); }
void en_fire(int i) { post(C_FIRE, i, 0); }
void en_fire_soft(int i) { post(C_FIRE, i, 1); }   /* the panic's breath: quieter than a played effect */
void en_play(int on) { post(C_PLAY, on, 0); }
void en_freeze(int on) { post(C_FREEZE, on, 0); }
void en_omen(void) { post(C_OMEN, 0, 0); }
void en_mutate(void) { post(C_MUTATE, 0, 0); }
void en_upheaval(void) { post(C_UPHEAVAL, 0, 0); }
void en_offering(int s, int o) { post(C_OFFER, s, o); }
void en_key(int k, int semi, int oct, int on) { post(C_KEY, k | (oct + 4) << 8 | (on ? 1 : 0) << 12, semi); }
void en_keys(int mode, int voice, int bell, int level, int fade) { post(C_KEYS, mode | voice << 4, level | fade << 8 | bell << 16); }
void en_panic(void) { post(C_PANIC, 0, 0); }
void en_letgo(void) { post(C_LETGO, 0, 0); }
void en_scene(int s) { post(C_SCENE, s + 1, 0); }
void en_output(int on) { post(C_OUTPUT, on, 0); }
void en_hold(int i, int on) { post(C_HOLD, i, on); }
void en_held_all(const uint32_t *m) { int i; for (i = 0; i < EN_MASK; i++) post(C_HELDW, i, (int32_t)m[i]); }
void en_evolve(int v) { post(C_EVOLVE, v, 0); }
void en_favorites(uint32_t m) { post(C_FAV, 0, (int32_t)m); }
void en_fade(float seconds, float to) { post(C_FADE, (int)(to * 100.0f), (int32_t)(seconds * 100.0f)); }
void en_seed(uint32_t s) { post(C_SEED, 0, (int32_t)s); }
void en_shape(int i, int v) { post(C_SHAPE, i, v); }
static int usb_split;
static int32_t usb_spk[BLK * 2];
void en_usb_split(int on) { usb_split = on; }
void en_after_tap(int32_t *lr, uint32_t n)
{
    uint32_t i;
    if (usb_split)
        for (i = 0; i < 2u * n && i < BLK * 2u; i++)
            lr[i] = usb_spk[i];
}
void en_swell(int on) { post(C_SWELL, on, 0); }
void en_duck(int on) { post(C_DUCK, on, 0); }
static uint32_t boot_entropy;
void en_entropy(uint32_t s) { boot_entropy = s; }
static en_fossil_t *keep_to;
static const en_fossil_t *revive_from;
static volatile uint8_t keep_done;
void en_keep(en_fossil_t *f) { keep_to = f; keep_done = 0; post(C_KEEP, 0, 0); }
int en_keep_ready(void) { return keep_done; }
void en_revive(const en_fossil_t *f) { revive_from = f; post(C_REVIVE, 0, 0); }

/* for the screen */
volatile en_state_t en_st;
#define EVN 16u
static en_ev_t evq[EVN];
static volatile uint32_t ev_w, ev_r;
static void ev_note(int kind, int note, int vel)   /* for MIDI out: kind 0 bell, 1 ground, 2 pad, 3 scene */
{
    uint32_t w = ev_w;
    if (w - ev_r >= EVN || note < 0 || note > 127)
        return;
    evq[w % EVN].type = EV_NOTE;
    evq[w % EVN].a = (uint8_t)note;
    evq[w % EVN].x = (uint8_t)vel;
    evq[w % EVN].y = (uint8_t)kind;
    BARRIER();
    ev_w = w + 1;
}
static int hz_note(float hz) { return (int)(69.5f + 12.0f * fm_log2f(fm_maxf(hz, 8.0f) / 440.0f)); }
static void ev_put(int t, float x, float y, float a)
{
    uint32_t w = ev_w;
    if (w - ev_r >= EVN)
        return;
    evq[w % EVN].type = (uint8_t)t;
    evq[w % EVN].x = (uint8_t)(x * 255.0f);
    evq[w % EVN].y = (uint8_t)(y * 255.0f);
    evq[w % EVN].a = (uint8_t)(fm_clampf(a, 0.0f, 1.0f) * 255.0f);
    BARRIER();
    ev_w = w + 1;
}
int en_ev_get(en_ev_t *e)
{
    uint32_t r = ev_r;
    if (r == ev_w)
        return 0;
    *e = evq[r % EVN];
    BARRIER();
    ev_r = r + 1;
    return 1;
}

/* ------------------------------------------------------ random numbers --- */
static uint32_t rng = 0x9E3779B9u, rng2 = 0x2545F491u, rng_ui = 0x6C078965u;
static inline uint32_t xs(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}
static inline float fr(void) { return (float)(xs(&rng) >> 8) * (1.0f / 16777216.0f); }
static inline int ri(int n) { return n > 0 ? (int)((xs(&rng) >> 8) % (uint32_t)n) : 0; }
static inline float n1(void) { return fr() * 2.0f - 1.0f; }
static inline float nz2(void) { return (float)(int32_t)xs(&rng2) * (1.0f / 2147483648.0f); }   /* the second core's */
static inline float fr2(void) { return (float)(xs(&rng2) >> 8) * (1.0f / 16777216.0f); }
uint32_t en_rand(void) { return xs(&rng_ui); }
static float between(float a, float b) { return a + fr() * (b - a); }

/* ---------------------------------------------------------- wave tables --- */
#define TN 1024
enum { W_SINE, W_SAW, W_PULSE, W_ODD, W_ORGAN, W_BRASS, W_GLOT, NW };
static int16_t wt[NW][TN + 1] EN_POOL;
static const float W2Q = 1.0f / 32767.0f;
static inline float rdw(const int16_t *t, float p)
{
    float x;
    int i;
    p -= (float)(int)p;
    if (p < 0.0f)
        p += 1.0f;
    x = p * (float)TN;
    i = (int)x;
    return ((float)t[i] + (float)(t[i + 1] - t[i]) * (x - (float)i)) * W2Q;
}
static float harm(int w, int n)
{
    static const float ORG[12] = {1, .62f, .3f, .34f, 0, .16f, 0, .12f, 0, .05f, 0, .05f};
    float fn = (float)n;
    switch (w) {
    case W_SAW: return 1.0f / fn;
    case W_PULSE: return fm_powf(fn, -.75f);
    case W_ODD: return (n & 1) ? 1.0f / fn : .06f / fn;
    case W_ORGAN: return n <= 12 ? ORG[n - 1] : 0.0f;
    case W_BRASS: return n <= 30 ? fm_expf(-(fn - 5.0f) * (fn - 5.0f) / 26.0f) + .7f / fn : 0.0f;
    case W_GLOT: return n <= 32 ? fm_powf(fn, -1.15f) : 0.0f;   /* a throat's buzz: every harmonic, falling gently */
    default: return n == 1 ? 1.0f : 0.0f;
    }
}
static void tables_init(void)
{
    static float tmp[TN];
    int w, i, n;
    for (w = 0; w < NW; w++) {
        float peak = 1e-6f;
        float g[44];
        for (n = 0; n < 44; n++)
            g[n] = harm(w, n + 1);
        for (i = 0; i < TN; i++) {
            float x = (float)i / (float)TN * TWO_PI, s = 0.0f, c1 = fm_cosf(x), s0 = 0.0f, s1 = fm_sinf(x);
            /* the harmonics by the recurrence sin((n+1)x) = 2 cos x sin nx - sin((n-1)x) */
            for (n = 0; n < 44; n++) {
                s += g[n] * s1;
                {
                    float t = 2.0f * c1 * s1 - s0;
                    s0 = s1;
                    s1 = t;
                }
            }
            tmp[i] = s;
            peak = fm_maxf(peak, fm_fabsf(s));
        }
        for (i = 0; i < TN; i++)
            wt[w][i] = (int16_t)(tmp[i] / peak * 32000.0f);
        wt[w][TN] = wt[w][0];
    }
}
#define SINE(p) rdw(wt[W_SINE], (p))

/* -------------------------------------------------------------- world --- */
static float macro_u[N_MACRO] = {.62f, .34f, .3f, .72f}, macro_m[N_MACRO], touched[N_MACRO];
static float lv[N_LAYER] = {1, 1, 1, 1}, tide = .65f, t_now;
static int playing = 1, frozen, sea_drift = 1;
static int sea, family, palette, era = 1, stratum, prev_stratum = -1;
static int root_offset;
static float root_hz, field[6], field_t[6];
static float trans[EN_NSTRATA][EN_NSTRATA];
static float st[5];
static float s_left, field_left, era_left, bell_left, fauna_left;
static int8_t pick[3] = {-1, -1, -1};
static uint8_t b_type, d_type, d_next, c_type;
static float d_mix;
static uint32_t cast[EN_MASK], held[EN_MASK], favorites;
static float evolve = 1.0f, fade_g = 1.0f, fade_to = 1.0f, fade_step;
static float held_gap[EN_NFA];
#define CAST_ON(i) (cast[(i) >> 5] >> ((i) & 31) & 1u)
static struct { float v, tgt, left; } wob[N_MACRO][3];
static int toll_n, toll_oct;
static float toll_gap, toll_ratio;
static uint32_t n_bells, n_born, n_strata, seas_seen;

/* the SHAPE page. All of it at the centre leaves the world exactly as it is. */
static const float CHORD[EN_NCHORD][2] = {{1, 1.0102f}, {1, 1.0595f}, {1, 1.1892f}, {1, 1.2599f}, {1, 1.3348f}, {1, 1.4142f}, {1, 1.4983f}, {1, 2}, {1, 3}};
static const char *const CHORD_NAME[EN_NCHORD] = {"UNISON", "MINOR 2ND", "MINOR 3RD", "MAJOR 3RD", "4TH", "TRITONE", "5TH", "OCTAVE", "12TH"};
const char *en_chord_name(int i) { return CHORD_NAME[(unsigned)i % EN_NCHORD]; }
static float sh_size = 1.0f, sh_dark = 1.0f, sh_space = 1.0f, sh_tape = .5f, sh_wob = 1.0f, sh_bells = 1.0f, sh_fx = 1.0f, sh_width = 1.0f, sh_pitch = 1.0f;
static int sh_chord, sh_key, sh_mood;
static int kd_shift, k_mode, k_vox, k_bell, k_lv = 70, k_fade = 50;
/* A world moves by itself: its ground walks through the scale, the drone's chord leans to new intervals, pads follow,
 * and every scene puts something else in front. Nothing a world does stays the same for long. */
static float walk_st, walk_left = 20.0f, dch = 1.5f, dch_t = 1.5f, bloom_ph, sc_mix[6] = {1, 1, 1, 1, 1, 1}, scn_left = 25.0f;
static float scene_fast, kb_room, dr_left = 90.0f;
static int16_t auto_fx = -1;
static uint16_t auto_seq;
static int busy_n, spk_on;               /* latched voices + kept effects: how full it is; the speaker on */
/* the speaker, when it is full: the bells it hides most (measured through its own chain) come up the most */
static const float SPK_LIFT[EN_NBELL] = {1.00f, 1.28f, 1.30f, 1.11f, 1.85f, 2.00f, 1.25f, 1.24f, 1.43f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.18f, 1.35f, 1.00f, 1.00f, 1.34f, 1.00f, 1.21f, 1.06f, 1.11f, 1.00f, 1.00f, 1.00f, 1.00f, 1.39f, 1.10f, 1.01f, 1.03f, 1.00f, 1.00f, 1.09f, 1.00f, 1.00f, 1.00f, 1.02f, 1.00f, 1.00f};
static int scene, scene_lock = -1, pd_semi[3] = {0, 3, 7}, stress;
static float wduck = 1.0f, wduck2 = 1.0f;   /* under stress the world's choir steps back, its back rows to silence */
static const float SCENE[6][6] = {       /* drone, pads, choir, bells, effects, brightness */
    {.7f, .35f, .25f, 1.6f, .6f, 1.0f},  /* bells alone */
    {.8f, 1.0f, 1.8f, .7f, .5f, 1.35f},  /* the choir rises */
    {1.0f, .5f, 1.5f, .5f, .6f, .8f},    /* chant */
    {1.3f, .55f, .6f, .45f, 1.7f, .55f}, /* the dark */
    {.5f, .85f, .3f, .4f, .4f, .9f},     /* emptiness */
    {1.0f, 1.4f, 1.0f, 1.25f, 1.0f, 1.7f}};/* bloom */
static float root_target(void)
{
    if (sh_key) {                        /* a chosen key stays in tune: the depth moves it by whole octaves */
        int n = sh_key - 10;
        if (n < -6)
            n += 12;
        return 55.0f * fm_exp2f(((float)(n + kd_shift) + walk_st) / 12.0f - fm_floorf(macro_m[M_DEPTH] * .85f + .5f));
    }
    return 55.0f * fm_exp2f(((float)(root_offset + W->root + kd_shift) + walk_st) / 12.0f - macro_m[M_DEPTH] * .85f);
}
static uint8_t list_pick(const uint8_t *a, int n)
{
    int c = 0;
    while (c < n && a[c] != 255)
        c++;
    return a[ri(c ? c : 1)];
}
static float make_ratio(void)
{
    if (FAM_N[family] == 0)
        return 16.0f / (float)UNDER_N[ri(13)];
    return fm_exp2f(FAM_ST[family][ri(FAM_N[family])] / 12.0f);
}
static void new_field(int now)
{
    int i;
    if (sh_mood)
        family = (sh_mood - 1) % NFAM;
    for (i = 0; i < 6; i++) {
        float v = i == 0 ? 1.0f : i == 1 ? (family == 3 ? 1.41421f : .75f) : make_ratio();
        field_t[i] = v;
        if (now)
            field[i] = v;
    }
}
static float pitch(int i, float lo, float hi)
{
    float f = root_hz * field[i % 6];
    while (f < lo)
        f *= 2.0f;
    while (f > hi)
        f *= .5f;
    return f;
}
static void load_cast(void)
{
    int k;
    __builtin_memset(cast, 0, sizeof cast);
    for (k = 0; k < 10; k++)
        if (SEAS[sea].sfx[k] != 255)
            cast[SEAS[sea].sfx[k] >> 5] |= 1u << (SEAS[sea].sfx[k] & 31);
    {                                    /* (the mask is empty since 0.99.9: every effect may come by itself) */
        static const uint32_t ODD[EN_MASK] = ODD_MASK;
        for (k = 0; k < EN_MASK; k++)
            cast[k] &= ~ODD[k];
    }
    if (sea == 5)                        /* the worlds that leaned on them get darker company */
        cast[0] |= (1u << 9) | (1u << 11) | (1u << 12);
    else if (sea == 7)
        cast[1] |= (1u << 18) | (1u << 20);
    else if (sea == 14)
        cast[1] |= (1u << 5) | (1u << 6) | (1u << 7);
}
static void next_stratum(void)
{
    float sum = 0.0f, x, w[EN_NSTRATA];
    const float *sw = SEAS[sea].w;
    float *row = trans[stratum];
    int i, n = 0;
    for (i = 0; i < EN_NSTRATA; i++) {
        w[i] = (i == prev_stratum ? .25f : 1.0f) * row[i] * sw[i] *
               (1.0f + macro_m[M_UNREST] * (i == 3 || i == 4 ? 1.5f : -.3f));
        sum += w[i];
    }
    x = fr() * sum;
    for (i = 0; i < EN_NSTRATA; i++) {
        x -= w[i];
        if (x <= 0.0f) {
            n = i;
            break;
        }
    }
    row[n] = fm_clampf(row[n] * (.72f + fr() * .5f), .02f, 2.0f);   /* a path walked wears: the map itself mutates */
    prev_stratum = stratum;
    stratum = n;
    s_left = (70.0f + fr() * fr() * 460.0f) * (1.25f - macro_m[M_UNREST] * .6f) * W->strata;
    n_strata++;
    ev_put(EV_STRATUM, 0, 0, 0);
    /* the colours move with the phases: mostly the world's own, sometimes a stranger's */
    palette = fr() < .55f ? list_pick(SEAS[sea].pal, 2) : ri(15);
}
#define BY_Q .22f
static float sw_on, sw, sw_tail;          /* the swell, and the room left ringing after it */
static float breath = 1.0f, br_ph, by_c[2], by_g[2], by_t = 1.0f, by_s[2][5];
static int mo_rate = 2, mo_len = 1, mo_voice, mo_n, mo_i, mo_use = -1, mo_oct, mo_drop = -1, mo_inv, mo_bell;
static float mo_left = 60.0f, mo_t, mo_gap = 2.0f;
static int mo_voiceb = -1, mo_twice, mo_striking;   /* the motif's own bell; play it twice; a motif note is sounding */
static uint8_t mo_seed = 1;
static void motif_make(void);
static int8_t mo_deg[6];
static float mo_dur[6];
static float sc_t = 3.0f, sc_gap, sc_rest, swell = 1.0f;
static int sc_n, sc_i, sc_deg;
static void set_sea(int i)
{
    const sea_t *z;
    sea = i;
    z = &SEAS[i];
    seas_seen |= 1u << i;
    family = list_pick(z->fam, 2);
    new_field(0);
    palette = list_pick(z->pal, 2);
    if (pick[1] < 0)
        d_next = list_pick(z->drone, 3);   /* any of the world's own: a different opening each time */
    b_type = pick[0] >= 0 ? (uint8_t)pick[0] : list_pick(z->bell, 4);
    c_type = pick[2] >= 0 ? (uint8_t)pick[2] : list_pick(z->choir, 4);
    load_cast();
    walk_st = 0.0f;
    walk_left = 14.0f + fr() * 20.0f;
    dch_t = SIG[i].r[1] / SIG[i].r[0];
    scn_left = 15.0f + fr() * 25.0f;
    era_left = 900.0f + fr() * 1800.0f;
    sc_n = 0;
    sc_t = 2.0f;
    toll_n = 0;
    mo_seed = (uint8_t)(1 + ri(255));
    motif_make();
    mo_use = -1;
    mo_left = 45.0f + fr() * 60.0f;
    ev_put(EV_SEA, 0, 0, 0);
}
static void upheaval(int automatic)
{
    const sea_t *z = &SEAS[sea];
    int a, b;
    era++;
    if (automatic && sea_drift && fr() < .6f) {
        int n = -1, tries = 40;
        while (tries-- > 0) {            /* a new sea, among the favourites if there are any */
            int c = ri(NSEA);
            if (c != sea && (!favorites || (favorites >> c & 1u))) {
                n = c;
                break;
            }
        }
        if (n >= 0)
            set_sea(n);
    } else {
        if (z->fam[1] != 255)
            family = z->fam[z->fam[0] == family ? 1 : 0];
        new_field(0);
        palette = list_pick(z->pal, 2);
        if (pick[1] < 0)
            d_next = list_pick(z->drone, 3);   /* any of the world's own: a different opening each time */
        b_type = pick[0] >= 0 ? (uint8_t)pick[0] : list_pick(z->bell, 4);
        c_type = pick[2] >= 0 ? (uint8_t)pick[2] : list_pick(z->choir, 4);
        era_left = 900.0f + fr() * 1800.0f;
    }
    root_offset = ri(5) - 2;
    for (a = 0; a < EN_NSTRATA; a++)
        for (b = 0; b < EN_NSTRATA; b++)
            if (a != b)
                trans[a][b] = fm_clampf(trans[a][b] * (.45f + fr() * 1.1f) + .03f, .02f, 2.0f);
    next_stratum();
    ev_put(EV_UPHEAVAL, 0, 0, 0);
}

/* ------------------------------------------------------------- voices --- */
#define NB 8
#define NP 7
static struct {
    float re[NP], im[NP], c[NP], s[NP], d[NP], w[NP];
    float env, att, bend, age, pan, g0, g1, kt;
    uint8_t on, key;                     /* key: struck from the keys, at the keys' own level */
} bl[NB];
static int b_next;

/* each bell's level on the keys, so all thirty sit together (measured: tests levels, keys) */
static const float KB_TRIM[EN_NBELL] = {1.19f, 1.56f, 0.91f, 0.97f, 1.36f, 0.93f, 0.63f, 0.54f, 0.88f, 0.70f, 0.79f, 1.61f, 1.00f, 0.56f, 0.51f, 0.52f, 0.47f, 0.67f, 0.69f, 1.48f, 1.79f, 0.63f, 0.79f, 2.07f, 0.70f, 1.02f, 0.65f, 1.02f, 1.77f, 1.44f, 0.54f, 1.25f, 0.82f, 0.76f, 1.64f, 0.81f, 0.78f, 0.83f, 2.00f, 0.84f, 0.53f};
static void strike_bell(int which, float ratio, int oct, float a)
{
    if (mo_use >= 0 && !mo_striking && which < 1000)
        a *= .35f;                       /* the motif is playing: the world's other bells step back for it */
    const bell_t *B;
    float dark = 1.0f - macro_m[M_DEPTH] * .55f, f, stretch;
    int i, v;
    int key = which >= 1000;             /* from the keys: its own bell, its own level, never cut by the sea's */
    if (key)
        which -= 1000;
    b_next = (b_next + 1) % NB;
    v = b_next;
    if (!key)                            /* the sea's bells leave ringing key bells alone while others are free */
        for (i = 0; i < NB && bl[v].on && bl[v].key; i++)
            v = b_next = (b_next + 1) % NB;
    if (which < 0 && pick[0] < 0 && fr() < .22f)
        b_type = fr() < .35f ? list_pick(SEAS[sea].bell, 4) : (uint8_t)ri(NBELL);   /* a light lean, else any bell */
    B = &BELLS[key && which >= 0 ? which : pick[0] >= 0 ? pick[0] : which >= 0 ? which : b_type];
    f = root_hz * ratio * fm_exp2f((float)oct);
    while (f < B->lo)
        f *= 2.0f;
    while (f > B->hi)
        f *= .5f;
    while (f * B->r[0] < (key ? 150.0f : 55.0f))   /* the lowest partial stays where ears and the FM-1 can hear it: */
        f *= 2.0f;                                 /* below that it is only rumble, and the drones cover it */
    stretch = 1.0f + fr() * .009f;
    bl[v].pan = n1() * .7f;
    {
        float att = key ? fm_minf(B->att, .04f) : B->att;   /* struck from the keys: it speaks at once, never a slow swell */
        bl[v].env = att > 0.0f ? 0.0f : 1.0f;
        bl[v].att = att > 0.0f ? CTL_DT / att : 0.0f;
    }
    bl[v].bend = B->bend * n1();
    bl[v].age = fr() * 6.0f;
    for (i = 0; i < NP; i++) {
        float fp = fm_minf(f * B->r[i] * (i > 3 ? stretch : 1.0f), EN_SR * .42f), w = TWO_PI * fp / EN_SR;
        float T = B->d[i] * (.8f + fr() * .5f) * (1.0f + macro_m[M_DEPTH] * .35f);
        bl[v].w[i] = w;
        bl[v].c[i] = fm_cosf(w);
        bl[v].s[i] = fm_sinf(w);
        bl[v].d[i] = fm_expf(-6.9f / (T * EN_SR));
        bl[v].re[i] = a * B->g[i] * (i > 3 ? dark : 1.0f) * (key && i > 2 ? (fp > 2500.0f ? .5f : .65f) : 1.0f) + bl[v].re[i] * .3f;   /* the keys' bells lean darker */
        bl[v].im[i] *= .3f;
    }
    bl[v].on = 1;
    bl[v].key = (uint8_t)key;
    bl[v].kt = KB_TRIM[B - BELLS];
    if (key && spk_on && busy_n >= 3)    /* the speaker with a lot going on: the keys' bells stay on top */
        bl[v].kt *= 1.3f * SPK_LIFT[B - BELLS];
    if (stress) {                        /* everything latched and held at once: only the five loudest bells ring */
        int n = 0, j, faint = -1;
        float fa2 = 1e9f;
        for (j = 0; j < NB; j++)
            n += bl[j].on;
        while (n > 4) {
            for (j = 0; j < NB; j++)
                if (bl[j].on && j != v) {
                    float lv2 = (fm_fabsf(bl[j].re[0]) + fm_fabsf(bl[j].re[1]) + fm_fabsf(bl[j].re[2])) * bl[j].env;
                    if (lv2 < fa2) {
                        fa2 = lv2;
                        faint = j;
                    }
                }
            if (faint < 0)
                break;
            bl[faint].on = 0;
            fa2 = 1e9f;
            faint = -1;
            n--;
        }
    }
    n_bells++;
    ev_put(EV_BELL, .15f + fr() * .7f, .12f + fr() * .62f, fm_minf(1.0f, a * 1.5f) );
    ev_note(0, hz_note(f), 30 + (int)(fm_minf(1.0f, a * 1.5f) * 90.0f));
}

static void strike(float ratio, int oct, float a) { strike_bell(-1, ratio, oct, a); }

typedef struct {
    float ph, ph2, phm, f, beat, mr, lp[2], amp, amp0, pan, w, wr, b[4], sw;
    int idx;
} drone_v;
static drone_v dr[EN_NDR];
static const float DR_PAN[EN_NDR] = {-.2f, .2f, -.55f, .55f};

typedef struct {
    float p[3], f, tgt, t, len, lp1, lp2, pan, amp, amp0, b[4], low, band, vph;
    int idx, type;
    float fz[10], vow, vtg, vt, gz, lz, sp;   /* a sung voice: its formants, its vowel, its breath, its undertone */
    uint8_t vsi;
} choir_v;
static choir_v ch[EN_NCH];
static const float CH_PAN[EN_NCH] = {-.55f, -.2f, .2f, .55f};

#define COMBN 2048
static int16_t comb_mem[2][COMBN] EN_POOL;
typedef struct {
    const sfx_t *r;
    uint8_t on, kind, forced, dn;
    int comb;
    float xe, t, life, ph, ph2, ph3, f0, f1, x, y, vx, vy, a, a0, b1, b2, d1, d2, lp, clk, clkR, pan, bt, bc, be, cf0, cf1, amp, r1f, rt, born;
    float shep[6], shp[6];
    int cw;
} fauna_v;
static fauna_v fa[EN_NFA];
/* each effect's level when you play it, so none hides and none jumps out (measured alone: tests levels, effects) */
static const float FX_LV[EN_NSFX] = {3.50f, 1.81f, 3.50f, 2.80f, 2.34f, 1.85f, 1.02f, 2.12f, 1.37f, 1.77f, 3.50f, 2.05f, 2.15f, 1.86f, 1.00f, 3.50f, 1.17f, 3.50f, 3.50f, 0.90f, 1.55f, 1.53f, 2.96f, 1.24f, 1.41f, 0.89f, 1.42f, 1.20f, 1.82f, 2.43f, 1.00f, 2.47f, 3.50f, 2.76f, 3.50f, 2.53f, 1.46f, 1.15f, 0.67f, 1.43f, 1.00f, 3.41f, 1.27f, 1.00f, 3.50f, 3.50f, 3.50f, 1.41f, 1.41f, 3.50f, 1.20f, 2.73f, 1.87f, 0.50f, 1.14f, 3.31f, 1.35f, 2.74f, 1.57f, 2.19f, 2.77f, 1.90f, 1.67f, 2.10f, 2.57f, 1.70f, 3.50f, 2.81f, 1.52f, 2.16f, 1.54f, 1.55f, 3.23f, 3.50f, 2.49f, 1.91f, 2.45f, 3.08f, 3.50f, 3.50f, 2.24f, 2.22f, 2.16f, 1.86f, 2.63f, 1.63f, 2.06f, 1.54f, 1.75f, 2.92f, 1.61f, 1.49f, 2.03f, 1.60f, 1.00f, 1.00f, 1.55f, 1.00f, 1.00f, 1.60f, 1.00f, 1.00f, 1.00f, 2.00f, 1.00f, 1.00f, 1.63f, 1.00f, 3.06f, 2.43f, 3.02f, 2.35f, 2.26f, 1.44f, 2.71f, 1.91f, 1.69f, 1.73f, 3.10f, 3.50f, 3.50f, 1.92f, 1.47f, 3.50f, 3.50f, 1.88f, 3.01f, 2.58f, 1.59f, 2.54f, 1.94f, 1.58f, 1.19f, 1.70f, 2.73f};
static float kv_room = 1.0f;     /* latched voices step back while an effect you played speaks */

static int spawn_soft;
static void spawn(int force)
{
    fauna_v *c = 0;
    const sfx_t *R;
    int i, k = force, comb = -1;
    float p;
    for (i = 0; i < EN_NFA && !c; i++)
        if (!fa[i].on)
            c = &fa[i];
    if (!c) {
        if (force < 0)
            return;
        for (i = 0; i < EN_NFA; i++)     /* the oldest of the sea's own first; yours only when all six are yours */
            if (!fa[i].forced && (!c || fa[i].born < c->born))
                c = &fa[i];
        if (!c)
            for (c = &fa[0], i = 1; i < EN_NFA; i++)
                if (fa[i].born < c->born)
                    c = &fa[i];
        c->on = 0;
    }
    if (k < 0) {
        int on[NSFX], m = 0;
        for (i = 0; i < NSFX; i++)
            if (CAST_ON(i))
                on[m++] = i;
        if (!m)
            return;
        {   /* a light lean (1 in 4) to the world's own; otherwise any of them. Never one of the last 20 again. */
            static int16_t recent[20] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
            static int rp;
            int t, j, x = on[ri(m)];
            for (t = 0; t < 24; t++) {
                int seen = 0;
                x = fr() < .25f ? on[ri(m)] : ri(NSFX);
                for (j = 0; j < 20; j++)
                    seen |= recent[j] == x;
                if (!seen)
                    break;
            }
            k = x;
            auto_fx = (int16_t)k;        /* for the screen: the world's own choice */
            auto_seq++;
            recent[rp] = (int16_t)k;
            rp = (rp + 1) % 20;
        }
    }
    if (k == 1) {                                /* the strained metal needs one of two delay lines */
        int used = 0;
        for (i = 0; i < EN_NFA; i++)
            if (fa[i].on && fa[i].kind == 1)
                used |= 1 << fa[i].comb;
        comb = !(used & 1) ? 0 : !(used & 2) ? 1 : -1;
        if (comb < 0)
            return;
    }
    R = &SFX[k];
    __builtin_memset(c, 0, sizeof *c);
    c->comb = comb;
    c->forced = force >= 0 ? 1 + spawn_soft : 0;   /* 2: soft (the panic's breath) */
    c->kind = (uint8_t)k;
    c->r = R;
    c->born = t_now;
    c->bc = 1e9f;
    c->be = 1.0f;
    c->amp = fr();
    c->x = fr();
    c->y = .15f + fr() * .7f;
    c->vx = n1() * .025f;
    c->vy = n1() * .012f;
    c->pan = c->x * 1.6f - .8f;
    p = pitch(ri(6), 45.0f, 320.0f);
    if (R->custom) {
        switch (k) {
        case 0: c->life = 7 + fr() * 14; c->f0 = p * 2; c->f1 = p * fm_exp2f(-(3 + fr() * 10) / 12) * 2; break;
        case 1: c->life = 4 + fr() * 10; c->f0 = pitch(ri(6), 110, 900); c->f1 = c->f0 * (.92f + fr() * .14f);
                __builtin_memset(comb_mem[comb], 0, sizeof comb_mem[0]); break;
        case 2: c->life = 4 + fr() * 11; c->f0 = 700 + fr() * 2400; c->clkR = .05f + fr() * .35f; break;
        case 3: c->life = 4 + fr() * 6; c->f0 = 180 + fr() * 340; c->f1 = c->f0 * (1.7f + fr() * 2.5f); break;
        case 4: c->life = 8 + fr() * 13; c->f0 = 37 + fr() * 190; c->f1 = c->f0 * (.7f + fr() * .7f); break;
        default: c->life = 14 + fr() * 16; c->f0 = .055f + fr() * .05f;
                 for (i = 0; i < 6; i++) c->shep[i] = fr();
                 break;
        }
    } else {
        c->life = between(R->life0, R->life1);
        c->f0 = R->f0 > 0 ? between(R->f0, R->f1) : 200.0f;
        c->f1 = c->f0 * between(R->to0, R->to1);
        c->r1f = R->r1f * (.85f + fr() * .3f);
        if (R->rate0 > 0) {
            c->rt = between(R->rate0, R->rate1);
            c->bc = 0.0f;
        }
        if (R->chk != 0.0f) {
            c->cf0 = between(R->c00, R->c01);
            c->cf1 = between(R->c10, R->c11);
        }
    }
    c->on = 1;
    n_born++;
}

/* -------------------------------------------------------------- space --- */
#define TAPE_N 55125                             /* 5 s at 11 025 Hz */
static int16_t tape[TAPE_N] EN_POOL;
#define MEM_N 77175                              /* 14 s at 5 512 Hz, in 8 bits: the undertow is meant to rot */
static int8_t mem[MEM_N] EN_POOL;
static const int FL[8] = {1423, 1789, 2137, 2557, 3011, 3491, 4133, 4789};
#define FTOTAL (1423 + 1789 + 2137 + 2557 + 3011 + 3491 + 4133 + 4789)
static int16_t fdn[FTOTAL] EN_POOL;
static const int AL[4] = {142, 379, 107, 277};
static int16_t apb[142 + 379 + 107 + 277] EN_POOL;
static int16_t *fb[8], *ab[4];
static int fp[8], ap[4];
static float fd[8], fg[8];
static int tw, tdiv;
static float t_time = 2.4f, t_tgt = 2.4f, tlp[2], wow, drp = 1.0f, drp_t = 1.0f, tacc, t_last, t_cur;
static int mw, mdiv;
static float macc;
static struct { float pos, len, t, rate, pan; } gr[2];
static float dc[2], agc = 1.0f, rmsv = 1e-4f, lp_l, lp_m, m_low, m_mid, m_hi;
#define FS 8192.0f
#define FSI (1.0f / 8192.0f)

static inline int16_t to16(float x)
{
    float y = x * FS;
    return (int16_t)(y > 32000.0f ? 32000.0f : y < -32000.0f ? -32000.0f : y);
}
static inline int8_t mu8(float x)                 /* a square-root law: quiet things keep their shape */
{
    float a = fm_minf(fm_fabsf(x), 1.0f), v = fm_sqrtf(a) * 127.0f;
    return (int8_t)(x < 0.0f ? -v : v);
}
static inline float un8(int8_t v)
{
    float a = (float)v * (1.0f / 127.0f);
    return a < 0.0f ? -a * a : a * a;
}
static void grain(int i)
{
    float pick_g = fr();
    float len = 1.1f + fr() * 2.6f;              /* seconds */
    gr[i].len = len * 5512.5f;
    gr[i].rate = pick_g < .55f ? -.5f : pick_g < .8f ? -.25f : pick_g < .93f ? .5f : -1.0f;
    gr[i].pos = (float)(mw - ri(MEM_N - (int)(gr[i].len * 2.0f) - 8) - (int)gr[i].len + MEM_N * 3 ) ;
    while (gr[i].pos >= (float)MEM_N)
        gr[i].pos -= (float)MEM_N;
    gr[i].t = 0.0f;
}

/* ----------------------------------------------------------- the block --- */
static float bL[BLK], bR[BLK], bT[BLK], bV[BLK], bB[BLK], bBprev[BLK];  /* core 1's, and the drone bus */
static float cL[BLK], cR[BLK], cT[BLK], cV[BLK];                        /* core 2's */
static float bm[2][BLK];                                                 /* one bell's partials, summed, per core */
static float pr[2];
static int two_cores;
void en_cores(int two) { two_cores = two; }

static void bells_render(uint32_t n, int v0, int v1, float *const *o, float *m)   /* bells v0..v1-1 into o */
{
    float a0 = (st[1] * .55f + .45f) * lv[L_BELL] * W->bell * fm_minf(1.3f, .55f + .45f * sc_mix[3]);
    int v, p;
    uint32_t i;
    for (v = v0; v < v1; v++) {
        float pl, prr, a, live = 0.0f;
        if (!bl[v].on)
            continue;
        pl = (.5f - bl[v].pan * .5f) * .55f;
        prr = (.5f + bl[v].pan * .5f) * .55f;
        a = (bl[v].key ? 1.9f * bl[v].kt : a0) * bl[v].env * .17f;   /* the keys' bells: above the world */
        for (i = 0; i < n; i++)          /* all of a bell's partials into one line, then out once */
            m[i] = 0.0f;
        for (p = 0; p < NP; p++) {
            float re = bl[v].re[p], im = bl[v].im[p], c = bl[v].c[p] * bl[v].d[p], s0 = bl[v].s[p] * bl[v].d[p];
            if (fm_fabsf(re) + fm_fabsf(im) < 1e-3f) {   /* under -60 dB, below the rest of the world: let it go */
                bl[v].re[p] = bl[v].im[p] = 0.0f;
                continue;
            }
            live += 1.0f;
            for (i = 0; i < n; i++) {    /* the turn and the decay in one step */
                float t = re * c - im * s0;
                im = re * s0 + im * c;
                re = t;
                m[i] += re;
            }
            bl[v].re[p] = re;
            bl[v].im[p] = im;
        }
        if (live > 0.0f)
            for (i = 0; i < n; i++) {
                float s = m[i] * a;
                o[0][i] += s * pl;
                o[1][i] += s * prr;
                o[2][i] += s * .5f;
                o[3][i] += s * .95f;
            }
        if (live == 0.0f)
            bl[v].on = 0;
    }
}

static inline float flushf(float x) { return fm_fabsf(x) < 1e-15f ? 0.0f : x; }

static void generic_voice(fauna_v *c, const sfx_t *Q, uint32_t n, float pl, float prr, float tt, float fgn)
{
    const int16_t *tb = Q->wave >= 0 ? wt[Q->wave] : 0;
    float fs = Q->arch ? c->f0 + (c->f1 - c->f0) * fm_sin_turns(tt * .5f) : c->f0 + (c->f1 - c->f0) * tt;
    float env = Q->perc ? fm_minf(1.0f, tt * 60.0f) * (1.0f - tt) * (1.0f - tt) : fm_sin_turns(tt * .5f);
    float rfb = Q->r1to > 0.0f ? c->r1f + (Q->r1to - c->r1f) * fm_sqrtf(tt) : c->r1f;
    float rf = Q->rw > 0.0f ? rfb * (1.0f + Q->rw * fm_sin_turns(c->ph3)) : rfb;
    float c1 = Q->r1f > 0.0f ? 2.0f * fm_sin_pi(FM_PI * fm_minf(rf, EN_SR * .2f) / EN_SR) : 0.0f, q1 = Q->r1q;
    float c2 = Q->r2f > 0.0f ? 2.0f * fm_sin_pi(FM_PI * fm_minf(Q->r2f, EN_SR * .2f) / EN_SR) : 0.0f, q2 = Q->r2q;
    float rate = Q->rate0 > 0.0f ? c->rt * (1.0f + (Q->rto - 1.0f) * tt) : 0.0f;
    float g = env * fgn * Q->gain * .5f, dt = 1.0f / EN_SR;
    float bk = Q->bdec > 0.0f ? 1.0f - Q->bdec * dt : 1.0f, ck = Q->chk != 0.0f ? 1.0f - Q->chk * dt : 1.0f;
    float amI = Q->am / EN_SR, amD = Q->amd, q1g = q1 < .1f ? q1 * 4.0f : 1.0f;
    float g0 = c->a0;
    uint32_t i;
    for (i = 0; i < n; i++) {
        float ex = 0.0f, s, f, gi = g0 + (g - g0) * (float)i / (float)n;
        if (rate > 0.0f) {
            c->bc -= dt;
            if (c->bc <= 0.0f) {
                ex = Q->imp;
                c->be = 1.0f;
                c->bt = 1.0f;
                if (Q->dbl && !c->dn) {
                    c->bc = .27f;
                    c->dn = 1;
                } else {
                    c->dn = 0;
                    c->bc = (1.0f / rate) * (1.0f + Q->jit * (fr2() - .5f) * 1.6f);
                    if (c->bc < .004f)
                        c->bc = .004f;
                }
                if (Q->syll1 > 0.0f)
                    c->r1f = Q->syll0 + fr2() * (Q->syll1 - Q->syll0);
                if (Q->fld1 > 0.0f)
                    c->f0 = c->f1 = pitch((int)(fr2() * 6.0f), Q->fld0, Q->fld1);
            }
        }
        c->be *= bk;
        f = Q->fld1 > 0.0f ? c->f0 : fs;
        if (Q->chk != 0.0f) {                     /* a chirp: bt falls from 1 at each burst */
            f = c->cf1 + (c->cf0 - c->cf1) * c->bt;
            c->bt *= ck;
        }
        c->xe += (ex * 6.0f - c->xe) * .09f;   /* a soft felt hammer, not a one-sample edge */
        s = c->xe * (ex != 0.0f ? 1.0f : .985f);
        if (tb) {
            c->ph += f / EN_SR;
            if (c->ph >= 1.0f)
                c->ph -= 1.0f;
            s += rdw(tb, c->ph) * Q->ss;
            if (Q->tw > 0.0f) {
                c->ph2 += f * Q->tw / EN_SR;
                if (c->ph2 >= 1.0f)
                    c->ph2 -= 1.0f;
                s += rdw(tb, c->ph2) * Q->ts;
            }
        }
        if (Q->n > 0.0f)
            s += nz2() * Q->n;
        if (c1 > 0.0f) {
            float y;
            c->b1 += c1 * c->b2;
            c->b2 += c1 * (s - c->b1 - q1 * c->b2);
            y = c->b2;
            if (c2 > 0.0f) {
                c->d1 += c2 * c->d2;
                c->d2 += c2 * (s - c->d1 - q2 * c->d2);
                y += c->d2;
            }
            s = y * q1g;
        }
        if (amI > 0.0f) {
            c->amp += amI;
            if (c->amp >= 1.0f)
                c->amp -= 1.0f;
            s *= 1.0f - amD * (.5f + .5f * SINE(c->amp));
        }
        if (Q->rw > 0.0f) {
            c->ph3 += 3.1f / EN_SR;
            if (c->ph3 >= 1.0f)
                c->ph3 -= 1.0f;
        }
        s = fm_clampf(s * c->be * gi, -1.2f, 1.2f);
        cL[i] += s * pl;
        cR[i] += s * prr;
        cT[i] += s * .45f;
        cV[i] += s;
    }
    c->a0 = g;
}

static void fauna_render(uint32_t n)
{
    float fg0 = (st[2] * .85f + .15f) * lv[L_FAUNA] * .6f;   /* 4.4 dB under the drones (0.98) */
    int v;
    uint32_t i;
    for (v = 0; v < EN_NFA; v++) {
        fauna_v *c = &fa[v];
        const sfx_t *R = c->r;
        float pl, prr, tt, env, fgn, dt = 1.0f / EN_SR;
        if (!c->on)
            continue;
        pl = .5f - c->pan * .5f;
        prr = .5f + c->pan * .5f;
        tt = fm_minf(1.0f, c->t);
        env = c->a;
        fgn = (c->forced ? fm_maxf(fg0, .75f) * FX_LV[c->kind] * (c->forced == 2 ? .45f : 1.0f)
                         : .75f * lv[L_FAUNA] * FX_LV[c->kind]) * R->trim;   /* by itself: as loud as when it is played by
                                                                            * hand (CREATURES still sets how loud) */
        if (!R->custom) {
            generic_voice(c, R, n, pl, prr, tt, fgn);
            continue;
        }
        if (c->kind == 0) {          /* a low call that sinks */
            float f = (c->f0 + (c->f1 - c->f0) * tt * tt) / EN_SR;
            for (i = 0; i < n; i++) {
                float sv, s;
                c->ph += f;
                if (c->ph >= 1.0f)
                    c->ph -= 1.0f;
                sv = SINE(c->ph);
                s = (sv + .34f * SINE(c->ph * 2.0f + sv * .16f)) * env * fgn * .1f;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .4f; cV[i] += s * .95f;
            }
        } else if (c->kind == 1) {   /* metal under strain: a comb that moves */
            float f = c->f0 + (c->f1 - c->f0) * tt, len = fm_clampf(EN_SR / f, 16.0f, (float)COMBN - 8.0f);
            int16_t *cb = comb_mem[c->comb];
            for (i = 0; i < n; i++) {
                float rd = (float)c->cw - len, fra, y, s;
                int r0;
                if (rd < 0.0f)
                    rd += (float)COMBN;
                r0 = (int)rd;
                fra = rd - (float)r0;
                y = ((float)cb[r0] + (float)(cb[(r0 + 1) & (COMBN - 1)] - cb[r0]) * fra) * FSI;
                c->lp += (y - c->lp) * .42f;
                cb[c->cw] = to16(nz2() * .018f + c->lp * .975f);
                c->cw = (c->cw + 1) & (COMBN - 1);
                s = y * env * fgn * .32f;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .4f; cV[i] += s * .85f;
            }
        } else if (c->kind == 2) {   /* clicks through a ringing filter */
            float fc2 = 2.0f * fm_sin_pi(FM_PI * fm_minf(c->f0, EN_SR * .3f) / EN_SR), q0 = .035f;
            for (i = 0; i < n; i++) {
                float ex = 0.0f, s;
                c->clk -= dt;
                if (c->clk <= 0.0f) {
                    ex = .22f;
                    c->clk = c->clkR * (.45f + .9f * fr2());
                }
                c->xe += (ex - c->xe) * .08f;
                c->b1 += fc2 * c->b2;
                c->b2 += fc2 * (c->xe - c->b1 - q0 * c->b2);
                s = c->b2 * env * fgn * .09f;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .5f; cV[i] += s;
            }
        } else if (c->kind == 3) {   /* a reversed gong: noise that sucks inward, then is gone */
            float f = c->f0 + (c->f1 - c->f0) * tt * tt * tt, ct = 2.0f * fm_sin_pi(FM_PI * fm_minf(f, EN_SR * .25f) / EN_SR);
            float a = tt * tt * tt * tt * (tt > .985f ? (1.0f - tt) / .015f : 1.0f);
            for (i = 0; i < n; i++) {
                float s;
                c->b1 += ct * c->b2;
                c->b2 += ct * (nz2() - c->b1 - .05f * c->b2);
                s = c->b2 * a * fgn * .12f;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .35f; cV[i] += s * 1.1f;
            }
        } else if (c->kind == 4) {   /* a ring-modulated shadow of the drone (a block late: too little to hear) */
            float f = (c->f0 + (c->f1 - c->f0) * tt) / EN_SR;
            for (i = 0; i < n; i++) {
                float s;
                c->ph += f;
                if (c->ph >= 1.0f)
                    c->ph -= 1.0f;
                s = SINE(c->ph) * (bBprev[i] * 2.4f + .02f * SINE(c->ph * 1.51f)) * env * fgn;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .5f; cV[i] += s * .9f;
            }
        } else {                     /* the endless fall: six octaves sinking under a fixed window */
            int k;
            float inc[6], w[6];
            for (k = 0; k < 6; k++) {
                float p = c->shep[k] - c->f0 * (float)n / EN_SR * .16f;
                float wv;
                if (p < 0.0f)
                    p += 1.0f;
                c->shep[k] = p;
                wv = fm_sin_turns(p * .5f);
                w[k] = wv * wv;
                inc[k] = 34.0f * fm_exp2f(p * 6.0f) / EN_SR;
            }
            for (i = 0; i < n; i++) {
                float y = 0.0f, s;
                for (k = 0; k < 6; k++) {
                    c->shp[k] += inc[k];
                    if (c->shp[k] >= 1.0f)
                        c->shp[k] -= 1.0f;
                    y += SINE(c->shp[k]) * w[k];
                }
                s = y * env * fgn * .05f;
                cL[i] += s * pl; cR[i] += s * prr; cT[i] += s * .3f; cV[i] += s * .9f;
            }
        }
    }
}

/* drums: a pitch that falls, a little noise for the skin */
enum { DR_SKIN, DR_SUB, DR_WOOD, DR_METAL, DR_TICK, DR_HAMMER, NDRUM };
static const struct { float f0, f1, k, dec, nz, g; } DRUM[NDRUM] = {
    {115, 62, 30, 7.0f, .22f, .34f}, {72, 34, 12, 2.6f, .05f, .5f}, {420, 350, 40, 26.0f, .1f, .16f},
    {1250, 1180, 8, 16.0f, .3f, .07f}, {2300, 2200, 10, 70.0f, .15f, .05f}, {150, 74, 34, 13.0f, .5f, .3f}};
#define NDV 3
static struct { uint8_t on, kind; float ph, bt, e, g, pan, atk, lp, f0, f1, ks, ed, nz; } dv[NDV];
static int dv_next;
static void drums_render(uint32_t n)
{
    int v;
    uint32_t i;
    for (v = 0; v < NDV; v++) {
        float pl, prr;
        if (!dv[v].on)
            continue;
        pl = .5f - dv[v].pan * .5f;
        prr = .5f + dv[v].pan * .5f;
        for (i = 0; i < n; i++) {
            float f = dv[v].f1 + (dv[v].f0 - dv[v].f1) * dv[v].bt, s, nzv;
            dv[v].bt *= dv[v].ks;
            dv[v].ph += f / EN_SR;
            if (dv[v].ph >= 1.0f)
                dv[v].ph -= 1.0f;
            nzv = nz2() * dv[v].nz;
            dv[v].lp += (nzv - dv[v].lp) * .25f;
            dv[v].atk = fm_minf(1.0f, dv[v].atk + .012f);    /* 2 ms: a thump, never a click */
            s = (SINE(dv[v].ph) + dv[v].lp) * dv[v].e * dv[v].atk * dv[v].g;
            dv[v].e *= dv[v].ed;
            cL[i] += s * pl * .45f;
            cR[i] += s * prr * .45f;
            cT[i] += s * .3f;
            cV[i] += s * 1.1f;
        }
        if (dv[v].e < 1e-4f)
            dv[v].on = 0;
    }
}
static void drum(int kind, float vel)
{
    const float d = W->drum;
    int v;
    if (d <= 0.0f)
        return;
    v = dv_next = (dv_next + 1) % NDV;
    dv[v].kind = (uint8_t)kind;
    dv[v].f0 = DRUM[kind].f0 * (.97f + fr() * .06f);
    dv[v].f1 = DRUM[kind].f1;
    dv[v].ks = 1.0f - DRUM[kind].k / EN_SR;
    dv[v].ed = 1.0f - DRUM[kind].dec / EN_SR;
    dv[v].nz = DRUM[kind].nz;
    dv[v].g = DRUM[kind].g * vel * d;
    dv[v].bt = 1.0f;
    dv[v].e = 1.0f;
    dv[v].atk = 0.0f;
    dv[v].pan = n1() * .35f;
    dv[v].on = 1;
}

/* the lead: one breathing line, the sea's own instrument */
#define NLV 2
static const struct { int wave; float cut, f1, q, breath, vib, vr, g; } LEADV[6] = {
    {W_BRASS, 2.2f, 520, .3f, .02f, .002f, 3.2f, .34f},  /* horn  */
    {W_SINE, 8.0f, 1600, .4f, .22f, .012f, 6.1f, .2f},   /* clay  */
    {W_SINE, 8.0f, 2200, .5f, .12f, .006f, 4.6f, .22f},  /* flute */
    {W_ODD, 2.6f, 900, .22f, 0, 0, 1, .22f},             /* synth */
    {W_PULSE, 4.0f, 650, .18f, .03f, .008f, 5.2f, .26f}, /* reed  */
    {W_PULSE, 1.6f, 310, .2f, .05f, .003f, 3.8f, .32f}}; /* pipe  */
static struct { uint8_t on; float ph, vph, f, ft, env, env0, atk, rel, left, lp, b1, b2, pan, vel; int type; } lv_[NLV];
static int lv_next;
static void leads_render(uint32_t n)
{
    int v;
    uint32_t i;
    for (v = 0; v < NLV; v++) {
        const float dt = (float)n / EN_SR;
        float fc, c1, g, g0, pl, prr, vb, inc;
        const int16_t *tb;
        int L;
        if (!lv_[v].on)
            continue;
        L = lv_[v].type;
        tb = wt[LEADV[L].wave];
        lv_[v].env0 = lv_[v].env;
        if (lv_[v].left > 0.0f) {
            lv_[v].left -= dt;
            lv_[v].env = fm_minf(1.0f, lv_[v].env + dt / lv_[v].atk);
        } else {
            lv_[v].env -= dt / lv_[v].rel;
            if (lv_[v].env <= 0.0f) {
                lv_[v].env = 0.0f;
                lv_[v].on = 0;
            }
        }
        lv_[v].f += (lv_[v].ft - lv_[v].f) * fm_minf(1.0f, dt * 7.0f);
        lv_[v].vph += dt * LEADV[L].vr;
        if (lv_[v].vph >= 1.0f)
            lv_[v].vph -= 1.0f;
        vb = 1.0f + LEADV[L].vib * fm_sin_turns(lv_[v].vph) * lv_[v].env;
        inc = lv_[v].f * vb / EN_SR;
        fc = 1.0f - fm_expf(-TWO_PI * fm_minf(lv_[v].f * LEADV[L].cut + 120.0f, 5000.0f) / EN_SR);
        c1 = 2.0f * fm_sin_pi(FM_PI * LEADV[L].f1 / EN_SR);
        g = lv_[v].env * lv_[v].env * lv_[v].vel * LEADV[L].g * W->lead;
        g0 = lv_[v].env0 * lv_[v].env0 * lv_[v].vel * LEADV[L].g * W->lead;
        pl = .5f - lv_[v].pan * .5f;
        prr = .5f + lv_[v].pan * .5f;
        for (i = 0; i < n; i++) {
            float s, y, gi = g0 + (g - g0) * (float)i / (float)n;
            lv_[v].ph += inc;
            if (lv_[v].ph >= 1.0f)
                lv_[v].ph -= 1.0f;
            s = rdw(tb, lv_[v].ph) + nz2() * LEADV[L].breath;
            lv_[v].lp += (s - lv_[v].lp) * fc;
            lv_[v].b1 += c1 * lv_[v].b2;
            lv_[v].b2 += c1 * (lv_[v].lp - lv_[v].b1 - LEADV[L].q * lv_[v].b2);
            y = (lv_[v].lp * .55f + lv_[v].b2 * .5f) * gi;
            cL[i] += y * pl;
            cR[i] += y * prr;
            cT[i] += y * .4f;
            cV[i] += y * .6f;
        }
    }
}
static void lead(float f, float dur, float vel, float atk, float rel)
{
    int v;
    if (W->lead_voice < 0 || W->lead <= 0.0f)
        return;
    v = lv_next = (lv_next + 1) % NLV;
    if (!lv_[v].on) {
        lv_[v].f = f;
        lv_[v].env = 0.0f;
    }
    lv_[v].ft = f;
    lv_[v].type = W->lead_voice;
    lv_[v].left = dur;
    lv_[v].atk = atk;
    lv_[v].rel = rel;
    lv_[v].vel = vel;
    lv_[v].pan = n1() * .4f;
    lv_[v].on = 1;
}

/* ---------------------------------------------------------------- pads --- */
/* three soft chord voices over the drone, two detuned saws each through a breathing filter; they follow the walk */
#define NPD 3
static struct { float ph[2], f, ft, amp, lo, bd; } pd[NPD];
static float pad_lv, pad_cut = 500.0f;
static void pads_render(uint32_t n)
{
    const int16_t *tb = wt[W_SAW];
    float c = 2.0f * fm_sin_pi(FM_PI * fm_clampf(pad_cut, 80.0f, 2600.0f) / EN_SR);
    int v;
    uint32_t i;
    for (v = 0; v < NPD; v++) {
        float g = pd[v].amp * pad_lv * .045f, pl = .5f - (float)(v - 1) * .3f, prr = 1.0f - pl;
        float d0 = pd[v].f * 1.0035f / EN_SR, d1 = pd[v].f * .9968f / EN_SR;
        if (g < 1e-6f)
            continue;
        for (i = 0; i < n; i++) {
            float x, y;
            pd[v].ph[0] += d0; if (pd[v].ph[0] >= 1.0f) pd[v].ph[0] -= 1.0f;
            pd[v].ph[1] += d1; if (pd[v].ph[1] >= 1.0f) pd[v].ph[1] -= 1.0f;
            x = rdw(tb, pd[v].ph[0]) + rdw(tb, pd[v].ph[1]);
            pd[v].lo += c * pd[v].bd;
            pd[v].bd += c * (x - pd[v].lo - .8f * pd[v].bd);
            y = pd[v].lo * g;
            cL[i] += y * pl; cR[i] += y * prr; cT[i] += y * .2f; cV[i] += y * .8f;
        }
        pd[v].lo = flushf(pd[v].lo);
        pd[v].bd = flushf(pd[v].bd);
    }
}

static void keyvox_render(uint32_t n);
static void core2_job(uint32_t n)
{
    pads_render(n);
    keyvox_render(n);
    {
        float *const o[4] = {cL, cR, cT, cV};
        bells_render(n, 2, NB, o, bm[1]);        /* six of the bells here, two on core 1 (it has the room) */
    }
    fauna_render(n);
    drums_render(n);
    leads_render(n);
}

static void drones_render(uint32_t n, float cut)
{
    const drone_t *T = &DRONES[d_type];
    const int16_t *tb = T->wt >= 0 ? wt[T->wt] : 0;
    float lpf = T->lp > 0.0f ? 1.0f - fm_expf(-TWO_PI * fm_minf(cut * T->lp, EN_SR * .2f) / EN_SR) : 1.0f;
    int v;
    uint32_t i;
    for (v = 0; v < EN_NDR; v++) {
        drone_v *d = &dr[v];
        float inc, inc2, incm, I, pl, prr, sweep, mul, c1, c2, g, g0;
        if (d->amp < 1e-5f && d->amp0 < 1e-5f)
            continue;
        inc = d->f / EN_SR;
        inc2 = (d->f + d->beat) / EN_SR;
        incm = d->f * d->mr / EN_SR;
        I = (.35f + macro_m[M_UNREST] * 1.9f + macro_m[M_ROT] * .8f) * (.7f + .3f * fm_sinf(d->w * 1.3f)) * T->fm / TWO_PI;
        pl = .5f - DR_PAN[v] * .5f;
        prr = .5f + DR_PAN[v] * .5f;
        sweep = T->sw > 0.0f ? T->sw * (.5f + .5f * fm_sinf(d->sw)) : 0.0f;
        mul = T->rel ? d->f : 1.0f;
        c1 = T->f1 > 0.0f ? 2.0f * fm_sin_pi(FM_PI * fm_minf(T->f1 * mul + (T->f2 > 0.0f ? 0.0f : sweep), 2600.0f) / EN_SR) : 0.0f;
        c2 = T->f2 > 0.0f ? 2.0f * fm_sin_pi(FM_PI * fm_minf(T->f2 * mul + sweep, 3200.0f) / EN_SR) : 0.0f;
        g = d->amp * .17f * T->g;
        g0 = d->amp0 * .17f * T->g;
        for (i = 0; i < n; i++) {
            float s, gi = g0 + (g - g0) * (float)i / (float)n;
            if (!tb)
                s = n1() * 1.4f;
            else {
                float mod;
                d->phm += incm;
                if (d->phm >= 1.0f) d->phm -= 1.0f;
                d->ph += inc;
                if (d->ph >= 1.0f) d->ph -= 1.0f;
                d->ph2 += inc2;
                if (d->ph2 >= 1.0f) d->ph2 -= 1.0f;
                mod = I != 0.0f ? I * SINE(d->phm) : 0.0f;
                s = rdw(tb, d->ph + mod + 1.0f) + rdw(tb, d->ph2 + mod * .75f + 1.0f);
            }
            if (T->lp > 0.0f) {
                d->lp[0] += (s - d->lp[0]) * lpf;
                d->lp[1] += (d->lp[0] - d->lp[1]) * lpf;
                s = d->lp[1];
            }
            if (c1 > 0.0f) {
                float y;
                d->b[0] += c1 * d->b[1];
                d->b[1] += c1 * (s - d->b[0] - T->q1 * d->b[1]);
                y = d->b[1] * T->a1 * (T->q1 < .1f ? T->q1 * 4.0f : 1.0f);
                if (c2 > 0.0f) {
                    d->b[2] += c2 * d->b[3];
                    d->b[3] += c2 * (s - d->b[2] - T->q2 * d->b[3]);
                    y += d->b[3] * T->a2 * (T->q2 < .1f ? T->q2 * 4.0f : 1.0f);
                }
                s = y + (tb ? s * .4f : 0.0f);
            }
            s *= gi;
            bL[i] += s * pl; bR[i] += s * prr; bV[i] += s * .35f; bB[i] += s;
        }
        d->amp0 = d->amp;
    }
    {
        float f = root_hz * 5.0f, c = 2.0f * fm_sin_pi(FM_PI * fm_minf(f, 1200.0f) / EN_SR), q0 = .1f;
        float a = .012f * W->noise * (.35f + st[0] + macro_m[M_ROT]) * lv[L_DRONE] * (playing ? 1.0f : 0.0f);
        for (i = 0; i < n; i++) {
            float s;
            pr[0] += c * pr[1];
            pr[1] += c * (n1() - pr[0] - q0 * pr[1]);
            s = pr[1] * a;
            bL[i] += s; bR[i] -= s * .6f; bV[i] += s * .4f;
        }
    }
}

/* ---------------------------------------------------------- sung voices --- */
/* A throat: a buzz through four formants that move between the vowels a e i o u, for a bass, a tenor and a
 * soprano. On it: chanted syllables, an undertone an octave down (the Gyuto monks), a growl, an overtone whistle
 * (khoomei), or breath alone (whispers). */
static const float VF[3][5][4] = {
    {{600, 1040, 2250, 2450}, {400, 1620, 2400, 2800}, {250, 1750, 2600, 3050}, {400, 750, 2400, 2600}, {350, 600, 2400, 2675}},
    {{650, 1080, 2650, 2900}, {400, 1700, 2600, 3200}, {290, 1870, 2800, 3250}, {400, 800, 2600, 2800}, {350, 600, 2700, 2900}},
    {{800, 1150, 2900, 3900}, {350, 2000, 2800, 3600}, {270, 2140, 2950, 3900}, {450, 800, 2830, 3800}, {325, 700, 2700, 3800}}};
static const float VG[3][5][4] = {
    {{0, -7, -9, -9}, {0, -12, -9, -12}, {0, -30, -16, -22}, {0, -11, -21, -20}, {0, -20, -32, -28}},
    {{0, -6, -7, -8}, {0, -14, -12, -14}, {0, -15, -18, -20}, {0, -10, -12, -12}, {0, -20, -17, -14}},
    {{0, -6, -32, -20}, {0, -20, -15, -40}, {0, -12, -26, -26}, {0, -11, -22, -22}, {0, -16, -35, -40}}};
static const float VBW[3][4] = {{60, 70, 110, 120}, {80, 90, 120, 130}, {80, 90, 120, 130}};
/* sung, throat (0 bass 1 tenor 2 soprano), how the vowels move (0 drifting, 1 chanted syllables, 2 a held dark o-u,
 * 3 whispered), growl, undertone, whistle, breath, level */
typedef struct { uint8_t sung, reg, mode; float growl, sub, whistle, breath, g; } vx_t;
static const vx_t VX_CH[NCHOIR] = {
    {1, 1, 0, 0, 0, 0, .06f, 0.63f},       /* REQUIEM */
    {1, 1, 1, 0.00f, 0.10f, 0.00f, 0.05f, 1.00f},   /* DISTANT MASS */
    {1, 2, 3, 0.00f, 0.00f, 0.00f, 0.45f, 3.00f},       /* WRAITHS */
    {1, 1, 0, 0.00f, 0.30f, 0.20f, 0.04f, 1.00f},   /* GOTHIC CHOIR */
    {0, 0, 0, 0, 0, 0, 0, 1.00f},          /* ONDES */
    {1, 1, 1, 0.00f, 0.00f, 0.00f, 0.10f, 1.00f},   /* BROKEN RECORD */
    {0, 0, 0, 0, 0, 0, 0, 1.00f},          /* NIGHT SYNTH */
    {1, 2, 8, 0.00f, 0.00f, 0.00f, 0.12f, 3.00f},       /* KEENING */
    {1, 1, 2, 0.00f, 0.00f, 0.00f, 0.20f, 1.00f},   /* WEEPING STATUES */
    {1, 0, 0, 0.00f, 0.00f, 0.00f, 0.15f, 1.00f},   /* ROBED ELDERS */
    {1, 2, 1, 0.00f, 0.00f, 0.00f, 0.08f, 1.10f},        /* NUNS */
    {1, 2, 7, 0.00f, 0.00f, 0.00f, 0.08f, 1.48f},       /* SIRENS */
    {1, 0, 1, 0.70f, 0.00f, 0.00f, 0.30f, 1.00f},   /* DEATH RATTLE */
    {1, 1, 5, 0.00f, 0.00f, 0.00f, 0.15f, 1.40f},       /* MOURNERS */
    {1, 0, 6, 0.00f, 0.10f, 1.00f, 0.04f, 1.32f},  /* THROAT CHOIR */
    {1, 1, 6, 0.00f, 0.15f, 0.00f, 0.04f, 1.53f},       /* VOX HUMANA */
    {1, 1, 3, 0.00f, 0.00f, 0.00f, 0.50f, 0.52f},        /* HOLLOW VOICES */
    {0, 0, 0, 0, 0, 0, 0, 1.00f},          /* GLASS CHOIR */
    {1, 2, 4, 0.00f, 0.00f, 0.00f, 0.06f, 1.00f},  /* SERAPHIM: open fifths, high */
    {1, 1, 7, 0.00f, 0.25f, 0.00f, 0.10f, 1.16f},   /* DROWNED CHOIR: muffled, slow wobble */
    {1, 1, 4, 0.00f, 0.30f, 0.00f, 0.04f, 1.00f},     /* ORGANUM: chanted fifths */
    {1, 0, 0, 0.25f, 0.20f, 0.00f, 0.12f, 1.00f},   /* GRAVE DIGGERS */
    {1, 2, 1, 0.50f, 0.00f, 0.00f, 0.40f, 1.00f},   /* CHOKING VOICES */
    {1, 0, 4, 0.00f, 0.00f, 0.00f, 0.10f, 1.35f},     /* LAMENT: a minor chord, held */
    {1, 2, 5, 0.00f, 0.00f, 0.25f, 0.08f, 1.23f},      /* HALO: a major chord of light */
    {1, 2, 6, 0.00f, 0.00f, 0.60f, 0.05f, 1.24f},   /* ICE MAIDENS: whistling sopranos */
    {1, 2, 2, 0.00f, 0.00f, 0.00f, 0.06f, 1.00f},   /* SOPRANO BELOW */
    {1, 2, 0, 0.00f, 0.00f, 0.00f, 0.05f, 1.00f},   /* CHERUBIM */
    {1, 2, 2, 0.00f, 0.00f, 0.00f, 0.18f, 1.00f},   /* LULLABY OF THE DEAD */
};
static const vx_t VX_KV[EN_NKVOX - 1] = {
    {1, 0, 1, .12f, .55f, 0, .05f, 0.47f}, /* GYUTO MONKS: the undertone chant */
    {1, 1, 1, 0, 0, 0, .05f, 0.48f},       /* PLAINCHANT */
    {1, 2, 0, 0, 0, 0, .04f, 0.97f},       /* BULGARIAN DIRGE */
    {1, 1, 0, 0, 0, 0, .08f, 0.91f},       /* REQUIEM CLUSTER */
    {1, 2, 0, 0, 0, 0, .12f, 0.97f},       /* FALLEN ANGELS */
    {1, 0, 2, .9f, .8f, 0, .25f, 0.68f},   /* DEMON GROWL */
    {1, 0, 2, 0, .1f, 1.0f, .03f, 0.54f},  /* KHOOMEI WHISTLE */
    {1, 2, 2, 0, 0, 0, .12f, 2.29f},       /* KEENING */
    {1, 1, 3, 0, 0, 0, 1.0f, 1.88f},       /* WHISPERING DEAD */
    {1, 2, 0, 0, 0, 0, .35f, .51f},       /* SHADOW SOPRANO: one dark angel, more breath than voice */
    {1, 2, 0, 0, 0, 0, .06f, 1.00f},  /* SERAPHIM: open fifths, high */
    {1, 1, 2, 0, .1f, 0, .08f, 1.00f},   /* DROWNED CHOIR: muffled, slow wobble */
    {1, 1, 1, 0, 0, 0, .04f, 1.00f},     /* ORGANUM: chanted fifths */
    {1, 1, 2, 0, 0, 0, .08f, 1.00f},     /* LAMENT: a minor chord, held */
    {1, 2, 0, 0, 0, 0, .1f, 1.00f},      /* HALO: a major chord of light */
    {1, 2, 0, 0, 0, .6f, .05f, 1.00f},   /* ICE MAIDENS: whistling sopranos */
    {1, 0, 1, 0.70f, 0.00f, 0.00f, 0.30f, 1.00f},   /* DEATH RATTLE */
    {1, 2, 0, 0.00f, 0.00f, 0.00f, 0.05f, 1.00f},   /* CHERUBIM */
    {1, 1, 1, 0.00f, 0.10f, 0.00f, 0.05f, 1.00f},   /* DISTANT MASS */
    {1, 2, 1, 0.50f, 0.00f, 0.00f, 0.40f, 1.00f},   /* CHOKING VOICES */
    {1, 0, 0, 0.25f, 0.20f, 0.00f, 0.12f, 1.00f},   /* GRAVE DIGGERS */
    {1, 1, 2, 0.00f, 0.00f, 0.00f, 0.20f, 1.00f},   /* WEEPING STATUES */
    {1, 2, 2, 0.00f, 0.00f, 0.00f, 0.18f, 1.00f},   /* LULLABY OF THE DEAD */
    {1, 1, 1, 0.00f, 0.00f, 0.00f, 0.10f, 1.00f},   /* BROKEN RECORD */
    {1, 2, 2, 0.00f, 0.00f, 0.00f, 0.06f, 1.00f},   /* SOPRANO BELOW */
    {1, 1, 0, 0.00f, 0.30f, 0.20f, 0.04f, 1.00f},   /* GOTHIC CHOIR */
    {1, 0, 0, 0.00f, 0.00f, 0.00f, 0.15f, 1.00f},   /* ROBED ELDERS */
};
static void vowel_step(choir_v *c, const vx_t *X, float dt)
{
    static const uint8_t SYL[8] = {0, 3, 4, 3, 1, 3, 0, 4};
    c->vt -= dt;
    if (c->vt <= 0.0f) {
        if (X->mode == 1) {
            c->vsi = (uint8_t)((c->vsi + 1) & 7);
            c->vtg = (float)SYL[c->vsi];
            c->vt = .55f + fr() * 1.2f;
        } else if (X->mode == 2) {
            c->vtg = 3.0f + fr();
            c->vt = 3.0f + fr() * 5.0f;
        } else if (X->mode == 3) {
            c->vtg = fr() * 4.0f;
            c->vt = .12f + fr() * .45f;
        } else if (X->mode == 4) {          /* an open AH, held */
            c->vtg = fr() * .35f;
            c->vt = 3.0f + fr() * 4.0f;
        } else if (X->mode == 5) {          /* a bright EH-EE, held */
            c->vtg = 1.3f + fr() * .7f;
            c->vt = 3.0f + fr() * 3.0f;
        } else if (X->mode == 6) {          /* a closed hum: MMM */
            c->vtg = 3.8f + fr() * .2f;
            c->vt = 4.0f + fr() * 4.0f;
        } else if (X->mode == 7) {          /* a slow sway, OO to AH and back */
            c->vsi ^= 1u;
            c->vtg = c->vsi & 1u ? .2f : 4.0f;
            c->vt = 2.5f + fr() * 2.0f;
        } else if (X->mode == 8) {          /* a wail: EE falling to OO, over and over */
            c->vsi ^= 1u;
            c->vtg = c->vsi & 1u ? 2.0f : 4.0f;
            c->vt = .7f + fr() * .7f;
        } else {
            c->vtg = fr() * 4.0f;
            c->vt = 2.5f + fr() * 6.0f;
        }
    }
    c->vow += (c->vtg - c->vow) * fm_minf(1.0f, dt * (X->mode == 1 ? 7.0f : X->mode == 3 ? 10.0f : X->mode == 8 ? 2.5f : X->mode == 7 ? .8f : .45f));
}
static void sung_render(choir_v *c, const choir_t *Y, const vx_t *X, float pan, uint32_t n, float *const *o)
{
    const int16_t *tb = wt[W_GLOT], *tp = wt[W_PULSE];
    float pl = .5f - pan * .5f, prr = .5f + pan * .5f, fc[4], fq[4], fg[4], wc = 0.0f, vb, d0, d1, ds, t, g, g0, kd;
    float gr = X->growl, br = X->mode == 3 ? 0.0f : 1.0f, nz = X->mode == 3 ? 2.6f : X->breath * 1.6f;
    int a, b2, k, r = X->reg;
    uint32_t i;
    g = c->amp * .1f * Y->g * X->g;
    g0 = c->amp0 * .1f * Y->g * X->g;
    a = (int)c->vow;
    if (a > 4) a = 4;
    if (a < 0) a = 0;
    b2 = a < 4 ? a + 1 : 4;
    t = c->vow - (float)a;
    for (k = 0; k < 3; k++) {
        float hz = (VF[r][a][k] * (1.0f - t) + VF[r][b2][k] * t) * (gr > .3f ? .84f : 1.0f);
        float db = VG[r][a][k] * (1.0f - t) + VG[r][b2][k] * t;
        hz = fm_minf(hz, 4200.0f);
        fc[k] = 2.0f * fm_sin_pi(FM_PI * hz / EN_SR);
        fq[k] = fm_minf(1.0f, VBW[r][k] * (1.0f + gr) / hz);
        fg[k] = fm_exp2f(db * .1661f) * fq[k];          /* unity at each peak, then the vowel's own balance */
    }
    if (X->mode == 6) {                                 /* the mouth closed: only the lowest resonance hums */
        fg[1] *= .12f;
        fg[2] *= .08f;
    }
    if (X->whistle > 0.0f) {                            /* khoomei: one harmonic near 1.7 kHz sings out */
        float h = fm_clampf(fm_floorf(1700.0f / fm_maxf(c->f, 40.0f) + .5f), 5.0f, 14.0f) * c->f;
        wc = 2.0f * fm_sin_pi(FM_PI * fm_minf(h, 4000.0f) / EN_SR);
    }
    c->vph += (float)n * Y->vr / EN_SR;
    if (c->vph >= 1.0f)
        c->vph -= 1.0f;
    vb = 1.0f + Y->vib * fm_sin_turns(c->vph) * (c->t > .1f ? 1.0f : c->t * 10.0f);
    d0 = c->f * Y->rt[0] * vb / EN_SR;
    d1 = c->f * Y->rt[1] * vb / EN_SR;
    ds = c->f * .5f * vb / EN_SR;
    kd = EN_SR / (TWO_PI * fm_maxf(c->f, 40.0f)) * .5f;   /* the flow's change, not the flow: what the mouth hears */
    for (i = 0; i < n; i++) {
        float s = 0.0f, y = 0.0f, gi = g0 + (g - g0) * (float)i / (float)n;
        c->p[0] += d0; if (c->p[0] >= 1.0f) c->p[0] -= 1.0f;
        c->p[1] += d1; if (c->p[1] >= 1.0f) c->p[1] -= 1.0f;
        if (br > 0.0f) {
            float src = rdw(tb, c->p[0]) + rdw(tb, c->p[1]);
            s = (src - c->band) * kd;
            c->band = src;
        }
        if (X->sub > 0.0f) {
            c->sp += ds;
            if (c->sp >= 1.0f) c->sp -= 1.0f;
            s += rdw(tp, c->sp) * X->sub * 1.4f;
        }
        c->gz += (n1() - c->gz) * .35f;                 /* breath, softened */
        s += c->gz * nz;
        if (gr > 0.0f) {                                /* a rough throat: the buzz shaken by slow noise */
            c->lz += (n1() - c->lz) * .05f;
            s *= 1.0f + gr * c->lz * 3.5f;
        }
        for (k = 0; k < 3; k++) {
            c->fz[2 * k] += fc[k] * c->fz[2 * k + 1];
            c->fz[2 * k + 1] += fc[k] * (s - c->fz[2 * k] - fq[k] * c->fz[2 * k + 1]);
            y += c->fz[2 * k + 1] * fg[k];
        }
        if (wc > 0.0f) {
            c->fz[8] += wc * c->fz[9];
            c->fz[9] += wc * (s - c->fz[8] - .012f * c->fz[9]);
            y += c->fz[9] * .012f * X->whistle * 3.0f;
        }
        if (gr > 0.0f)
            y = y * (1.0f + gr * 2.0f) / (1.0f + fm_fabsf(y) * gr * 2.0f);
        c->lp1 += (y - c->lp1) * .5f;                   /* the top of the throat rolls away */
        y = c->lp1 * gi;
        o[0][i] += y * pl; o[1][i] += y * prr; o[2][i] += y * .35f; o[3][i] += y * .7f;
    }
    for (k = 0; k < 10; k++)
        c->fz[k] = flushf(c->fz[k]);
    c->amp0 = c->amp;
}

/* the keys' voices: held as long as the key is down, then they fade. Same machinery as the choir. */
/* voices from the world's darkest singing: Tibetan monks' undertone chant, the Russian octavist, plainchant, the
 * Bulgarian women's harsh seconds, a Ligeti-like cluster, keening, Tuvan whistle overtones, and things not human */
static const choir_t KVOX[EN_NKVOX - 1] = {
  {"GYUTO MONKS", 2, {1, .5f, 1.002f}, 45, 95, 1.4f, 420, .08f, 820, .1f, .001f, 3, .01f, 0, 0, 2.0f},
  {"PLAINCHANT", 2, {1, 1.004f, .996f}, 90, 200, 1.6f, 520, .12f, 880, .14f, .003f, 4.8f, .01f, 0, 0, 1.9f},
  {"BULGARIAN DIRGE", 2, {1, 1.122f, 1.06f}, 220, 520, 2.6f, 760, .1f, 1320, .12f, .007f, 6, .01f, 0, 0, .95f},
  {"REQUIEM CLUSTER", 1, {1, 1.06f, 1.122f}, 160, 450, 1.3f, 620, .14f, 1020, .16f, .002f, 2, .02f, 0, 0, 1.35f},
  {"FALLEN ANGELS", 2, {1, 1.189f, 2}, 180, 480, 1.6f, 600, .12f, 1000, .14f, .004f, 4, .02f, 0, 0, 1.5f},
  {"DEMON GROWL", 1, {1, .5f, 1.06f}, 40, 90, 1.0f, 300, .1f, 560, .14f, .03f, 9, .15f, 0, 0, 1.0f},
  {"KHOOMEI WHISTLE", 2, {1, 1.001f, 2}, 90, 160, 4.0f, 380, .1f, 1600, .03f, .002f, 3, 0, 0, 0, 1.2f},
  {"KEENING", 0, {1, 1.004f, 2.003f}, 300, 700, 5.0f, 0, 0, 0, 0, .03f, 1.5f, .1f, 0, 0, .42f},
  {"WHISPERING DEAD", 0, {1, 1.001f, 1}, 120, 360, 2.0f, 500, .2f, 1300, .2f, .004f, 3, .6f, 0, 0, 2.2f},
  {"SHADOW SOPRANO", 2, {1, 2.0f, 1.002f}, 200, 520, 1.4f, 600, .1f, 1100, .12f, .005f, 4.6f, .02f, 0, 0, 1.2f},
  {"SERAPHIM", 2, {1.00000f, 1.49800f, 2.00000f}, 260.00000f, 700.00000f, 2.40000f, 700.00000f, 0.12000f, 1200.00000f, 0.14000f, 0.00500f, 5.00000f, 0.02000f, 0, 0, 1.05926f},
  {"DROWNED CHOIR", 1, {1.00000f, 1.00300f, 0.50000f}, 90.00000f, 260.00000f, 0.55000f, 400.00000f, 0.20000f, 800.00000f, 0.20000f, 0.01500f, 1.20000f, 0.02000f, 0, 0, 0.78074f},
  {"ORGANUM", 2, {1.00000f, 1.49800f, 0.50000f}, 100.00000f, 240.00000f, 1.60000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00200f, 4.00000f, 0.01000f, 0, 0, 1.22487f},
  {"LAMENT", 2, {1.00000f, 1.18900f, 1.49800f}, 160.00000f, 420.00000f, 1.80000f, 600.00000f, 0.12000f, 1050.00000f, 0.14000f, 0.00800f, 4.80000f, 0.03000f, 0, 0, 1.18168f},
  {"HALO", 2, {1.00000f, 1.26000f, 1.50000f}, 300.00000f, 800.00000f, 2.60000f, 760.00000f, 0.10000f, 1300.00000f, 0.12000f, 0.00400f, 5.00000f, 0.04000f, 0, 0, 2.41630f},
  {"ICE MAIDENS", 2, {1.00000f, 1.00300f, 1.50000f}, 300.00000f, 700.00000f, 2.20000f, 700.00000f, 0.12000f, 1200.00000f, 0.14000f, 0.00500f, 5.00000f, 0.02000f, 0, 0, 0.83657f},
  {"DEATH RATTLE", 2, {1.00000f, 1.03000f, 0.50000f}, 55.00000f, 130.00000f, 1.20000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00400f, 4.00000f, 0.05000f, 0, 0, 0.67530f},
  {"CHERUBIM", 2, {1.00000f, 1.33500f, 1.00000f}, 350.00000f, 850.00000f, 2.60000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00800f, 5.50000f, 0.02000f, 0, 0, 2.06301f},
  {"DISTANT MASS", 2, {1.00000f, 1.50000f, 1.00000f}, 110.00000f, 260.00000f, 0.50000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00400f, 4.50000f, 0.02000f, 0, 0, 0.86198f},
  {"CHOKING VOICES", 2, {1.00000f, 1.02000f, 1.00000f}, 250.00000f, 600.00000f, 2.00000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.01200f, 5.00000f, 0.40000f, 0, 0, 1.01158f},
  {"GRAVE DIGGERS", 2, {1.00000f, 0.75000f, 1.00000f}, 70.00000f, 160.00000f, 1.40000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.01000f, 3.50000f, 0.08000f, 0, 0, 0.74904f},
  {"WEEPING STATUES", 2, {1.00000f, 1.18900f, 1.00000f}, 200.00000f, 500.00000f, 1.40000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.02000f, 1.50000f, 0.20000f, 0, 0, 1.37246f},
  {"LULLABY OF THE DEAD", 2, {1.00000f, 1.00000f, 1.00000f}, 260.00000f, 560.00000f, 1.60000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00700f, 4.50000f, 0.12000f, 0, 0, 0.75509f},
  {"BROKEN RECORD", 2, {1.00000f, 1.50000f, 1.00000f}, 130.00000f, 300.00000f, 1.00000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.02000f, 0.80000f, 0.10000f, 0, 0, 1.11429f},
  {"SOPRANO BELOW", 2, {1.00000f, 1.00000f, 1.00000f}, 330.00000f, 700.00000f, 0.50000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.01500f, 5.00000f, 0.05000f, 0, 0, 1.04713f},
  {"GOTHIC CHOIR", 2, {1.00000f, 1.50000f, 1.00000f}, 120.00000f, 320.00000f, 2.00000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.00500f, 5.00000f, 0.03000f, 0, 0, 0.77002f},
  {"ROBED ELDERS", 2, {1.00000f, 1.00400f, 1.00000f}, 70.00000f, 160.00000f, 1.40000f, 500.00000f, 0.12000f, 900.00000f, 0.14000f, 0.01500f, 4.50000f, 0.15000f, 0, 0, 1.11045f},
};
const char *en_kvox_name(int i) { return i <= 0 ? "WORLD'S CHOIR" : KVOX[(i - 1) % (EN_NKVOX - 1)].name; }
#define NKV 4
static choir_v kv[NKV];
static uint8_t kv_gate[NKV], kv_lat[NKV];   /* lat: latched, so it stays while the keys play bells */
static int16_t kv_key[NKV];                 /* which key: 0..24 the FM-1's, 100 + note from MIDI */
static const float KV_PAN[NKV] = {-.35f, .35f, -.1f, .1f};
static void voice_render(choir_v *c, const choir_t *Y, float pan, float open0, uint32_t n, float *const *o)
{
    const int16_t *tb = wt[Y->wt];
    float pl, prr, g, g0, open, fcc, cc1, cc2, sr2, vb, d0, d1, d2;
    uint32_t i;
    pl = .5f - pan * .5f;
    prr = .5f + pan * .5f;
    g = c->amp * .1f * Y->g;
    g0 = c->amp0 * .1f * Y->g;
    open = open0 * Y->cut;
    fcc = 1.0f - fm_expf(-TWO_PI * fm_minf(open, EN_SR * .22f) / EN_SR);
    cc1 = Y->f1 > 0.0f ? 2.0f * fm_sin_pi(FM_PI * Y->f1 * (.85f + c->t * .4f) / EN_SR) : 0.0f;
    cc2 = Y->f2 > 0.0f ? 2.0f * fm_sin_pi(FM_PI * Y->f2 * (1.1f - c->t * .3f) / EN_SR) : 0.0f;
    sr2 = Y->res ? 2.0f * fm_sin_pi(FM_PI * fm_minf(open * (1.1f + fm_sinf(t_now * .27f + pan * 3.0f) * .6f), 3000.0f) / EN_SR) : 0.0f;
    c->vph += (float)n * Y->vr / EN_SR;
    if (c->vph >= 1.0f)
        c->vph -= 1.0f;
    vb = 1.0f + Y->vib * fm_sin_turns(c->vph) * (c->t > .1f ? 1.0f : c->t * 10.0f);
    d0 = c->f * Y->rt[0] * vb / EN_SR;
    d1 = c->f * Y->rt[1] * vb / EN_SR;
    d2 = c->f * Y->rt[2] * vb / EN_SR;
    for (i = 0; i < n; i++) {
        float s, y, gi = g0 + (g - g0) * (float)i / (float)n;
        c->p[0] += d0; if (c->p[0] >= 1.0f) c->p[0] -= 1.0f;
        c->p[1] += d1; if (c->p[1] >= 1.0f) c->p[1] -= 1.0f;
        c->p[2] += d2; if (c->p[2] >= 1.0f) c->p[2] -= 1.0f;
        s = rdw(tb, c->p[0]) + rdw(tb, c->p[1]) + rdw(tb, c->p[2]) * .7f;
        if (Y->br > 0.0f)
            s += n1() * Y->br;
        if (sr2 > 0.0f) {
            c->low += sr2 * c->band;
            c->band += sr2 * (s - c->low - .22f * c->band);
            s = c->low * .6f;
        } else {
            c->lp1 += (s - c->lp1) * fcc;
            c->lp2 += (c->lp1 - c->lp2) * fcc;
            s = c->lp2;
        }
        if (cc1 > 0.0f) {
            c->b[0] += cc1 * c->b[1];
            c->b[1] += cc1 * (s - c->b[0] - Y->q1 * c->b[1]);
            c->b[2] += cc2 * c->b[3];
            c->b[3] += cc2 * (s - c->b[2] - Y->q2 * c->b[3]);
            s = (c->b[1] + c->b[3] * .7f) * .34f + s * .22f;
        }
        y = s * gi;
        o[0][i] += y * pl; o[1][i] += y * prr; o[2][i] += y * .35f; o[3][i] += y * .7f;
    }
    c->amp0 = c->amp;
}
static void keyvox_render(uint32_t n);
static void choir_render(uint32_t n)
{
    float *const o[4] = {bL, bR, bT, bV};
    int v;
    float open0 = 170.0f + (1.0f - macro_m[M_DEPTH]) * 880.0f + st[3] * 180.0f;
    for (v = 0; v < EN_NCH; v++) {
        choir_v *c = &ch[v];
        if (c->amp < 1e-5f && c->amp0 < 1e-5f)
            continue;
        if (VX_CH[c->type].sung)
            sung_render(c, &CHOIRS[c->type], &VX_CH[c->type], CH_PAN[v], n, o);
        else
            voice_render(c, &CHOIRS[c->type], CH_PAN[v], open0, n, o);
    }
}
static void keyvox_render(uint32_t n)    /* the keys' voices: on the second core, beside the bells and effects */
{
    float *const o[4] = {cL, cR, cT, cV};
    int v;
    for (v = 0; v < NKV; v++) {
        choir_v *c = &kv[v];
        if (c->amp < 1e-5f && c->amp0 < 1e-5f)
            continue;
        {
            const choir_t *Y = c->type < 0 ? &CHOIRS[ch[0].type] : &KVOX[c->type];
            const vx_t *X = c->type < 0 ? &VX_CH[ch[0].type] : &VX_KV[c->type];
            if (X->sung)
                sung_render(c, Y, X, KV_PAN[v], n, o);
            else
                voice_render(c, Y, KV_PAN[v], 650.0f, n, o);
        }
    }
}

/* SPEAKER mode. A speaker this small cannot move air at 30-100 Hz, but the ear hears a missing bass note from its
 * harmonics. So only the drones are shaped into harmonics: never the bells, drums or effects, whose edges would turn
 * into clicks and grit. They are kept to the band the speaker can play, and the unplayable lows are taken out. */
/* (spk_on: declared above) */
static float spk_e, pr_l1, pr_r1, pr_l2, pr_r2;
#define LA_N 96
static float la_buf[2][LA_N], la_g = 1.0f, la_min = 1.0f;
static int la_i, la_hold;
static float spk_mix;
static struct { float l1, l2, l3, h, h2, h3, b, b2, lm, e; } spk[4];   /* 0, 1 the speaker; 2, 3 the recording's phone tone */
static inline float speaker(int c, float x, float bass)
{
    float hp, hi, d, a, harm;
    spk[c].l1 += (x - spk[c].l1) * .0185f;       /* 130 Hz: the headphone sound, with the rumble it cannot play taken off */
    hp = x - spk[c].l1;
    spk[c].l2 += (hp - spk[c].l2) * .0185f;
    hi = hp - spk[c].l2;
    spk[c].l3 += (bass - spk[c].l3) * .03f;      /* the drones, without their edges */
    d = spk[c].l3 * 1.6f;                       /* a light touch of overtones: the heavy drive was heard as distortion */
    a = fm_fabsf(d);
    d = 1.2f * d / (1.2f + a);                  /* leans toward 1.2 smoothly: never a hard stop to pop on */
    harm = d - .2f * d * d * d + .3f * d * d;   /* a smooth curve that never folds back: only the 2nd and 3rd harmonics */
    spk[c].h += (harm - spk[c].h) * .036f;
    harm -= spk[c].h;
    spk[c].h2 += (harm - spk[c].h2) * .036f;
    harm -= spk[c].h2;
    spk[c].b2 += (harm - spk[c].b2) * .11f;      /* a warm 800 Hz ceiling, twice: no fizz */
    spk[c].b += (spk[c].b2 - spk[c].b) * .11f;
    {   /* only the deep part is watched: when a big drone pushes the cone, the band under 500 Hz is held back; the bells
         * and everything bright above it are never touched */
        float lo, top, g, e;
        spk[c].lm += (hi - spk[c].lm) * .0688f;
        lo = spk[c].lm + spk[c].b * .3f;
        top = hi - spk[c].lm;
        e = fm_fabsf(lo);
        spk[c].e = e > spk[c].e ? spk[c].e + (e - spk[c].e) * .004f : spk[c].e * .99995f;
        g = spk[c].e > .32f ? .32f / spk[c].e : 1.0f;
        return top + lo * g;
    }
}
static inline float softclip(float x)    /* untouched below .6 (no distortion at all), then a smooth knee that never passes 1 */
{
    float a = fm_fabsf(x), d;
    if (a <= .6f)
        return x;
    d = (a - .6f) * 2.5f;
    a = .6f + .4f * d / (1.0f + d);
    return x < 0.0f ? -a : a;
}

static float wow_last, mod_last[2] = {5.0f, 5.0f};
static float rmsc = .02f, agc_c = 1.0f, duck_g = 1.0f, duck_to = 1.0f, lim_e, main_lim;
static int usb_phone;
void en_usb_tone(int phone) { usb_phone = phone; }
static void space(int32_t *lr, uint32_t n, uint32_t gain_q12)
{
    float m_rot = macro_m[M_ROT], m_vast = macro_m[M_VAST], m_depth = macro_m[M_DEPTH];
    float fbk = fm_minf((.28f + m_vast * .3f + m_rot * .14f) * W->tape_fb * (.6f + sh_tape * .8f), .92f);
    float tmix = .55f * W->tape_mix * sh_tape * sh_tape * 4.0f, tsend = sh_tape * 2.0f;
    float tlpc = 1.0f - fm_expf(-TWO_PI * (2600.0f - m_rot * 2050.0f) * (1.5f - sh_tape) / 11025.0f);
    float wows = (.0035f + m_rot * .0045f) * sh_wob * 11025.0f * fm_sinf(wow * TWO_PI), dwows = (wows - wow_last) / (float)n;
    float T60 = frozen ? 1e5f : (4.0f + m_vast * m_vast * 40.0f) * W->t60 * sh_size * (1.0f + sw * 2.0f + sw_tail * 4.0f);
    float dmp = frozen ? 1.0f : 1.0f - fm_expf(-TWO_PI * W->damp * (1.3f - m_depth * .6f) * sh_dark / EN_SR);
    float ghost = st[4] * (.2f + m_rot * .25f + m_vast * .1f) * W->ghost, wet = (.34f + m_vast * .42f) * W->wet * sh_space * (1.0f + sw * .7f + sw_tail);
    float gain = (float)gain_q12 / 4096.0f * 8300000.0f;
    float tsamp = t_time * 11025.0f + wow_last, dtsamp = dwows, sinw = fm_cosf(wow);
    int k;
    uint32_t i;
    float m0t = 5.0f + 4.0f * fm_sinf(t_now * .53f * TWO_PI), m1t = 5.0f + 4.0f * fm_sinf(t_now * .71f * TWO_PI + 1.0f);
    float g_mod0 = mod_last[0], g_mod1 = mod_last[1], dm0 = (m0t - g_mod0) / (float)n, dm1 = (m1t - g_mod1) / (float)n;
    wow_last = wows;
    mod_last[0] = m0t;
    mod_last[1] = m1t;
    for (k = 0; k < 8; k++)
        fg[k] = frozen ? .99999f : fm_exp2f(-9.9658f * (float)FL[k] / (EN_SR * T60));   /* 10^(-3 len / (sr T60)) */
    for (i = 0; i < n; i++) {
        float T = bT[i] + cT[i], ty, gl = 0.0f, grr = 0.0f, x, sum = 0.0f, rl = 0.0f, rrr = 0.0f, oL, oR, gainT, mono;
        float L = bL[i] + cL[i], R = bR[i] + cR[i], V = bV[i] + cV[i], y[8], clL = 0.0f, clR = 0.0f;
        float rp;
        int r0, g;
        /* the tape, at a quarter of the rate: a block of four feeds it, the reads are smoothed */
        tacc += frozen ? 0.0f : T;
        if (++tdiv >= 4) {
            float fp0, in;
            tdiv = 0;
            rp = (float)tw - (tsamp + dtsamp * (float)i);
            while (rp < 0.0f)
                rp += (float)TAPE_N;
            r0 = (int)rp;
            fp0 = rp - (float)r0;
            ty = ((float)tape[r0] + (float)(tape[r0 + 1 >= TAPE_N ? 0 : r0 + 1] - tape[r0]) * fp0) * FSI;
            tlp[0] += (ty - tlp[0]) * tlpc;
            tlp[1] += (tlp[0] - tlp[1]) * tlpc;
            t_last = t_cur;
            t_cur = tlp[1] * drp;
            in = tacc * .25f * tsend + t_cur * fbk;
            tape[tw] = to16(in / (1.0f + fm_fabsf(in) * .6f));
            tw = tw + 1 >= TAPE_N ? 0 : tw + 1;
            tacc = 0.0f;
        }
        ty = t_last + (t_cur - t_last) * ((float)tdiv + 1.0f) * .25f;

        /* the undertow: memory read backwards */
        for (g = 0; g < 2; g++) {
            float u = gr[g].t / gr[g].len, w = fm_sin_turns(fm_minf(u, 1.0f) * .5f), p = gr[g].pos;
            int p0;
            float pf, s;
            while (p < 0.0f)
                p += (float)MEM_N;
            while (p >= (float)MEM_N)
                p -= (float)MEM_N;
            p0 = (int)p;
            pf = p - (float)p0;
            {
                float a = un8(mem[p0]), b = un8(mem[p0 + 1 >= MEM_N ? 0 : p0 + 1]);
                s = (a + (b - a) * pf) * w * w;
            }
            if (g == 0) {
                gl += s * .75f;
                grr += s * .25f;
            } else {
                gl += s * .25f;
                grr += s * .75f;
            }
            gr[g].pos = p + gr[g].rate * .125f;
            gr[g].t += .125f;
            if (gr[g].t >= gr[g].len)
                grain(g);
        }
        gl *= ghost;
        grr *= ghost;
        if (!frozen) {
            macc += (L + R) * .5f + ty * .4f;
            if (++mdiv >= 8) {
                mdiv = 0;
                mem[mw] = mu8(macc * .125f * 2.0f);
                mw = mw + 1 >= MEM_N ? 0 : mw + 1;
                macc = 0.0f;
            }
        }

        /* the reverb */
        x = (frozen ? 0.0f : V + ty * .5f + (gl + grr) * .6f) * .5f;
        for (k = 0; k < 4; k++) {
            int p = ap[k];
            float d = (float)ab[k][p] * FSI, yy = d - .62f * x;
            ab[k][p] = to16(x + .62f * yy);
            ap[k] = p + 1 >= AL[k] ? 0 : p + 1;
            x = yy;
        }
        for (k = 0; k < 8; k++) {
            int16_t *b0 = fb[k];
            int len = FL[k], p = fp[k];
            float v;
            if (k < 2) {
                float qq = (float)p - (k ? g_mod1 + dm1 * (float)i : g_mod0 + dm0 * (float)i), qf;
                int q0;
                if (qq < 0.0f)
                    qq += (float)len;
                q0 = (int)qq;
                qf = qq - (float)q0;
                v = ((float)b0[q0] + (float)(b0[q0 + 1 >= len ? 0 : q0 + 1] - b0[q0]) * qf) * FSI;
            } else
                v = (float)b0[p] * FSI;
            fd[k] = flushf(fd[k] + (v - fd[k]) * dmp);
            y[k] = fd[k];
            sum += fd[k];
        }
        sum *= .25f;
        for (k = 0; k < 8; k++) {
            float inp = (y[k] - sum) * fg[k] + (k < 4 ? (k & 1 ? -x : x) : 0.0f);
            fb[k][fp[k]] = to16(fm_clampf(inp, -2.0f, 2.0f));
            fp[k] = fp[k] + 1 >= FL[k] ? 0 : fp[k] + 1;
            if (k & 1)
                rrr += y[k];
            else
                rl += y[k];
        }
        oL = L * .95f + ty * tmix + gl + rl * wet * .42f;
        oR = R * .95f + ty * tmix * sinw + grr + rrr * wet * .42f;
        dc[0] += (oL - dc[0]) * .0008f;
        dc[1] += (oR - dc[1]) * .0008f;
        oL -= dc[0];
        oR -= dc[1];
        {                                /* the world's body: two resonances and a slope, on everything */
            float *z = by_s[0];
            z[0] += by_c[0] * z[1];
            z[1] += by_c[0] * (oL - z[0] - BY_Q * z[1]);
            z[2] += by_c[1] * z[3];
            z[3] += by_c[1] * (oL - z[2] - BY_Q * z[3]);
            z[4] += (oL + z[1] * by_g[0] + z[3] * by_g[1] - z[4]) * by_t;
            oL = z[4];
            z = by_s[1];
            z[0] += by_c[0] * z[1];
            z[1] += by_c[0] * (oR - z[0] - BY_Q * z[1]);
            z[2] += by_c[1] * z[3];
            z[3] += by_c[1] * (oR - z[2] - BY_Q * z[3]);
            z[4] += (oR + z[1] * by_g[0] + z[3] * by_g[1] - z[4]) * by_t;
            oR = z[4];
        }
        {                                /* width: mid stays, the side moves */
            float md = (oL + oR) * .5f, sd = (oL - oR) * .5f * sh_width;
            oL = md + sd;
            oR = md - sd;
        }
        gainT = fm_clampf(.15f * fm_rsqrtf(rmsv + 1e-6f), .4f, 4.5f);   /* rmsv is what actually leaves, in any mode */
        gainT *= 1.0f - .35f * spk_mix;   /* the speaker: 3.7 dB less pushed into it, so at full volume nothing presses the cone */
#ifdef EN_NOAGC
        gainT = 1.0f;
        agc = 1.0f;
#endif
        agc += (gainT - agc) * .0000015f;
        if (usb_split) {                 /* what USB records: never the speaker's tuning, and its own level */
            float ul = oL, ur = oR, pk;
            if (usb_phone) {
                float bs = bBprev[i];
                ul = speaker(2, oL, bs);
                ur = speaker(3, oR, bs);
            }
            rmsc += (ul * ul + ur * ur - rmsc) * .000003f;
            agc_c += (fm_clampf(.26f * fm_rsqrtf(rmsc + 1e-6f), .5f, spk_on ? 3.0f : 6.0f) - agc_c) * .0000015f;   /* the speaker: a lone drone is not pushed into its cone */   /* 5 dB hotter than before */
            ul *= agc_c;
            ur *= agc_c;
            pk = fm_maxf(fm_fabsf(ul), fm_fabsf(ur));    /* a limiter: the loud moments come down, nothing is cut off */
            lim_e = pk > lim_e ? lim_e + (pk - lim_e) * .3f : lim_e * .99996f;
            if (lim_e > .7f) {
                ul *= .7f / lim_e;
                ur *= .7f / lim_e;
            }
            clL = softclip(ul) * fade_g;
            clR = softclip(ur) * fade_g;
        }
        oL *= agc;
        oR *= agc;
        {   /* the main limiter: the loudest moments (everything latched, held and played at once) come down smoothly,
             * so the soft clip after it only ever rounds a stray peak and never grinds a whole passage */
            float pk = fm_maxf(fm_fabsf(oL), fm_fabsf(oR));
            main_lim = pk > main_lim ? main_lim + (pk - main_lim) * .3f : main_lim * .99996f;
            if (main_lim > .75f) {
                oL *= .75f / main_lim;
                oR *= .75f / main_lim;
            }
        }
        spk_mix += ((spk_on ? 1.0f : 0.0f) - spk_mix) * .0005f;   /* about 45 ms: no click */
        if (spk_mix > .0005f) {
            float bs = bBprev[i] * agc;
            oL += (speaker(0, oL, bs) - oL) * spk_mix;
            oR += (speaker(1, oR, bs) - oR) * spk_mix;
        }
        rmsv += ((oL * oL + oR * oR) * (1.0f / (agc * agc)) - rmsv) * .000003f;
        oL = softclip(oL) * fade_g;
        oR = softclip(oR) * fade_g;
        if (spk_mix > .0005f) {          /* the speaker's own limiter: its amplifier and cone crackle above about -8 dB */
            float pk = fm_maxf(fm_fabsf(oL), fm_fabsf(oR)), g;
            spk_e = pk > spk_e ? spk_e + (pk - spk_e) * .003f : spk_e * .99997f;   /* 8 ms: slower than a deep drone's wave, so it never bends it */
            g = spk_e > .25f ? .25f / spk_e : 1.0f;   /* under the crackle line, with room to spare at full volume */
            g = 1.0f - (1.0f - g * .8f) * spk_mix;    /* and 2 dB under it always */
            oL *= g;
            oR *= g;
            {   /* presence: a broad, gentle +1 dB around 2.5 kHz, where ears hear best. The level stays the same */
                float bl, br;
                pr_l1 += (oL - pr_l1) * .52f;       /* under ~5 kHz */
                pr_r1 += (oR - pr_r1) * .52f;
                pr_l2 += (pr_l1 - pr_l2) * .16f;    /* over ~1.2 kHz */
                pr_r2 += (pr_r1 - pr_r2) * .16f;
                bl = pr_l1 - pr_l2;
                br = pr_r1 - pr_r2;
                oL += bl * .122f * spk_mix;
                oR += br * .122f * spk_mix;
            }
        }
        {   /* the speaker's peak catcher: it sees each peak 2 ms ahead and turns down smoothly before it arrives, so a
             * fast swell never reaches the cone's crackle line and nothing is ever clipped. Headphones: only the delay */
            float pk = fm_maxf(fm_fabsf(oL), fm_fabsf(oR)), want, dl, dr;
            want = pk > .3f ? .3f / pk : 1.0f;
            want = 1.0f - (1.0f - want) * spk_mix;
            if (want <= la_min) {
                la_min = want;
                la_hold = LA_N;
            } else if (la_hold > 0)
                la_hold--;
            else
                la_min += (1.0f - la_min) * .00005f;
            la_g += (la_min - la_g) * (la_min < la_g ? .06f : 1.0f);
            if (la_g > la_min && la_min >= la_g * .9999f) la_g = la_min;
            dl = la_buf[0][la_i];
            dr = la_buf[1][la_i];
            la_buf[0][la_i] = oL;
            la_buf[1][la_i] = oR;
            la_i = la_i + 1 < LA_N ? la_i + 1 : 0;
            oL = dl * la_g * (1.0f - .5f * spk_mix);   /* the speaker: 6 dB lower at the very end, so nothing can undo it (tested clean; 5 and 4.5 clicked) */
            oR = dr * la_g * (1.0f - .5f * spk_mix);
        }
        mono = (oL + oR) * .5f;
        lp_l += (mono - lp_l) * .0045f;
        lp_m += (mono - lp_m) * .07f;
        m_low += (fm_fabsf(lp_l) - m_low) * .002f;
        m_mid += (fm_fabsf(lp_m - lp_l) - m_mid) * .002f;
        m_hi += (fm_fabsf(mono - lp_m) - m_hi) * .004f;
        if (oL != oL || oR != oR) {
            en_st.bad++;
            oL = oR = 0.0f;
        }
        {
            float pk = fm_maxf(fm_fabsf(oL), fm_fabsf(oR));
            if (pk > en_st.peak)
                en_st.peak = pk;
        }
        duck_g = duck_g < duck_to ? fm_minf(duck_g + .000076f, duck_to) : fm_maxf(duck_g - .00028f, duck_to);   /* 80 ms down, 300 ms back: a short fade of a deep drone is itself a thump */
        oL *= duck_g;
        oR *= duck_g;
        if (usb_split) {
            usb_spk[2 * i] = (int32_t)(oL * gain);
            usb_spk[2 * i + 1] = (int32_t)(oR * gain);
            oL = clL * duck_g;
            oR = clR * duck_g;
        }
        lr[2 * i] = (int32_t)(oL * gain);
        lr[2 * i + 1] = (int32_t)(oR * gain);
    }
}

/* ------------------------------------------------------------ control --- */
static void spawn_free(int k)
{
    int i;
    for (i = 0; i < EN_NFA; i++)
        if (!fa[i].on) {
            spawn(k);
            return;
        }
}
/* ---- each sea's composer. All of them are slow: long drones first, layers of bells and bowls, things moving in the
 * dark. A series is a few events and a long rest; what the series is, is what makes one sea unlike another. */
static int toll_bell = -1;
static uint8_t sc_tune[8];
static void layer(int bell, float ratio, int oct, int n, float gap)
{
    toll_bell = bell;
    toll_ratio = ratio;
    toll_oct = oct;
    toll_n = n;
    toll_gap = gap;
    bell_left = 0.0f;
}
static float range(float a, float b) { return between(a, b); }
static float lead_pitch(int deg, float lo, float hi) { return pitch(deg, lo, hi); }
static void series(int n, float gap, float rest)
{
    sc_n = n;
    sc_i = 0;
    sc_gap = gap;
    sc_rest = rest;
    sc_t = 0.0f;
}
static void sc_begin(void)                  /* the next thing this sea does */
{
    const uint8_t *b = SEAS[sea].bell;
    int b0 = b[0], b1 = b[1] != 255 ? b[1] : b[0], i;
    swell = fm_maxf(swell, 1.0f);
    switch (W->style) {
    case ST_SWELL:                          /* a pressure wave, and iron a long way off */
        swell = 1.5f;
        layer(ri(3) ? b0 : b1, field[ri(3)], 1 + ri(2), 1 + ri(3), range(6, 13));
        series(0, 0, range(26, 68));
        break;
    case ST_CHORALE:                        /* the harmony turns slowly, and the bells answer */
        for (i = 2; i < 5; i++)
            field_t[i] = make_ratio();
        for (i = 0; i < EN_NCH; i++)
            if (ch[i].t > .7f)
                ch[i].tgt = pitch(i + 2, CHOIRS[ch[i].type].lo, CHOIRS[ch[i].type].hi);
        swell = 1.3f;
        layer(b0, field[ri(2)], 2, 3 + ri(4), range(5, 9));
        series(0, 0, range(30, 60));
        break;
    case ST_CHANT:                          /* a horn on the root, bowls blooming under it */
        lead(lead_pitch(0, 44, 92), range(7, 14), range(.6f, 1), 3.0f, 5.0f);
        layer(b0, field[ri(4)], 2, 2 + ri(4), range(4, 8));
        series(0, 0, range(20, 42));
        break;
    case ST_RITUAL:                         /* a skin struck somewhere below, then nothing */
        series(3 + ri(5), range(1.6f, 2.6f), range(16, 40));
        if (!ri(3))
            lead(lead_pitch(ri(6), 240, 520), range(3, 6), .5f, 1.5f, 2.5f);
        layer(b0, field[ri(6)], 2, 1 + ri(2), range(5, 9));
        break;
    case ST_SHIMMER:                        /* glass in the wind */
        layer(b0, field[ri(6)], 4, 3 + ri(6), range(.9f, 3));
        if (!ri(4))
            lead(lead_pitch(ri(6), 300, 800), range(6, 11), .4f, 3.0f, 5.0f);
        series(0, 0, range(10, 30));
        break;
    case ST_MA:                             /* one strike, and the silence around it */
        layer(b0, 1.0f, 1, 1, 1);
        if (fr() < .35f)
            lead(lead_pitch(ri(6), 300, 640), range(4, 8), .4f, 2.0f, 4.0f);
        series(0, 0, range(20, 46));
        break;
    case ST_COLOTOMIC:                      /* a slow turning ring of pelog, with the gong closing it */
        series(8, range(1.9f, 2.8f), range(12, 26));
        for (i = 0; i < 8; i++)
            if (!ri(3))
                sc_tune[i] = (uint8_t)ri(6);
        break;
    case ST_SEQUENCE:                       /* a tone on a dead frequency, a relay somewhere */
        lead(lead_pitch(ri(2) ? 0 : 1, 55, 130), range(6, 12), .55f, 2.5f, 4.0f);
        series(1 + ri(3), range(2, 4), range(14, 30));
        layer(b0, field[ri(6)], 3, 1 + ri(2), range(6, 11));
        break;
    case ST_PULSAR:                         /* a dead star's clock, and tones that take minutes */
        series(10 + ri(16), range(1.9f, 3.7f), range(12, 30));
        lead(lead_pitch(ri(6), 90, 300), range(8, 14), .45f, 4.0f, 6.0f);
        break;
    case ST_PROCESSION:                     /* something walks, slowly, and a reed mourns */
        series(4 + ri(5), range(2.6f, 3.6f), range(12, 26));
        sc_deg = ri(6);
        layer(b0, field[ri(6)], 2, 2 + ri(3), range(5, 9));
        break;
    case ST_RIVER:                          /* strings touched by water */
        series(6 + ri(9), range(1.1f, 2.5f), range(14, 30));
        sc_deg = ri(6);
        break;
    case ST_DIRGE:                          /* the ground */
        swell = 1.45f;
        series(2 + ri(4), range(5, 9), range(14, 30));
        break;
    case ST_MONITOR:                        /* a heart, and a tune that has forgotten itself */
        series(10 + ri(12), range(1.7f, 2.3f), range(14, 28));
        for (i = 0; i < 8; i++)
            if (sc_tune[i] > 5 || !ri(4))
                sc_tune[i] = (uint8_t)ri(6);
        layer(b0, field[sc_tune[0] % 6], 3, 1, 1);
        break;
    case ST_MACHINE:                        /* far off, a works that never quite stopped */
        swell = 1.3f;
        series(3 + ri(5), range(2, 5), range(14, 30));
        break;
    default:                                /* something knocking, and pipes in the trees */
        series(2 + ri(4), range(.5f, 1.1f), range(10, 26));
        if (!ri(2))
            lead(lead_pitch(ri(6), 55, 150), range(5, 10), .55f, 2.5f, 4.0f);
        layer(ri(2) ? b0 : b1, field[ri(6)], 2, 1 + ri(3), range(5, 10));
        break;
    }
    (void)b1;
}
static void auto_bell(int b, float r, int o, float a)
{
    if (sh_bells > 0.0f)
        strike_bell(b, r, o, a);
}
static void sc_hit(void)                     /* one event of the series */
{
    const uint8_t *b = SEAS[sea].bell;
    int b0 = b[0], b1 = b[1] != 255 ? b[1] : b[0];
    switch (W->style) {
    case ST_RITUAL: drum(DR_SKIN, range(.5f, 1)); break;
    case ST_COLOTOMIC:
        auto_bell(sc_i == 7 ? b0 : b1, field[sc_tune[sc_i] % 6], sc_i == 7 ? 1 : 3, sc_i == 7 ? .75f : .32f);
        break;
    case ST_SEQUENCE: drum(DR_TICK, .6f); break;
    case ST_PULSAR:
        drum(DR_TICK, .7f);
        if (!(sc_i % 5))
            auto_bell(b0, field[ri(6)], 4, .2f);
        break;
    case ST_PROCESSION:
        drum(DR_SKIN, sc_i & 1 ? .55f : .85f);
        if (!(sc_i % 2)) {
            sc_deg = (sc_deg + (ri(2) ? 1 : 5)) % 6;
            lead(lead_pitch(sc_deg, 110, 330), sc_gap * 1.6f, .7f, .7f, 1.8f);
        }
        break;
    case ST_RIVER:
        sc_deg = (sc_deg + (ri(3) ? 1 : 2)) % 6;
        auto_bell(b0, field[sc_deg], 3 + (sc_i & 1), range(.22f, .42f));
        sc_gap = fm_clampf(sc_gap * range(.85f, 1.2f), .9f, 3.0f);
        break;
    case ST_DIRGE:
        drum(DR_SUB, 1);
        swell = 1.35f;
        auto_bell(b0, field[0], 1, .5f);
        break;
    case ST_MONITOR:
        drum(DR_SUB, .55f);
        if (!(sc_i % 3))
            auto_bell(b0, field[sc_tune[(sc_i / 3) & 7] % 6], 3, range(.12f, .24f));
        break;
    case ST_MACHINE:
        drum(DR_HAMMER, range(.4f, .8f));
        if (!ri(3))
            auto_bell(ri(2) ? b0 : b1, field[ri(6)], 2, .3f);
        break;
    case ST_ROOTS:
        drum(DR_WOOD, range(.5f, 1));
        sc_gap *= .78f;
        break;
    default: break;
    }
    sc_i++;
}
/* ---- the motif: a few notes a world makes up, that come back changed. Only its intervals are kept, in today's scale. */
static void motif_make(void)
{
    static const float D[4] = {1.0f, 1.0f, 1.5f, 2.0f};
    uint32_t r = (uint32_t)mo_seed * 16843009u + 12345u;
    int i, d = 0;
    mo_n = 3 + mo_len;
    {   /* its own voice, kept with the tune so it is known when it comes back: a music box, a celesta, a wine glass,
         * or ghost chimes */
        static const char *const MB[4] = {"MUSIC BOX", "CELESTA", "WINE GLASS", "GHOST CHIME"};
        const char *want = MB[(r >> 3) & 3u];
        int b;
        mo_voiceb = -1;
        for (b = 0; b < NBELL && mo_voiceb < 0; b++) {
            const char *x = BELLS[b].name, *y = want;
            while (*x && *x == *y)
                x++, y++;
            if (!*x && !*y)
                mo_voiceb = b;
        }
    }
    mo_twice = 1;                        /* a new tune is played twice the first time, so the ear catches it */
    for (i = 0; i < 6; i++) {
        r ^= r << 13;
        r ^= r >> 17;
        r ^= r << 5;
        mo_deg[i] = (int8_t)d;
        mo_dur[i] = D[r >> 28 & 3u];
        d += (int)(r >> 8 & 3u) + 1;
        if (r >> 12 & 1u)
            d -= 2 * ((int)(r >> 8 & 3u) + 1);
        d = d > 7 ? 7 : d < -5 ? -5 : d;
    }
}
static float mo_ratio(int d)
{
    int n = FAM_N[family], o = 0;
    if (!n)
        return 16.0f / (float)UNDER_N[((d % 13) + 13) % 13];
    while (d < 0) {
        d += n;
        o--;
    }
    while (d >= n) {
        d -= n;
        o++;
    }
    return fm_exp2f(FAM_ST[family][d] / 12.0f + (float)o);
}
static void motif_begin(void)
{
    int kind = ri(6);
    mo_i = 0;
    mo_t = 0.0f;
    mo_gap = .7f + fr() * .5f;           /* close enough together to be heard as a tune */
    mo_oct = 2;
    mo_drop = -1;
    mo_inv = 0;
    mo_bell = mo_voiceb >= 0 ? mo_voiceb : SEAS[sea].bell[0];
    if (kind == 1)
        mo_gap *= 1.5f;                  /* slower */
    else if (kind == 2)
        mo_oct = 1;                      /* lower */
    else if (kind == 3 && SEAS[sea].bell[2] != 255)
        mo_bell = SEAS[sea].bell[2];     /* on another bell */
    else if (kind == 4)
        mo_drop = 1 + ri(mo_n - 1);      /* a note missing */
    else if (kind == 5)
        mo_inv = 1;                      /* upside down */
    mo_use = mo_voice ? mo_voice : (W->lead_voice >= 0 && fr() < .2f ? 2 : 1);   /* mostly its own bell */
    if (W->lead_voice < 0 || W->lead <= 0.0f)
        mo_use = 1;
}
static void motif_step(float dt)
{
    static const float LO[4] = {0, 360, 180, 90}, HI[4] = {0, 600, 360, 180};
    if (!mo_rate) {
        mo_use = -1;
        return;
    }
    if (mo_use < 0) {
        mo_left -= dt;
        if (mo_left <= 0.0f)
            motif_begin();
        return;
    }
    mo_t -= dt;
    if (mo_t > 0.0f)
        return;
    if (mo_i >= mo_n && mo_twice) {     /* once more, the same, after a breath */
        mo_twice = 0;
        mo_i = 0;
        mo_t = mo_gap * 2.5f;
        return;
    }
    if (mo_i >= mo_n) {
        mo_use = -1;
        mo_left = between(LO[mo_rate], HI[mo_rate]);
        return;
    }
    if (mo_i != mo_drop) {
        float r = mo_ratio(mo_inv ? -mo_deg[mo_i] : mo_deg[mo_i]);
        if (mo_use != 2) {
            mo_striking = 1;             /* clear and in front: struck like a key's bell, a little louder */
            strike_bell(1000 + mo_bell, r, mo_oct, .8f);
            mo_striking = 0;
        }
        if (mo_use != 1) {
            float f = root_hz * r;
            while (f < 110.0f)
                f *= 2.0f;
            while (f > 440.0f)
                f *= .5f;
            lead(f, mo_gap * mo_dur[mo_i] * 1.2f, .6f, .35f, 1.4f);
        }
    }
    mo_t = mo_gap * mo_dur[mo_i];
    mo_i++;
}
static void score_step(float dt)
{
    float act = .45f + st[1] * .35f + st[2] * .2f;      /* the strata lean on the composer: VOID slows it */
    swell += (1.0f - swell) * dt / 14.0f;
    sc_t -= dt * act * fm_maxf(sh_bells, .25f);
    if (sc_t > 0.0f)
        return;
    if (sc_n > 0) {
        sc_hit();
        sc_n--;
        sc_t = sc_n ? sc_gap * (.92f + fr() * .16f) : sc_rest;
        return;
    }
    sc_begin();
    if (!sc_n)
        sc_t = sc_rest;
}
static void wander(float dt)
{
    static const float PERIOD[3] = {50.0f, 400.0f, 2400.0f};
    int i, j;
    for (i = 0; i < N_MACRO; i++) {
        float age, w;
        for (j = 0; j < 3; j++) {
            wob[i][j].left -= dt;
            if (wob[i][j].left <= 0.0f) {
                wob[i][j].tgt = n1();
                wob[i][j].left = PERIOD[j] * (.5f + fr());
            }
            wob[i][j].v += (wob[i][j].tgt - wob[i][j].v) * fm_minf(1.0f, dt / (PERIOD[j] * .35f));
        }
        age = fm_minf(1.0f, (t_now - touched[i]) / 300.0f);
        w = (wob[i][0].v * .22f + wob[i][1].v * .34f + wob[i][2].v * .44f) * tide * .55f * age;
        macro_m[i] = fm_clampf(macro_u[i] + w, 0.0f, 1.0f);
    }
}

static void control(void)
{
    float dt = CTL_DT, ease = dt / 16.0f;
    int i;
    t_now += dt;
    if (t_now > 7200.0f) {               /* a float stops counting after a day and a half: 7200 s is whole cycles of its sines */
        int q;
        t_now -= 7200.0f;
        for (q = 0; q < N_MACRO; q++)
            touched[q] -= 7200.0f;
        for (q = 0; q < EN_NFA; q++)
            fa[q].born -= 7200.0f;
    }
    sw += (sw_on - sw) * fm_minf(1.0f, dt / (sw_on > sw ? 5.0f : 1.2f));
    sw_tail *= 1.0f - dt / 7.0f;
    if (!frozen)
        wander(dt);
    for (i = 0; i < 5; i++)
        st[i] += (STRATA[stratum][i] - st[i]) * ease;
    root_hz += (root_target() - root_hz) * (dt / (k_mode == 1 ? 6.0f : 9.0f));
    {
        int v;
        float lv_k, lv_w, rel = dt / (.4f + (float)k_fade * .1f);
        {
            float room = 1.0f;
            for (v = 0; v < EN_NFA; v++)
                if (fa[v].on && fa[v].forced && t_now - fa[v].born < 7.0f && t_now >= fa[v].born)
                    room = .6f;
            if (kb_room > 0.0f) {        /* a bell from the keys: the voices step back while it speaks */
                kb_room -= dt;
                room = fm_minf(room, .55f);
            }
            kv_room += (room - kv_room) * fm_minf(1.0f, dt / (room < kv_room ? .5f : 2.5f));
        }
        lv_k = (float)k_lv * .01f * 1.6f * kv_room;
        lv_w = lv_k * .06f;   /* the world's choirs are made for far smaller amplitudes */
        for (v = 0; v < NKV; v++) {
            choir_v *c = &kv[v];
            if (kv_gate[v])
                c->amp += ((c->type < 0 ? lv_w : lv_k) - c->amp) * fm_minf(1.0f, dt / 1.3f);
            else
                c->amp = fm_maxf(0.0f, c->amp - fm_maxf(c->amp, .05f) * rel);
            c->t = fm_minf(1.0f, c->t + dt / 9.0f);
            vowel_step(c, c->type < 0 ? &VX_CH[ch[0].type] : &VX_KV[c->type], dt);
        }
    }
    for (i = 0; i < 6; i++)
        field[i] += (field_t[i] - field[i]) * (dt / 9.0f);
    if (!frozen && playing) {
        s_left -= dt * evolve;
        field_left -= dt * evolve;
        era_left -= dt * evolve;
        if (s_left <= 0.0f)
            next_stratum();
        if (field_left <= 0.0f) {
            field_left = (28.0f + fr() * 120.0f) * (1.25f - macro_m[M_UNREST]);
            field_t[2 + ri(4)] = make_ratio();
            if (fr() < .22f)
                t_tgt = 1.1f + fr() * 3.6f;
        }
        if (era_left <= 0.0f)
            upheaval(1);
        {                                /* the relief: 4 voices latched and 3+ effects held is the red zone */
            int v2, nv = 0, nh = 0;
            uint32_t w2;
            for (v2 = 0; v2 < NKV; v2++)
                nv += kv_gate[v2] ? 1 : 0;
            for (v2 = 0; v2 < EN_MASK; v2++)
                for (w2 = held[v2]; w2; w2 &= w2 - 1u)
                    nh++;
            busy_n = nv + nh;
            stress = nv + nh >= (stress ? 5 : 6);   /* in at 6 (the most there can be), out at 4: no flapping */
        }
        wduck += ((stress ? .15f : 1.0f) - wduck) * fm_minf(1.0f, dt / 1.5f);
        wduck2 += ((stress ? 0.0f : 1.0f) - wduck2) * fm_minf(1.0f, dt / 1.0f);
        walk_left -= dt * evolve;
        if (walk_left <= 0.0f) {         /* the ground takes a step: home, or a note of the world's scale near it */
            int nn = FAM_N[family], j;
            float to = 0.0f;
            if (fr() > .35f && k_mode != 1) {
                if (nn == 0) {
                    static const float U[5] = {0, -2, 5, 7, -5};
                    to = U[ri(5)];
                } else {
                    to = FAM_ST[family][ri(nn)];
                    if (to > 7.0f)
                        to -= 12.0f;
                }
            }
            walk_st = to;
            ev_note(1, hz_note(root_target()), 80);
            if (fr() < .45f)             /* the drone's chord: the world's own, or it leans somewhere new */
                dch_t = SIG[sea].r[1] / SIG[sea].r[0];
            else {
                static const float IV[7] = {1.5f, 1.3348f, 2.0f, 1.1892f, 1.0595f, 3.0f, 1.4142f};
                dch_t = IV[ri(7)];
            }
            pd_semi[0] = 0;              /* the pads: a third from the scale, a fifth, and one more on top */
            pd_semi[1] = 3;
            for (j = 0; j < nn; j++)
                if (FAM_ST[family][j] >= 2.5f && FAM_ST[family][j] <= 4.5f)
                    pd_semi[1] = (int)(FAM_ST[family][j] + .5f);
            pd_semi[2] = fr() < .5f ? 7 : nn ? (int)(FAM_ST[family][ri(nn)] + 12.5f) : 12;
            for (j = 0; j < 3; j++)
                ev_note(2, hz_note(root_target()) + 12 + pd_semi[j], 60);
            walk_left = (16.0f + fr() * 38.0f) * (1.25f - macro_m[M_UNREST] * .5f);
        }
        dr_left -= dt * evolve;
        if (dr_left <= 0.0f) {           /* the drone moves on by itself: every 1 to 5 minutes (sooner with UNREST) */
            dr_left = (60.0f + fr() * 240.0f) * (1.2f - macro_m[M_UNREST] * .4f);
            if (pick[1] < 0) {
                int tries = 6;
                do {
                    d_next = fr() < .4f ? list_pick(SEAS[sea].drone, 3) : (uint8_t)ri(NDRONE);   /* a light lean, else any drone */
                } while (d_next == d_type && --tries);
            }
        }
        scn_left -= dt * evolve;
        if (scn_left <= 0.0f) {          /* a new scene: something else in front, and the voices themselves change */
            int nx = scene_lock >= 0 ? scene_lock : ri(6);
            if (nx == scene && scene_lock < 0)
                nx = (nx + 1 + ri(5)) % 6;
            scene = nx;
            ev_note(3, scene, 0);
            scn_left = (28.0f + fr() * 72.0f) * (1.3f - macro_m[M_UNREST] * .5f);
            if (pick[2] < 0 && fr() < .6f) {
                if (scene == 2) {
                    static const uint8_t CHANT[4] = {1, 5, 12, 20};
                    c_type = CHANT[ri(4)];
                } else {                 /* a light lean (about 1 in 3) to the world's own; otherwise any choir at all */
                    int tries = 6, was = c_type;
                    do c_type = fr() < .35f ? list_pick(SEAS[sea].choir, 4) : (uint8_t)ri(NCHOIR);
                    while (c_type == was && --tries);
                }
            }

            if (pick[0] < 0 && fr() < .4f)
                b_type = fr() < .35f ? list_pick(SEAS[sea].bell, 4) : (uint8_t)ri(NBELL);   /* a light lean, else any bell */
        }
        bloom_ph += dt * .0135f * TWO_PI;
        if (bloom_ph > 1000.0f)
            bloom_ph -= 942.47780f;
        bell_left -= dt * sh_bells * sc_mix[3];
        if (toll_n > 0 && bell_left <= 0.0f && sh_bells > 0.0f) {     /* the layers: each bell still ringing when the next arrives */
            strike_bell(toll_bell, toll_ratio, toll_oct, .3f + fr() * .3f);
            toll_n--;
            bell_left = toll_gap * (.8f + fr() * .4f);
            if (fr() < .4f)
                toll_ratio = field[ri(6)];
        }
        score_step(dt);
        motif_step(dt);
        fauna_left -= dt;
        if (fauna_left <= 0.0f) {
            float rate = W->fx_min / 60.0f * (.45f + st[2] * 1.3f + macro_m[M_UNREST] * .5f) * fm_maxf(sh_fx, .01f) * sc_mix[4];
            if (stress)
                fauna_left = 4.0f;           /* six at once: none by itself just now, but look again soon (a rate of 0 here
                                              * once meant never again, for the rest of the session) */
            else {
                if (sh_fx > 0.0f)
                    spawn(-1);
                fauna_left = -fm_logf(1.0f - fr() * .999f) / fm_maxf(rate, 1e-4f);
                if (fauna_left > 600.0f)
                    fauna_left = 600.0f;
            }
        }
    }
    if (!frozen && playing && (held[0] | held[1] | held[2] | held[3] | held[4])) {   /* a held effect comes back, with a breath between */
        int h, v2;
        for (h = 0; h < NSFX; h++) {
            int on = 0;
            if (!(held[h >> 5] >> (h & 31) & 1u))
                continue;
            for (v2 = 0; v2 < EN_NFA; v2++)
                if (fa[v2].on && fa[v2].kind == h)
                    on = 1;
            if (!on) {
                held_gap[h % EN_NFA] -= dt;
                if (held_gap[h % EN_NFA] <= 0.0f) {
                    spawn_free(h);
                    held_gap[h % EN_NFA] = .4f + fr() * 1.4f;
                }
            }
        }
    }
    if (d_next != d_type) {
        d_mix -= dt / 4.0f;
        if (d_mix <= 0.0f) {
            d_mix = 0.0f;
            d_type = d_next;
        }
    } else
        d_mix = fm_minf(1.0f, d_mix + dt / 6.0f);
    br_ph += dt * SIG[sea].br * TWO_PI;
    if (br_ph > 1000.0f)
        br_ph -= 942.47780f;
    breath = 1.0f - SIG[sea].bd * (.5f + .5f * fm_sinf(br_ph));
    {                                   /* the body follows a world change over a few seconds */
        float k = fm_minf(1.0f, dt / 2.5f);
        by_c[0] += (2.0f * fm_sin_pi(FM_PI * SIG[sea].f1 / EN_SR) - by_c[0]) * k;
        by_c[1] += (2.0f * fm_sin_pi(FM_PI * SIG[sea].f2 / EN_SR) - by_c[1]) * k;
        by_g[0] += (SIG[sea].g1 * BY_Q - by_g[0]) * k;
        by_g[1] += (SIG[sea].g2 * BY_Q - by_g[1]) * k;
        by_t += (1.0f - fm_expf(-TWO_PI * SIG[sea].tilt * sh_dark / EN_SR) - by_t) * k;
    }
    for (i = 0; i < 6; i++)
        sc_mix[i] += ((scene_lock >= 0 ? fm_maxf(.05f, 1.0f + (SCENE[scene][i] - 1.0f) * 1.5f) : SCENE[scene][i]) - sc_mix[i]) *
                     fm_minf(1.0f, dt / (scene_fast > 0.0f ? 2.5f : 12.0f));
    if (scene_fast > 0.0f)
        scene_fast -= dt;
    dch += (dch_t - dch) * fm_minf(1.0f, dt / 8.0f);
    for (i = 0; i < NPD; i++) {          /* the pads follow the ground, folded into their own range */
        float f = root_hz * fm_exp2f((float)pd_semi[i] / 12.0f) * sh_pitch;
        while (f < 98.0f)
            f *= 2.0f;
        while (f > 392.0f)
            f *= .5f;
        pd[i].ft = f;
        if (pd[i].f < 1.0f)
            pd[i].f = f;
        pd[i].f += (pd[i].ft - pd[i].f) * fm_minf(1.0f, dt / 4.0f);
        pd[i].amp += (1.0f - pd[i].amp) * fm_minf(1.0f, dt / 6.0f);
    }
    pad_lv = sc_mix[1] * (.35f + st[3] * .4f + st[1] * .25f) * W->drone * swell * lv[L_DRONE] * (playing ? 1.0f : 0.0f) * (1.0f - sw_tail * .75f) * (stress ? 0.0f : 1.0f);
    pad_cut = (240.0f + (1.0f - macro_m[M_DEPTH]) * 650.0f) * sc_mix[5] * (1.0f + .5f * fm_sinf(bloom_ph + 1.3f)) * sh_dark;
    for (i = 0; i < EN_NDR; i++) {
        drone_v *d = &dr[i];
        d->f = (i < 2 ? root_hz * (sh_chord ? CHORD[sh_chord - 1][i] : i ? SIG[sea].r[0] * dch : SIG[sea].r[0]) : pitch(d->idx, 33.0f, 175.0f)) * sh_pitch;
        d->w += d->wr * dt * TWO_PI;
        d->sw += (.04f + d->wr) * dt * TWO_PI;
        if (d->w > 1000.0f)
            d->w -= 942.47780f;
        if (d->sw > 1000.0f)
            d->sw -= 942.47780f;
        d->amp = (.5f + .22f * fm_sinf(d->w)) * (.5f + st[0] * .5f) * W->drone * sc_mix[0] * swell * lv[L_DRONE] * d_mix * (i < 2 ? 1.0f : wduck2) * (playing ? 1.0f : 0.0f) * breath * (1.0f + sw * 1.3f) * (1.0f - sw_tail * .75f);
    }
    for (i = 0; i < NB; i++) {
        if (!bl[i].on)
            continue;
        if (bl[i].att > 0.0f)
            bl[i].env = fm_minf(1.0f, bl[i].env + bl[i].att);
        if (bl[i].bend != 0.0f) {
            float r;
            int p;
            bl[i].age += dt;
            r = fm_exp2f(bl[i].bend * fm_sinf(bl[i].age * .9f) * dt / 12.0f);
            for (p = 0; p < NP; p++) {
                bl[i].w[p] = fm_minf(bl[i].w[p] * r, 2.6f);
                bl[i].c[p] = fm_cosf(bl[i].w[p]);
                bl[i].s[p] = fm_sinf(bl[i].w[p]);
            }
        }
    }
    for (i = 0; i < EN_NCH; i++) {
        choir_v *c = &ch[i];
        const choir_t *Y;
        float e, t;
        {   /* a voice still singing the old choir: it bows out in about 2 seconds, from wherever it is, and the new
             * choir comes in on the next breath. Not 18 to 46 seconds of waiting */
            int want = pick[2] >= 0 ? pick[2] : c_type, hurry = !frozen && c->type != want;
            if (hurry) {
                if (c->t < .4f)
                    c->t = .95f - .37f * c->t / .4f;     /* the same loudness on the way down: no jump */
                else if (c->t < .58f)
                    c->t = .58f;
                c->t += dt * .19f;
            }
            if (!frozen)
                c->t += dt / c->len;
            if (c->t >= 1.0f) {
                c->t = hurry ? .1f : c->t - 1.0f;
                c->len = 18.0f + fr() * 28.0f;
                c->idx = ri(6);
                if (pick[2] < 0 && fr() < .03f)    /* now and then a new choir: any of them, a light lean to the world's own */
                    c_type = fr() < .35f ? list_pick(SEAS[sea].choir, 4) : (uint8_t)ri(NCHOIR);
                c->type = pick[2] >= 0 ? pick[2] : c_type;
                c->tgt = pitch(c->idx, CHOIRS[c->type].lo, CHOIRS[c->type].hi);
            }
        }
        Y = &CHOIRS[c->type];
        c->f += (c->tgt - c->f) * (dt / Y->glide);
        t = c->t;
        e = t < .4f ? fm_sin_turns(t * 2.5f * .25f) : t < .58f ? 1.0f : t < .95f ? fm_cosf((t - .58f) / .37f * FM_PI * .5f) : 0.0f;
        c->amp = e * e * (st[3] * .8f + .1f) * W->choir * sc_mix[2] * (i < 2 ? wduck : wduck * wduck2) * lv[L_CHOIR] * (playing ? 1.0f : 0.0f);
        vowel_step(c, &VX_CH[c->type], dt);
    }
    for (i = 0; i < EN_NFA; i++) {
        fauna_v *c = &fa[i];
        if (!c->on)
            continue;
        c->t += dt / c->life;
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        c->a = fm_sin_turns(fm_minf(1.0f, c->t) * .5f);
        if (c->t >= 1.0f)
            c->on = 0;
    }
    t_time += (t_tgt - t_time) * (dt / 7.0f);
    wow += dt * (.31f + macro_m[M_ROT] * .4f);
    if (wow > 1000.0f)
        wow -= 1000.0f;
    if (fr() < dt * (.1f + macro_m[M_ROT] * .9f))
        drp_t = .25f + fr() * .75f;
    else if (fr() < dt * 2.0f)
        drp_t = 1.0f;
    drp += (drp_t - drp) * fm_minf(1.0f, dt / .08f);
}

static void keep_world(en_fossil_t *f)
{
    int i, j;
    __builtin_memset(f, 0, sizeof *f);
    f->used = 1;
    f->sea = (uint8_t)sea;
    f->family = (uint8_t)family;
    f->palette = (uint8_t)palette;
    f->stratum = (uint8_t)stratum;
    f->era = (uint8_t)era;
    f->root_offset = (int8_t)root_offset;
    f->pick_bell = pick[0];
    f->pick_drone = pick[1];
    f->pick_choir = pick[2];
    f->bell = b_type;
    f->drone = d_next;
    f->choir = c_type;
    for (i = 0; i < 6; i++)
        f->field[i] = field_t[i];
    for (i = 0; i < N_MACRO; i++)
        f->macro[i] = macro_u[i];
    f->tape = t_tgt;
    __builtin_memcpy(f->cast, cast, sizeof cast);
    for (i = 0; i < EN_NSTRATA; i++)
        for (j = 0; j < EN_NSTRATA; j++)
            f->trans[i][j] = (uint8_t)(trans[i][j] * 100.0f);
    f->scene1 = (uint8_t)(scene + 1);    /* the moment itself */
    f->walk = (int8_t)walk_st;
    f->dchx = (uint8_t)(dch_t * 50.0f + .5f);
    for (i = 0; i < 3; i++)
        f->pds[i] = (int8_t)pd_semi[i];
}
static void revive_world(const en_fossil_t *f)
{
    int i, j;
    sea = f->sea % NSEA;
    family = f->family % NFAM;
    palette = f->palette % 15;
    prev_stratum = stratum;
    stratum = f->stratum % EN_NSTRATA;
    era = f->era;
    root_offset = f->root_offset;
    pick[0] = f->pick_bell < NBELL ? f->pick_bell : -1;   /* a fossil from another build: never past a table's end */
    pick[1] = f->pick_drone < NDRONE ? f->pick_drone : -1;
    pick[2] = f->pick_choir < NCHOIR ? f->pick_choir : -1;
    b_type = f->bell % NBELL;
    d_next = f->drone % NDRONE;
    c_type = f->choir % NCHOIR;
    for (i = 0; i < 6; i++)
        field_t[i] = f->field[i];
    for (i = 0; i < N_MACRO; i++)
        macro_u[i] = f->macro[i];
    t_tgt = f->tape;
    __builtin_memcpy(cast, f->cast, sizeof cast);
    for (i = 0; i < EN_NSTRATA; i++)
        for (j = 0; j < EN_NSTRATA; j++)
            trans[i][j] = (float)f->trans[i][j] * .01f;
    s_left = 90.0f + fr() * 180.0f;
    if (f->scene1) {                     /* a fossil kept since 0.99.8 brings its moment back too */
        scene = (f->scene1 - 1) % 6;
        walk_st = (float)f->walk;
        dch_t = (float)f->dchx * .02f;
        for (i = 0; i < 3; i++)
            pd_semi[i] = f->pds[i];
        scn_left = 30.0f + fr() * 40.0f;
        walk_left = 20.0f + fr() * 30.0f;
    }
    ev_put(EV_SEA, 0, 0, 0);
}

static void commands(void)
{
    while (q_r != q_w) {
        cmd_t c = q[q_r % QN];
        float v = (float)c.b * (1.0f / 4096.0f);
        BARRIER();
        q_r++;
        switch (c.c) {
        case C_MACRO: macro_u[c.a] = v; touched[c.a] = t_now; break;
        case C_LAYER: lv[c.a] = v; break;
        case C_TIDE: tide = v; break;
        case C_SEA: set_sea(c.a); break;
        case C_DRIFT: sea_drift = c.a; break;
        case C_PICK:
            if (c.a < 0 || c.a > 2)
                break;
            if (c.b >= (c.a == 0 ? NBELL : c.a == 1 ? NDRONE : NCHOIR))   /* a choice past the end: the world's own */
                c.b = -1;
            pick[c.a] = (int8_t)c.b;
            if (c.a == 1)
                d_next = c.b >= 0 ? (uint8_t)c.b : SEAS[sea].drone[0];
            if (c.a == 0 && c.b >= 0)
                b_type = (uint8_t)c.b;
            if (c.a == 2 && c.b >= 0)
                c_type = (uint8_t)c.b;
            break;
        case C_CAST:
            if (c.b)
                cast[c.a >> 5] |= 1u << (c.a & 31);
            else
                cast[c.a >> 5] &= ~(1u << (c.a & 31));
            break;
        case C_CASTW: cast[c.a] = (uint32_t)c.b; break;
        case C_FIRE: spawn_soft = c.b; spawn(c.a); spawn_soft = 0; break;
        case C_PLAY: playing = c.a; break;
        case C_FREEZE: frozen = c.a; break;
        case C_OMEN: next_stratum(); ev_put(EV_OMEN, 0, 0, 0); break;
        case C_MUTATE:
            field_t[2 + ri(4)] = make_ratio();
            field_left = 15.0f;
            dr[ri(EN_NDR)].mr = MR[ri(6)];
            t_tgt = 1.1f + fr() * 3.6f;
            break;
        case C_UPHEAVAL: upheaval(0); break;
        case C_OFFER: {
            float ratio = fm_exp2f((float)c.a / 12.0f);
            field_t[2 + ri(4)] = ratio;
            strike(ratio, 2 + c.b, .63f);
            rng ^= (uint32_t)plat_ticks24();
            if (!rng)
                rng = 1;
            break;
        }
        case C_KEEP:
            if (keep_to) {
                keep_world(keep_to);
                keep_done = 1;
            }
            break;
        case C_REVIVE:
            if (revive_from)
                revive_world(revive_from);
            break;
        case C_PANIC: playing = 0; agc = 1.0f; break;
        case C_SCENE:                    /* a scene held: the world stays in it until let go (-1: its own choice) */
            scene_lock = c.a - 1;
            scene_fast = 4.0f;           /* chosen by hand: heard within seconds */
            if (scene_lock >= 0 && scene != scene_lock) {
                scene = scene_lock;
                ev_note(3, scene, 0);
            }
            if (scene_lock == 2 && pick[2] < 0) {   /* chant: a chanting choir */
                static const uint8_t CH2[4] = {1, 5, 12, 20};
                c_type = CH2[ri(4)];
            }
            break;
        case C_LETGO: {                  /* every latched voice lets go and fades */
            int v;
            for (v = 0; v < NKV; v++)
                kv_gate[v] = 0;
            break;
        }
        case C_KEYS:
            if ((c.a & 15) != k_mode) { /* latched voices sing on whatever the keys play now: only you let them go */
                int v;
                kd_shift = 0;
                for (v = 0; v < NKV; v++)   /* a voice still held from the old mode lets go, so it can't get stuck */
                    if (!kv_lat[v])
                        kv_gate[v] = 0;
            }
            k_mode = c.a & 15;
            k_vox = c.a >> 4 & 255;
            k_bell = c.b >> 16 & 255;
            if (k_bell > EN_NBELL)
                k_bell = 0;
            if (k_vox >= EN_NKVOX)
                k_vox = 0;
            if (k_mode > 4)
                k_mode = 0;    /* in b: a is 16 bits, and bells past the 7th fell off its top */
            k_lv = c.b & 255;
            k_fade = c.b >> 8 & 255;
            break;
        case C_KEY: {
            int k = c.a & 255, oct = (c.a >> 8 & 15) - 4, on = c.a >> 12 & 1, v, best = 0, mode = k_mode;
            if (mode == 4 && k >= 100)   /* split from MIDI: below middle C bells, above it voices */
                mode = k - 100 < 60 ? 0 : 2;
            else if (mode == 4) {        /* split on the FM-1: the bottom row rings, the top row sings, both in the scale */
                static const float U[5] = {0, 3, 5, 7, 10};
                int black = (int)(0x54Au >> ((k + 5) % 12) & 1u), pos = 0, j, nn = FAM_N[family], deg;
                for (j = 0; j < k; j++)
                    pos += (int)(0x54Au >> ((j + 5) % 12) & 1u) == black;
                deg = black ? pos + 2 : pos;    /* the voices start a little higher in the scale */
                c.b = nn ? (int)(FAM_ST[family][deg % nn] + .5f) + 12 * (deg / nn) : (int)U[deg % 5] + 12 * (deg / 5);
                mode = black ? 2 : 0;
            }
            if (!on) {
                for (v = 0; v < NKV; v++)
                    if (kv_key[v] == k && !kv_lat[v])
                        kv_gate[v] = 0;
                break;
            }
            if (mode == 3) {             /* latched: the same key again lets it go; 4 at most */
                int found = 0, n = 0;
                for (v = 0; v < NKV; v++)
                    if (kv_gate[v] && kv_key[v] == k) {
                        kv_gate[v] = 0;
                        found = 1;
                    }
                for (v = 0; v < NKV; v++)
                    n += kv_gate[v] && kv_lat[v];   /* only latched ones count: a stray held voice never blocks */
                if (found || n >= NKV)
                    break;
                {                        /* six at once at most: latched voices + kept effects */
                    uint32_t w2;
                    int j2;
                    for (j2 = 0; j2 < EN_MASK; j2++)
                        for (w2 = held[j2]; w2; w2 &= w2 - 1u)
                            n++;
                }
                if (n >= 6)
                    break;
            }
            if (mode == 0) {
                float ratio = fm_exp2f((float)c.b / 12.0f);
                field_t[2 + ri(4)] = ratio;
                kb_room = 2.0f;          /* the latched voices step back while this bell speaks */
                strike_bell(1000 + (k_bell ? k_bell - 1 : SEAS[sea].bell[0]), ratio, 2 + oct, .63f * (float)k_lv / 70.0f);
            } else if (mode == 1) {
                kd_shift += c.b > 6 ? c.b - 12 : c.b;   /* the nearest way to that note */
                while (kd_shift > 6)
                    kd_shift -= 12;
                while (kd_shift < -6)
                    kd_shift += 12;
            } else {
                const choir_t *Y = k_vox ? &KVOX[k_vox - 1] : &CHOIRS[ch[0].type];
                float f = root_hz * fm_exp2f((float)c.b / 12.0f + (float)oct);
                best = -1;
                for (v = 0; v < NKV; v++) {             /* a free voice, or the quietest let-go one; never a latched one */
                    if (kv_gate[v] && kv_lat[v])
                        continue;
                    if (best < 0 || (!kv_gate[v] && kv[v].amp < kv[best].amp + (kv_gate[best] ? 9.0f : 0.0f)))
                        best = v;
                }
                if (best < 0)
                    break;                               /* all four are latched: they stay, this key stays quiet */
                f *= fm_exp2f((float)-oct);   /* folded into the voice's own range, then the octave buttons move it */
                while (f < Y->lo)
                    f *= 2.0f;
                while (f > Y->hi * 2.0f)
                    f *= .5f;
                f *= fm_exp2f((float)oct);
                kv[best].f = f;
                kv[best].type = k_vox ? k_vox - 1 : -1;
                kv[best].t = 0.0f;
                kv_key[best] = (int16_t)k;
                kv_gate[best] = 1;
                kv_lat[best] = (uint8_t)(mode == 3);
            }
            rng ^= (uint32_t)plat_ticks24();
            if (!rng)
                rng = 1;
            break;
        }
        case C_OUTPUT: spk_on = c.a; break;
        case C_HOLD:
            if (c.b)
                held[c.a >> 5] |= 1u << (c.a & 31);
            else
                held[c.a >> 5] &= ~(1u << (c.a & 31));
            break;
        case C_HELDW: held[c.a] = (uint32_t)c.b; break;
        case C_EVOLVE: evolve = c.a == 0 ? .5f : c.a == 2 ? 2.0f : 1.0f; break;
        case C_FAV: favorites = (uint32_t)c.b; break;
        case C_DUCK: duck_to = c.a == 2 ? .35f : c.a ? 0.0f : 1.0f; break;   /* 2: a shallow dip, under a quick write */
        case C_SWELL:
            sw_on = (float)c.a;
            if (!c.a && sw > .15f) {     /* the collapse: the ground drops, one huge stroke, and the room rings on */
                sw_tail = fm_minf(1.0f, sw * 1.3f);
                strike_bell(SEAS[sea].bell[0], 1.0f, 1, .4f + sw * .5f);
                drum(DR_SUB, .6f + sw * .4f);
                ev_put(EV_UPHEAVAL, 0, 0, 0);
            }
            break;
        case C_SHAPE: {
            float v = (float)c.b * .01f, r = c.b <= 1 ? 0.0f : fm_exp2f((v - .5f) * 4.0f);
            switch (c.a) {
            case S_SIZE: sh_size = fm_exp2f((v - .5f) * 4.0f); break;
            case S_DARK: sh_dark = fm_exp2f((.5f - v) * 3.0f); break;
            case S_SPACE: sh_space = v * 2.0f; break;
            case S_TAPE: sh_tape = v; break;
            case S_WOBBLE: sh_wob = 4.0f * v * v; break;
            case S_BELLS: sh_bells = r; break;
            case S_FX: sh_fx = r; break;
            case S_WIDTH: sh_width = v * 2.0f; break;
            case S_PITCH: sh_pitch = fm_exp2f((float)c.b / 12.0f); break;
            case S_CHORD: sh_chord = c.b; break;
            case S_KEY: sh_key = c.b; break;
            case S_MORATE:
                mo_rate = c.b & 3;
                mo_left = fm_minf(mo_left, 30.0f);
                break;
            case S_MOLEN: mo_len = c.b & 3; motif_make(); break;
            case S_MOVOICE: mo_voice = c.b & 3; break;
            case S_MOSEED:
                mo_seed = c.b ? (uint8_t)c.b : (uint8_t)(1 + ri(255));
                motif_make();
                if (!c.b && mo_rate) {   /* a new tune, heard at once */
                    mo_use = -1;
                    mo_left = 1.0f;
                }
                break;
            case S_MOOD:
                sh_mood = c.b;
                if (!sh_mood)
                    family = list_pick(SEAS[sea].fam, 2);
                new_field(0);
                break;
            default: break;
            }
            break;
        }
        case C_FADE:
            fade_to = (float)c.a * .01f;
            fade_step = fm_fabsf(fade_to - fade_g) / fm_maxf((float)c.b * .01f * (EN_SR / (float)BLK), 1.0f);
            break;
        case C_SEED:
            rng ^= (uint32_t)c.b * 2654435761u;
            rng2 ^= (uint32_t)c.b * 40503u + 1u;
            rng_ui ^= (uint32_t)c.b;
            if (!rng) rng = 1;
            if (!rng2) rng2 = 1;
            if (!rng_ui) rng_ui = 1;
            break;
        }
    }
}

static void publish(void)
{
    int i;
    en_st.low = m_low; en_st.mid = m_mid; en_st.hi = m_hi; en_st.root = root_hz; en_st.left = s_left;
    for (i = 0; i < N_MACRO; i++)
        en_st.m[i] = macro_m[i];
    for (i = 0; i < 5; i++)
        en_st.st[i] = st[i];
    for (i = 0; i < 6; i++)
        en_st.field[i] = field[i];
    {
        float d = 0.0f;
        for (i = 0; i < EN_NDR; i++)
            d += dr[i].amp;
        en_st.dr = d;
    }
    for (i = 0; i < EN_NCH; i++)
        en_st.ch[i] = ch[i].amp;
    for (i = 0; i < EN_NFA; i++) {
        en_st.fa_on[i] = fa[i].on;
        en_st.fa_x[i] = fa[i].x;
        en_st.fa_y[i] = fa[i].y;
        en_st.fa_a[i] = fa[i].a;
        en_st.fa_v[i] = fa[i].r ? (uint8_t)fa[i].r->v : 0;
        en_st.fa_id[i] = fa[i].kind;
    }
    en_st.sea = (uint8_t)sea; en_st.stratum = (uint8_t)stratum; en_st.family = (uint8_t)family;
    en_st.palette = (uint8_t)palette; en_st.era = (uint8_t)era; en_st.frozen = (uint8_t)frozen;
    en_st.playing = (uint8_t)playing; en_st.drift = (uint8_t)sea_drift;
    en_st.bell = pick[0] >= 0 ? (uint8_t)pick[0] : b_type; en_st.drone = d_type; en_st.choir = c_type;
    en_st.pick[0] = pick[0]; en_st.pick[1] = pick[1]; en_st.pick[2] = pick[2];
    en_st.speaker = (uint8_t)spk_on;
    en_st.swell = sw;
    en_st.duck = duck_g;
    {
        int v;
        uint32_t m = 0;
        for (v = 0; v < NKV; v++)
            if (kv_gate[v] && kv_lat[v] && kv_key[v] < 25)
                m |= 1u << kv_key[v];
        en_st.latched = m;
    }
    en_st.mo_seed = mo_seed;
    en_st.motif = (uint8_t)(mo_use >= 0);
    { int w_; for (w_ = 0; w_ < EN_MASK; w_++) { en_st.cast[w_] = cast[w_]; en_st.held[w_] = held[w_]; } }
    en_st.auto_fx = auto_fx; en_st.auto_seq = auto_seq;
    en_st.bells = n_bells; en_st.born = n_born; en_st.strata = n_strata; en_st.seas_seen = seas_seen;
}

void en_render(int32_t *lr, uint32_t n, uint32_t gain_q12)
{
    uint32_t i;
    int handed = 0;
    if (n > BLK)
        n = BLK;
    commands();
    control();
    for (i = 0; i < n; i++) {
        bBprev[i] = bB[i];
        bL[i] = bR[i] = bT[i] = bV[i] = bB[i] = 0.0f;
        cL[i] = cR[i] = cT[i] = cV[i] = 0.0f;
    }
    if (two_cores)
        handed = plat_cpu2_run(core2_job, n);
    drones_render(n, (150.0f + (1.0f - macro_m[M_DEPTH]) * 780.0f + macro_m[M_UNREST] * 280.0f) * W->cut * (1.0f + sw * 1.6f) *
                         sc_mix[5] * (1.0f + .45f * fm_sinf(bloom_ph)));
    choir_render(n);
    {
        float *const o[4] = {bL, bR, bT, bV};
        bells_render(n, 0, 2, o, bm[0]);         /* the other two bells */
    }
    if (handed) {
        if (plat_cpu2_wait() != 0) {
            for (i = 0; i < n; i++)
                cL[i] = cR[i] = cT[i] = cV[i] = 0.0f;
            core2_job(n);
        }
    } else
        core2_job(n);
    fade_g = fade_g < fade_to ? fm_minf(fade_g + fade_step, fade_to) : fm_maxf(fade_g - fade_step, fade_to);
    space(lr, n, gain_q12);
    publish();
}

void en_init(void)
{
    int i, j, off = 0;
    tables_init();
    for (i = 0; i < 8; i++) {
        fb[i] = fdn + off;
        off += FL[i];
    }
    off = 0;
    for (i = 0; i < 4; i++) {
        ab[i] = apb + off;
        off += AL[i];
    }
    rng ^= (plat_ticks24() ^ boot_entropy) * 2654435761u;
    rng_ui ^= boot_entropy;
    if (!rng)
        rng = 1;
    if (!rng_ui)
        rng_ui = 1;
    for (i = 0; i < 16; i++)
        (void)xs(&rng);
    sea = ri(NSEA);
    mo_seed = (uint8_t)(1 + ri(255));
    motif_make();
    seas_seen = 1u << sea;
    family = list_pick(SEAS[sea].fam, 2);
    palette = list_pick(SEAS[sea].pal, 2);
    root_offset = ri(5) - 2;
    for (i = 0; i < N_MACRO; i++) {
        macro_m[i] = macro_u[i];
        touched[i] = -1e6f;
    }
    root_hz = root_target();
    new_field(1);
    for (i = 0; i < EN_NSTRATA; i++)
        for (j = 0; j < EN_NSTRATA; j++)
            trans[i][j] = i == j ? 0.0f : .35f + fr();
    stratum = ri(EN_NSTRATA);
    for (i = 0; i < 5; i++)
        st[i] = STRATA[stratum][i];
    s_left = 60.0f + fr() * 120.0f;
    field_left = 25.0f;
    era_left = 900.0f + fr() * 1800.0f;
    bell_left = 6.0f;
    fauna_left = 5.0f;
    load_cast();
    d_next = d_type = list_pick(SEAS[sea].drone, 3);   /* power-on: any of the world's own drones */
    b_type = list_pick(SEAS[sea].bell, 4);
    c_type = list_pick(SEAS[sea].choir, 4);
    for (i = 0; i < EN_NDR; i++) {
        dr[i].ph = fr();
        dr[i].ph2 = fr();
        dr[i].phm = fr();
        dr[i].idx = i < 2 ? i : 2 + i;
        dr[i].mr = MR[ri(6)];
        dr[i].beat = .05f + fr() * .28f;
        dr[i].w = fr() * TWO_PI;
        dr[i].wr = .03f + fr() * .08f;
        dr[i].sw = fr() * TWO_PI;
        dr[i].f = root_hz;
    }
    {
        static const float T0[EN_NCH] = {0.0f, .3f, .55f, .8f};
        for (i = 0; i < EN_NCH; i++) {
            ch[i].t = T0[i];
            ch[i].len = 18.0f + (float)i * 7.0f + fr() * 8.0f;
            ch[i].idx = i;
            ch[i].type = c_type;
            ch[i].vph = fr();
            ch[i].f = ch[i].tgt = pitch(i, CHOIRS[c_type].lo, CHOIRS[c_type].hi);
        }
    }
    grain(0);
    grain(1);
    gr[1].t = gr[1].len * .5f;
    gr[0].pan = -.5f;
    gr[1].pan = .5f;
}
