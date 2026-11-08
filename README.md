# FoMni for the M-VAVE FM-1

FoMni turns your M-VAVE FM-1 into a chord harp, inspired by the Suzuki Omnichord. The 16 white
keys are the strum plate: run a finger across them and they ring out the chord. The 11 black keys
are the chord buttons. Under the strings, an organ-like chord and a bass hold the harmony, and the
OM-84's ten rhythms can play them in time (auto bass sync).

It's meant to be simple. There's one screen with four knobs a page, and no patterns to program.

[![FoMni running on the FM-1 (watch on YouTube)](https://img.youtube.com/vi/pahaM6nzucE/maxresdefault.jpg)](https://www.youtube.com/watch?v=pahaM6nzucE)

![FoMni's screen: a C chord strummed](docs/img/screen-strum.png) ![A minor held](docs/img/screen-chord.png) ![The rhythm page](docs/img/screen-rhythm.png)

- **Try it in your browser** (no FM-1 needed): <https://charlesvestal.github.io/fm1-fomni/emu/>
- **Install it** (Chrome or Edge, with the FM-1 plugged in): <https://charlesvestal.github.io/fm1-fomni/install/>
- **Download the firmware file**: [releases](https://github.com/charlesvestal/fm1-fomni/releases)

Install at your own risk, though it's hard to get stuck. The web installer can always put M-VAVE's
firmware back, and an FM-1 that keeps crashing starts in a safe mode.

## Playing

This version adds the Suzuki Q Chord's way of playing: EZ Play, Auto Chord and Chord Plus, a choice of 70 voices
for the strum plate, the chord and the chord plus, Vibrato and Reverb buttons, three level controls, and a pitch
bend for the strum plate.

| | |
|---|---|
| White keys | The strum plate. Each key plucks one string of the chord. As on the Omnichord, each group of three keys is root, third and fifth, five groups up, folded into an F#–F window, with one more root on top. |
| Black keys | The chord buttons, F C G, Dm Am, Em G7 E7, D7 Bb and A7 at first. Hold **ENV** for the minor, **LFO** for the 7th, or both for the m7, like the Omnichord's three rows. |
| REC | **EZ PLAY** on or off. It's on at first: no rhythm, just the chord buttons and the strum plate. |
| SEQ | **AUTO CHORD** on or off: the chord section. With it off (and Chord Plus off), the chord buttons only change the strum plate's notes. |
| SEL | **CHORD PLUS** on or off. With the rhythm, it adds a figure of its own to each style. Played by hand (EZ Play, or the rhythm stopped), it adds the chord an octave up and the bass. |
| FX | **REVERB** on or off (Rev depth and Space on the Sound page). |
| GLO | **VIBRATO** on or off, on the strum plate (Vib depth on the Sound page). It fades in over a quarter second on each pluck. |
| PLAY | Starts and stops the rhythm. Pressing it in EZ Play turns EZ Play off first. Turning EZ Play on stops the rhythm. |
| ARP | Turns on CHORD HOLD, so the chord keeps playing after you let go. Press the same chord button again to stop it. |
| OCT− / OCT+ | Move the strum plate down or up an octave. |
| SELECT | Picks the **CHORD SET**, what the black keys play: **As set** (each button's own chord, from the Chords page), or one type on every button: **Major, Minor, 7th, Major 7, Minor 7, Augmented, Diminished, Blues** (7#9) or **Minor blues** (m6). In a set, the roots go round the circle of fifths like a Q Chord row: Ab Eb Bb F C G D A E B F#. The strum plate follows whatever you press. A tag in the header shows the set. ENV and LFO still work on top. The key (transpose) is on the Chords page; tempo is on the Rhythm page. |
| ALGORITHM | Picks the rhythm. |
| PRESETS | **SUSTAIN** of the strum plate, from any page. At 0 every voice (even violin or organ) stops in about a quarter second. |
| HOME / EDIT | HOME goes to the Play page. EDIT steps through the pages. |
| KNOB 1–4 | Set the four values on the screen. The pages below list them. On the Play page, **KNOB 4 is the PITCH BEND** on the strum plate: each click bends further (five reach the Bend range on the Setup page, 2 semitones at first) and it springs back when you stop turning. On the other pages KNOB 4 is that page's value. |
| SAVE | Saves. FoMni also saves by itself a few seconds after a change, once it's quiet. |

The top right of the screen shows which of AUTO, PLUS, VIB and REV are on, and the buttons light up to match.

### The pages

Each page puts four values on KNOB 1–4. Press EDIT to step through the pages.

- **Play** (HOME) has Strum, the strum plate's voice; Chord, the chord's voice; Plus, Chord Plus's voice; and Bend. Sustain (0.25 s to 12 s) is on PRESETS. The top of the page shows the chord on a keyboard octave: the root in coral, the other notes in teal.
- **Mix** has the three Q Chord levels, Strum vol, Chord vol (the chord and chord plus) and Drum vol, plus Bass vol.
- **Rhythm** has Rhythm, Tempo, Bass (the bass line on or off) and Sync start (the rhythm waits for your first chord).
- **Sound** has Rev depth, Space (the plate's size), Vib depth and Width. The drums stay dry.
- On **Chords**, you can change the chord on any black key. Press the key, then turn Root (KNOB 1) and Type
  (KNOB 2). KNOB 3 and 4 set Transpose and Octave.
- **Setup** has Tune, MIDI out, Lights and Bend (the pitch bend's range, 1 to 12 semitones).

### The voices

All three parts choose from the same list. **Omni 1** is the Omnichord's shimmering harp on the strum plate (and its
organ on the chord), and **Omni 2** is its plain harp. The others: Piano, E.Piano, Harpsichd, Clavinet, Celesta,
Glocken, Music Box, Vibes, Marimba, Xylophone, Bells, Kalimba, SteelDrum, Organ, PercOrgan, RockOrgan, Church,
Accordion, Harmonica, Nylon Gtr, Steel Gtr, Jazz Gtr, Mute Gtr, Dist Gtr, Banjo, Sitar, Dulcimer, Harp, Pizzicato,
Bass, SynthBass, Violin, Cello, Strings, SynString, Choir, Trumpet, Brass, Sax, Clarinet, Oboe, Flute, Pan Flute,
Ocarina, Calliope, Sq Lead, Saw Lead, PolySynth, Warm Pad, Halo Pad, Metal Pad; more synths: Supersaw, Sync Lead, PWM Pad, PluckSyn, SynBrass, Acid Bass, Sub Bass, Chiptune; layered, many-voice sounds: Ensemble, ChoirEns, 12-String, OctaveGtr, Fifths; two singing voices, Singer (a sung "aah", with vibrato) and Chant (a low "ooh"), made with vowel formants; and two experimental ones, Laser Zap (each pluck dives two octaves) and Glitch (a crushed, warbling FM tone). They're small synth patches
(wavetable and two-operator FM; `firmware/src/dsp/om_voices.h`), levelled to match the Omni harp. Every voice
fades with Sustain. Held voices like organ, strings and winds fade slowly on the strum plate and hold on the chord.

When you change chord while strings are still ringing, they slide to the new chord's notes instead of stopping.

### How the chord section plays

| | Chord button held, by hand (EZ Play, or the rhythm stopped) | With the rhythm running |
|---|---|---|
| AUTO CHORD | the chord, held | the chord, in the rhythm's pattern |
| CHORD PLUS | the chord an octave up, and the bass | the style's own figure |
| Bass (Rhythm page) | the bass, held (not in EZ Play) | the rhythm's bass line |

MIDI out sends Chord Plus on channel 4.

### MIDI

MIDI comes in over USB and the TRS jack, and goes out over USB. The FM-1's jack is input only.

- Notes coming in on channel 1 pluck the nearest string.
- Play a chord on a keyboard on channel 2, and it becomes the chord, as if you'd pressed a black key.
- When MIDI clock comes in, the rhythm follows it, along with start and stop. The screen shows EXT for the tempo.
- MIDI out is on by default, and you can turn it off on the Setup page. The strings go out on channel 1, the
  chord on 2, the bass on 3 and the drums on 10. When the rhythm runs on its own tempo, FoMni also sends clock,
  start and stop.

The drums on channel 10:

| Sound | Note |
|---|---|
| Bass drum | 36 |
| Snare | 38 |
| Closed hat | 42 |
| Ride | 51 |
| Claves | 75 |

### Screen reader

FoMni-1 tells [FM-1 Screen Reader](https://charlesvestal.github.io/fm1-reader/) what its screen shows, over
USB. You'll hear the page and its knobs, a knob's value as you turn it, the chord you play, play and stop, and
what each button did. Open the page in Chrome with the FM-1 plugged in. It speaks with its own voice, or hands
the text to your computer's screen reader.

## Building and testing

The toolchain is the same as X0X's (see [BUILDING.md](BUILDING.md)):

```
tests/run_tests.sh      # the instrument, the platform pieces, the app in a simulator
./build.sh              # build/omni.fwsc
python3 tools/fm1_install.py build/omni.fwsc     # install it over USB
```

`build/host/omni_host SCRIPT OUTDIR` runs the whole app on a computer from a script
(`tests/scenarios/*.omni`), with audio and screenshots coming out. `web/emu/build.sh` builds the
same code for the browser.

## Credits

The rhythms, waves, drum sounds and envelope shapes come from
[Chordian](https://github.com/Jan125/pb.chordian), Jan125's OM-84 emulator (CC0). The platform
(hardware layer, USB, update loader, storage, installer) is
[Felucca](https://github.com/hugelton/Felucca)'s by Leo Kuroshita (Hügelton Instruments), by way of
[X0X](https://github.com/charlesvestal/fm1-x0x). GPL-3.0-only; see [LICENSING.md](LICENSING.md).

Omnichord is a trademark of Suzuki. FoMni isn't affiliated with or endorsed by Suzuki, M-VAVE or
Hügelton Instruments.
