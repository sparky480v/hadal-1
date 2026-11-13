/* The second half of the world: appended to the sketch's tables by tools_gen_world.js. */
const NEW_BELLS = [
  { name: 'DEEP BOWL', r: [1, 1.004, 2.68, 2.70, 4.9, 4.94, 8.1], d: [30, 28, 16, 14, 7, 6, 3], g: [.9, .8, .42, .34, .16, .12, .05], att: 1.8, lo: 70, hi: 260 },
  { name: 'CRYSTAL BOWL', r: [1, 1.002, 3.0, 3.01, 6.02, 6.05, 9.1], d: [34, 32, 18, 16, 8, 7, 4], g: [.9, .85, .18, .14, .07, .05, .02], att: 2.4, lo: 180, hi: 760 },
  { name: 'WATER BOWL', r: [1, 1.012, 2.74, 2.79, 5.2, 5.3, 8.6], d: [18, 16, 10, 9, 5, 4, 2], g: [.8, .7, .46, .36, .18, .14, .05], att: 1.1, lo: 120, hi: 520, bend: 2.2 },
  { name: 'TINGSHA', r: [1, 1.009, 2.41, 2.44, 4.1, 5.6, 7.3], d: [8, 7.5, 5, 4.5, 3, 2, 1.4], g: [.8, .7, .4, .32, .2, .1, .05], att: 0, lo: 700, hi: 2400 },
  { name: 'TEMPLE BELL', r: [.5, .502, 1, 1.004, 1.5, 2.2, 3.3], d: [42, 38, 28, 24, 14, 8, 4], g: [.9, .8, .68, .5, .24, .16, .06], att: .04, lo: 50, hi: 190 },
  { name: 'BURIED BELL', r: [.5, 1, 1.19, 1.5, 2, 2.5, 3], d: [14, 11, 8, 6, 3, 2, 1.2], g: [.8, .8, .5, .24, .2, .06, .03], att: .08, lo: 70, hi: 300 },
  { name: 'CRACKED BELL', r: [.5, .53, 1, 1.07, 1.21, 1.56, 2.63], d: [10, 9, 8, 7, 6, 4, 2], g: [.7, .6, .8, .6, .5, .3, .16], att: 0, lo: 90, hi: 560 },
  { name: 'SHIP BELL', r: [1, 1.5, 2, 2.42, 3.03, 4.2, 5.4], d: [11, 8, 7, 5, 4, 2.5, 1.6], g: [.8, .32, .6, .3, .24, .12, .06], att: 0, lo: 240, hi: 980 },
  { name: 'TUBULAR BELL', r: [1, 2.76, 5.40, 8.93, 13.3, 1.003, 2.77], d: [20, 14, 9, 5, 3, 18, 12], g: [.6, .9, .6, .24, .08, .3, .4], att: 0, lo: 80, hi: 360 },
  { name: 'CARILLON', r: [.5, 1, 1.19, 1.5, 2, 2.67, 4], d: [16, 13, 10, 8, 6, 4, 2.4], g: [.5, .9, .7, .32, .4, .14, .08], att: 0, lo: 240, hi: 1200 },
  { name: 'GENDER', r: [1, 1.004, 2.01, 3.89, 4.02, 6.8, 7.2], d: [9, 8.5, 5, 3, 3, 1.6, 1.4], g: [.95, .8, .3, .2, .14, .06, .05], att: .002, lo: 160, hi: 1050 },
  { name: 'STONE CHIME', r: [1, 2.31, 3.47, 4.29, 5.83, 7.1, 8.9], d: [4.5, 3.4, 2.8, 2.2, 1.6, 1.2, .8], g: [.95, .5, .4, .3, .18, .1, .06], att: 0, lo: 170, hi: 800 },
  { name: 'SLIT DRUM', r: [1, 1.48, 2.1, 2.9, 3.4, 4.3, 5.2], d: [2.8, 2.2, 1.6, 1.2, .9, .7, .5], g: [.95, .4, .3, .14, .1, .05, .03], att: .002, lo: 90, hi: 420 },
  { name: 'FROZEN BELL', r: [1, 2.17, 3.73, 5.81, 8.2, 11.1, 1.003], d: [15, 11, 8, 6, 4, 2.4, 13], g: [.8, .38, .3, .18, .1, .05, .4], att: .004, lo: 340, hi: 1500 },
  { name: 'BRONZE RING', r: [1, 1.26, 1.68, 2.24, 3.1, 4.3, 5.6], d: [22, 20, 16, 12, 8, 5, 3], g: [.8, .5, .5, .4, .28, .16, .08], att: .3, lo: 90, hi: 360 },
  { name: 'SAW BLADE', r: [1, 1.61, 2.59, 3.82, 5.13, 7.27, 9.91], d: [9, 7, 6, 5, 4, 3, 2], g: [.7, .6, .6, .5, .34, .2, .1], att: .02, lo: 160, hi: 700, bend: 1.4 },
  { name: 'DARK VIBES', r: [1, 3.98, 4.01, 9.8, 1.002, 3.99, 9.9], d: [10, 5, 4.6, 2, 9, 4.4, 1.8], g: [.95, .26, .2, .05, .7, .16, .03], att: .004, lo: 120, hi: 640 },
];
const NEW_DRONES = [
  { name: 'DARK CELLO', wt: 'saw', fm: .02, lp: 2.4, f1: 330, q1: .18, a1: .45, g: .8 },
  { name: 'SUB SINE', wt: 'sine', fm: .02, lp: 2, g: 1 },
  { name: 'HARMONIUM', wt: 'organ', fm: .01, lp: 2.4, f1: 700, q1: .3, a1: .3, g: .8 },
  { name: 'DIDGERIDOO', wt: 'pulse', fm: 0, lp: 5, f1: 460, q1: .1, a1: .75, f2: 1300, q2: .12, a2: .35, sw: 620, g: .7 },
  { name: 'BAGPIPE', wt: 'brass', fm: 0, lp: 6, f1: 1000, q1: .16, a1: .6, g: .62 },
  { name: 'CONTRABASS', wt: 'saw', fm: .01, lp: 1.6, f1: 180, q1: .22, a1: .6, g: .9 },
  { name: 'FOG ORGAN', wt: 'organ', fm: .06, lp: 1.8, g: .9 },
  { name: 'BURIED ENGINE', wt: 'odd', fm: .4, lp: 2.2, f1: 240, q1: .2, a1: .5, g: .8 },
  { name: 'GHOST STRINGS', wt: 'saw', fm: .03, lp: 4, f1: 1400, q1: .3, a1: .32, g: .62 },
  { name: 'SHRUTI BOX', wt: 'pulse', fm: 0, lp: 3.4, f1: 620, q1: .2, a1: .46, f2: 1500, q2: .3, a2: .2, g: .72 },
  { name: 'DEAD CITY', wt: 'odd', fm: .12, lp: 2.8, f1: 900, q1: .08, a1: .5, sw: 400, g: .68 },
  { name: 'TAMBURA', wt: 'saw', fm: 0, lp: 3, f1: 1800, q1: .1, a1: .4, sw: 1200, g: .66 },
  { name: 'BLACK TIDE', wt: 'sine', fm: 1.6, lp: 1.6, f1: 150, q1: .2, a1: .5, sw: 90, g: .9 },
];
const NEW_CHOIRS = [
  { name: 'BASSO PROFUNDO', wt: 'pulse', rt: [1, 1.0015, .5], lo: 44, hi: 120, cut: 1.8, f1: 300, q1: .14, f2: 620, q2: .18, vib: .002, vr: 3.8, g: 1.3 },
  { name: 'NUNS', wt: 'pulse', rt: [1, 1.0025, 2], lo: 220, hi: 620, cut: 2.2, f1: 680, q1: .14, f2: 1200, q2: .16, vib: .005, vr: 5, g: 1 },
  { name: 'SIRENS', wt: 'sine', rt: [1, 1.5, 2.003], lo: 180, hi: 620, cut: 7, br: .04, vib: .012, vr: 3.4, glide: 7, g: .8 },
  { name: 'CRYPT CHOIR', wt: 'saw', rt: [1, .997, 1.5], lo: 70, hi: 220, cut: .8, f1: 420, q1: .2, f2: 900, q2: .24, vib: .002, vr: 4.2, g: 1.1 },
  { name: 'MOURNERS', wt: 'saw', rt: [1, 1.006, .75], lo: 110, hi: 330, cut: 1.1, f1: 540, q1: .16, f2: 1000, q2: .2, vib: .012, vr: 2.2, glide: 4, g: 1 },
  { name: 'THROAT CHOIR', wt: 'pulse', rt: [1, 1.001, 3], lo: 60, hi: 150, cut: 3, f1: 340, q1: .1, f2: 1800, q2: .06, vib: .001, vr: 4, g: 1.1 },
  { name: 'VOX HUMANA', wt: 'organ', rt: [1, 1.004, .5], lo: 90, hi: 420, cut: 1.6, f1: 500, q1: .2, f2: 1400, q2: .24, vib: .006, vr: 5.4, g: 1 },
  { name: 'HOLLOW VOICES', wt: 'odd', rt: [1, 1.003, 2], lo: 90, hi: 300, cut: 1.2, f1: 320, q1: .12, f2: 1100, q2: .12, vib: .004, vr: 3, br: .03, g: 1 },
  { name: 'GLASS CHOIR', wt: 'sine', rt: [1, 2.004, 3.01], lo: 320, hi: 1100, cut: 6, br: .02, vib: .003, vr: 4.5, g: .75 },
];
const NEW_SFX = [
  G('WHALE MOAN', 0, { life: [8, 15], w: 'sine', ss: 1, tw: 1.5, ts: .4, f: [90, 190], to: [.55, 1.8], arch: 1, r1: [400, .2], am: [.4, .3], gain: .5 }),
  G('ABYSS BREATH', 3, { life: [8, 14], n: 1, r1: [240, .25], am: [.22, .9], gain: .9 }),
  G('PRESSURE CRACK', 1, { life: [4, 9], imp: 1, rate: [.5, .9], jit: 1, r1: [560, .03], r2: [1500, .05], gain: 1.1 }),
  G('SUNKEN HULL', 1, { life: [9, 16], imp: 1, rate: [8, 18], rto: .4, jit: .5, r1: [95, .04], r2: [260, .05], gain: 1.7 }),
  G('RISING BUBBLES', 6, { life: [5, 10], w: 'sine', ss: 1, rate: [3, 6], jit: 1, chirp: [[260, 420], [700, 1300], 30], bdec: 26, gain: .24 }),
  G('DEEP CURRENT', 5, { life: [10, 18], n: 1, r1: [130, .4], r2: [340, .5], am: [.3, .5], gain: 1.1 }),
  G('LEVIATHAN', 4, { life: [7, 12], w: 'saw', ss: 1, tw: .5, ts: .7, f: [28, 42], to: [.75, 1.15], arch: 1, r1: [190, .24], am: [1.3, .4], gain: 1 }),
  G('BOWED CYMBAL', 4, { life: [8, 15], w: 'sine', ss: 1, tw: 2.71, ts: .6, n: .05, f: [640, 980], to: [1, 1.02], r1: [2400, .1], am: [6.2, .25], arch: 1, gain: .2 }),
  G('CHAIN DRAG', 2, { life: [4, 8], imp: 1, rate: [9, 14], jit: 1, r1: [1250, .018], r2: [2400, .02], gain: .9 }),
  G('COFFIN LID', 1, { life: [4, 7], imp: 1, rate: [18, 34], rto: .15, jit: .4, r1: [190, .05], r2: [560, .06], env: 'perc', gain: 1.5 }),
  G('IRON GATE', 1, { life: [4, 8], imp: 1, rate: [10, 18], rto: 1.8, jit: .3, r1: [640, .03], r2: [1720, .03], gain: 1.2 }),
  G('RUSTED SWING', 1, { life: [7, 12], w: 'sine', ss: 1, rate: [.55, .75], jit: .1, chirp: [[1300, 1450], [980, 1080], 3], bdec: 2.8, r1: [1400, .2], gain: .2 }),
  G('TOWER GROAN', 0, { life: [8, 14], imp: 1, rate: [22, 34], rto: .5, jit: .2, r1: [230, .03], r2: [690, .04], gain: 1.4 }),
  G('CABLE HUM', 5, { life: [8, 15], w: 'odd', ss: 1, f: [50, 62], to: [1, 1.1], r1: [410, .08], am: [.5, .4], gain: .5 }),
  G('GHOST CHOIR', 4, { life: [8, 14], w: 'saw', ss: 1, tw: 1.004, ts: 1, f: [160, 300], to: [.95, 1.05], r1: [640, .12], r2: [1100, .14], am: [4.6, .14], arch: 1, gain: .4 }),
  G('DISTANT SCREAM', 0, { life: [3, 6], w: 'saw', ss: 1, n: .08, f: [640, 980], to: [1.3, 1.6], arch: 1, r1: [1500, .16], r2: [2700, .2], am: [6.5, .2], gain: .16 }),
  G('LAMENT', 0, { life: [7, 12], w: 'pulse', ss: 1, f: [150, 260], to: [.75, .9], arch: 1, r1: [520, .15], r2: [1100, .16], am: [4.2, .18], gain: .45 }),
  G('LAST BREATH', 3, { life: [5, 8], n: 1, r1: [900, .4], r1to: 260, env: 'perc', gain: .7 }),
  G('DEATH RATTLE', 2, { life: [3, 6], imp: 1, rate: [13, 22], rto: .35, jit: .6, r1: [320, .12], r2: [900, .16], gain: 1.4 }),
  G('MOANING WALLS', 4, { life: [9, 15], w: 'sine', ss: .7, n: .3, f: [120, 210], to: [.8, 1.25], arch: 1, r1: [360, .12], am: [.9, .4], gain: .6 }),
  G('VOID WHISPER', 6, { life: [7, 13], n: 1, rate: [1.6, 2.6], jit: .8, syll: [320, 1200], bdec: 5, r1: [640, .2], r2: [1500, .3], gain: .8 }),
  G('SOLAR WIND', 5, { life: [10, 18], n: 1, r1: [2600, .7], rw: .5, am: [.17, .7], gain: .24 }),
  G('BLACK HOLE', 3, { life: [8, 14], w: 'sine', ss: 1, f: [300, 460], to: [.08, .12], n: .05, r1: [160, .3], gain: .7 }),
  G('STATIC STORM', 5, { life: [6, 12], n: .8, r1: [3100, .6], am: [11, .85], gain: .2 }),
  G('DYING STAR', 4, { life: [10, 16], w: 'sine', ss: 1, tw: 1.004, ts: 1, f: [55, 80], to: [.5, .7], am: [.23, .5], r1: [180, .3], gain: .7 }),
  G('EARTHQUAKE', 5, { life: [6, 11], n: 1, r1: [58, .25], am: [8, .55], env: 'perc', gain: 2.4 }),
  G('AVALANCHE', 5, { life: [7, 12], n: 1, r1: [320, .8], r1to: 120, env: 'perc', gain: 1 }),
  G('ROCK FALL', 2, { life: [4, 8], imp: 1, rate: [5, 12], rto: .3, jit: 1.2, r1: [170, .1], r2: [520, .14], gain: 1.6 }),
  G('CAVE WIND', 5, { life: [9, 16], n: 1, r1: [340, .08], rw: .3, am: [.3, .5], gain: 1 }),
  G('GLACIER SHIFT', 1, { life: [8, 14], imp: 1, rate: [3, 7], rto: .2, jit: .6, r1: [120, .03], r2: [410, .04], gain: 1.8 }),
  G('FAR FIRE', 5, { life: [8, 14], n: 1, r1: [190, .4], am: [1.7, .5], rate: [6, 11], jit: 1, gain: .8 }),
  G('TOMB SEAL', 1, { life: [4, 7], n: .7, r1: [120, .3], r2: [420, .4], am: [14, .5], env: 'perc', gain: 1.2 }),
  G('MINE SHAFT', 5, { life: [9, 15], w: 'odd', ss: .4, n: .7, f: [42, 56], r1: [180, .1], r2: [460, .12], am: [.7, .5], gain: .9 }),
  G('FOG HORN', 4, { life: [4, 7], w: 'brass', ss: 1, tw: 1.5, ts: .4, f: [68, 92], r1: [260, .22], env: 'perc', gain: .7 }),
  G('FUNERAL BELL', 2, { life: [9, 15], w: 'sine', ss: 1, tw: 2.76, ts: .4, f: [150, 230], rate: [.18, .24], jit: .05, bdec: 1.2, gain: .34 }),
  G('WAR DRUMS', 2, { life: [8, 14], w: 'sine', ss: 1, n: .1, rate: [.55, .8], jit: .08, dbl: 1, chirp: [[105, 120], [52, 60], 16], bdec: 6, gain: .7 }),
  G('ASH FALL', 5, { life: [8, 14], n: 1, rate: [8, 15], jit: 1.2, bdec: 40, r1: [1400, .5], gain: .4 }),
  G('BLOOD PULSE', 2, { life: [8, 14], w: 'sine', ss: 1, rate: [.7, .85], jit: .03, chirp: [[62, 70], [38, 42], 10], bdec: 5, gain: .9 }),
  G('BONE RATTLE', 2, { life: [3, 6], imp: 1, rate: [14, 24], jit: .9, r1: [1900, .1], r2: [3300, .16], am: [3, .7], gain: .7 }),
];
const NEW_ODD = [
  G('THREE KNOCKS', 2, { life: [2.5, 4], imp: 1, rate: [1.6, 2.2], jit: 0.15, r1: [180, 0.06], r2: [520, 0.08], gain: 1.3 }),
  G('GRAVE WIND', 5, { life: [9, 15], n: 1, r1: [520, 0.1], rw: 0.5, am: [0.2, 0.6], gain: 0.8 }),
  G('PROCESSION BELLS', 2, { life: [5, 9], w: 'sine', ss: 1, tw: 2.4, ts: 0.4, f: [600, 900], rate: [2, 3], jit: 0.6, bdec: 3, gain: 0.12 }),
  G('RAVEN', 6, { life: [4, 7], w: 'saw', ss: 1, rate: [1.2, 1.7], jit: .5, chirp: [[420, 500], [240, 300], 11], bdec: 9, r1: [1050, .2], am: [46, .7], gain: .7 }),
  G('DISTANT HYMN', 4, { life: [9, 14], w: 'saw', ss: 1, tw: 1.5, ts: 0.8, f: [130, 180], arch: 1, r1: [600, 0.12], r2: [1050, 0.14], am: [4.3, 0.12], gain: 0.4 }),
  G('SOMETHING BREATHES', 3, { life: [9, 14], n: 1, r1: [300, 0.3], am: [0.28, 0.95], gain: 0.9 }),
  G('SCRAPING CLAWS', 1, { life: [3, 6], imp: 1, rate: [16, 26], jit: 1, r1: [2600, 0.04], r2: [3800, 0.05], am: [2.5, 0.8], gain: 0.5 }),
  G('DEEP GONG', 4, { life: [8, 13], w: 'sine', ss: 1, tw: 1.47, ts: 0.7, f: [55, 75], rate: [0.12, 0.16], jit: 0.05, bdec: 0.6, gain: 0.6 }),
  G('DYING SIGH', 0, { life: [3, 5], n: 0.8, w: 'saw', ss: 0.4, f: [180, 240], to: [0.6, 0.7], arch: 1, r1: [800, 0.3], gain: 0.4 }),
  G('SUNKEN ORGAN', 4, { life: [10, 15], w: 'organ', ss: 1, tw: 1.5, ts: 0.6, f: [45, 60], arch: 1, r1: [380, 0.25], am: [0.18, 0.3], gain: 0.5 }),
  G('CLOCK TICK', 2, { life: [8, 14], imp: 1, rate: [1, 1], jit: 0, r1: [2100, .05], gain: .7 }),
  G('TEMPLE HORN', 4, { life: [6, 10], w: 'brass', ss: 1, tw: 2, ts: 0.25, f: [90, 120], to: [0.98, 1.02], arch: 1, r1: [500, 0.2], gain: 0.45 }),
  G('STONE FOOTSTEPS', 1, { life: [5, 9], imp: 1, rate: [1.1, 1.5], jit: 0.1, r1: [140, 0.05], r2: [380, 0.06], gain: 1.2 }),
  G('LAUGHING DOLL', 6, { life: [3, 6], w: 'sine', ss: 1, rate: [5, 6.5], jit: .1, chirp: [[1000, 1300], [800, 950], 16], bdec: 12, gain: .16 }),
  G('BLACK WATER', 5, { life: [10, 16], n: 1, r1: [180, 0.5], r2: [500, 0.6], am: [0.4, 0.5], gain: 1.0 }),
];
const NEW_DARK2 = [
  G('DUNGCHEN', 4, { life: [7, 12], w: 'brass', ss: 1, tw: 2.0, ts: .3, f: [45, 62], to: [.98, 1.02], arch: 1, r1: [300, .2], am: [.3, .2], gain: .8 }),
  G('KANGLING', 0, { life: [3, 6], w: 'saw', ss: 1, f: [300, 420], to: [.85, 1.1], arch: 1, r1: [900, .15], am: [6, .3], gain: .3 }),
  G('SINGING SAW', 4, { life: [6, 11], w: 'sine', ss: 1, f: [300, 600], to: [.7, 1.4], arch: 1, am: [5.5, .08], gain: .3 }),
  G('WATERPHONE', 4, { life: [5, 9], w: 'sine', ss: 1, tw: 1.41, ts: .5, f: [500, 1100], to: [.6, 1.6], am: [7, .3], r1: [1800, .2], gain: .2 }),
  G('TAM-TAM SWELL', 5, { life: [8, 14], n: .6, r1: [180, .3], r2: [700, .4], am: [.25, .6], arch: 1, gain: .9 }),
  G('CATACOMB DRIP', 6, { life: [8, 14], w: 'sine', ss: 1, rate: [.8, 1.6], jit: 1, chirp: [[900, 1400], [500, 800], 40], bdec: 30, gain: .25 }),
  G('CRYPT DOOR', 1, { life: [4, 7], imp: 1, rate: [14, 22], rto: .25, jit: .3, r1: [260, .05], r2: [780, .06], gain: 1.2 }),
  G('HOLLOW KNOCK', 2, { life: [4, 8], imp: 1, rate: [1.5, 2.5], jit: .3, r1: [140, .06], r2: [420, .08], gain: 1.4 }),
  G('SPIRIT BOX', 5, { life: [5, 9], n: .7, r1: [1300, .4], am: [11, .9], w: 'sine', ss: .5, rate: [3, 5], jit: 1, fld: [300, 900], bdec: 6, gain: .2 }),
  G('EVP VOICES', 6, { life: [6, 11], n: 1, rate: [1.2, 2], jit: .8, syll: [250, 900], bdec: 4, r1: [500, .25], r2: [1300, .3], gain: .7 }),
  G('UNDERWORLD GATE', 1, { life: [6, 10], imp: 1, rate: [5, 9], rto: .2, jit: .4, r1: [70, .04], r2: [210, .05], gain: 2 }),
  G('ABYSSAL PULSE', 2, { life: [10, 16], w: 'sine', ss: 1, rate: [.35, .5], jit: .05, chirp: [[48, 56], [30, 36], 8], bdec: 2, gain: 1 }),
  G('DEAD STAR HUM', 5, { life: [10, 18], w: 'sine', ss: 1, tw: 1.5, ts: .5, f: [38, 55], am: [.15, .5], gain: .9 }),
  G('OBSIDIAN RAIN', 5, { life: [8, 14], n: 1, rate: [12, 20], jit: 1.2, bdec: 50, r1: [2200, .5], gain: .35 }),
  G('WRAITH', 0, { life: [4, 7], w: 'saw', ss: 1, n: .25, f: [400, 700], to: [.6, .8], arch: 1, r1: [1100, .2], r2: [2300, .25], am: [5, .3], gain: .14 }),
  G('LOW MOAN', 0, { life: [6, 10], w: 'pulse', ss: 1, f: [70, 110], to: [.8, .95], arch: 1, r1: [300, .15], am: [3, .2], gain: .6 }),
  G('NIGHT TIDE', 5, { life: [12, 18], n: 1, r1: [400, .6], am: [.12, .8], gain: .7 }),
  G('IRON LUNG', 5, { life: [10, 16], n: .8, r1: [600, .3], am: [.25, .95], w: 'odd', ss: .3, f: [40, 48], gain: .6 }),
  G('SHIP GRAVEYARD', 1, { life: [10, 16], imp: 1, rate: [3, 6], rto: .3, jit: 1, r1: [110, .03], r2: [330, .04], gain: 1.6 }),
  G('ROLLING DARK', 5, { life: [6, 10], n: 1, r1: [90, .3], am: [2, .6], env: 'perc', gain: 1.8 }),
  G('HOWLING VOID', 5, { life: [8, 14], n: 1, r1: [700, .06], r1to: 1100, rw: .4, am: [.3, .6], gain: .7 }),
  G('CURSED BELL', 2, { life: [9, 15], w: 'sine', ss: 1, tw: 2.4, ts: .5, f: [180, 260], rate: [.25, .35], jit: .05, bdec: 1, am: [3.7, .3], gain: .35 }),
  G('ROLMO CYMBALS', 2, { life: [6, 10], w: 'sine', ss: 1, tw: 2.71, ts: .7, n: .3, f: [800, 1100], rate: [.3, .5], jit: .2, bdec: 1.5, gain: .2 }),
  G('DRAGGED STONE', 5, { life: [4, 7], n: 1, r1: [150, .2], r2: [500, .3], am: [7, .6], gain: 1.2 }),
  G('SOUL BELL', 6, { life: [6, 10], w: 'sine', ss: 1, tw: 2.01, ts: .3, f: [900, 1300], am: [8, .1], rate: [.5, .7], jit: .1, bdec: 1.4, gain: .14 }),
  G('BLACK MASS', 4, { life: [9, 14], w: 'saw', ss: 1, tw: 1.5, ts: .7, f: [65, 95], to: [.98, 1.0], arch: 1, r1: [420, .12], r2: [800, .14], am: [4.4, .12], gain: .5 }),
  G('THE DEEP CALLS', 0, { life: [8, 13], w: 'sine', ss: 1, f: [60, 90], to: [.5, 1.6], arch: 1, am: [.5, .4], r1: [240, .2], gain: .7 }),
];
BELLS.push(...NEW_BELLS); DRONES.push(...NEW_DRONES); CHOIRS.push(...NEW_CHOIRS);
const OLD_B = ['GEAR GRIND','WOLF HOWL','BRANCH SNAP','WIND CHIMES','RIVER FLOW','OAR CREAK','RITUAL DRUM','SISTRUM','SAND WIND','TEMPLE GATE','SUB DROP','TAPE REWIND','DISTANT TRAIN','DEAD RADIO','SONAR PING'];
const OLD_ODD = ['CROWS','OWL','MOURNING HORN','DROWNED BELL','BONE FLUTE','BATS','SONAR CLICKS','GEIGER','BROKEN MUSIC BOX','AIR SIREN','MONITOR BEEP','CHILD HUMMING'];
const OLD_A = SFX.map(s=>s.name).filter(n=>!OLD_B.includes(n)&&!OLD_ODD.includes(n));
SFX.push(...NEW_SFX, ...NEW_ODD, ...NEW_DARK2);
const names = a => a.map(s => s.name);
const BANKS = [OLD_A, OLD_B.concat(names(NEW_SFX).slice(0, 12)), names(NEW_SFX).slice(12), names(NEW_DARK2), OLD_ODD.concat(names(NEW_ODD))];
const ODD_NAMES = [];   /* since 0.99.9 every effect may come by itself */
const MORE = {
  HADAL: [['WHALE MOAN','LEVIATHAN','ABYSS BREATH'],'DEEP BOWL','BASSO PROFUNDO'],
  CATHEDRAL: [['GHOST CHOIR','TOWER GROAN','FUNERAL BELL'],'CARILLON','VOX HUMANA'],
  BARDO: [['LAST BREATH','VOID WHISPER','BOWED CYMBAL'],'CRYSTAL BOWL','THROAT CHOIR'],
  MICTLAN: [['BLOOD PULSE','BONE RATTLE','WAR DRUMS'],'STONE CHIME','MOURNERS'],
  NIFLHEIM: [['GLACIER SHIFT','AVALANCHE','CAVE WIND'],'FROZEN BELL','GLASS CHOIR'],
  YOMI: [['LAMENT','TOMB SEAL','ASH FALL'],'TEMPLE BELL','HOLLOW VOICES'],
  PATALA: [['BOWED CYMBAL','EARTHQUAKE','RUSTED SWING'],'GENDER','MOURNERS'],
  SIGNAL: [['STATIC STORM','CABLE HUM','MINE SHAFT'],'SHIP BELL','HOLLOW VOICES'],
  HELIOPAUSE: [['SOLAR WIND','BLACK HOLE','DYING STAR'],'BRONZE RING','SIRENS'],
  DUAT: [['COFFIN LID','ASH FALL','TOMB SEAL'],'BRONZE RING','CRYPT CHOIR'],
  TUONELA: [['DEEP CURRENT','RISING BUBBLES','LAST BREATH'],'WATER BOWL','SIRENS'],
  KURNUGI: [['ROCK FALL','DEATH RATTLE','WAR DRUMS'],'BURIED BELL','CRYPT CHOIR'],
  'WARD 9': [['MOANING WALLS','DISTANT SCREAM','LAST BREATH'],'CRACKED BELL','NUNS'],
  FOUNDRY: [['CHAIN DRAG','IRON GATE','FAR FIRE'],'SAW BLADE','THROAT CHOIR'],
  HOLLOW: [['RUSTED SWING','FAR FIRE','CAVE WIND'],'SLIT DRUM','MOURNERS'],
};
const LEVEL = (() => { try { return JSON.parse(require('fs').readFileSync(process.argv[4] || 'tools_levels.json', 'utf8')); } catch (e) { return {}; } })();
BELLS.forEach(b => { const t = LEVEL['b:' + b.name]; if (t) b.g = b.g.map(x => x * t); });
DRONES.forEach(d => { const t = LEVEL['d:' + d.name]; if (t) d.g *= t; });
CHOIRS.forEach(c => { const t = LEVEL['c:' + c.name]; if (t) c.g *= t; });
SFX.forEach(s => { const t = LEVEL['s:' + s.name]; if (t) { if (s.custom) s.trim = (s.trim || 1) * t; else s.gain *= t; } });
SEAS.forEach(q => { const m = MORE[q.name]; if (!m) throw new Error('no more for ' + q.name);
  q.sfxI = q.sfxI.concat(m[0].map(n => SFX.findIndex(s => s.name === n)));
  q.bellI = q.bellI.slice(0, 2).concat(BELLS.findIndex(b => b.name === m[1]));
  q.choirI = q.choirI.slice(0, 2).concat(CHOIRS.findIndex(c => c.name === m[2]));
  if (q.bellI.length < 3 || q.choirI.includes(-1) || q.bellI.includes(-1) || q.sfxI.includes(-1)) throw new Error('bad ' + q.name); });
