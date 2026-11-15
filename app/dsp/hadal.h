/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1: the engine. Fifteen dark worlds that generate themselves. en_render() runs in the audio interrupt, on the
 * first core, and hands bells and creatures to the second core when it has one. Everything else posts commands,
 * drained at the start of a block. Float only, no libm: fastmath.h. */
#pragma once
#include <stdint.h>

#define EN_SR 44100.0f
#define EN_NSEA 15
#define EN_NBELL 41
#define EN_NDRONE 34
#define EN_NCHOIR 29
#define EN_NSFX 135
#define EN_MASK 5
#define EN_BANK 27
#define EN_NSTRATA 6
#define EN_NFA 6
#define EN_NCH 4
#define EN_NDR 4

enum { M_DEPTH, M_UNREST, M_ROT, M_VAST, N_MACRO };
enum { L_DRONE, L_BELL, L_FAUNA, L_CHOIR, N_LAYER };
enum { S_SIZE, S_DARK, S_SPACE, S_TAPE, S_WOBBLE, S_BELLS, S_FX, S_WIDTH, S_PITCH, S_CHORD, S_KEY, S_MOOD, N_SHAPE, S_MORATE = N_SHAPE, S_MOLEN, S_MOVOICE, S_MOSEED };   /* motif: rate 0 off, 1 rare, 2 normal, 3 often; length 0..3 is 3..6 notes; voice 0 the world's, 1 bells, 2 lead, 3 both; seed 0: a new tune */
enum { EV_BELL, EV_STRATUM, EV_UPHEAVAL, EV_SEA, EV_OMEN, EV_NOTE };   /* EV_NOTE: a note, x velocity, y kind (0 bell, 1 ground, 2 pad, 3 scene): for MIDI out */

typedef struct {
    uint8_t type, a;     /* a: 0..255 */
    uint8_t x, y;        /* 0..255 */
} en_ev_t;

/* a fossil: a world kept, to grow again differently */
typedef struct {
    uint8_t used, sea, family, palette, stratum, era;
    int8_t root_offset;
    int8_t pick_bell, pick_drone, pick_choir;
    uint8_t bell, drone, choir, rsv;
    float field[6];
    float macro[N_MACRO];
    float tape;
    uint32_t cast[EN_MASK];
    uint8_t trans[EN_NSTRATA][EN_NSTRATA];
    uint8_t layer[N_LAYER];              /* the app's: the levels, the screen and the held effects that went with it */
    int8_t screen;
    int8_t shape[N_SHAPE];
    uint8_t rsv2[3];                      /* the motif: rate + 1, (length + 1) | (voice + 1) << 4, seed */
    uint32_t held[EN_MASK];
    uint8_t scene1, dchx;                /* the moment: scene + 1 (0: kept before 0.99.8), the drone's chord x50 */
    int8_t walk, pds[3];                 /* the ground's step, the pads' notes */
    uint8_t rsv3[2];
} en_fossil_t;

void en_entropy(uint32_t s);          /* before en_init: whatever differs from one power-on to the next */
void en_init(void);
void en_seed(uint32_t s);
void en_shape(int i, int v);          /* 0..7: 0..100, 50 is the world as it is. pitch -12..12; chord, key, mood: 0 the world's own */
void en_macro(int m, float v);       /* 0..1, the centre of the drift */
void en_layer(int l, float v);
void en_tide(float v);
void en_sea(int i);
void en_sea_drift(int on);
void en_pick(int what, int i);       /* what: 0 bell, 1 drone, 2 choir; i: -1 from the sea */
void en_cast_set(int i, int on);
void en_hold(int i, int on);          /* a latched effect: it keeps coming back until released */
void en_held_all(const uint32_t *m);
void en_evolve(int speed);            /* 0 slow, 1 normal, 2 restless */
void en_favorites(uint32_t mask);     /* the seas the drift may visit: 0 all */
void en_fade(float seconds, float to);
void en_cast_all(const uint32_t *m);
void en_fire(int i);
void en_fire_soft(int i);
void en_play(int on);
void en_freeze(int on);
void en_omen(void);
void en_mutate(void);
void en_upheaval(void);
void en_swell(int on);                /* held: the whole world rises; let go and it collapses into the room */
void en_offering(int semi_from_c, int oct);
void en_key(int k, int semi, int oct, int on);  /* a key pressed or let go: what it plays depends on en_keys; k 0..24 the FM-1's keys, 100 + note from MIDI */
void en_keys(int mode, int voice, int bell, int level, int fade); /* mode: 0 bells, 2 voices while held, 3 voices latched, 4 split (low keys bells, high keys voices); voice 0: the world's choir */
#define EN_NKVOX 28
const char *en_kvox_name(int i);
void en_keep(en_fossil_t *f);        /* main loop: a copy of the world (at the next block) */
int en_keep_ready(void);
void en_revive(const en_fossil_t *f);
void en_panic(void);
void en_letgo(void);                     /* every latched voice lets go */
void en_scene(int s);                    /* -1: the world picks its scenes; 0..5 holds one: bells, choir, chant, dark, empty, bloom */
void en_output(int speaker);          /* 1: tuned for the FM-1's own speaker; 0: full range */
void en_render(int32_t *lr, uint32_t n, uint32_t gain_q12);
void en_duck(int on);                 /* a 10 ms fade to silence and back: the flash write stops the audio dead, and that is a pop */
void en_usb_split(int on);            /* on: en_render leaves the full-range mix for USB, en_after_tap swaps in what the speaker should get */
void en_after_tap(int32_t *lr, uint32_t n);
void en_usb_tone(int phone);           /* 1: the recording gets the speaker's shaping too, for phones and small speakers */
void en_overload(void);              /* the audio interrupt: a block ran over 90 % of its time */
void en_cores(int two);              /* app_init: the second core takes jobs */
uint32_t en_rand(void);              /* main loop: its own stream, seeded with the engine */

/* the names */
const char *en_sea_name(int i);
const char *en_sea_from(int i);
const char *en_bell_name(int i);
const char *en_drone_name(int i);
const char *en_choir_name(int i);
const char *en_sfx_name(int i);
const char *en_stratum_name(int i);
const char *en_family_name(int i);
const char *en_chord_name(int i);
#define EN_NCHORD 9
#define EN_NFAM 11

/* for the screen: written by the render, read anywhere */
typedef struct {
    float low, mid, hi;
    float root;
    float m[N_MACRO];
    float left;
    float dr, ch[EN_NCH];
    float st[5];
    float field[6];
    uint8_t sea, stratum, family, palette, era, frozen, playing, drift;
    uint8_t bell, drone, choir;
    int8_t pick[3];
    uint32_t cast[EN_MASK], held[EN_MASK];
    uint32_t bells, born, strata;        /* counters, for the tests */
    uint32_t seas_seen;
    float fa_x[EN_NFA], fa_y[EN_NFA], fa_a[EN_NFA];
    uint8_t fa_on[EN_NFA], fa_v[EN_NFA], fa_id[EN_NFA];
    uint8_t speaker, mo_seed;
    float swell, duck;
    uint8_t motif;                       /* the motif is playing now */
    uint32_t latched;                    /* the keys holding a latched voice */
    uint32_t bad;                        /* samples that were not finite (tests) */
    float peak;
    int16_t auto_fx;                     /* the effect the world last chose by itself */
    uint16_t auto_seq;                   /* counts them, so the screen can tell a new one */
} en_state_t;
extern volatile en_state_t en_st;
int en_ev_get(en_ev_t *e);               /* main loop: the next event for the screen; 0 none */
