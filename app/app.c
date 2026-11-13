/* SPDX-License-Identifier: MPL-2.0
 * HADAL-1 on fm1-base: the app's unity build, after the platform. Three menus: GLOBAL, FX and SAVE. The controls and the saved world are here, the
 * sound in dsp/hadal.c, the picture in dsp/vis.c. */
#include "fastmath.h"

#define PROJ_MAGIC 0x384C4448u           /* "HDL8" */
#define GLOBAL_MAGIC 0x32474C47u          /* "GLG2" */
#define MAX_HELD 4
#define KEEP_MS 450u      /* an effect key held this long: kept on, or let go (a tap just plays it once) */
#define FX_TAP_MS 600u
#define FX_SHOW_MS 4600u
#define TITLE_MS 2600u
#ifdef APP_VERSION
#define BETA_LABEL "VERSION " APP_VERSION   /* on the power-on screen: which build is on the FM-1 */
#else
#define BETA_LABEL "VERSION 1.0"
#endif
#define GUIDE_SEEN 64u    /* gset.rsv[2]: the first-time guide has been read */
#define PANIC_QUIET 128u  /* gset.rsv[2]: PANIC SOUND off */
#define START_NEW 1u      /* gset.favs (no longer favourites): START ON: NEW WORLD */
#define N_GUIDE 7         /* pages in the first-time guide */
#define NFOSSIL 4
typedef struct {                         /* what SAVE keeps */
    uint32_t magic;
    uint8_t macro[N_MACRO], layer[N_LAYER];
    uint8_t sea, rsv[3];
    int8_t pick[3], screen;
    int8_t shape[N_SHAPE];
    uint32_t cast[EN_MASK], held[EN_MASK];
    en_fossil_t fossil[NFOSSIL];
    uint8_t keys[8];                     /* mode, voice, level, fade, bell */
} project_t;
typedef struct {                         /* GLOBAL: kept by itself whenever it changes */
    uint32_t magic;
    uint8_t speaker, power_msg, drift, tide, lights, evolve, screen, rest_fade, fav_lock, rsv[3];   /* rsv: power-on count, 2: control help off */
    uint32_t favs;
} global_set_t;
static project_t proj;
static global_set_t gset;

enum { PG_WATER, PG_LIFE, PG_VOICES, PG_ROOM, PG_MOVE, PG_NOTES, PG_KEYS, NPAGE };
static const char *const PAGE_NAME[NPAGE] = {"MAIN", "LEVELS", "VOICES", "SHAPE: ROOM", "SHAPE: MOVEMENT", "SHAPE: NOTES", "KEYS"};
static const char *const KNOB[NPAGE][4] = {{"DARKNESS", "ACTIVITY", "DECAY", "SPACE"}, {"DRONES", "BELLS", "EFFECTS", "CHOIR"},
                                           {"BELL", "DRONE", "CHOIR", "SCENE"}, {"ROOM SIZE", "ROOM DARK", "ROOM AMOUNT", "TAPE WEAR"},
                                           {"TAPE WOBBLE", "BELL RATE", "FX RATE", "WIDTH"}, {"KEY", "MOOD", "MOTIF", "NEW TUNE"},
                                           {"KEYS PLAY", "KEY VOICE", "KEY VOLUME", "VOICE FADE"}};
static const char *const KNOB_HELP[NPAGE][4][2] = {
    {{"LOW: HIGHER AND BRIGHTER.", "HIGH: DEEPER AND DARKER."},
     {"LOW: CALM, SLOW CHANGES.", "HIGH: RESTLESS, MORE HAPPENS."},
     {"HOW WORN AND DECAYED IT SOUNDS.", "HIGH: MORE OLD TAPE AND GHOSTS."},
     {"HOW HUGE THE SPACE AROUND THE SOUND IS.", "HIGH: VERY LONG ECHOES."}},
    {{"VOLUME OF THE LONG LOW DRONES.", "0 TURNS THEM OFF."},
     {"VOLUME OF THE BELLS AND BOWLS.", "0 TURNS THEM OFF."},
     {"VOLUME OF THE SOUND EFFECTS", "THAT PLAY BY THEMSELVES. 0: OFF."},
     {"VOLUME OF THE CHOIR VOICES.", "0 TURNS THEM OFF."}},
    {{"CHOOSE THE BELL SOUND.", "WORLD'S OWN: THE WORLD CHOOSES."},
     {"CHOOSE THE DRONE SOUND.", "WORLD'S OWN: THE WORLD CHOOSES."},
     {"CHOOSE THE CHOIR SOUND.", "WORLD'S OWN: THE WORLD CHOOSES."},
     {"HOLD THE WORLD IN ONE SCENE.", "WORLD'S OWN: IT MOVES BETWEEN THEM."}},
    {{"HOW LONG ECHOES LAST.", "HALFWAY: THE WORLD'S OWN LENGTH."},
     {"LOW: OPEN AND CLEAR.", "HIGH: DARK AND MUFFLED."},
     {"HOW MUCH SOUND GOES INTO THE ROOM.", "0: DRY AND CLOSE. HALFWAY: NORMAL."},
     {"HOW OLD AND WORN THE TAPE ECHO IS.", "0: NO TAPE ECHO. HALFWAY: NORMAL."}},
    {{"HOW MUCH THE TAPE PITCH WAVERS.", "0: STEADY. HALFWAY: NORMAL."},
     {"HOW OFTEN BELLS RING BY THEMSELVES.", "0: NEVER. HALFWAY: NORMAL. KEYS PLAY."},
     {"HOW OFTEN EFFECTS PLAY BY THEMSELVES.", "0: NEVER. HALFWAY: NORMAL. FX PLAYS."},
     {"LOW: NARROW, ALMOST MONO.", "HIGH: VERY WIDE. BEST ON HEADPHONES."}},
    {{"THE ROOT NOTE EVERYTHING TUNES TO.", "WORLD'S OWN: IT WANDERS BY ITSELF."},
     {"THE SCALE THE NOTES ARE PICKED FROM.", "WORLD'S OWN: THE WORLD CHOOSES."},
     {"A SHORT TUNE THAT COMES BACK CHANGED.", "OFF: NO TUNE, ONLY RANDOM NOTES."},
     {"TURN TO MAKE UP A NEW TUNE.", "YOU HEAR IT A SECOND LATER."}},
    {{"BELLS: TAP. VOICES: HOLD. LATCH: STAYS.", "SPLIT: BOTTOM ROW BELLS, TOP ROW VOICES."},
     {"THE SOUND THE KEYS SING WITH", "WHEN KEYS PLAY IS SET TO VOICES."},
     {"HOW LOUD THE KEYS PLAY.", ""},
     {"HOW SLOWLY A VOICE FADES", "AFTER YOU LET GO OF THE KEY."}}};
#define FX_BANK_TABLE
#include "fx_banks.h"
#define BLANK 255
#define BANK FX_BANK
enum { MENU_NONE, MENU_GLOBAL, MENU_FX, MENU_SAVE };
enum { GL_GUIDE, GL_OUTPUT, GL_DRIFT, GL_TIDE, GL_LIGHTS, GL_SLEEP, GL_SCREEN, GL_HELP, GL_USB, GL_BRIGHT, GL_TEXT, GL_MIDI, GL_PANIC, GL_START, GL_ABOUT, N_GLOBAL, GL_CPU = N_GLOBAL, GL_MSG };   /* version 1: the CPU and POWER-ON MESSAGE rows are gone */
static const uint8_t SLEEP_MIN[4] = {0, 30, 60, 120};
#define TWIST_RESET_MS 2500u
static struct {
    int page, octave, voct, frozen, playing;
    int phase, menu;
    int gl_row, global_dirty;
    int save_due, gl_wait;              /* gl_wait: GLOBAL changed while playing; written when it cannot be heard */
    const char *save_a, *save_b;
    uint32_t save_at;
    int knob_last, fx_bank, fired, tap_key, tap_fx, swelling, fx_down, fx_used, keep_slot;
    const char *fx_word;
    uint32_t fx_at, fx_word_until, tap_at, title_until, sleep_at, rest_at, wake_until;
    int sleep_pick, sleeping, blanked;
    int slot, keep_wait, twist[3], replace;   /* replace: slot+1 asked about, waiting for a second SAVE */
    uint32_t btn, keys, touch_until, msg_until, fired_at, twist_at;
    uint32_t auto_at;                    /* when the world last chose an effect by itself (its name shows) */
    uint16_t auto_seq;
    char msg[3][40];
    uint32_t last_ms, leds_at;
    uint32_t led_b, led_k;
    uint8_t led_mode;
    int guide;                          /* the page of the how-to (0: closed) */
} ui;

/* ---- what is kept */
static void keys_defaults(void)
{
    proj.keys[0] = 0;
    proj.keys[1] = 0;
    proj.keys[2] = 70;
    proj.keys[3] = 50;
    proj.keys[4] = 0;
}
static void defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.macro[0] = 62; proj.macro[1] = 34; proj.macro[2] = 30; proj.macro[3] = 72;
    for (i = 0; i < N_LAYER; i++)
        proj.layer[i] = 100;
    proj.sea = 255;                      /* none saved: a new world at every power-on */
    proj.pick[0] = proj.pick[1] = proj.pick[2] = -1;
    proj.screen = -1;
    keys_defaults();
    for (i = 0; i < S_PITCH; i++)
        proj.shape[i] = 50;
}
static void global_defaults(void)
{
    memset(&gset, 0, sizeof gset);
    gset.magic = GLOBAL_MAGIC;
    gset.speaker = 1;                   /* most people will start on the built-in speaker */
    gset.power_msg = 0;                 /* the speaker is clean now: straight to the music (GLOBAL can turn it on) */
    gset.drift = 1;
    gset.tide = 65;
    gset.lights = 1;
    gset.evolve = 1;
    gset.screen = 1;
    gset.rsv[2] = 2u << 2 | 16u;         /* from the start: PICTURE BRIGHTNESS BRIGHTEST, TEXT SIZE BIG; help text on (bit 0 clear) */
}
static void apply_global(void)
{
    en_tide((float)gset.tide * .01f);
    en_sea_drift(gset.drift);
    en_output(gset.speaker);
    en_evolve(gset.evolve);
    en_favorites(0u);                    /* favourites are gone: a world you like is kept with SAVE */
}
static void apply(void)
{
    int i;
    for (i = 0; i < N_MACRO; i++)
        en_macro(i, (float)proj.macro[i] * .01f);
    for (i = 0; i < N_LAYER; i++)
        en_layer(i, (float)proj.layer[i] * .01f);
    if (proj.sea < EN_NSEA) {
        en_sea(proj.sea);
        en_cast_all(proj.cast);
    }
    for (i = 0; i < 3; i++)
        en_pick(i, proj.pick[i]);
    for (i = 0; i < N_SHAPE; i++)
        en_shape(i, proj.shape[i]);
    en_shape(S_MORATE, proj.rsv[0] ? proj.rsv[0] - 1 : 2);
    en_shape(S_MOLEN, proj.rsv[1] & 15u ? (int)(proj.rsv[1] & 15u) - 1 : 1);
    en_shape(S_MOVOICE, proj.rsv[1] >> 4 ? (int)(proj.rsv[1] >> 4) - 1 : 0);
    if (proj.rsv[2])
        en_shape(S_MOSEED, proj.rsv[2]);
    vis_set_mode(proj.screen);
    en_held_all(proj.held);
    if (proj.keys[0] == 1)
        proj.keys[0] = 2;
    en_keys(proj.keys[0], proj.keys[1], proj.keys[4], proj.keys[2], proj.keys[3]);
    en_scene((int)proj.keys[5] - 1);
}
/* A microcontroller boots the same way every time, so its timer can't be the only randomness: a count kept in flash
 * makes every power-on differ, and the chip's own number makes every FM-1 differ. */
/* A save from another build, or one bent by a bad write the storage's own checks let through: every value back
 * inside its range before anything uses it, so nothing can read past a table's end. */
static int8_t lim8(int v, int lo, int hi, int dflt) { return (int8_t)(v < lo || v > hi ? dflt : v); }
static void sane(void)
{
    int i;
    if (proj.sea >= EN_NSEA)
        proj.sea = 0;
    for (i = 0; i < N_MACRO; i++)
        if (proj.macro[i] > 100)
            proj.macro[i] = 50;
    for (i = 0; i < N_LAYER; i++)
        if (proj.layer[i] > 100)
            proj.layer[i] = 100;
    proj.pick[0] = lim8(proj.pick[0], -1, EN_NBELL - 1, -1);
    proj.pick[1] = lim8(proj.pick[1], -1, EN_NDRONE - 1, -1);
    proj.pick[2] = lim8(proj.pick[2], -1, EN_NCHOIR - 1, -1);
    proj.screen = lim8(proj.screen, -1, VIS_NMODE - 1, -1);
    for (i = 0; i < N_SHAPE; i++)
        proj.shape[i] = lim8(proj.shape[i], i == S_PITCH ? -12 : 0,
                             i == S_PITCH || i == S_KEY ? 12 : i == S_CHORD ? EN_NCHORD : i == S_MOOD ? EN_NFAM : 100, 0);
    if (proj.keys[0] > 4) proj.keys[0] = 0;
    if (proj.keys[1] >= EN_NKVOX) proj.keys[1] = 0;
    if (proj.keys[2] > 100) proj.keys[2] = 70;
    if (proj.keys[3] > 100) proj.keys[3] = 50;
    if (proj.keys[4] > EN_NBELL) proj.keys[4] = 0;
    if (proj.keys[5] > 6) proj.keys[5] = 0;
    if (proj.keys[7] > NFOSSIL) proj.keys[7] = 0;   /* the slot saved last, + 1 (0: none) */
    if (proj.rsv[0] > 4) proj.rsv[0] = 0;
    for (i = 0; i < NFOSSIL; i++)
        if (proj.fossil[i].used && (proj.fossil[i].sea >= EN_NSEA || proj.fossil[i].screen >= VIS_NMODE))
            proj.fossil[i].used = 0;    /* one that cannot be read right is set aside, never half-loaded */
}
/* GLOBAL's notebook: each save is a 64-byte line written onto a page already cleared, so leaving GLOBAL costs one
 * short write and no pause. Two pages; the slow clearing happens only at power-on or when stopped, and never to the
 * page holding the newest line. A line counts only whole (its CRC): a power cut mid-write leaves the line before. */
#define GL_BASE 0xDE000u                 /* two 4 KiB sectors above the old A/B pair (0xDC000), inside the data area */
#define GL_REC 64u
#define GL_SLOTS (0xF00u / GL_REC)       /* 60 lines a page (the last 256 bytes of a sector stay untouched) */
#define GL_MAGIC 0x314C4748u             /* "HGL1" */
typedef struct { uint32_t magic, seq; uint16_t len, rsv; uint8_t body[48]; uint32_t crc; } gl_rec_t;
_Static_assert(sizeof(gl_rec_t) == GL_REC && sizeof(global_set_t) <= 48, "a GLOBAL line is 64 bytes");
static uint32_t gl_seq;
static int gl_sec = -1, gl_slot, gl_tidy_due = 1;
static uint32_t gl_crc(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    uint32_t c = 0xFFFFFFFFu;
    int k;
    while (n--)
        for (c ^= *b++, k = 0; k < 8; k++)
            c = c >> 1 ^ (0xEDB88320u & (0u - (c & 1u)));
    return ~c;
}
static uint32_t gl_at(int sec, int slot) { return GL_BASE + (uint32_t)sec * 0x1000u + (uint32_t)slot * GL_REC; }
static int gl_read(int sec, int slot, gl_rec_t *r) { return plat_data_read(gl_at(sec, slot), r, GL_REC); }
static int gl_good(const gl_rec_t *r) { return r->magic == GL_MAGIC && r->len == sizeof gset && r->crc == gl_crc(r, GL_REC - 4u); }
static int gl_blank(const gl_rec_t *r)
{
    const uint8_t *b = (const uint8_t *)r;
    uint32_t i;
    for (i = 0; i < GL_REC; i++)
        if (b[i] != 0xFFu)
            return 0;
    return 1;
}
static int gl_page_blank(int sec)
{
    gl_rec_t r;
    int i;
    for (i = 0; i < GL_SLOTS; i++)
        if (gl_read(sec, i, &r) || !gl_blank(&r))
            return 0;
    return 1;
}
static int gl_load(void)                 /* the newest whole line, wherever it is */
{
    gl_rec_t r;
    int s, i;
    gl_sec = -1;
    for (s = 0; s < 2; s++)
        for (i = 0; i < GL_SLOTS; i++)
            if (gl_read(s, i, &r) == 0 && gl_good(&r) && (gl_sec < 0 || (int32_t)(r.seq - gl_seq) > 0)) {
                gl_seq = r.seq;
                gl_sec = s;
                gl_slot = i;
                memcpy(&gset, r.body, sizeof gset);
            }
    return gl_sec < 0 ? -1 : 0;
}
static int gl_write(void)                /* one line on a cleared spot: 0 done, -1 no cleared spot (a tidy first) */
{
    gl_rec_t r;
    uint32_t off = 0;
    int s = gl_sec < 0 ? 0 : gl_sec, i = gl_sec < 0 ? 0 : gl_slot + 1, found = 0, turned = 0;
    for (; i < GL_SLOTS && !found; i++)
        if (gl_read(s, i, &r) == 0 && gl_blank(&r))
            found = 1, off = gl_at(s, i), gl_slot = i;
    if (!found && gl_sec >= 0 && gl_page_blank(s ^ 1))
        found = 1, turned = 1, s ^= 1, gl_slot = 0, off = gl_at(s, 0);
    if (!found)
        return -1;
    memset(&r, 0, sizeof r);
    r.magic = GL_MAGIC;
    r.seq = gl_seq + 1u;
    r.len = (uint16_t)sizeof gset;
    memcpy(r.body, &gset, sizeof gset);
    r.crc = gl_crc(&r, GL_REC - 4u);
    if (plat_data_write(off, &r, GL_REC) != 0)
        return -2;
    {
        gl_rec_t chk;                    /* read back: a failing part must not count as saved */
        if (plat_data_read(off, &chk, GL_REC) || !gl_good(&chk) || chk.seq != r.seq)
            return -2;
    }
    gl_seq = r.seq;
    gl_sec = s;
    if (turned || gl_slot >= GL_SLOTS / 2)
        gl_tidy_due = 1;                 /* the old page wants clearing at the next quiet moment */
    return 0;
}
static void gl_tidy(void)                /* clear the page the newest line is not on (slow: power-on, or stopped) */
{
    int o = gl_sec < 0 ? 0 : gl_sec ^ 1;
    if (!gl_page_blank(o))
        (void)plat_data_erase(GL_BASE + (uint32_t)o * 0x1000u);
    if (gl_sec < 0 && !gl_page_blank(1))
        (void)plat_data_erase(GL_BASE + 0x1000u);
    gl_tidy_due = 0;
}
static int gl_save(void)                 /* GLOBAL kept; 0 = done */
{
    int rc = gl_write();
    if (rc == -1 && gl_sec >= 0 && gl_page_blank(gl_sec ^ 1))
        rc = gl_write();
#ifdef PLAT_HOST
    if (rc == 0)                         /* the emulator keeps only the store between visits: the old copy too */
        (void)plat_store_save(OBJ_PANEL, &gset, sizeof gset);
#endif
    return rc;
}
static uint32_t boot_entropy(void)
{
    uint32_t count = (uint32_t)gset.rsv[0] | (uint32_t)gset.rsv[1] << 8, h = 2166136261u;
    const uint8_t *uid = plat_chip_uid();
    int i;
    count = (count + 1u) & 0xFFFFu;
    gset.rsv[0] = (uint8_t)count;
    gset.rsv[1] = (uint8_t)(count >> 8);
    gl_tidy();                           /* power-on, before any sound: the slow part, where no one hears it */
    if (gl_save() != 0)
        (void)plat_store_save(OBJ_PANEL, &gset, sizeof gset);
    if (uid)
        for (i = 0; i < 16; i++)
            h = (h ^ uid[i]) * 16777619u;
    return (count * 2654435761u) ^ h ^ (count << 17);
}
static void load_fossil(const en_fossil_t *f);
void app_init(void)
{
    if (gl_load() != 0 || gset.magic != GLOBAL_MAGIC) {   /* the notebook; the first time, the old copy */
        int g = plat_store_load(OBJ_PANEL, &gset, sizeof gset);
        if (g != (int)sizeof gset || gset.magic != GLOBAL_MAGIC)
            global_defaults();
    }
    gset.screen = 1;                     /* black never survives a power-off */
    en_entropy(boot_entropy());
    en_init();
    vis_init();
#if FM1B_CPU2
    en_cores(plat_cpu2_set(1) == 0 && plat_cpu2_on());
#endif
    int n = plat_store_load(OBJ_PROJ, &proj, sizeof proj);
    if (n == (int)sizeof proj - 4 && proj.magic == PROJ_MAGIC)
        keys_defaults();
    else if (n != (int)sizeof proj || proj.magic != PROJ_MAGIC)
        defaults();
    sane();
    {   /* power-on: the knobs start fresh (levels, picks, shapes, motif). Only the saved slots and the KEYS page carry
         * over; a slot you saved brings its own knobs back below. */
        static en_fossil_t fs[NFOSSIL];
        uint8_t ks[8];
        memcpy(fs, proj.fossil, sizeof fs);
        memcpy(ks, proj.keys, sizeof ks);
        defaults();
        memcpy(proj.fossil, fs, sizeof fs);
        memcpy(proj.keys, ks, sizeof ks);
        proj.keys[5] = 0;                /* SCENE lives here too, and it starts on WORLD'S OWN like every other knob */
        proj.keys[4] = 0;                /* and the keys' bell: WORLD'S OWN */
    }
    proj.sea = 255;                      /* power-on: a new world, unless a saved slot is asked for below */
    memset(proj.held, 0, sizeof proj.held);   /* and never an effect still kept on from before */
    apply();
    if (!(gset.favs & START_NEW) && proj.keys[7] && proj.fossil[proj.keys[7] - 1].used) {
        static const uint32_t none[EN_MASK];
        load_fossil(&proj.fossil[proj.keys[7] - 1]);   /* START ON: SAVED WORLD: the slot you saved last */
        en_held_all(none);
    }
#if defined FELUCCA_UAC && FELUCCA_UAC
    plat_usb_audio_full(1);              /* the recording: one level, whatever MASTER says, and never the speaker's tuning */
    en_usb_split(1);
    en_usb_tone(gset.rsv[2] >> 1 & 1);
    vis_bright(gset.rsv[2] >> 2 & 3);
#endif
    apply_global();
#if defined PLAT_HOST && !defined PLAT_WEB
    if (getenv("HADAL_SEA"))             /* a scripted capture starts in a known sea */
        en_sea(atoi(getenv("HADAL_SEA")));
#endif
    en_seed(plat_ticks24());
    ui.playing = 1;
}
void app_render(int32_t *lr, uint32_t n, uint32_t gain_q12) { en_render(lr, n, gain_q12); }
void app_after_tap(int32_t *lr, uint32_t n) { en_after_tap(lr, n); }
void app_panic(void) { en_panic(); }

/* ---- words */
static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static int scmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return *a - *b; }
static void copy(char *d, const char *s, int n)
{
    int i = 0;
    while (s && s[i] && i < n - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}
static void say3(const char *a, const char *b, const char *c, uint32_t ms)
{
    copy(ui.msg[0], a, (int)sizeof ui.msg[0]);
    copy(ui.msg[1], b ? b : "", (int)sizeof ui.msg[1]);
    copy(ui.msg[2], c ? c : "", (int)sizeof ui.msg[2]);
    ui.msg_until = plat_ms() + ms + 2000u;   /* two seconds longer, so it can be read in full */
    if (ann_on()) {
        char t[84];
        copy(t, ui.msg[0], 40);
        if (ui.msg[1][0]) {
            int n = slen(t);
            t[n] = ' ';
            copy(t + n + 1, ui.msg[1], 40);
        }
        ann_say(t, 0, ANN_NOW);
    }
}
void app_say(const char *a, const char *b) { say3(a, b, 0, 1400u); }
static int helping(void) { return !(gset.rsv[2] & 1u); }   /* CONTROL HELP TEXT in GLOBAL */
static int big(void) { return (gset.rsv[2] & 16u) != 0; }  /* TEXT SIZE: BIG */
static int latched_n(void);
static void panic(void);
static void midi_silence(void);
static void tell(const char *a, const char *b, const char *c)    /* b a value (stays), c the explaining (GLOBAL can turn it off) */
{
    say3(a, b, helping() ? c : 0, 0u);
    ui.msg_until = plat_ms() + (helping() ? 2200u : 1600u);   /* a quick look: the 4 sound knobs keep their longer help */
}
static void explain(const char *a, const char *b, const char *c)  /* b and c are both explaining: with help off, only the name */
{
    if (helping())
        say3(a, b, c, 1600u);              /* with help on: long enough to read, not longer */
    else
        say3(a, "", 0, 1200u);
}
static const char *pick_name(int what, int i)
{
    if (i < 0)
        return "WORLD'S OWN";
    return what == 0 ? en_bell_name(i) : what == 1 ? en_drone_name(i) : en_choir_name(i);
}
static int wrap(int v, int lo, int hi) { return v < lo ? hi : v > hi ? lo : v; }

static int held_count(void)
{
    int i, n = 0;
    for (i = 0; i < EN_NSFX; i++)
        n += (int)(en_st.held[i >> 5] >> (i & 31) & 1u);
    return n;
}

/* ---- the three menus */
static int gl_save(void);
static void global_later(void)           /* a short write onto a cleared page: no pause; else when it cannot be heard */
{
    if (ui.playing && !(ui.save_due & ~8)) {
        ui.save_due |= 8;                /* a shallow dip (not silence) for the moment of the write: no click */
        ui.save_at = plat_ms();
        en_duck(2);
        return;
    }
    if (gl_save() == 0) {
        ui.gl_wait = 0;
        return;
    }
    if (ui.playing) {
        ui.gl_wait = 1;                  /* a dot in the corner until it is written: PLAY to stop, or SAVE */
        return;
    }
    ui.gl_wait = 0;
    ui.save_due |= 1;
    ui.save_at = plat_ms();
    en_duck(1);
}
static void menu_set(int m)
{
    ui.msg_until = plat_ms();            /* an old message never covers a menu */
    if (ui.menu == MENU_GLOBAL && m != MENU_GLOBAL && ui.global_dirty) {
        global_later();                  /* GLOBAL keeps itself when you leave it: at once if quiet, else later */
        ui.global_dirty = 0;
    }
    ui.menu = m;
    ui.replace = 0;
    ui.twist[0] = ui.twist[1] = ui.twist[2] = 0;
}
static void global_change(int d)
{
    int s = d > 0 ? 1 : -1;
    switch (ui.gl_row) {
    case GL_GUIDE: menu_set(MENU_NONE); ui.guide = 1; return;
    case GL_OUTPUT: gset.speaker = !gset.speaker; en_output(gset.speaker); break;
    case GL_DRIFT: {                     /* one row for both: off, or moving on at a pace */
        int v = gset.drift ? gset.evolve + 1 : 0;
        v = wrap(v + s, 0, 3);
        gset.drift = v > 0;
        gset.evolve = (uint8_t)(v ? v - 1 : 1);
        en_sea_drift(gset.drift);
        en_evolve(gset.evolve);
        break;
    }
    case GL_TIDE: gset.tide = (uint8_t)clamp((int32_t)gset.tide + d * 2, 0, 100); en_tide((float)gset.tide * .01f); break;
    case GL_LIGHTS: gset.lights = (uint8_t)wrap(gset.lights + s, 0, 2); break;
    case GL_SLEEP:                       /* for tonight only: never kept */
        ui.sleep_pick = wrap(ui.sleep_pick + s, 0, 3);
        ui.sleep_at = ui.sleep_pick ? plat_ms() + (uint32_t)SLEEP_MIN[ui.sleep_pick] * 60000u : 0;
        ui.sleeping = 0;
        return;
    case GL_SCREEN: gset.screen = !gset.screen; break;
    case GL_HELP: gset.rsv[2] ^= 1u; break;
    case GL_USB: gset.rsv[2] ^= 2u; en_usb_tone(gset.rsv[2] >> 1 & 1); break;
    case GL_BRIGHT: gset.rsv[2] = (uint8_t)((gset.rsv[2] & ~12u) | (((gset.rsv[2] >> 2 & 3u) + 1u) % 3u) << 2); vis_bright(gset.rsv[2] >> 2 & 3); break;
    case GL_TEXT: gset.rsv[2] ^= 16u; break;
    case GL_MIDI: gset.rsv[2] ^= 32u; midi_silence(); break;
    case GL_PANIC: gset.rsv[2] ^= PANIC_QUIET; break;
    case GL_START: gset.favs ^= START_NEW; break;
    default: return;
    }
    ui.global_dirty = 1;
}
static void store_fossils(const char *a, const char *b)
{
    ui.save_a = a;
    ui.save_b = b;
    ui.save_due |= 2 | (ui.gl_wait ? 1 : 0);   /* waiting GLOBAL settings ride along in the same fade */
    ui.gl_wait = 0;
    ui.save_at = plat_ms();
    en_duck(1);
}
static void save_settings(void)
{
    proj.sea = en_st.sea;
    {
        int w_;
        proj.rsv[2] = en_st.mo_seed;
        for (w_ = 0; w_ < EN_MASK; w_++) {
            proj.cast[w_] = en_st.cast[w_];
            proj.held[w_] = en_st.held[w_];
        }
    }
    store_fossils("SAVED", "SETTINGS AND FOSSILS");
}
static int mo_val(int k)                 /* 0 in the file: the default */
{
    uint32_t b = proj.rsv[1];
    if (k == 0)
        return proj.rsv[0] ? proj.rsv[0] - 1 : 2;
    if (k == 1)
        return b & 15u ? (int)(b & 15u) - 1 : 1;
    return b >> 4 ? (int)(b >> 4) - 1 : 0;
}
/* a fossil brings back everything that was set when it was kept, and the knobs on the screen follow it */
static void load_fossil(const en_fossil_t *f)
{
    int i;
    en_revive(f);
    for (i = 0; i < N_MACRO; i++)
        proj.macro[i] = (uint8_t)(f->macro[i] * 100.0f + .5f);
    for (i = 0; i < N_LAYER; i++) {
        proj.layer[i] = f->layer[i];
        en_layer(i, (float)proj.layer[i] * .01f);
    }
    proj.pick[0] = f->pick_bell;
    proj.pick[1] = f->pick_drone;
    proj.pick[2] = f->pick_choir;
    proj.screen = f->screen;
    vis_set_mode(proj.screen);
    for (i = 0; i < N_SHAPE; i++) {
        proj.shape[i] = f->shape[i];
        en_shape(i, proj.shape[i]);
    }
    for (i = 0; i < 3; i++)
        proj.rsv[i] = f->rsv2[i];
    en_shape(S_MORATE, mo_val(0));
    en_shape(S_MOLEN, mo_val(1));
    en_shape(S_MOVOICE, mo_val(2));
    if (proj.rsv[2])
        en_shape(S_MOSEED, proj.rsv[2]);
    en_held_all(f->held);
    proj.keys[5] = f->rsv3[0] <= 6 ? f->rsv3[0] : 0;   /* the SCENE knob it was saved with */
    en_scene((int)proj.keys[5] - 1);
}
static const int SLOT_KEY[NFOSSIL] = {0, 2, 4, 6};   /* the save menu: the first four white keys are the slots */
static void load_slot(int s)
{
    en_fossil_t *f = &proj.fossil[s];
    ui.slot = s;
    if (!f->used) {
        app_say("SLOT EMPTY", "PRESS SAVE: KEEP THIS WORLD THERE");
        return;
    }
    load_fossil(f);
    menu_set(MENU_NONE);
    {
        char t[24];
        copy(t, "LOADED SLOT 1", 16);
        t[12] = (char)('1' + s);
        app_say(t, en_sea_name(f->sea));
    }
}
static void keep_slot(void)              /* SAVE pressed in the save menu: this world, and all settings, into the slot */
{
    en_fossil_t *f = &proj.fossil[ui.slot];
    if (f->used && ui.replace != ui.slot + 1) {
        char t[28];
        ui.replace = ui.slot + 1;        /* a world is there: ask once */
        copy(t, "REPLACE ", 10);
        copy(t + 8, en_sea_name(f->sea), 18);
        say3(t, "PRESS SAVE AGAIN: YES", "HOME: NO", 2600u);
        return;
    }
    en_keep(f);
    ui.keep_slot = ui.slot;
    ui.keep_wait = 1;
    menu_set(MENU_NONE);
}
/* ERASE asks for a twist: a bar fills to the right, and stopping lets it fall back */
static void twist(int which, int d)
{
    int *t = &ui.twist[which];
    en_fossil_t *f = &proj.fossil[ui.slot];
    if (which != 2)
        return;
    ui.twist_at = plat_ms();
    *t = clamp(*t + d * 34, 0, 100);     /* 3 clicks to the right */
    if (*t < 100)
        return;
    *t = 0;
    if (f->used) {
        f->used = 0;
        if (proj.keys[7] == ui.slot + 1)
            proj.keys[7] = 0;            /* erased the one power-on came back to: power-on starts new */
        store_fossils("SLOT ERASED", "");
    } else
        app_say("SLOT EMPTY", "NOTHING TO ERASE");
}

static void knob(int k, int d)
{
    uint8_t *v = 0;
    int p = ui.page;
    if (ui.menu == MENU_GLOBAL) {
        global_change(d);
        return;
    }
    if (ui.menu == MENU_FX)
        return;
    if (ui.menu == MENU_SAVE) {
        if (k == 0) {
            ui.slot = wrap(ui.slot + (d > 0 ? 1 : -1), 0, NFOSSIL - 1);
            ui.replace = 0;
        } else
            twist(k - 1, d);
        return;
    }
    ui.touch_until = plat_ms() + 5600u;
    ui.knob_last = k;
    if (p == PG_KEYS) {
        int st = d > 0 ? 1 : -1;
        if (k == 0)
        {                                /* bells, voices, latch, split */
            static const uint8_t NEXT[5] = {2, 0, 3, 4, 0}, PREV[5] = {4, 0, 0, 2, 3};
            proj.keys[0] = st > 0 ? NEXT[proj.keys[0]] : PREV[proj.keys[0]];
        }
        else if (k == 1 && !proj.keys[0])
            proj.keys[4] = (uint8_t)wrap(proj.keys[4] + st, 0, EN_NBELL);   /* bells: which bell the keys strike */
        else if (k == 1)
            proj.keys[1] = (uint8_t)wrap(proj.keys[1] + st, 0, EN_NKVOX - 1);
        else if (k == 3 && proj.keys[0] == 4)
            proj.keys[4] = (uint8_t)wrap(proj.keys[4] + st, 0, EN_NBELL);   /* split: the bell, beside the voice */
        else if (k == 3 && !proj.keys[0]) {   /* on bells: this knob lets the latched voices go */
            if (latched_n()) {
                en_letgo();
                tell("LATCHED VOICES", "LET GO", "");
            }
            return;
        }
        else
            proj.keys[k] = (uint8_t)clamp((int32_t)proj.keys[k] + d * 2, 0, 100);
        en_keys(proj.keys[0], proj.keys[1], proj.keys[4], proj.keys[2], proj.keys[3]);
        return;
    }
    if (p == PG_NOTES) {                 /* the key, the mood, and the motif */
        int st = d > 0 ? 1 : -1, x;
        if (k == 3) {
            en_shape(S_MOSEED, 0);
            return;
        }
        if (k == 2) {
            x = wrap(mo_val(0) + st, 0, 3);
            proj.rsv[0] = (uint8_t)(x + 1);
            en_shape(S_MORATE, x);
            return;
        }
        x = wrap(proj.shape[S_KEY + k] + st, 0, k == 0 ? 12 : EN_NFAM);
        proj.shape[S_KEY + k] = (int8_t)x;
        en_shape(S_KEY + k, x);
        return;
    }
    if (p >= PG_ROOM) {
        int s = (p - PG_ROOM) * 4 + k, x = clamp((int32_t)proj.shape[s] + d * 2, 0, 100);
        proj.shape[s] = (int8_t)x;
        en_shape(s, x);
        return;
    }
    if (p == PG_WATER)
        v = &proj.macro[k];
    else if (p == PG_LIFE)
        v = &proj.layer[k];
    if (v) {
        int x = clamp((int32_t)*v + d * 2, 0, 100);
        *v = (uint8_t)x;
        if (p == PG_WATER)
            en_macro(k, (float)x * .01f);
        else
            en_layer(k, (float)x * .01f);
        return;
    }
    if (k < 3) {
        static const int N[3] = {EN_NBELL, EN_NDRONE, EN_NCHOIR};
        proj.pick[k] = (int8_t)wrap(proj.pick[k] + (d > 0 ? 1 : -1), -1, N[k] - 1);
        en_pick(k, proj.pick[k]);
    } else {                             /* SCENE: hold the world in one, or let it move between them */
        proj.keys[5] = (uint8_t)wrap((int)proj.keys[5] + (d > 0 ? 1 : -1), 0, 6);
        en_scene((int)proj.keys[5] - 1);
    }
}

static void fx_say(const char *a, const char *b);
static int latched_n(void)
{
    int k, n = 0;
    for (k = 0; k < NKEYS; k++)
        n += (int)(en_st.latched >> k & 1u);
    return n;
}
static void key_down(int k)
{
    en_seed(plat_ticks24());
    if (ui.menu == MENU_SAVE) {          /* a lit key loads its slot; the rest do nothing here */
        int s;
        for (s = 0; s < NFOSSIL; s++)
            if (SLOT_KEY[s] == k)
                load_slot(s);
        return;
    }
    if (ui.menu == MENU_FX || ui.fx_down) {  /* a tap plays it now; the same press, held, keeps it */
        int i = k < EN_BANK ? BANK[ui.fx_bank][k] : BLANK;
        uint32_t now = plat_ms();
        if (i == BLANK)
            return;
        ui.fired = i;
        ui.fired_at = now;
        ui.fx_used = 1;
        ui.tap_key = k + 1;              /* still down at KEEP_MS: keep it on, or let it go (input) */
        ui.tap_at = now;
        ui.tap_fx = i;
        ui.fx_word = 0;
        if (!(en_st.held[i >> 5] >> (i & 31) & 1u))
            en_fire(i);
    } else {
        float rootm = 69.0f + 12.0f * fm_log2f(fm_maxf(en_st.root, 1.0f) / 440.0f);
        int r = (int)(rootm + 1000.5f) - 1000, semi = (((53 + k - r) % 12) + 12) % 12;
        if (proj.keys[0] == 3 && !(en_st.latched >> k & 1u) && latched_n() >= 4)
            tell("4 LATCHED IS THE MOST", "", "PRESS A LIT KEY TO LET IT GO");
        else if (proj.keys[0] == 3 && !(en_st.latched >> k & 1u) && latched_n() + held_count() >= 6)
            tell("6 AT ONCE IS THE MOST", "LATCHED VOICES + KEPT EFFECTS", "LET ONE GO FIRST");
        else if (proj.keys[0] == 4 && (0x54Au >> ((k + 5) % 12) & 1u) && latched_n() >= 4)
            tell("4 VOICES ARE LATCHED", "SO THE TOP ROW STAYS QUIET", "ENV LETS THEM ALL GO");
        if (proj.keys[0] == 4)           /* split: the top row (the black keys) sings, the bottom row rings */
            en_key(k, semi, (0x54Au >> ((k + 5) % 12) & 1u) ? ui.voct : ui.octave, 1);
        else
            en_key(k, semi, proj.keys[0] ? ui.voct : ui.octave, 1);
    }
}

static void button(int k)
{
    en_seed(plat_ticks24());
    switch (k) {
    case B_GLO: menu_set(ui.menu == MENU_GLOBAL ? MENU_NONE : MENU_GLOBAL); break;
    case B_FX:                           /* decided when it lets go: a tap opens the menu, held it plays over the picture */
        ui.fx_down = 1;
        ui.fx_used = 0;
        ui.fx_at = plat_ms();
        break;
    case B_SAVE:
        if (ui.menu == MENU_GLOBAL)      /* nothing: GLOBAL keeps itself, and SAVE here should not jump away */
            break;
        if (ui.menu == MENU_SAVE)        /* the second press keeps this world in the slot, with everything */
            keep_slot();
        else
            menu_set(MENU_SAVE);
        break;
    case B_HOME:
        menu_set(MENU_NONE);
        explain("HOME", "BACK TO THE FIRST PAGE", "");
        ui.page = 0;
        ui.knob_last = -1;
        ui.touch_until = plat_ms() + 5600u;
        break;
    case B_PLAY:
        ui.playing = !ui.playing;
        ui.sleeping = 0;
        ui.rest_at = 0;
        if (ui.playing) {
            en_play(1);
            en_fade(2.0f, 1.0f);
        } else {                         /* about four seconds to die away: never abrupt */
            en_fade(4.0f, 0.0f);
            ui.rest_at = plat_ms() + 4500u;
        }
        if (ui.playing)
            explain("PLAY", "THE SOUND STARTS", "PRESS AGAIN TO STOP IT");
        else
            explain("STOP", "THE SOUND FADES OUT", "PRESS AGAIN TO START IT");
        break;
    case B_REC:
        ui.frozen = !ui.frozen;
        en_freeze(ui.frozen);
        if (ui.frozen)
            explain("HOLD", "THE SOUND IS FROZEN", "PRESS AGAIN TO RELEASE IT");
        else
            explain("RELEASE", "THE SOUND MOVES AGAIN", "");
        break;
    case B_ENV: panic(); break;      /* PANIC: every latched voice and kept effect lets go */
    case B_EDIT: en_omen(); explain("OMEN", "JUMPS TO A NEW PHASE", "OF THIS WORLD"); break;
    case B_SEQ: {                        /* RANDOM: a new world, its own picture and voices, and a new scale */
        int s = wrap((int)en_st.sea + 1 + (int)(plat_ticks24() % (EN_NSEA - 1)), 0, EN_NSEA - 1);
        proj.pick[0] = proj.pick[1] = proj.pick[2] = -1;
        en_pick(0, -1);
        en_pick(1, -1);
        en_pick(2, -1);
        proj.screen = -1;
        vis_set_mode(-1);
        proj.keys[5] = 0;
        en_scene(-1);
        en_sea(s);
        en_upheaval();
        tell(en_sea_name(s), "RANDOM", helping() ? "A NEW WORLD, NEW SOUNDS" : "");
        break;
    }
    case B_ARP: en_upheaval(); explain("UPHEAVAL", "NEW SCALE, SOUNDS, COLORS", ""); break;
    case B_LFO:                          /* the swell: it builds while held, and collapses when let go */
        ui.swelling = 1;
        en_swell(1);
        break;
    case B_OCTDN:
    case B_OCTUP: {                      /* the octave of whatever the keys play now: bells and voices each keep their own */
        int *o = proj.keys[0] ? &ui.voct : &ui.octave;
        *o = clamp(*o + (k == B_OCTUP ? 1 : -1), -1, 1);
        tell(proj.keys[0] ? "VOICE OCTAVE" : "BELL OCTAVE", *o < 0 ? "LOW" : *o ? "HIGH" : "MIDDLE", proj.keys[0] ? "HOW HIGH THE KEYS SING" : "HOW HIGH THE BELLS PLAY");
        break;
    }
    default: break;
    }
}

static void save_when_quiet(uint32_t now)    /* the flash write stops the audio: fade first, and there is no pop */
{
    if (ui.gl_wait && !ui.playing)
        global_later();                  /* the music stopped: now GLOBAL can be written */
    if (gl_tidy_due && !ui.playing && !ui.save_due && (int32_t)(now - ui.save_at) > 3000) {
        ui.save_due |= 4;                /* stopped a while: clear the spare page, faded, unheard */
        ui.save_at = now;
        en_duck(1);
    }
    if (ui.save_due == 8) {              /* the quiet write: once the dip is in, write, come back */
        if (en_st.duck > .4f && now - ui.save_at < 200u)
            return;
        ui.save_due = 0;
        en_duck(0);
        if (gl_save() == 0)
            ui.gl_wait = 0;
        else
            ui.gl_wait = 1;              /* no cleared spot: it waits for a stop, the dot shows */
        return;
    }
    if (!ui.save_due || (en_st.duck > .002f && now - ui.save_at < 500u))
        return;
    if (ui.save_due & 9) {               /* (8: a quick write that a bigger save took over) */
        gl_tidy();
        if (gl_save() != 0 && plat_store_save(OBJ_PANEL, &gset, sizeof gset) != 0)
            app_say("SETTINGS NOT SAVED", "STORAGE FULL OR FAILED");
    }
    if (ui.save_due & 4)
        gl_tidy();                       /* stopped: the spare page cleared, ready for the next quiet save */
    if (ui.save_due & 2)
        app_say(plat_store_save(OBJ_PROJ, &proj, sizeof proj) == 0 ? ui.save_a : "SAVE FAILED", ui.save_b);
    ui.save_due = 0;
    en_duck(0);
}
static void fx_say(const char *a, const char *b)     /* over the picture it stays a small caption */
{
    if (ui.menu == MENU_FX)
        app_say(a, b);
    else {
        ui.fx_word = a;
        ui.fx_word_until = plat_ms() + FX_SHOW_MS;
        ui.fired_at = plat_ms();
    }
}
/* ---- MIDI. In: notes play what the keys play (100 + note, so they never clash with the FM-1's own keys), program
 * change picks the world, CC 16-31 move the first four pages' knobs, CC 1 is the swell, CC 123 lets everything go.
 * Out (GLOBAL: MIDI OUT): the notes the world plays, for another synth or a DAW: bells on channel 1, the ground on
 * channel 2 when it steps, the pads on channel 3, and the scene as CC 20 on channel 1. */
#define MIDI_OFFS 8
static struct { uint8_t note, ch; uint32_t at; } midi_off[MIDI_OFFS];
static uint8_t midi_ground, midi_pad[3], midi_pad_new[3], midi_pad_n, midi_in_seen;
static int midi_on(void) { return !(gset.rsv[2] & 32u); }
static void midi_send(uint32_t st, uint32_t d1, uint32_t d2)
{
    if (!plat_midi_out_room(TO_USB))
        return;
    plat_midi_out((st >> 4) | st << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24);
}
static void midi_note_off_later(int note, int ch, uint32_t ms)
{
    int i, o = 0;
    for (i = 0; i < MIDI_OFFS; i++)
        if (!midi_off[i].at) {
            o = i;
            break;
        } else if ((int32_t)(midi_off[i].at - midi_off[o].at) < 0)
            o = i;
    if (midi_off[o].at)
        midi_send(0x80u | (uint32_t)midi_off[o].ch, midi_off[o].note, 0);
    midi_off[o].note = (uint8_t)note;
    midi_off[o].ch = (uint8_t)ch;
    midi_off[o].at = plat_ms() + ms;
}
static void midi_silence(void)           /* everything this sent, off */
{
    int i;
    for (i = 0; i < MIDI_OFFS; i++)
        if (midi_off[i].at) {
            midi_send(0x80u | (uint32_t)midi_off[i].ch, midi_off[i].note, 0);
            midi_off[i].at = 0;
        }
    if (midi_ground)
        midi_send(0x81u, midi_ground, 0);
    for (i = 0; i < 3; i++)
        if (midi_pad[i])
            midi_send(0x82u, midi_pad[i], 0);
    midi_ground = midi_pad[0] = midi_pad[1] = midi_pad[2] = 0;
}
void app_midi_note(int kind, int note, int vel)   /* from the engine's events, in the main loop */
{
    int i;
    if (!midi_on())
        return;
    if (kind == 0) {
        midi_send(0x90u, (uint32_t)note, (uint32_t)vel);
        midi_note_off_later(note, 0, 600u);
    } else if (kind == 1) {
        if (midi_ground)
            midi_send(0x81u, midi_ground, 0);
        midi_ground = (uint8_t)note;
        midi_send(0x91u, (uint32_t)note, (uint32_t)vel);
    } else if (kind == 2) {
        midi_pad_new[midi_pad_n++] = (uint8_t)note;
        if (midi_pad_n < 3)
            return;
        midi_pad_n = 0;
        for (i = 0; i < 3; i++)
            if (midi_pad[i])
                midi_send(0x82u, midi_pad[i], 0);
        for (i = 0; i < 3; i++) {
            midi_pad[i] = midi_pad_new[i];
            midi_send(0x92u, midi_pad[i], 60u);
        }
    } else
        midi_send(0xB0u, 20u, (uint32_t)(note * 20));
}
static void midi_tick(uint32_t now)
{
    int i;
    for (i = 0; i < MIDI_OFFS; i++)
        if (midi_off[i].at && (int32_t)(now - midi_off[i].at) >= 0) {
            midi_send(0x80u | (uint32_t)midi_off[i].ch, midi_off[i].note, 0);
            midi_off[i].at = 0;
        }
}
static void midi_cc(int cc, int v)
{
    int k = cc & 3, x = v * 100 / 127;
    if (cc >= 16 && cc <= 19) {
        proj.macro[k] = (uint8_t)x;
        en_macro(k, (float)x * .01f);
    } else if (cc >= 20 && cc <= 23) {
        proj.layer[k] = (uint8_t)x;
        en_layer(k, (float)x * .01f);
    } else if (cc >= 24 && cc <= 31) {
        proj.shape[cc - 24] = (int8_t)x;
        en_shape(cc - 24, x);
    } else if (cc == 1) {
        if ((v >= 64) != ui.swelling) {
            ui.swelling = v >= 64;
            en_swell(ui.swelling);
        }
    } else if (cc == 123 || cc == 120) {
        en_letgo();
        for (k = 0; k < 128; k++)
            en_key(100 + k, 0, 0, 0);
    }
}
static void midi_in(void)
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = pkt >> 24 & 0x7Fu;
        if (st == 0x90u && d2) {
            float rootm = 69.0f + 12.0f * fm_log2f(fm_maxf(en_st.root, 1.0f) / 440.0f);
            int r = (int)(rootm + 1000.5f) - 1000, semi = (((int)d1 - r) % 12 + 12) % 12;
            int oct = (proj.keys[0] ? ui.voct : ui.octave) + ((int)d1 - 53 + 120) / 12 - 10;
            en_key(100 + (int)d1, semi, clamp(oct, -3, 6), 1);
            if (!midi_in_seen) {
                midi_in_seen = 1;
                tell("MIDI IN", "NOTES PLAY LIKE THE KEYS DO", helping() ? "SET ON THE KEYS PAGE" : "");
            }
        } else if (st == 0x80u || st == 0x90u)
            en_key(100 + (int)d1, 0, 0, 0);
        else if (st == 0xB0u)
            midi_cc((int)d1, (int)d2);
        else if (st == 0xC0u && (int)d1 < EN_NSEA) {
            en_sea((int)d1);
            tell(en_sea_name((int)d1), "MIDI PROGRAM CHANGE", "");
        }
    }
}
static int sigh_fx = -2;                 /* the panic's breath: DYING SIGH, found by name (-1: none) */
static void panic(void)                  /* ENV: every latched voice and kept effect lets go; the world plays on */
{
    int i, n = latched_n() + held_count();
    if (sigh_fx == -2)
        for (sigh_fx = -1, i = 0; i < EN_NSFX && sigh_fx < 0; i++)
            if (!scmp(en_sfx_name(i), "DYING SIGH"))
                sigh_fx = i;
    en_letgo();
    for (i = 0; i < EN_NSFX; i++)
        if (en_st.held[i >> 5] >> (i & 31) & 1u)
            en_hold(i, 0);
    if (ui.swelling) {
        ui.swelling = 0;
        en_swell(0);
    }
    ui.tap_key = 0;
    ui.fired_at = 0;                     /* the last effect's name goes too */
    if (!(gset.rsv[2] & PANIC_QUIET) && sigh_fx >= 0 && ui.playing)
        en_fire_soft(sigh_fx);           /* a soft breath out as it all fades */
    say3("ALL LET GO", n ? "VOICES AND KEPT EFFECTS FADE" : "NOTHING WAS HELD", "THE WORLD PLAYS ON", 1400u);
}
static void power_message(void);
static void guide_next(void)
{
    if (++ui.guide > N_GUIDE) {
        ui.guide = 0;
        if (!(gset.rsv[2] & GUIDE_SEEN)) {
            gset.rsv[2] |= GUIDE_SEEN;   /* read once: it does not open by itself again */
            global_later();
            power_message();             /* and then the power-on message it stood in for */
            return;
        }
        say3("GUIDE CLOSED", "SEE IT AGAIN: GLOBAL,", "HOW TO PLAY", 1400u);
    }
}
static void input(void)
{
    uint32_t btn = plat_buttons(), keys = plat_keys(), now = plat_ms();
    int k;
    int32_t e;
    midi_in();
    midi_tick(now);
    if (btn != ui.btn || keys != ui.keys)
        ui.wake_until = now + 8000u;
    if (ui.guide) {                      /* the guide: any key or button turns the page, and does nothing else */
        if ((btn & ~ui.btn) || (keys & ~ui.keys))
            guide_next();
        ui.btn = btn;
        ui.keys = keys;
        for (k = 0; k < 4; k++)
            plat_enc(EN_K1 + k);
        plat_enc(EN_SELECT);
        plat_enc(EN_PRESET);
        plat_enc(EN_ALGO);
        return;
    }
    for (k = 0; k < NB; k++)
        if ((btn >> k & 1u) && !(ui.btn >> k & 1u))
            button(k);
    if (ui.swelling && !(btn >> B_LFO & 1u)) {
        ui.swelling = 0;
        en_swell(0);
    }
    if (ui.fx_down && !(btn >> B_FX & 1u)) {
        ui.fx_down = 0;
        if (!ui.fx_used && now - ui.fx_at < FX_TAP_MS)
            menu_set(ui.menu == MENU_FX ? MENU_NONE : MENU_FX);
    }
    ui.btn = btn;
    for (k = 0; k < NKEYS; k++) {
        if (!(keys >> k & 1u) && (ui.keys >> k & 1u))
            en_key(k, 0, 0, 0);      /* let go: a held voice fades */
        if ((keys >> k & 1u) && !(ui.keys >> k & 1u))
            key_down(k);
    }
    if (ui.tap_key && !(keys >> (ui.tap_key - 1) & 1u))
        ui.tap_key = 0;                  /* let go before KEEP_MS: it was a tap */
    else if (ui.tap_key && now - ui.tap_at >= KEEP_MS) {
        int i = ui.tap_fx;               /* held: keep it on, or let it go */
        ui.tap_key = 0;
        if (en_st.held[i >> 5] >> (i & 31) & 1u) {
            en_hold(i, 0);
            fx_say("STOPPED", en_sfx_name(i));
        } else if (held_count() >= MAX_HELD)
            fx_say("4 KEPT ON IS THE MOST", helping() ? "HOLD ONE OF THEM TO STOP IT" : "");
        else if (held_count() + latched_n() >= 6)
            fx_say("6 AT ONCE IS THE MOST", helping() ? "LATCHED VOICES + KEPT EFFECTS" : "");
        else {
            en_hold(i, 1);
            fx_say("KEPT ON", en_sfx_name(i));
        }
    }
    ui.keys = keys;
    if ((e = plat_enc(EN_SELECT)) != 0) {
        int s = e > 0 ? 1 : -1;
        if (ui.fx_down || ui.menu == MENU_FX) {
            ui.fx_bank = wrap(ui.fx_bank + s, 0, NBANK - 1);
            ui.fx_used = 1;
        } else if (ui.menu == MENU_GLOBAL)
            ui.gl_row = (int)clamp(ui.gl_row + e, 0, N_GLOBAL - 1);
        else if (ui.menu == MENU_NONE) {
            ui.page = wrap(ui.page + s, 0, NPAGE - 1);
            ui.knob_last = -1;
            ui.touch_until = now + 5600u;
        }
    }
    if ((e = plat_enc(EN_PRESET)) != 0) {
        int s = wrap((int)en_st.sea + (e > 0 ? 1 : -1), 0, EN_NSEA - 1);
        en_sea(s);
        tell(en_sea_name(s), en_sea_from(s), "PRESET KNOB: NEXT WORLD");
    }
    if ((e = plat_enc(EN_ALGO)) != 0) {
        proj.screen = (int8_t)wrap(proj.screen + (e > 0 ? 1 : -1), -1, VIS_NMODE - 1);
        vis_set_mode(proj.screen);
        tell("SCREEN", proj.screen < 0 ? "WORLD'S OWN" : VIS_MODE[proj.screen], "CHANGES THE PICTURE");
    }
    for (k = 0; k < 4; k++)
        if ((e = plat_enc(EN_K1 + k)) != 0) {
            ui.wake_until = now + 8000u;
            knob(k, (int)e);
        }
    if (ui.sleep_at && !ui.sleeping && (int32_t)(now - ui.sleep_at) >= 0) {  /* the sleep timer: a minute's fade */
        ui.sleeping = 1;
        ui.sleep_pick = 0;
        ui.sleep_at = 0;
        en_fade(60.0f, 0.0f);
        ui.rest_at = now + 60000u;
        ui.playing = 0;
    }
    if (ui.rest_at && (int32_t)(now - ui.rest_at) >= 0) {
        ui.rest_at = 0;
        en_play(0);
    }
    if (ui.twist[0] + ui.twist[1] + ui.twist[2] && now - ui.twist_at > TWIST_RESET_MS)
        ui.twist[0] = ui.twist[1] = ui.twist[2] = 0;
    if (ui.keep_wait && en_keep_ready()) {
        en_fossil_t *f = &proj.fossil[ui.keep_slot];
        ui.keep_wait = 0;
        for (k = 0; k < N_LAYER; k++)
            f->layer[k] = proj.layer[k];
        f->screen = proj.screen;
        for (k = 0; k < N_SHAPE; k++)
            f->shape[k] = proj.shape[k];
        for (k = 0; k < EN_MASK; k++)
            f->held[k] = en_st.held[k];
        f->rsv2[0] = proj.rsv[0];
        f->rsv2[1] = proj.rsv[1];
        f->rsv2[2] = en_st.mo_seed;
        f->rsv3[0] = proj.keys[5];       /* the SCENE knob (0: WORLD'S OWN) */
        {
            static char t[16];           /* kept: the words show after the write */
            proj.keys[7] = (uint8_t)(ui.keep_slot + 1);   /* power-on comes back to this one (START ON: SAVED WORLD) */
            copy(t, "SAVED IN SLOT 1", 16);
            t[14] = (char)('1' + ui.keep_slot);
            store_fossils(t, en_sea_name(f->sea));
        }
    }
}
void app_input_only(void)
{
    save_when_quiet(plat_ms());
    input();
}

/* ---- the screen: text over a strip */
#define STRIP 24
#define BONE RGB(238, 232, 220)
#define DIMC RGB(136, 134, 130)
#define HOT RGB(255, 170, 70)
static void label(int x, int y, const felucca_font_t *f, const char *s, uint16_t c)
{
    if (y + (int)f->h + 2 < -cv_oy || y - 2 >= -cv_oy + STRIP)
        return;                          /* not in this strip: five passes of nothing, skipped (BIG text draws many) */
    cv_text(x - 1, y, f, s, 0);
    cv_text(x + 1, y, f, s, 0);
    cv_text(x, y - 1, f, s, 0);
    cv_text(x, y + 1, f, s, 0);
    cv_text(x, y, f, s, c);
}
static void itoa_u(uint32_t v, char *b)
{
    char t[12];
    int n = 0;
    do
        t[n++] = (char)('0' + v % 10u);
    while ((v /= 10u) && n < 11);
    while (n)
        *b++ = t[--n];
    *b = 0;
}
static void strip_begin(int y0)
{
    cv_oy = -y0;
    cv_y0 = 0;
    cv_y1 = STRIP;
}
static void footer(const char *s) { cv_text(8, 224, &FONT_XS, s, DIMC); }

static const char *const BANK_TITLE[NBANK] = {"EFFECTS   BANK A", "EFFECTS   BANK B", "EFFECTS   BANK C", "EFFECTS   BANK D", "EFFECTS   BANK E"};
static void fx_menu(int y0)
{
    static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint32_t now = plat_ms();
    int k, nh;
    char t[12];
    nh = held_count();
    if (y0 < 16) {
        cv_text(6, 1, &FONT_XS, BANK_TITLE[ui.fx_bank], HOT);
        copy(t, "HELD ", 6);
        t[5] = (char)('0' + nh);
        t[6] = '/';
        t[7] = (char)('0' + MAX_HELD);
        t[8] = 0;
        cv_text(234 - text_w(&FONT_XS, t), 1, &FONT_XS, t, nh ? HOT : DIMC);
    }
    for (k = 0; k < EN_BANK; k++) {
        int i = BANK[ui.fx_bank][k], row = k % 14, y = 17 + row * 13, x = k < 14 ? 0 : 120, m = 53 + k, n, held, hot;
        if (i == BLANK)
            continue;
        held = en_st.held[i >> 5] >> (i & 31) & 1u;
        hot = ui.fired_at && ui.fired == i && (int32_t)(now - ui.fired_at) < 900;
        if (y + 13 <= y0 || y >= y0 + STRIP)
            continue;
        if (row & 1)                     /* every other line a shade lighter: the eye keeps its place */
            cv_rect(x, y, 120, 13, RGB(26, 27, 34));
        copy(t, NOTE[m % 12], 3);
        n = slen(t);
        t[n] = (char)('0' + m / 12 - 1);
        t[n + 1] = 0;
        cv_text(x + 3, y + 1, &FONT_T, t, RGB(120, 225, 190));
        cv_text(x + 24, y - 1, &FONT_XS, en_sfx_name(i), held || hot ? HOT : BONE);
    }
    if (y0 + STRIP > 198) {
        if (helping()) {
            cv_text(6, 200, &FONT_XS, "TAP: PLAY ONCE", BONE);
            cv_text(96, 200, &FONT_XS, "HOLD THE KEY: KEEP ON / OFF", BONE);
            cv_text(6, 212, &FONT_XS, "HOLD FX + KEY: PLAY OVER THE PICTURE", BONE);
            cv_text(6, 224, &FONT_XS, "SELECT KNOB: NEXT BANK", DIMC);
            cv_text(168, 224, &FONT_XS, "FX: DONE", DIMC);
        }
    }
}
#define GL_ROWS 6
static void global_menu(int y0)
{
    static const char *const NAME[N_GLOBAL] = {"HOW TO PLAY", "SOUND MODE", "WORLD CHANGES", "KNOBS WANDER", "KEY LIGHTS",
        "SOUND SLEEP TIMER", "SCREEN", "CONTROL HELP TEXT", "USB RECORDING", "PICTURE BRIGHTNESS", "TEXT SIZE", "MIDI OUT", "PANIC SOUND", "START ON", "ABOUT"};
    static const char *const HELP[N_GLOBAL][2] = {
        {"A SHORT GUIDE TO THE CONTROLS.", "TURN KNOB 1 TO OPEN IT."},
        {"SPEAKER: FOR THE FM-1'S OWN SPEAKER.", "HEADPHONES: ALSO BEST FOR RECORDING."},
        {"OFF: STAYS IN THIS WORLD. OTHERWISE IT", "MOVES ON BY ITSELF: SLOW TO RESTLESS."},
        {"HOW FAR THE KNOB SOUNDS WANDER ALONE.", "0: THEY STAY EXACTLY WHERE YOU SET THEM."},
        {"ALL ON: EVERY KEY AND BUTTON IS LIT.", "KEY NOTES ONLY: LIT KEYS FIT THE MUSIC."},
        {"SOUND FADES OUT, THEN STOPS AT THIS TIME.", "SCREEN STAYS ON. PRESS PLAY TO RESTART."},
        {"BLACK: DARK AFTER 8 SEC. SOUND STAYS ON.", "TOUCH A CONTROL. RESETS AT POWER-ON."},
        {"ON: EXPLAINS EACH CONTROL AS YOU USE IT.", "OFF: LEAVES THE PICTURE CLEAR."},
        {"FULL DEPTH: FOR HEADPHONES, EDITING.", "FOR PHONES: LOW SOUNDS MADE AUDIBLE."},
        {"BRIGHTER PICTURES, FOR OUTSIDE.", "THE SCREEN LIGHT ITSELF IS ALWAYS FULL."},
        {"BIG: LARGE WORDS WHEN YOU TURN A KNOB.", "THE PICTURE STAYS, JUST DIMMED BEHIND."},
        {"ON: SENDS THE NOTES IT PLAYS OVER USB.", "BELLS CH 1, GROUND CH 2, PADS CH 3."},
        {"ENV LETS EVERYTHING GO. ON: A SOFT", "BREATH AS IT FADES. OFF: SILENT."},
        {"AT POWER-ON: THE WORLD YOU SAVED,", "OR A NEW WORLD EVERY TIME."},
        {"IDEAS FOR FUTURE UPDATES:", "polystellarflow@gmail.com"}};
    int top = clamp(ui.gl_row - GL_ROWS / 2, 0, N_GLOBAL - GL_ROWS), q;
    char t[20], d[12];
    if (y0 < 20)
        cv_text(10, 4, &FONT_XS, "GLOBAL SETTINGS", HOT);
    for (q = 0; q < GL_ROWS; q++) {
        int r = top + q, y = 25 + q * 24, sel = r == ui.gl_row, n;
        const char *v = "";
        if (y + 20 <= y0 || y - 3 >= y0 + STRIP)
            continue;
        if (sel)
            cv_rect(6, y - 3, 228, 22, RGB(44, 44, 50));
        cv_text(12, y, &FONT_XS, NAME[r], sel ? BONE : DIMC);
        switch (r) {
        case GL_GUIDE: v = "TURN TO OPEN"; break;
        case GL_OUTPUT: v = gset.speaker ? "SPEAKER" : "HEADPHONES"; break;
        case GL_DRIFT: v = !gset.drift ? "OFF" : gset.evolve == 0 ? "SLOW" : gset.evolve == 1 ? "NORMAL" : "RESTLESS"; break;
        case GL_LIGHTS: v = gset.lights == 0 ? "OFF" : gset.lights == 1 ? "ALL ON" : "KEY NOTES ONLY"; break;
        case GL_TIDE: {                  /* a percent, not a bare number */
            int n2 = 0;
            itoa_u(gset.tide, t);
            while (t[n2])
                n2++;
            t[n2] = '%';
            t[n2 + 1] = 0;
            v = gset.tide ? t : "OFF";
            break;
        }
        case GL_SLEEP:
            if (ui.sleep_pick) {
                itoa_u(SLEEP_MIN[ui.sleep_pick], t);
                n = slen(t);
                copy(t + n, " MIN", 5);
                v = t;
            } else
                v = "OFF";
            break;
        case GL_SCREEN: v = gset.screen ? "PICTURE" : "BLACK"; break;
        case GL_HELP: v = gset.rsv[2] & 1u ? "OFF" : "ON"; break;
        case GL_USB: v = gset.rsv[2] & 2u ? "FOR PHONES" : "FULL DEPTH"; break;
        case GL_BRIGHT: v = (gset.rsv[2] >> 2 & 3u) == 2 ? "BRIGHTEST" : (gset.rsv[2] >> 2 & 3u) == 1 ? "BRIGHT" : "NORMAL"; break;
        case GL_TEXT: v = big() ? "BIG" : "NORMAL"; break;
        case GL_MIDI: v = gset.rsv[2] & 32u ? "OFF" : "ON"; break;
        case GL_PANIC: v = gset.rsv[2] & PANIC_QUIET ? "OFF" : "ON"; break;
        case GL_START: v = gset.favs & START_NEW ? "NEW WORLD" : "SAVED WORLD"; break;
        case GL_ABOUT: v = BETA_LABEL; break;
        default:
            itoa_u(plat_cpu_pct(), d);
            copy(t, d, 6);
            n = slen(t);
            copy(t + n, plat_cpu2_on() ? "%  2 CORES" : "%  1 CORE", 11);
            v = t;
            break;
        }
        cv_text(228 - text_w(&FONT_XS, v), y, &FONT_XS, v, sel ? HOT : BONE);
    }
    if (y0 < 192 && y0 + STRIP > 166) {
        if (helping() || ui.gl_row == GL_HELP || ui.gl_row == GL_ABOUT) {   /* (its own row always says, so it can be found again) */
            cv_text(8, 167, &FONT_XS, HELP[ui.gl_row][0], HOT);
            cv_text(8, 179, &FONT_XS, HELP[ui.gl_row][1], HOT);
        }
    }
    if (y0 + STRIP > 192) {
        if (helping()) {
            cv_text(10, 193, &FONT_XS, "TURN SELECT KNOB: CHOOSE A SETTING", BONE);
            cv_text(10, 208, &FONT_XS, "TURN KNOB 1: CHANGE IT", BONE);
        }
        if (helping())
            cv_text(10, 223, &FONT_XS, "PRESS GLOBAL: DONE. IT SAVES ITSELF.", DIMC);
    }
}
static void save_menu(int y0)
{
    static const char *const KEYN[NFOSSIL] = {"KEY F", "KEY G", "KEY A", "KEY B"};
    static const char *const HOW[4] = {"KNOB 1: CHOOSE A SLOT", "SAVE: KEEP THIS WORLD THERE", "TAP A LIT KEY: LOAD IT",
                                       "HOME: LEAVE WITHOUT SAVING"};
    int s, y;
    char t[28];
    if (y0 < 20)
        cv_text(10, 4, &FONT_XS, "SAVE   4 SLOTS FOR YOUR WORLDS", HOT);
    for (s = 0; s < NFOSSIL; s++) {
        const en_fossil_t *f = &proj.fossil[s];
        int sel = s == ui.slot;
        y = 24 + s * 22;
        if (y + 20 <= y0 || y >= y0 + STRIP)
            continue;
        t[0] = (char)('1' + s);
        t[1] = ' ';
        copy(t + 2, f->used ? en_sea_name(f->sea) : "EMPTY", 20);
        if (sel)
            cv_rect(6, y - 3, 228, 21, RGB(44, 44, 50));
        cv_text(12, y, &FONT_S, t, sel ? BONE : f->used ? BONE : DIMC);
        cv_text(228 - text_w(&FONT_XS, KEYN[s]), y + 2, &FONT_XS, KEYN[s], f->used ? HOT : DIMC);
    }
    if ((int32_t)(ui.msg_until - plat_ms()) > 0) {   /* the menu's own answers, in place of the how-to */
        if (y0 < 136 && y0 + STRIP > 116)
            cv_text(12, 116, &FONT_B, ui.msg[0], HOT);
        for (s = 1; s < 3; s++) {
            y = 124 + s * 16;
            if (y + 14 > y0 && y < y0 + STRIP)
                cv_text(12, y, &FONT_XS, ui.msg[s], BONE);
        }
    } else
        for (s = 0; s < 4; s++) {
            y = 118 + s * 16;
            if (y + 14 > y0 && y < y0 + STRIP)
                cv_text(12, y, &FONT_XS, HOW[s], s == 1 ? HOT : BONE);
        }
    y = 190;
    if (y + 22 > y0 && y < y0 + STRIP) {
        cv_text(12, y, &FONT_XS, "KNOB 4: ERASE THE SLOT", DIMC);
        cv_text(228 - text_w(&FONT_XS, "3 CLICKS RIGHT"), y, &FONT_XS, "3 CLICKS RIGHT", DIMC);
        cv_rect(12, y + 15, 216, 3, RGB(52, 52, 56));
        cv_rect(12, y + 15, ui.twist[2] * 216 / 100, 3, HOT);
    }
}
static int knob_value(int p, int k)
{
    if (p == PG_WATER) return proj.macro[k];
    if (p == PG_LIFE) return proj.layer[k];
    if (p == PG_ROOM || p == PG_MOVE) return proj.shape[(p - PG_ROOM) * 4 + k];
    if (p == PG_KEYS && k == 3 && (!proj.keys[0] || proj.keys[0] == 4)) return -1;
    if (p == PG_KEYS && k >= 2) return proj.keys[k];
    return -1;
}
/* the battery beside the depth: three cells (the hardware reads four levels, not a percentage), USB on a host's power */
static void battery_mark(int x, int y)
{
    int b = plat_battery(), i;
    if (b < 0)
        return;
    if (b > 3) {
        label(x, y - 2, &FONT_XS, "USB", DIMC);
        return;
    }
    cv_rect(x - 1, y - 1, 18, 9, 0);
    cv_rect(x, y, 14, 1, BONE);
    cv_rect(x, y + 6, 14, 1, BONE);
    cv_rect(x, y, 1, 7, BONE);
    cv_rect(x + 13, y, 1, 7, BONE);
    cv_rect(x + 14, y + 2, 2, 3, BONE);
    for (i = 0; i < b; i++)
        cv_rect(x + 2 + i * 4, y + 2, 3, 3, BONE);
    if (!b)
        cv_rect(x + 2, y + 2, 2, 3, HOT);
}
static const char *knob_name(int p, int k)   /* on the KEYS page the knobs follow what the keys play */
{
    if (p == PG_KEYS && !proj.keys[0])
        return k == 1 ? "KEY BELL" : k == 3 ? (latched_n() ? "LET GO VOICES" : "") : KNOB[p][k];
    if (p == PG_KEYS && proj.keys[0] == 4 && k == 3)
        return "KEY BELL";
    return KNOB[p][k];
}
static int help_up(uint32_t now) { return !(gset.rsv[2] & 1u) && (int32_t)(ui.touch_until - now) > 0; }
static const char *value_text(int p, int k, char *t)
{
    static const char *const KEY[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int s, v;
    if (p == PG_KEYS) {
        if (k == 0)
            return proj.keys[0] == 3 ? "LATCH" : proj.keys[0] == 4 ? "SPLIT" : proj.keys[0] ? "VOICES" : "BELLS";
        if (!proj.keys[0] && k == 3)
            return "TURN TO STOP THEM";
        if (proj.keys[0] == 4 && k == 3)
            return proj.keys[4] ? en_bell_name(proj.keys[4] - 1) : "WORLD'S OWN";
        if (!proj.keys[0])
            return proj.keys[4] ? en_bell_name(proj.keys[4] - 1) : "WORLD'S OWN";
        return en_kvox_name(proj.keys[1]);
    }
    if (p == PG_VOICES) {
        static const char *const SC[7] = {"WORLD'S OWN", "BELLS ALONE", "THE CHOIR", "CHANT", "THE DARK", "EMPTINESS", "BLOOM"};
        return k < 3 ? pick_name(k, proj.pick[k]) : SC[proj.keys[5] % 7];
    }
    if (p == PG_NOTES) {
        static const char *const R[4] = {"OFF", "RARE", "NORMAL", "OFTEN"};
        if (k == 2)
            return R[mo_val(0)];
        if (k == 3)
            return "TURN IT";
    }
    (void)t;
    s = S_KEY + k;
    v = proj.shape[s];
    if (!v)
        return "WORLD'S OWN";
    return s == S_KEY ? KEY[v - 1] : en_family_name(v - 1);
}
static void fit_text(int x, int y, const char *s, uint16_t c, int large)   /* as big as the line allows */
{
    const felucca_font_t *f = large ? &FONT_M : &FONT_S;
    if (text_w(f, s) > 236 - x)
        f = large ? &FONT_B : &FONT_XS;
    if (text_w(f, s) > 236 - x)
        f = &FONT_XS;
    label(x, y, f, s, c);
}
static void knob_help(int p, int k, const char **a, const char **b)
{
    int bell = p == PG_KEYS && ((!proj.keys[0] && k == 1) || (proj.keys[0] == 4 && k == 3)), go = p == PG_KEYS && !proj.keys[0] && k == 3;
    *a = bell ? "THE BELL THE KEYS STRIKE." : go ? "TURN TO LET THE LATCHED VOICES GO." : k >= 0 ? KNOB_HELP[p][k][0] : "TURN A KNOB TO SEE WHAT IT DOES.";
    *b = bell ? "WORLD'S OWN: THE WORLD CHOOSES." : go ? "THEY FADE OUT. THE BELLS STAY." : k >= 0 ? KNOB_HELP[p][k][1] : "";
}
static int bar_shown[4] = {-1, -1, -1, -1}, bar_page = -1;   /* the bars as drawn, in 1/16ths: they glide to the knob */
static void ease_bars(void)              /* once a picture: part of the way there, so a turn reads as one smooth sweep */
{
    int k;
    for (k = 0; k < 4; k++) {
        int v = knob_value(ui.page, k), t = v * 16;
        if (v < 0 || bar_page != ui.page || bar_shown[k] < 0)
            bar_shown[k] = v < 0 ? -1 : t;
        else
        {   /* a quick glide: two thirds of the way each picture, and never stuck one step short */
            int d = t - bar_shown[k], m = d * 2 / 3;
            if (m == 0 && d)
                m = d > 0 ? 1 : -1;
            bar_shown[k] += m;
        }
    }
    bar_page = ui.page;
}
static int half_mark(int k)             /* a knob whose halfway means something (THE WORLD'S OWN, NORMAL): a tick shows it */
{
    const char *h = KNOB_HELP[ui.page][k][1];
    for (; *h; h++)
        if (h[0] == 'H' && h[1] == 'A' && h[2] == 'L' && h[3] == 'F' && h[4] == 'W')
            return ui.page != PG_KEYS;
    return 0;
}
static uint16_t mix565(uint16_t a, uint16_t b, int f)   /* a blended with b, f of 16 parts toward b */
{
    int r = ((a >> 11) * (16 - f) + (b >> 11) * f) >> 4, g = ((a >> 5 & 63) * (16 - f) + (b >> 5 & 63) * f) >> 4,
        bl = ((a & 31) * (16 - f) + (b & 31) * f) >> 4;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static void bar_draw(int k, int x, int y, int w, int h, uint16_t on, uint16_t off)   /* the end pixel shaded: no stair-steps */
{
    int v = bar_shown[k] < 0 ? knob_value(ui.page, k) * 16 : bar_shown[k], full = v * w / 1600, f = (v * w % 1600) * 16 / 1600;
    cv_rect(x, y, w, h, off);
    cv_rect(x, y, full, h, on);
    if (full < w && f)
        cv_rect(x + full, y, 1, h, mix565(off, on, f));
}
static const felucca_font_t *fit_font(const char *s, int w)   /* the biggest that fits w */
{
    if (text_w(&FONT_B, s) <= w)
        return &FONT_B;
    if (text_w(&FONT_S, s) <= w)
        return &FONT_S;
    return &FONT_XS;
}
static void big_knob(void)               /* TEXT SIZE: BIG: the whole page at once, four knobs in large letters */
{
    int p = ui.page, k, last = ui.knob_last;
    const char *a, *b;
    char t[32];
    label(10, 114, &FONT_B, PAGE_NAME[p], HOT);
    if (helping() && text_w(&FONT_B, PAGE_NAME[p]) + text_w(&FONT_XS, "SELECT KNOB: NEXT PAGE") < 212)
        label(230 - text_w(&FONT_XS, "SELECT KNOB: NEXT PAGE"), 118, &FONT_XS, "SELECT KNOB: NEXT PAGE", DIMC);   /* only when it fits beside the page name */
    for (k = 0; k < 4; k++) {
        int x = 10 + (k & 1) * 114, y = 140 + (k >> 1) * 36, v = knob_value(p, k), on = k == last;
        uint16_t c = on ? HOT : BONE;
        if (!knob_name(p, k)[0])
            continue;                    /* a knob with nothing to do on this page */
        t[0] = (char)('1' + k);
        t[1] = ' ';
        copy(t + 2, knob_name(p, k), 24);
        label(x, y, fit_font(t, 108), t, c);
        if (v >= 0) {
            bar_draw(k, x, y + 20, 104, 6, c, RGB(50, 50, 54));
            if (half_mark(k))
                cv_rect(x + 51, y + 18, 2, 10, DIMC);
        } else {
            const char *q = value_text(p, k, t);
            label(x, y + 17, fit_font(q, 108) == &FONT_B ? &FONT_S : &FONT_XS, q, on ? HOT : DIMC);
        }
    }
    if (helping() && last >= 0 && knob_name(p, last)[0]) {
        knob_help(p, last, &a, &b);
        label(10, 212, &FONT_XS, a, BONE);
        label(10, 225, &FONT_XS, b, BONE);
    } else if (helping())
        label(10, 218, &FONT_XS, "TURN A KNOB: IT LIGHTS UP HERE", DIMC);
}
static void overlays(int y0)
{
    uint32_t now = plat_ms();
    char t[28];
    if (ui.gl_wait && y0 < 10)
        cv_rect(230, 4, 5, 5, DIMC);     /* GLOBAL settings wait to be written: PLAY to stop, or SAVE */
    int w;
    if (y0 + STRIP > 222 && !help_up(now)) {     /* the depth and the sea */
        battery_mark(8, 226);
        copy(t, en_st.frozen ? "HELD" : en_sea_name(en_st.sea), 14);
        w = slen(t);
        if (!en_st.frozen) {
            t[w] = ' '; t[w + 1] = '/'; t[w + 2] = ' ';
            copy(t + w + 3, en_stratum_name(en_st.stratum), 12);
        }
        w = text_w(&FONT_XS, t);
        label(233 - w, 224, &FONT_XS, t, BONE);
    }
    if (en_st.latched && y0 < 20 && !ui.swelling && !ui.fx_down) {   /* the latched notes, in the corner */
        static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int k, n = 8;
        copy(t, "LATCHED ", 10);
        for (k = 0; k < NKEYS && n < 24; k++)
            if (en_st.latched >> k & 1u) {
                const char *q = NN[(53 + k) % 12];
                while (*q)
                    t[n++] = *q++;
                t[n++] = ' ';
            }
        t[n] = 0;
        label(7, 5, &FONT_XS, t, HOT);
    }
    if (en_st.motif && y0 < 20 && !ui.fx_down) {
        int w = text_w(&FONT_XS, "MOTIF");
        label(233 - w, 5, &FONT_XS, "MOTIF", DIMC);
    }
    if (ui.swelling && y0 < 20)
        label(7, 5, &FONT_XS, helping() ? "SWELL: LET GO TO COLLAPSE" : "SWELL", HOT);
    if (en_st.auto_seq != ui.auto_seq) {     /* the world chose an effect: its name, small, at the top */
        ui.auto_seq = en_st.auto_seq;
        ui.auto_at = now;
    }
    if (ui.auto_at && (int32_t)(now - ui.auto_at) < 4000 && en_st.auto_fx >= 0 && !ui.swelling && !ui.fx_down &&
        proj.layer[L_FAUNA] &&                    /* CREATURES at 0: nothing is heard, so no name either */
        !(ui.fired_at && (int32_t)(now - ui.fired_at) < (int32_t)FX_SHOW_MS) && !ui.menu && y0 < (en_st.latched ? 32 : 20))
        label(7, en_st.latched ? 18 : 5, &FONT_XS, en_sfx_name(en_st.auto_fx), DIMC);   /* under the latched notes, never on them */
    {                                    /* FX held down: a line of small words, and the picture stays whole */
        int show = ui.fired_at && (int32_t)(now - ui.fired_at) < (int32_t)FX_SHOW_MS;
        if ((ui.fx_down || show) && y0 < 44) {
            if (ui.fx_down) {
                copy(t, "FX BANK A", 12);
                t[8] = (char)('A' + ui.fx_bank);
                label(7, 5, &FONT_XS, t, HOT);
            }
            if (show) {
                label(7, 18, &FONT_XS, en_sfx_name(ui.fired), BONE);
                if (ui.fx_word && (int32_t)(ui.fx_word_until - now) > 0)
                    label(7, 30, &FONT_XS, ui.fx_word, HOT);
            }
        }
    }
    if ((en_st.held[0] | en_st.held[1] | en_st.held[2] | en_st.held[3] | en_st.held[4]) && y0 < 222 && y0 + STRIP > 206 && !help_up(now))
        label(7, 208, &FONT_XS, "FX HELD", HOT);
    if ((int32_t)(ui.msg_until - now) > 0 && ui.menu != MENU_SAVE && y0 < 150 && y0 + STRIP > 48) {
        if (big()) {
            fit_text(12, 52, ui.msg[0], BONE, 1);   /* above the knob's own words */
            fit_text(12, 78, ui.msg[1], BONE, 0);
            fit_text(12, 94, ui.msg[2], BONE, 0);
        } else {
            cv_text(12, 92, &FONT_B, ui.msg[0], BONE);
            cv_text(12, 112, &FONT_S, ui.msg[1], BONE);
            cv_text(12, 130, &FONT_S, ui.msg[2], BONE);
        }
    }
    if ((int32_t)(ui.touch_until - now) > 0 && big() && y0 + STRIP > 112) {
        big_knob();                      /* TEXT SIZE: BIG: one knob at a time, in large letters */
        return;
    }
    if ((int32_t)(ui.touch_until - now) > 0 && y0 + STRIP > 146) {
        int k, p = ui.page;
        cv_text(10, 148, &FONT_XS, PAGE_NAME[p], HOT);
        if (helping())
            cv_text(230 - text_w(&FONT_XS, "SELECT KNOB: NEXT PAGE"), 148, &FONT_XS, "SELECT KNOB: NEXT PAGE", DIMC);
        for (k = 0; k < 4; k++) {
            int x = 10 + (k & 1) * 114, y = 164 + (k >> 1) * 24, v = knob_value(p, k);
            if (!knob_name(p, k)[0])
                continue;                /* a knob with nothing to do on this page */
            t[0] = (char)('1' + k);
            t[1] = ' ';
            copy(t + 2, knob_name(p, k), 20);
            cv_text(x, y, &FONT_XS, t, BONE);
            if (v >= 0) {
                bar_draw(k, x, y + 14, 104, 3, BONE, RGB(50, 50, 54));
                if (half_mark(k))
                    cv_rect(x + 52, y + 12, 1, 7, DIMC);
            } else
                cv_text(x, y + 11, &FONT_XS, value_text(p, k, t), DIMC);
        }
        if (!(gset.rsv[2] & 1u) && y0 < 240 && y0 + STRIP > 208) {
            const char *a, *b;
            knob_help(p, ui.knob_last, &a, &b);
            cv_text(10, 209, &FONT_XS, a, HOT);
            cv_text(10, 222, &FONT_XS, b, HOT);
        }
    }
}
static void lights(void)
{
    uint32_t now = plat_ms(), keys = 0, b = 0;
    int k, i;
    if (now - ui.leds_at < 120u)
        return;
    ui.leds_at = now;
    if (ui.menu == MENU_SAVE) {
        for (k = 0; k < NFOSSIL; k++)
            if (proj.fossil[k].used)
                keys |= 1u << SLOT_KEY[k];
    } else if (gset.lights) {
        if (en_st.latched && ui.menu != MENU_FX && !ui.fx_down)
            keys = en_st.latched;        /* a lit key is a latched voice */
        else if (ui.menu == MENU_FX || ui.fx_down) {   /* a lit key is a held effect */
            for (k = 0; k < NKEYS; k++) {
                int e = BANK[ui.fx_bank][k];
                if (e != BLANK && en_st.held[e >> 5] >> (e & 31) & 1u)
                    keys |= 1u << k;
            }
        } else {
            float rootm = 69.0f + 12.0f * fm_log2f(fm_maxf(en_st.root, 1.0f) / 440.0f);
            uint32_t pcs = 0;
            for (i = 0; i < 6; i++) {
                int m = (int)(rootm + 12.0f * fm_log2f(fm_maxf(en_st.field[i], .01f)) + 1000.5f) - 1000;
                pcs |= 1u << (((m % 12) + 12) % 12);
            }
            for (k = 0; k < NKEYS; k++)
                if (pcs >> ((53 + k) % 12) & 1u)
                    keys |= 1u << k;
        }
    }
    if (ui.playing)
        b |= 1u << B_PLAY;
    if (ui.frozen)
        b |= 1u << B_REC;
    if (ui.menu == MENU_FX || ui.fx_down)
        b |= 1u << B_FX;
    if (ui.menu == MENU_GLOBAL)
        b |= 1u << B_GLO;
    if (ui.menu == MENU_SAVE)
        b |= 1u << B_SAVE;
    if (b != ui.led_b || keys != ui.led_k || gset.lights != ui.led_mode) {   /* a new setting shows at once */
        ui.led_mode = gset.lights;
        plat_glow(gset.lights == 1 ? 2 : 0);
        plat_leds(b, keys);
        ui.led_b = b;
        ui.led_k = keys;
    }
}
static void power_message(void)      /* version 1: gone. The guide's first page says it instead */
{
    if (1)
        return;
    if (gset.speaker)
        say3("SPEAKER MODE", "HEADPHONES RECOMMENDED", "CHANGE IT IN GLOBAL", 3500u);
    else
        say3("HEADPHONE MODE", "SPEAKER MODE IS IN GLOBAL", "", 3500u);
}
void app_ui_init(void)
{
    ui.last_ms = plat_ms();
    ui.title_until = ui.last_ms + TITLE_MS;
    vis_title(1);
}

static const char *const GUIDE[N_GUIDE][6] = {
    {"WELCOME TO HADAL-1", "IT PLAYS ITSELF, ENDLESSLY.", "PLAY: START OR STOP THE SOUND.", "FM-1 SPEAKER MODE IS ON AT FIRST.",
     "IT SOUNDS BEST ON HEADPHONES:", "GLOBAL, SOUND MODE, HEADPHONES."},
    {"THE KNOBS", "THE 4 KNOBS SHAPE THE SOUND.", "SELECT KNOB: NEXT PAGE OF KNOBS.", "HOME: BACK TO THE FIRST PAGE.",
     "FIRST PAGE, MAIN: DARKNESS,", "ACTIVITY, DECAY AND SPACE."},
    {"WORLDS", "PRESET KNOB: THE NEXT WORLD.", "SEQ: A RANDOM NEW WORLD.", "ARP: NEW SCALE AND SOUNDS.",
     "EDIT: A NEW PHASE OF THIS WORLD.", "SAVE: KEEP THIS WORLD."},
    {"THE KEYS", "THE KEYS PLAY BELLS IN TUNE.", "KEYS PAGE, KNOB 1: BELLS,", "VOICES, LATCH OR SPLIT.",
     "LATCH: A VOICE STAYS ON.", "PRESS ITS KEY AGAIN: IT LETS GO."},
    {"EFFECTS", "TAP FX: THE EFFECTS MENU.", "THEN TAP A KEY: IT PLAYS ONCE.", "HOLD THE KEY: IT STAYS ON.",
     "HOLD IT AGAIN: IT STOPS.", "HOLD LFO: THE SOUND SWELLS."},
    {"TOO MUCH AT ONCE?", "PRESS ENV (PANIC):", "EVERY LATCHED VOICE AND", "KEPT EFFECT LETS GO.",
     "REC: FREEZE THE SOUND.", "GLOBAL: SETTINGS, AND THIS GUIDE."},
    {"THANK YOU", "HADAL-1 BY POLYSTELLARFLOW.", "", "IDEAS FOR FUTURE UPDATES?", "polystellarflow@gmail.com",
     "(ALSO IN GLOBAL, ABOUT)"}};
static void guide(int y0)
{
    const char *const *g = GUIDE[ui.guide - 1];
    char f[8];
    int i;
    cv_rect(0, 0, 240, 240, 0);
    cv_text((240 - text_w(&FONT_B, g[0])) / 2, 28, &FONT_B, g[0], HOT);
    for (i = 1; i < 6; i++)
        cv_text((240 - text_w(&FONT_S, g[i])) / 2, 48 + i * 26, &FONT_S, g[i], BONE);
    f[0] = (char)('0' + ui.guide);
    f[1] = '/';
    f[2] = (char)('0' + N_GUIDE);
    f[3] = 0;
    cv_text(10, 216, &FONT_XS, ui.guide < N_GUIDE ? "ANY KEY OR BUTTON: NEXT" : "ANY KEY OR BUTTON: START PLAYING", DIMC);
    cv_text(230 - text_w(&FONT_XS, f), 216, &FONT_XS, f, DIMC);
    (void)y0;
}
static void title(int y0)
{
    int w = text_w(&FONT_L, "HADAL-1"), by = text_w(&FONT_S, "By Polystellarflow");
    if (y0 < 136 && y0 + STRIP > 96)
        label((240 - w) / 2, 104, &FONT_L, "HADAL-1", BONE);
    if (y0 < 160 && y0 + STRIP > 136)
        label((240 - by) / 2, 140, &FONT_S, "By Polystellarflow", BONE);
    if (y0 < 186 && y0 + STRIP > 166)
        label((240 - text_w(&FONT_S, BETA_LABEL)) / 2, 168, &FONT_S, BETA_LABEL, HOT);
}
void app_frame(void)
{
    uint32_t now = plat_ms();
    int y, msg, touch, intro = ui.title_until != 0;
    save_when_quiet(plat_ms());
    input();
    lights();
    if (ann_on() && ann_where())
        ann_say(en_sea_name(en_st.sea), 0, ANN_NOW);
    ann_tick(now);
    if (intro && (int32_t)(now - ui.title_until) >= 0) {
        ui.title_until = 0;
        intro = 0;
        vis_title(0);
        if (!(gset.rsv[2] & GUIDE_SEEN))
            ui.guide = 1;                /* the first time: a short guide before anything else */
        else
            power_message();
    }
    /* SCREEN BLACK: the picture rests until something is touched, and the sound carries on */
    if (!intro && !ui.menu && !ui.guide && !gset.screen && (int32_t)(ui.wake_until - now) <= 0) {
        if (!ui.blanked) {
            ui.blanked = 1;
            for (y = 0; y < 240; y += STRIP) {
                uint32_t i;
                lcd_sync();
                for (i = 0; i < 240u * STRIP; i++)
                    cv_px[i] = 0;
                lcd_blit(0, (uint32_t)y, 240, STRIP, cv_px);
            }
        }
        return;
    }
    ui.blanked = 0;
    /* one frame computes, the next sends it: about 30 pictures a second */
    if (ui.phase == 0) {
        float dt = (float)(now - ui.last_ms) * .001f;
        ui.last_ms = now;
        vis_step(fm_clampf(dt, .005f, .08f));
        ui.phase = 1;
        return;
    }
    ui.phase = 0;
    ease_bars();
    msg = (int32_t)(ui.msg_until - now) > 0;
    touch = (int32_t)(ui.touch_until - now) > 0;
    for (y = 0; y < 240; y += STRIP) {
        int d0 = ui.menu ? 0 : touch ? (big() ? (msg ? 48 : 112) : 146) : msg ? (big() ? 48 : 88) : 0, d1 = ui.menu ? 240 : touch ? (gset.rsv[2] & 1u && !big() ? 216 : 240) : msg ? (big() ? 116 : 150) : 0;
        lcd_sync();
        cv_w = 240;
        cv_h = STRIP;
        if (ui.menu == MENU_FX) {        /* black behind the words: nothing blends */
            uint32_t i;
            for (i = 0; i < 240u * STRIP; i++)
                cv_px[i] = 0;
        } else
            vis_scan(cv_px, y, STRIP, d0, d1);
        strip_begin(y);
        if (intro)
            title(y);
        else if (ui.guide)
            guide(y);
        else if (ui.menu == MENU_FX)
            fx_menu(y);
        else if (ui.menu == MENU_GLOBAL)
            global_menu(y);
        else if (ui.menu == MENU_SAVE)
            save_menu(y);
        else
            overlays(y);
        cv_oy = 0;
        if (ui.menu && ui.menu != MENU_SAVE && msg && y < 134 && y + STRIP > 98) {  /* a result stays readable over a menu (SAVE shows its own) */
            strip_begin(y);
            cv_rect(0, 98, 240, 36, 0);
            cv_text(12, 100, &FONT_B, ui.msg[0], HOT);
            cv_text(12, 116, &FONT_S, ui.msg[1], BONE);
            cv_oy = 0;
        }
        lcd_blit(0, (uint32_t)y, 240, STRIP, cv_px);
    }
}

#ifdef APP_HOST_EXPECT
int app_expect(const char *what, int *got) { (void)what; (void)got; return 0; }
#endif
