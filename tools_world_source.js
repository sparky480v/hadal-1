/* HADAL-1: the worlds. Everything here is a table, so on the FM-1 it costs flash, not pool memory. No samples. */
const FAMILIES = ['UNDERTONE', 'PHRYGIAN', 'LOCRIAN', 'TRITONE', 'SHROUD', 'PELOG', 'IN', 'HIJAZ', 'OCTATONIC', 'DORIAN', 'PENTA'];
const UNDER = [16, 17, 18, 19, 20, 21, 22, 24, 26, 27, 28, 30, 32];
const ET = {
  PHRYGIAN: [0, 1, 3, 5, 7, 8, 10], LOCRIAN: [0, 1, 3, 5, 6, 8, 10], TRITONE: [0, 1, 6, 7], SHROUD: [0, 1, 2, 7, 8],
  PELOG: [0, 1.2, 2.7, 5.4, 6.7, 7.85, 9.5], IN: [0, 1, 5, 7, 8], HIJAZ: [0, 1, 4, 5, 7, 8, 10],
  OCTATONIC: [0, 1, 3, 4, 6, 7, 9, 10], DORIAN: [0, 2, 3, 5, 7, 9, 10], PENTA: [0, 2, 3, 5, 7],
};

const TN = 2048;
function table(fn) {
  const t = new Float32Array(TN + 1);
  let peak = 0;
  for (let i = 0; i < TN; i++) { t[i] = fn(i / TN * Math.PI * 2); peak = Math.max(peak, Math.abs(t[i])); }
  for (let i = 0; i < TN; i++) t[i] /= peak;
  t[TN] = t[0];
  return t;
}
const sumH = (h) => (x) => h.reduce((a, [n, g]) => a + g * Math.sin(x * n), 0);
const harm = (n, fn) => Array.from({ length: n }, (_, i) => [i + 1, fn(i + 1)]);
const WT = {
  sine: table(Math.sin),
  saw: table(sumH(harm(44, n => 1 / n))),
  pulse: table(sumH(harm(44, n => 1 / Math.pow(n, .75)))),
  odd: table(sumH(harm(44, n => n % 2 ? 1 / n : .06 / n))),
  organ: table(sumH([[1, 1], [2, .62], [3, .3], [4, .34], [6, .16], [8, .12], [10, .05], [12, .05]])),
  brass: table(sumH(harm(30, n => Math.exp(-Math.pow(n - 5, 2) / 26) + .7 / n))),
};
const rd = (t, p) => { p -= Math.floor(p); const x = p * TN, i = x | 0; return t[i] + (t[i + 1] - t[i]) * (x - i); };

const BELLS = [
  { name: 'CHURCH', r: [.5, 1, 1.2, 1.5, 2, 2.51, 3.01], d: [24, 18, 12, 9, 6, 3.5, 2.4], g: [.55, .8, .62, .34, .46, .18, .1], att: 0, lo: 95, hi: 780 },
  { name: 'BONSHO', r: [.5, .503, 1, 1.006, 1.59, 2.13, 2.97], d: [38, 34, 26, 22, 12, 7, 4], g: [.9, .8, .72, .55, .3, .2, .1], att: .02, lo: 60, hi: 240 },
  { name: 'BOWL', r: [1, 1.006, 2.71, 2.722, 5.15, 5.18, 8.5], d: [22, 20, 12, 11, 6, 5, 3], g: [.8, .7, .5, .4, .2, .15, .06], att: 1.4, lo: 110, hi: 520 },
  { name: 'TAM-TAM', r: [1, 1.37, 1.81, 2.19, 2.83, 3.57, 4.41], d: [26, 22, 20, 16, 13, 10, 7], g: [.6, .42, .42, .32, .3, .22, .18], att: 1.8, lo: 80, hi: 300 },
  { name: 'GONG AGENG', r: [1, 1.018, 2, 2.03, 3.01, 4.07, 5.5], d: [30, 28, 15, 13, 8, 5, 3], g: [.95, .85, .4, .34, .16, .1, .05], att: .05, lo: 55, hi: 180 },
  { name: 'IRON BAR', r: [1, 1.003, 2.756, 2.764, 5.404, 8.933, 13.34], d: [9, 8, 5, 4, 2.4, 1.4, .8], g: [.9, .8, .5, .4, .25, .12, .06], att: 0, lo: 130, hi: 900 },
  { name: 'GLASS', r: [1, 2.32, 4.25, 6.63, 9.38, 1.002, 2.33], d: [12, 9, 6, 4, 2, 10, 8], g: [.8, .4, .24, .12, .05, .5, .2], att: .01, lo: 240, hi: 1300 },
  { name: 'WATERPHONE', r: [1, 1.41, 2.12, 3.31, 4.07, 5.6, 7.21], d: [9, 8, 7, 6, 5, 4, 3], g: [.7, .55, .5, .36, .3, .2, .12], att: .5, lo: 170, hi: 900, bend: 3.2 },
  { name: 'DEAD PIANO', r: [1, 2.003, 3.01, 4.03, 5.06, 6.1, 7.16], d: [10, 8, 6, 5, 3, 2, 1.5], g: [.9, .5, .42, .25, .18, .1, .06], att: .001, lo: 70, hi: 620 },
  { name: 'ANVIL', r: [1, 2.02, 2.76, 3.9, 5.4, 6.8, 8.9], d: [5, 4, 3.5, 2.6, 1.8, 1.2, .8], g: [.9, .6, .6, .4, .3, .18, .1], att: 0, lo: 180, hi: 760 },
  { name: 'KANTELE', r: [1, 2.001, 3.003, 4.006, 5.01, 6.02, 7.03], d: [7, 5, 4, 3, 2, 1.5, 1], g: [.9, .55, .35, .22, .14, .08, .05], att: 0, lo: 130, hi: 660 },
  { name: 'RUSTED PIPE', r: [1, 2.76, 5.4, 8.9, 1.004, 2.77, 5.42], d: [6, 4, 2.5, 1.4, 5, 3, 2], g: [.9, .4, .2, .1, .7, .3, .1], att: 0, lo: 90, hi: 420 },
  { name: 'OIL DRUM', r: [1, 1.59, 2.14, 2.3, 2.65, 2.92, 3.16], d: [3, 2.6, 2.2, 2, 1.8, 1.6, 1.4], g: [.95, .6, .5, .4, .3, .25, .2], att: .002, lo: 50, hi: 170 },
];

const DRONES = [
  { name: 'ABYSS FM', wt: 'sine', fm: 1, lp: 1, g: 1 },
  { name: 'ORGAN 32 FT', wt: 'organ', fm: 0, lp: 3.2, g: .85 },
  { name: 'TEMPLE HORN', wt: 'brass', fm: .05, lp: 2.5, f1: 420, q1: .3, a1: .6, g: .9 },
  { name: 'THROAT', wt: 'pulse', fm: 0, lp: 7, f1: 340, q1: .28, a1: .5, f2: 1500, q2: .04, a2: .42, sw: 1000, g: .75 },
  { name: 'BOWED IRON', wt: 'saw', fm: .08, lp: 2.2, f1: 720, q1: .25, a1: .4, g: .78 },
  { name: 'TURBINE', wt: 'odd', fm: .22, lp: 3.6, g: .72 },
  { name: 'POLAR WIND', noise: 1, f1: 7, q1: .06, a1: .32, f2: 17, q2: .08, a2: .2, rel: 1, g: 1.1 },
  { name: 'EARTH PIPE', wt: 'pulse', fm: 0, lp: 6, f1: 240, q1: .12, a1: .75, sw: 520, g: .68 },
  { name: 'DUDUK', wt: 'pulse', fm: 0, lp: 5, f1: 520, q1: .16, a1: .6, f2: 1250, q2: .2, a2: .25, g: .8 },
  { name: 'FURNACE', noise: 1, f1: 3, q1: .12, a1: .5, f2: 9, q2: .1, a2: .3, rel: 1, g: 1 },
  { name: 'MAINS HUM', wt: 'odd', fm: 0, lp: 5, g: .6 },
];

const CHOIRS = [
  { name: 'REQUIEM', wt: 'saw', rt: [1, 1.003, .4985], lo: 62, hi: 262, cut: 1, vib: 0, g: 1 },
  { name: 'MONKS', wt: 'pulse', rt: [1, 1.002, .5], lo: 58, hi: 165, cut: 2.2, f1: 390, q1: .14, f2: 760, q2: .16, vib: .003, vr: 4.2, g: 1.35 },
  { name: 'WRAITHS', wt: 'sine', rt: [1, 1.006, 2.003], lo: 260, hi: 880, cut: 6, br: .08, vib: .006, vr: 5.1, g: .78 },
  { name: 'CELLOS', wt: 'saw', rt: [1, 1.0018, .9982], lo: 62, hi: 280, cut: 1.5, vib: .004, vr: 4.6, g: .75 },
  { name: 'ONDES', wt: 'sine', rt: [1, 2.001, 1.002], lo: 200, hi: 1040, cut: 8, vib: .012, vr: 5.6, glide: 8, g: .85 },
  { name: 'TAPE CHOIR', wt: 'pulse', rt: [1, 1.004, .501], lo: 98, hi: 392, cut: 1.7, f1: 620, q1: .2, f2: 1150, q2: .24, vib: .0065, vr: .9, g: 1.1 },
  { name: 'NIGHT SYNTH', wt: 'odd', rt: [1, 1.004, .5], lo: 55, hi: 247, cut: 1.2, res: 1, vib: 0, g: .92 },
  { name: 'KEENING', wt: 'sine', rt: [1, 1.004, 2.002], lo: 300, hi: 900, cut: 6, br: .05, vib: .02, vr: 3, glide: 6, g: .8 },
  { name: 'MONOTONE', wt: 'saw', rt: [1, .999, .5], lo: 50, hi: 130, cut: 1, vib: 0, g: 1 },
];

const C = (name, v) => ({ name, v, custom: 1 });
const G = (name, v, o) => ({ name, v, ...o });
const SFX = [
  C('SINKING CALL', 0), C('STRAINED METAL', 1), C('SONAR CLICKS', 2), C('REVERSE GONG', 3), C('RING SHADOW', 4), C('ENDLESS FALL', 3),
  G('ICE CRACK', 1, { life: [7, 16], w: 'sine', ss: 1, n: .03, rate: [.3, .55], jit: 1, chirp: [[6000, 9000], [120, 300], 9], bdec: 3.2, gain: .2 }),
  G('ICE GROAN', 0, { life: [8, 18], w: 'saw', ss: 1, f: [38, 85], to: [.8, 1.3], r1: [220, .12], r2: [610, .1], am: [.7, .5], gain: .85 }),
  G('HULL GROAN', 1, { life: [7, 15], imp: 1, rate: [16, 40], rto: .5, jit: .3, r1: [140, .05], r2: [420, .06], gain: 1.7 }),
  G('DOOR CREAK', 1, { life: [3, 7], imp: 1, rate: [7, 14], rto: 3.4, jit: .25, r1: [820, .04], r2: [2300, .05], gain: 1.2 }),
  G('CHAINS', 2, { life: [3, 7], imp: 1, rate: [6, 9], rto: .25, jit: 1, r1: [1900, .012], r2: [3350, .012], gain: 1.1 }),
  G('WHISPERS', 6, { life: [6, 12], n: 1, rate: [3, 4.5], jit: .7, syll: [700, 2600], bdec: 8, r1: [1200, .18], r2: [3100, .25], gain: .65 }),
  G('CAVE DRIP', 2, { life: [8, 16], w: 'sine', ss: 1, rate: [.45, .8], jit: 1, chirp: [[380, 520], [1100, 1500], 40], bdec: 30, gain: .36 }),
  G('CROWS', 6, { life: [3, 7], w: 'saw', ss: 1, rate: [.9, 1.3], jit: .6, chirp: [[560, 680], [320, 380], 9], bdec: 7, r1: [1350, .2], r2: [2500, .25], am: [38, .6], gain: .85 }),
  G('BONE FLUTE', 0, { life: [4, 7], w: 'saw', ss: 1, n: 0.15, f: [520, 700], to: [0.9, 1.05], arch: 1, r1: [1300, 0.12], am: [5, 0.15], gain: 0.12 }),
  G('THUNDER', 5, { life: [8, 16], n: 1, r1: [90, .35], am: [3.1, .7], env: 'perc', gain: 1.4 }),
  G('DEATH WHISTLE', 4, { life: [3, 6], w: 'sine', ss: 1, tw: 1.055, ts: 1, n: .7, f: [1050, 1500], to: [1.3, 1.7], arch: 1, r1: [2100, .22], am: [73, .7], gain: .34 }),
  G('BULLROARER', 3, { life: [6, 13], n: 1, r1: [245, .05], rw: .35, am: [3.4, .85], gain: 1.2 }),
  G('HEARTBEAT', 2, { life: [8, 16], w: 'sine', ss: 1, rate: [.95, 1.1], jit: .05, dbl: 1, chirp: [[80, 90], [40, 46], 14], bdec: 7, gain: .9 }),
  G('BROKEN MUSIC BOX', 6, { life: [7, 15], w: 'sine', ss: 1, tw: 3.01, ts: .25, rate: [1.5, 2], rto: .3, jit: .35, fld: [900, 2700], bdec: 4.5, gain: .2 }),
  G('NUMBER STATION', 5, { life: [6, 13], w: 'sine', ss: 1, n: .02, f: [780, 1040], rate: [2, 2.6], jit: .1, fld: [700, 1100], bdec: 11, gain: .22 }),
  G('SHORTWAVE', 5, { life: [6, 14], w: 'sine', ss: .35, n: .9, f: [400, 2800], to: [.4, 2.4], r1: [1700, .3], am: [7, .7], gain: .32 }),
  G('GEIGER', 2, { life: [5, 11], imp: 1, rate: [4, 7], rto: 2.4, jit: 1.8, r1: [3300, .18], gain: .9 }),
  G('AIR SIREN', 3, { life: [14, 24], w: 'sine', ss: 1, tw: 1.008, ts: 1, f: [150, 210], to: [3, 3.8], arch: 1, gain: .14 }),
  G('PULSAR', 2, { life: [8, 18], imp: 1, rate: [.8, 11], jit: 0, r1: [950, .08], gain: 1 }),
  G('WHISTLER', 0, { life: [7, 15], w: 'sine', ss: 1, rate: [.4, .6], jit: .8, chirp: [[3400, 5200], [350, 500], 1.7], bdec: 1.1, gain: .14 }),
  G('DISTANT BRAAAM', 4, { life: [4, 8], w: 'saw', ss: 1, tw: .5, ts: .8, f: [38, 58], r1: [1400, .6], r1to: 240, env: 'perc', gain: .95 }),
  G('FLATLINE', 2, { life: [6, 10], w: 'sine', ss: 1, f: [980, 1010], gain: .14 }),
  G('MONITOR BEEP', 2, { life: [6, 11], w: 'sine', ss: 1, f: [880, 960], rate: [.9, 1.2], jit: .02, bdec: 14, gain: .2 }),
  G('FLUORESCENT', 5, { life: [7, 14], w: 'odd', ss: 1, n: .06, f: [118, 122], r1: [2400, .5], am: [120, .25], gain: .25 }),
  G('CHILD HUMMING', 6, { life: [5, 9], w: 'sine', ss: 1, f: [400, 520], to: [0.94, 1.06], arch: 1, am: [5.2, 0.06], r1: [900, 0.3], gain: 0.1 }),
  G('ANVIL STRIKE', 1, { life: [5, 9], imp: 1, rate: [.7, 1.2], jit: .3, r1: [1450, .015], r2: [3900, .02], gain: 1 }),
  G('FURNACE ROAR', 5, { life: [7, 14], n: 1, r1: [120, .3], am: [1.3, .5], gain: 1 }),
  G('STEAM HISS', 3, { life: [2, 5], n: 1, r1: [5200, .6], env: 'perc', gain: .5 }),
  G('PISTON', 2, { life: [8, 14], w: 'sine', ss: 1, n: .08, rate: [1.3, 1.7], jit: .02, chirp: [[85, 95], [46, 50], 18], bdec: 9, gain: .7 }),
  G('GEAR GRIND', 1, { life: [5, 10], w: 'saw', ss: 1, f: [60, 95], to: [.9, 1.1], r1: [900, .1], r2: [1800, .12], am: [23, .7], gain: .6 }),
  G('OWL', 6, { life: [7, 13], w: 'sine', ss: 1, rate: [.35, .55], jit: .4, chirp: [[400, 440], [350, 370], 3], bdec: 3, gain: .25 }),
  G('WOLF HOWL', 0, { life: [5, 9], w: 'sine', ss: 1, n: .02, f: [260, 340], to: [1.5, 1.9], arch: 1, r1: [900, .3], am: [5.5, .12], gain: .35 }),
  G('BRANCH SNAP', 1, { life: [5, 10], imp: 1, rate: [.6, 1], jit: 1, r1: [2600, .3], gain: 1 }),
  G('WIND CHIMES', 6, { life: [6, 12], w: 'sine', ss: 1, tw: 2.76, ts: .3, rate: [2.4, 3.6], jit: 1, fld: [1400, 3200], bdec: 3.2, gain: .15 }),
  G('DROWNED BELL', 2, { life: [9, 14], w: 'sine', ss: 1, tw: 2.76, ts: 0.4, f: [110, 160], rate: [0.15, 0.22], jit: 0.05, bdec: 1.0, am: [0.7, 0.4], gain: 0.34 }),
  G('BATS', 6, { life: [4, 9], w: 'sine', ss: 1, rate: [7, 11], jit: 1, chirp: [[7000, 9000], [3500, 4100], 60], bdec: 35, gain: .1 }),
  G('RIVER FLOW', 0, { life: [9, 16], n: 1, r1: [420, .5], r2: [1200, .6], am: [.6, .4], gain: .5 }),
  G('MOURNING HORN', 4, { life: [7, 12], w: 'brass', ss: 1, f: [55, 75], to: [0.96, 1], arch: 1, r1: [280, 0.2], am: [0.25, 0.2], gain: 0.7 }),
  G('OAR CREAK', 1, { life: [2, 4], imp: 1, rate: [9, 14], rto: 2, jit: .25, r1: [520, .05], r2: [1400, .06], env: 'perc', gain: 1.2 }),
  G('RITUAL DRUM', 2, { life: [8, 14], w: 'sine', ss: 1, n: .12, rate: [1.1, 1.5], jit: .05, chirp: [[140, 160], [70, 76], 22], bdec: 8, gain: .8 }),
  G('SISTRUM', 6, { life: [4, 9], n: 1, rate: [4, 7], jit: .3, bdec: 34, r1: [5200, .4], r2: [7000, .4], gain: .5 }),
  G('SAND WIND', 5, { life: [8, 14], n: 1, r1: [1800, .7], rw: .3, am: [.4, .6], gain: .4 }),
  G('TEMPLE GATE', 4, { life: [6, 10], w: 'saw', ss: .5, n: .9, f: [32, 44], r1: [95, .3], r2: [260, .25], am: [7, .5], gain: 1 }),
  G('SUB DROP', 3, { life: [4, 6], w: 'sine', ss: 1, rate: [.22, .26], jit: 0, chirp: [[85, 95], [27, 30], 1.5], bdec: .8, gain: .9 }),
  G('TAPE REWIND', 5, { life: [2, 5], w: 'saw', ss: 1, f: [260, 500], to: [3, 5], r1: [1600, .3], am: [31, .5], gain: .5 }),
  G('DISTANT TRAIN', 0, { life: [9, 15], w: 'odd', ss: 1, tw: 1.19, ts: 1, f: [330, 340], to: [.96, .98], r1: [800, .4], gain: .3 }),
  G('DEAD RADIO', 5, { life: [5, 10], n: .9, r1: [2200, .5], am: [2.5, .9], gain: .4 }),
  G('SONAR PING', 2, { life: [6, 10], w: 'sine', ss: 1, f: [1250, 1350], rate: [.22, .28], jit: 0, bdec: 2.2, gain: .25 }),
];
const sfxIdx = (...n) => n.map(x => SFX.findIndex(s => s.name === x));
const nm = (list, names) => names.map(x => list.findIndex(s => s.name === x));

/* w: how strongly each stratum is favoured: ABYSS PROCESSION REQUIEM SWARM WRECK VOID */
const SEAS = [
  { name: 'HADAL', from: 'The deep ocean', fam: ['UNDERTONE', 'PHRYGIAN'], bells: ['CHURCH', 'IRON BAR'], drones: ['ABYSS FM', 'BOWED IRON'], choirs: ['REQUIEM', 'NIGHT SYNTH'], sfx: ['SINKING CALL', 'STRAINED METAL', 'SONAR CLICKS', 'REVERSE GONG', 'RING SHADOW', 'HULL GROAN', 'ENDLESS FALL'], pal: [0, 1], w: [1.3, 1, 1, 1, 1, 1] },
  { name: 'CATHEDRAL', from: 'A drowned church', fam: ['DORIAN', 'PHRYGIAN'], bells: ['CHURCH', 'DEAD PIANO'], drones: ['ORGAN 32 FT', 'BOWED IRON'], choirs: ['MONKS', 'TAPE CHOIR'], sfx: ['DOOR CREAK', 'CHAINS', 'WHISPERS', 'CAVE DRIP', 'REVERSE GONG', 'CROWS'], pal: [3, 2], w: [.8, 1.6, 1.7, .5, .6, 1] },
  { name: 'BARDO', from: 'The Himalayan in-between', fam: ['DORIAN', 'TRITONE'], bells: ['BOWL', 'TAM-TAM'], drones: ['TEMPLE HORN', 'THROAT'], choirs: ['MONKS'], sfx: ['REVERSE GONG', 'RING SHADOW', 'ENDLESS FALL', 'WHISPERS', 'HEARTBEAT'], pal: [5, 2], w: [1.4, 1.4, 1.4, .4, .4, 1] },
  { name: 'MICTLAN', from: 'The Aztec underworld', fam: ['SHROUD', 'PHRYGIAN'], bells: ['TAM-TAM', 'IRON BAR'], drones: ['EARTH PIPE', 'POLAR WIND'], choirs: ['WRAITHS'], sfx: ['DEATH WHISTLE', 'BULLROARER', 'HEARTBEAT', 'BONE FLUTE', 'THUNDER', 'CAVE DRIP'], pal: [6, 2], w: [1, .8, .6, 1.5, 1.4, .9] },
  { name: 'NIFLHEIM', from: 'The Norse realm of ice', fam: ['LOCRIAN', 'UNDERTONE'], bells: ['GLASS', 'IRON BAR'], drones: ['POLAR WIND', 'BOWED IRON'], choirs: ['WRAITHS', 'ONDES'], sfx: ['ICE CRACK', 'ICE GROAN', 'THUNDER', 'WHISPERS', 'ENDLESS FALL'], pal: [7, 3], w: [1, .9, .8, 1, .8, 1.5] },
  { name: 'YOMI', from: 'The Japanese land of the dead', fam: ['IN'], bells: ['BONSHO', 'BOWL'], drones: ['POLAR WIND', 'BOWED IRON'], choirs: ['ONDES', 'WRAITHS'], sfx: ['CROWS', 'BONE FLUTE', 'CAVE DRIP', 'WHISPERS', 'BROKEN MUSIC BOX'], pal: [8, 3], w: [1, 1.7, .9, .5, .4, 1.6] },
  { name: 'PATALA', from: 'The Javanese netherworld', fam: ['PELOG'], bells: ['GONG AGENG', 'IRON BAR'], drones: ['BOWED IRON', 'ABYSS FM'], choirs: ['MONKS', 'CELLOS'], sfx: ['BONE FLUTE', 'RING SHADOW', 'SINKING CALL', 'CAVE DRIP', 'HEARTBEAT'], pal: [4, 1], w: [.9, 1.8, 1.1, .8, .7, .9] },
  { name: 'SIGNAL', from: 'A Cold War bunker', fam: ['TRITONE', 'OCTATONIC'], bells: ['IRON BAR', 'DEAD PIANO'], drones: ['TURBINE', 'ORGAN 32 FT'], choirs: ['NIGHT SYNTH', 'TAPE CHOIR'], sfx: ['NUMBER STATION', 'SHORTWAVE', 'GEIGER', 'AIR SIREN', 'DISTANT BRAAAM', 'BROKEN MUSIC BOX'], pal: [9, 2], w: [.9, .7, 1, 1.4, 1.6, 1] },
  { name: 'HELIOPAUSE', from: 'The edge of the solar system', fam: ['UNDERTONE', 'OCTATONIC'], bells: ['GLASS', 'WATERPHONE'], drones: ['ABYSS FM', 'POLAR WIND'], choirs: ['ONDES', 'NIGHT SYNTH'], sfx: ['PULSAR', 'WHISTLER', 'ENDLESS FALL', 'RING SHADOW', 'REVERSE GONG'], pal: [0, 7], w: [1.3, .7, .9, .9, .6, 1.8] },
  { name: 'DUAT', from: 'The Egyptian night river', fam: ['HIJAZ', 'PHRYGIAN'], bells: ['TAM-TAM', 'BOWL'], drones: ['DUDUK', 'POLAR WIND'], choirs: ['MONKS', 'KEENING'], sfx: ['SISTRUM', 'SAND WIND', 'TEMPLE GATE', 'RITUAL DRUM', 'REVERSE GONG'], pal: [10, 2], w: [1, 1.6, 1.3, .7, .5, 1] },
  { name: 'TUONELA', from: 'The Finnish river of death', fam: ['PENTA', 'UNDERTONE'], bells: ['KANTELE', 'GLASS'], drones: ['BOWED IRON', 'POLAR WIND'], choirs: ['KEENING', 'WRAITHS'], sfx: ['MOURNING HORN', 'OAR CREAK', 'RIVER FLOW', 'WHISPERS', 'ICE GROAN'], pal: [11, 7], w: [1, 1.2, 1.4, .6, .4, 1.6] },
  { name: 'KURNUGI', from: 'The Mesopotamian land of no return', fam: ['HIJAZ', 'SHROUD'], bells: ['OIL DRUM', 'TAM-TAM'], drones: ['EARTH PIPE', 'DUDUK'], choirs: ['MONOTONE', 'MONKS'], sfx: ['SAND WIND', 'RITUAL DRUM', 'TEMPLE GATE', 'SINKING CALL', 'WHISPERS'], pal: [10, 6], w: [1.3, 1.2, 1.3, .7, .8, 1.3] },
  { name: 'WARD 9', from: 'An abandoned hospital', fam: ['OCTATONIC', 'LOCRIAN'], bells: ['GLASS', 'DEAD PIANO'], drones: ['MAINS HUM', 'TURBINE'], choirs: ['WRAITHS', 'TAPE CHOIR'], sfx: ['FLATLINE', 'MONITOR BEEP', 'FLUORESCENT', 'CHILD HUMMING', 'DOOR CREAK', 'BROKEN MUSIC BOX', 'WHISPERS'], pal: [12, 3], w: [1, .7, 1.2, 1.4, 1, 1.4] },
  { name: 'FOUNDRY', from: 'A dead steel works', fam: ['TRITONE', 'LOCRIAN'], bells: ['ANVIL', 'OIL DRUM'], drones: ['FURNACE', 'TURBINE'], choirs: ['NIGHT SYNTH', 'MONOTONE'], sfx: ['ANVIL STRIKE', 'FURNACE ROAR', 'STEAM HISS', 'PISTON', 'GEAR GRIND', 'CHAINS', 'DISTANT TRAIN'], pal: [13, 2], w: [.8, 1, .8, 1.4, 1.8, .7] },
  { name: 'HOLLOW', from: 'A haunted black forest', fam: ['PHRYGIAN', 'PENTA'], bells: ['RUSTED PIPE', 'WATERPHONE'], drones: ['POLAR WIND', 'EARTH PIPE'], choirs: ['WRAITHS', 'KEENING'], sfx: ['OWL', 'WOLF HOWL', 'BRANCH SNAP', 'WIND CHIMES', 'DROWNED BELL', 'BATS', 'CROWS'], pal: [14, 4], w: [1, .8, 1, 1.6, .9, 1.3] },
].map(s => ({ ...s, famI: s.fam.map(f => FAMILIES.indexOf(f)), bellI: nm(BELLS, s.bells), droneI: nm(DRONES, s.drones), choirI: nm(CHOIRS, s.choirs), sfxI: sfxIdx(...s.sfx) }));


/* level trims, measured one sound at a time (check.js) */
const TRIM = {'bell': [0.813, 0.55, 1.259, 1.531, 0.933, 1.496, 1.928, 2.265, 1.531, 1.622, 2.113, 1.109, 1.38], 'drone': [0.519, 0.794, 2.661, 3.388, 2.344, 0.661, 12.589, 2.63, 2.884, 4.677, 0.776], 'choir': [0.767, 2.344, 1.096, 1.047, 1.175, 3.981, 0.741, 1.288, 0.676], 'sfx': [1.175, 17.783, 14.791, 1.758, 8.222, 1.349, 3.589, 0.389, 2.917, 1.479, 3.273, 0.575, 2.239, 0.596, 9.886, 0.989, 0.452, 2.163, 0.479, 3.273, 2.884, 1.603, 0.495, 1.334, 3.126, 4.677, 0.724, 2.985, 3.846, 2.483, 5.129, 2.951, 1.148, 1.012, 0.543, 0.575, 1.82, 0.966, 0.537, 3.631, 1.035, 7.079, 1.496, 1.122, 1.95, 0.562, 0.569, 2.239, 0.562, 0.676, 0.861, 0.794, 1.799, 2.985]};
BELLS.forEach((b, i) => { b.g = b.g.map(x => x * TRIM.bell[i]); });
DRONES.forEach((d, i) => { d.g *= TRIM.drone[i]; });
CHOIRS.forEach((c, i) => { c.g *= TRIM.choir[i]; });
SFX.forEach((s, i) => { if (s.custom) s.trim = TRIM.sfx[i]; else s.gain *= TRIM.sfx[i]; });
