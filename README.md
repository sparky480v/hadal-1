# HADAL-1 for the M-VAVE FM-1

HADAL-1 is a generative dark ambient firmware by Polystellarflow. Drones, demonic and angelic choirs, bells and distant
sounds keep changing by themselves, for as long as it stays on. There are 15 worlds and 40 full-screen pictures that melt
into each other. Leave it alone, or steer it with the knobs and play over it on the keys.

**Works on every FM-1:** older units (Puya P25Q80 flash) and newer ones sold since autumn 2026 (UC25HQ80 flash). Both
chips are accepted, so saves and updates work on either.

**Best on headphones.** It starts in speaker mode for the FM-1's small built-in speaker. For the full, deep sound:
GLOBAL, SOUND MODE, HEADPHONES.

## First steps

The first time it starts, a short guide shows. Press any key to turn its pages. You can open it again any time in
GLOBAL, HOW TO PLAY. With CONTROL HELP TEXT on (the default), every control explains itself on screen as you use it.

## Controls

| Control | What it does |
| --- | --- |
| PLAY | Start or stop the sound |
| PRESET knob | Choose the world |
| SELECT knob | The next page of knobs |
| KNOB 1-4 | The page's four settings |
| HOME | Back to the first page, MAIN; closes a menu |
| ALGO knob | The picture: the world's own, or any of the 40 |
| Keys | Play what the KEYS page says: bells, voices, latch or split |
| OCT- / OCT+ | The octave of what the keys play |
| FX | The effects menu: tap a key to play an effect, hold it to keep it on |
| SAVE | Save a world in one of four slots, load or erase it |
| GLO | GLOBAL settings |
| ENV | Panic: every latched voice and kept effect lets go, with a soft breath |
| REC | Hold: freeze the sound and the picture |
| SEQ | A new world, new sounds, a new scale |
| EDIT | The next part of the world now |
| ARP | Upheaval: a new era |
| LFO | Hold: the swell builds. Let go: it collapses |

### The knob pages

| Page | Knobs |
| --- | --- |
| MAIN | DARKNESS, ACTIVITY, DECAY, SPACE |
| LEVELS | DRONES, BELLS, EFFECTS, CHOIR |
| VOICES | BELL, DRONE, CHOIR (each: the world's own or one you choose), SCENE |
| SHAPE: ROOM | ROOM SIZE, ROOM DARK, ROOM AMOUNT, TAPE WEAR |
| SHAPE: MOVEMENT | TAPE WOBBLE, BELL RATE, FX RATE, WIDTH |
| SHAPE: NOTES | KEY, MOOD, MOTIF (a short tune that comes back changed), NEW TUNE |
| KEYS | KEYS PLAY (bells, voices, latch, split), KEY VOICE, KEY VOLUME, VOICE FADE |

Latch: press a key to start a voice, press it again to let it go, up to 4. Split: the bottom row rings bells, the top
row sings voices.

### FX menu

Tap a key to play an effect once. Hold a key for half a second to keep it playing; hold it again to stop it. Up to 4
can be kept on. Turn SELECT for the next bank. Hold FX and press keys to play effects without the menu. Effects also
play by themselves (LEVELS, EFFECTS sets how loud; SHAPE: MOVEMENT, FX RATE how often), and their name shows small at
the top left.

### SAVE menu

Press SAVE. KNOB 1 chooses one of four slots. Press SAVE again to save there (it asks before replacing a used slot).
Tap a lit key (F, G, A or B) to load that slot. Knob 4, three clicks right, erases it. HOME leaves without saving.
GLOBAL, START ON chooses whether power-on brings back your saved world or a new one.

### GLOBAL

Press GLO. Turn SELECT to choose a setting, KNOB 1 to change it, GLO when you are done. It saves itself.
How to play, sound mode, world changes, how far the knobs wander, key lights, sound sleep timer, screen, control help
text, USB recording, picture brightness, text size, MIDI out, panic sound, start on, and ABOUT (the version, and where
to send ideas).

### MIDI

In, over USB or the TRS jack: notes play what the KEYS page says. Program change 0-14 picks the world. CC 16-19 are
MAIN's knobs, CC 20-23 LEVELS, CC 24-27 SHAPE: ROOM, CC 28-31 SHAPE: MOVEMENT. CC 1 (mod wheel) is the swell. CC 120 or
123 lets every note go.

Out, over USB (GLOBAL, MIDI OUT): the notes the world plays, for another synth, a DAW or a MIDI-to-CV box. Bells on
channel 1, the ground note on channel 2, the pad notes on channel 3, and the scene as CC 20 on channel 1.

## Install

Install it from The Firmware Hub, or with an FM-1 web installer in Chrome or Edge, or run
`python3 base/tools/fm1_install.py hadal-1.0.fwsc`. The update loader is included, so you can always install another
firmware afterwards. Keep M-VAVE's stock V15 firmware file handy. Installing custom firmware is at your own risk; if it
fails to start twice, the FM-1 comes up in a safe mode the installer can reach.

## Build it

On Linux x86-64, or with Docker elsewhere: `./build.sh --release 1.0`. See `BUILDING.md`. A GitHub workflow in
`.github` can also build a release when you push a tag such as `v1.0`.

`sh tests/run_tests.sh` runs the engine and the app through every world in the simulator.

## Ideas and feedback

polystellarflow@gmail.com

## Credits

Built on [fm1-base](https://github.com/charlesvestal/fm1-base) and [felucca-bsp](https://github.com/hugelton/felucca-bsp)
(`base/`), from Felucca by Leo Kuroshita (Hugelton Instruments), and on the design of
[AMB-1](https://github.com/charlesvestal/fm1-amb) by Charles Vestal. The fonts are under the SIL OFL. MPL-2.0: see
`LICENSE`. M-VAVE and FM-1 are trademarks of their owners. HADAL-1 is not affiliated with them.
