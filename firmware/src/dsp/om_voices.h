/* SPDX-License-Identifier: GPL-3.0-only */
/* The voices ("textures"), after the Q Chord's: one small engine, many settings. Each voice is a
 * wavetable carrier, optionally phase-modulated by a sine (two-operator FM: bells, vibes, e-piano,
 * plucks), through a one-pole lowpass that follows the note (and can close as the note fades), with
 * its own attack, decay, tremolo and vibrato. Cheap enough for 16 strings, the chord and the chord
 * plus at once. Included by omni.c only. */
#pragma once

/* the carriers: 100 points a cycle, made at om_init (om_wave_init) */
enum { WT_SINE, WT_TRI, WT_SAW, WT_SQR, WT_PULSE, WT_ORGAN, WT_REED, WT_HOLLOW, WT_VOX, WT_BRIGHT, WT_OMCHORD, WT_N };

typedef struct {
    const char *name;      /* the screen's (9 characters at most) */
    uint8_t wave;          /* WT_* */
    int8_t shift;          /* semitones from the string's note (a bass voice an octave down, bells up) */
    float ratio;           /* the modulator's frequency over the carrier's; 0: no FM */
    float index;           /* FM depth at the attack (radians, up to 2.5) */
    float ifloor;          /* the part of it that stays (0..1) */
    float itime;           /* how fast the rest goes (s) */
    float atk;             /* attack (s) */
    float sdec;            /* strum plate: the decay's length, times the SUSTAIN knob's */
    float curve;           /* strum plate: 1 a pluck (a fast fall, a long tail), 0 an even fade (organ) */
    float cdec;            /* chord: the fall from the attack to the sustain level (time constant, s) */
    float sus;             /* chord: the level it holds while the button is down (0: it dies away) */
    float rel;             /* chord: release (s) */
    float bright;          /* the lowpass: 1 open, 0 dull */
    float track;           /* how much it closes as the note fades */
    float trem, vib;       /* its own tremolo (0..1) and vibrato (cents), at the LFO's 5.5 Hz */
    float gain;
    /* optional, left out (zero) by most voices: */
    uint8_t w2;            /* a second carrier's wave: a layer (WT_*) */
    float l2;              /* its frequency over the first's (1.006 a detuned twin, 2 an octave, 1.5 a fifth); 0 none */
    float l2mix;           /* how much of it (0..1) */
    float sweep, swt;      /* the pitch starts this many semitones away and glides home (time constant, s) */
    uint8_t crush;         /* sample-and-hold: every Nth sample only (lo-fi); 0 off */
} om_inst_t;

/*   name         wave       sh  ratio idx   iflr itime  atk    sdec curve cdec  sus   rel   brt   trk  trem vib gain */
static const om_inst_t INST[] = {
    {"Omni 1",    WT_OMCHORD, 0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f, 1.0f, 0.333f, 0.95f, 0.18f, 1.0f, 0.0f, 0.0f, 0, 1.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Omni 2",    WT_OMCHORD, 0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f, 1.0f, 0.333f, 0.95f, 0.18f, 1.0f, 0.0f, 0.0f, 0, 1.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* keys */
    {"Piano",     WT_BRIGHT, 0, 1.0f, 1.2f, 0.1f, 0.5f, 0.0f, 1.2f, 1.0f, 2.5f, 0.0f, 0.25f, 0.75f, 0.6f, 0.0f, 0, 0.92f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"E.Piano",   WT_SINE,   0, 1.0f, 1.8f, 0.15f, 0.25f, 0.0f, 1.3f, 1.0f, 2.5f, 0.0f, 0.3f, 1.0f, 0.0f, 0.0f, 0, 0.48f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Harpsichd", WT_PULSE,  0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.7f, 1.0f, 1.2f, 0.0f, 0.15f, 1.0f, 0.5f, 0.0f, 0, 1.14f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Clavinet",  WT_PULSE,  0, 1.0f, 1.0f, 0.2f, 0.1f, 0.0f, 0.5f, 1.0f, 0.8f, 0.0f, 0.1f, 0.9f, 0.7f, 0.0f, 0, 1.33f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* mallets and bells */
    {"Celesta",   WT_SINE,  12, 4.0f, 0.8f, 0.0f, 0.15f, 0.0f, 1.0f, 1.0f, 1.5f, 0.0f, 0.3f, 1.0f, 0.0f, 0.0f, 0, 0.49f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Glocken",   WT_SINE,  24, 3.5f, 1.0f, 0.1f, 0.3f, 0.0f, 1.2f, 1.0f, 2.0f, 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 0, 0.47f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Music Box", WT_SINE,  12, 7.0f, 0.6f, 0.0f, 0.05f, 0.0f, 1.1f, 1.0f, 2.0f, 0.0f, 0.4f, 1.0f, 0.0f, 0.0f, 0, 0.48f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Vibes",     WT_SINE,   0, 4.0f, 1.0f, 0.0f, 0.08f, 0.0f, 1.6f, 1.0f, 3.0f, 0.0f, 0.4f, 1.0f, 0.0f, 0.4f, 0, 0.61f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Marimba",   WT_SINE,   0, 4.0f, 1.6f, 0.0f, 0.03f, 0.0f, 0.35f, 1.0f, 0.5f, 0.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0, 0.78f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Xylophone", WT_SINE,  12, 3.0f, 1.5f, 0.0f, 0.03f, 0.0f, 0.25f, 1.0f, 0.4f, 0.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0, 0.92f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Bells",     WT_SINE,   0, 3.5f, 2.5f, 0.2f, 1.2f, 0.0f, 2.5f, 0.5f, 4.0f, 0.0f, 0.8f, 1.0f, 0.0f, 0.0f, 0, 0.39f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Kalimba",   WT_SINE,   0, 6.0f, 0.8f, 0.0f, 0.02f, 0.0f, 0.5f, 1.0f, 0.8f, 0.0f, 0.2f, 1.0f, 0.0f, 0.0f, 0, 0.65f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"SteelDrum", WT_SINE,   0, 2.0f, 1.4f, 0.2f, 0.15f, 0.0f, 0.8f, 1.0f, 1.2f, 0.0f, 0.3f, 1.0f, 0.0f, 0.0f, 0, 0.54f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* organs */
    {"Organ",     WT_ORGAN,  0, 0.0f, 0.0f, 0.0f, 0.1f, 0.005f, 1.5f, 0.0f, 99.0f, 1.0f, 0.06f, 1.0f, 0.0f, 0.0f, 0, 0.41f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"PercOrgan", WT_ORGAN,  0, 2.0f, 1.0f, 0.0f, 0.08f, 0.005f, 1.5f, 0.0f, 99.0f, 1.0f, 0.06f, 1.0f, 0.0f, 0.0f, 0, 0.41f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"RockOrgan", WT_ORGAN,  0, 1.0f, 0.8f, 1.0f, 1.0f, 0.005f, 1.5f, 0.0f, 99.0f, 1.0f, 0.06f, 1.0f, 0.0f, 0.3f, 0, 0.49f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Church",    WT_ORGAN,  0, 2.0f, 0.6f, 1.0f, 1.0f, 0.06f, 1.5f, 0.0f, 99.0f, 1.0f, 0.3f, 1.0f, 0.0f, 0.0f, 0, 0.47f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Accordion", WT_REED,   0, 0.0f, 0.0f, 0.0f, 0.1f, 0.03f, 1.5f, 0.0f, 99.0f, 1.0f, 0.08f, 0.8f, 0.0f, 0.2f, 0, 0.38f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Harmonica", WT_REED,   0, 0.0f, 0.0f, 0.0f, 0.1f, 0.04f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 0.6f, 0.0f, 0.0f, 15, 0.35f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* guitars and plucked */
    {"Nylon Gtr", WT_TRI,    0, 1.0f, 0.9f, 0.0f, 0.08f, 0.0f, 0.9f, 1.0f, 1.5f, 0.0f, 0.2f, 0.55f, 0.7f, 0.0f, 0, 0.65f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Steel Gtr", WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f, 1.0f, 1.8f, 0.0f, 0.2f, 0.7f, 0.8f, 0.0f, 0, 0.69f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Jazz Gtr",  WT_TRI,    0, 1.0f, 0.4f, 0.2f, 0.2f, 0.0f, 0.8f, 1.0f, 1.5f, 0.0f, 0.2f, 0.4f, 0.5f, 0.0f, 0, 0.68f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Mute Gtr",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.15f, 1.0f, 0.2f, 0.0f, 0.08f, 0.45f, 0.8f, 0.0f, 0, 2.20f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Dist Gtr",  WT_SQR,    0, 1.0f, 2.0f, 0.8f, 0.5f, 0.0f, 2.0f, 0.3f, 3.0f, 0.6f, 0.1f, 0.7f, 0.3f, 0.0f, 0, 0.36f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Banjo",     WT_PULSE,  0, 1.0f, 0.6f, 0.0f, 0.05f, 0.0f, 0.35f, 1.0f, 0.5f, 0.0f, 0.1f, 1.0f, 0.9f, 0.0f, 0, 1.36f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Sitar",     WT_PULSE,  0, 1.0f, 2.0f, 0.5f, 0.4f, 0.0f, 1.4f, 1.0f, 2.0f, 0.0f, 0.3f, 1.0f, 0.3f, 0.0f, 0, 1.24f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Dulcimer",  WT_SAW,    0, 2.0f, 0.7f, 0.2f, 0.2f, 0.0f, 1.0f, 1.0f, 1.5f, 0.0f, 0.3f, 0.8f, 0.6f, 0.0f, 0, 0.71f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Harp",      WT_TRI,    0, 1.0f, 0.5f, 0.0f, 0.1f, 0.0f, 1.4f, 1.0f, 2.0f, 0.0f, 0.3f, 0.7f, 0.5f, 0.0f, 0, 0.59f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Pizzicato", WT_TRI,    0, 1.0f, 0.5f, 0.0f, 0.05f, 0.0f, 0.3f, 1.0f, 0.4f, 0.0f, 0.1f, 0.6f, 0.6f, 0.0f, 0, 0.99f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* basses */
    {"Bass",      WT_SINE, -12, 1.0f, 1.2f, 0.2f, 0.15f, 0.0f, 0.9f, 1.0f, 1.5f, 0.0f, 0.15f, 0.6f, 0.5f, 0.0f, 0, 0.53f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"SynthBass", WT_SAW,  -12, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.8f, 1.0f, 1.0f, 0.0f, 0.1f, 0.5f, 0.9f, 0.0f, 0, 0.84f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* bowed and ensembles */
    {"Violin",    WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.08f, 1.5f, 0.0f, 99.0f, 1.0f, 0.25f, 0.65f, 0.0f, 0.0f, 18, 0.41f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Cello",     WT_SAW,  -12, 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 1.5f, 0.0f, 99.0f, 1.0f, 0.3f, 0.55f, 0.0f, 0.0f, 15, 0.41f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Strings",   WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.25f, 1.5f, 0.0f, 99.0f, 1.0f, 0.5f, 0.55f, 0.0f, 0.1f, 10, 0.41f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"SynString", WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.15f, 1.5f, 0.0f, 99.0f, 1.0f, 0.4f, 0.7f, 0.0f, 0.2f, 6, 0.47f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Choir",     WT_VOX,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 1.5f, 0.0f, 99.0f, 1.0f, 0.5f, 0.8f, 0.0f, 0.0f, 8, 0.50f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* brass and winds */
    {"Trumpet",   WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.04f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 0.6f, 0.0f, 0.0f, 8, 0.45f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Brass",     WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.06f, 1.5f, 0.0f, 99.0f, 1.0f, 0.15f, 0.75f, 0.0f, 0.0f, 5, 0.46f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Sax",       WT_REED,   0, 0.0f, 0.0f, 0.0f, 0.1f, 0.03f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 0.6f, 0.0f, 0.0f, 12, 0.36f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Clarinet",  WT_SQR,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.03f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 0.5f, 0.0f, 0.0f, 6, 0.24f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Oboe",      WT_PULSE,  0, 0.0f, 0.0f, 0.0f, 0.1f, 0.03f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 0.55f, 0.0f, 0.0f, 8, 0.52f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Flute",     WT_HOLLOW, 0, 0.0f, 0.0f, 0.0f, 0.1f, 0.06f, 1.5f, 0.0f, 99.0f, 1.0f, 0.15f, 1.0f, 0.0f, 0.0f, 10, 0.28f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Pan Flute", WT_HOLLOW, 0, 0.0f, 0.0f, 0.0f, 0.1f, 0.08f, 1.5f, 0.0f, 99.0f, 1.0f, 0.2f, 0.5f, 0.0f, 0.0f, 6, 0.29f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Ocarina",   WT_SINE,   0, 0.0f, 0.0f, 0.0f, 0.1f, 0.03f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 1.0f, 0.0f, 0.0f, 8, 0.28f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Calliope",  WT_HOLLOW, 12, 0.0f, 0.0f, 0.0f, 0.1f, 0.02f, 1.5f, 0.0f, 99.0f, 1.0f, 0.1f, 1.0f, 0.0f, 0.2f, 0, 0.34f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* synths */
    {"Sq Lead",   WT_SQR,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.005f, 2.5f, 0.2f, 99.0f, 1.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0, 0.26f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Saw Lead",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.005f, 2.5f, 0.2f, 99.0f, 1.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0, 0.45f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"PolySynth", WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.01f, 1.5f, 0.6f, 2.0f, 0.3f, 0.3f, 0.8f, 0.7f, 0.0f, 0, 0.58f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Warm Pad",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.4f, 1.5f, 0.0f, 99.0f, 1.0f, 0.6f, 0.35f, 0.0f, 0.1f, 4, 0.42f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Halo Pad",  WT_VOX,   12, 0.0f, 0.0f, 0.0f, 0.1f, 0.5f, 1.5f, 0.0f, 99.0f, 1.0f, 0.7f, 0.7f, 0.0f, 0.15f, 0, 0.48f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Metal Pad", WT_SINE,   0, 2.76f, 1.2f, 1.0f, 1.0f, 0.3f, 1.5f, 0.0f, 99.0f, 1.0f, 0.6f, 1.0f, 0.0f, 0.0f, 0, 0.27f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    /* more synths                                                                                                 w2       l2     mix  sweep swt crush */
    {"Supersaw",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.01f, 1.5f, 0.3f, 99.0f, 1.0f, 0.2f, 0.9f, 0.0f, 0.0f, 4, 0.74f, WT_SAW, 1.0071f, 0.5f, 0.0f, 0.0f, 0},
    {"Sync Lead", WT_SAW,    0, 2.0f, 1.6f, 0.6f, 0.3f, 0.005f, 2.0f, 0.3f, 99.0f, 1.0f, 0.1f, 1.0f, 0.0f, 0.0f, 6, 0.51f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"PWM Pad",   WT_PULSE,  0, 0.0f, 0.0f, 0.0f, 0.1f, 0.3f, 1.5f, 0.0f, 99.0f, 1.0f, 0.6f, 0.6f, 0.0f, 0.1f, 3, 0.48f, WT_SQR, 1.004f, 0.5f, 0.0f, 0.0f, 0},
    {"PluckSyn",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.5f, 1.0f, 0.6f, 0.0f, 0.15f, 0.95f, 0.95f, 0.0f, 0, 1.24f, WT_SAW, 1.005f, 0.4f, 0.0f, 0.0f, 0},
    {"SynBrass",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.05f, 1.5f, 0.0f, 99.0f, 1.0f, 0.15f, 0.7f, 0.4f, 0.0f, 3, 0.53f, WT_SAW, 0.997f, 0.5f, 0.0f, 0.0f, 0},
    {"Acid Bass", WT_SAW,  -12, 1.0f, 1.4f, 0.0f, 0.12f, 0.0f, 0.5f, 1.0f, 0.6f, 0.0f, 0.1f, 0.55f, 0.9f, 0.0f, 0, 0.89f, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0},
    {"Sub Bass",  WT_SINE, -12, 0.0f, 0.0f, 0.0f, 0.1f, 0.005f, 1.0f, 0.5f, 1.5f, 0.0f, 0.15f, 1.0f, 0.0f, 0.0f, 0, 0.55f, WT_TRI, 2.0f, 0.25f, 0.0f, 0.0f, 0},
    {"Chiptune",  WT_SQR,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0.8f, 0.4f, 99.0f, 1.0f, 0.05f, 1.0f, 0.0f, 0.0f, 0, 0.50f, WT_PULSE, 2.0f, 0.3f, 0.0f, 0.0f, 3},
    /* many voices: layers and ensembles */
    {"Ensemble",  WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 1.5f, 0.0f, 99.0f, 1.0f, 0.5f, 0.6f, 0.0f, 0.15f, 8, 0.60f, WT_SAW, 1.0062f, 0.5f, 0.0f, 0.0f, 0},
    {"ChoirEns",  WT_VOX,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.2f, 1.5f, 0.0f, 99.0f, 1.0f, 0.5f, 0.8f, 0.0f, 0.1f, 8, 0.64f, WT_VOX, 2.003f, 0.4f, 0.0f, 0.0f, 0},
    {"12-String", WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 1.1f, 1.0f, 1.8f, 0.0f, 0.2f, 0.75f, 0.8f, 0.0f, 0, 1.00f, WT_SAW, 2.004f, 0.4f, 0.0f, 0.0f, 0},
    {"OctaveGtr", WT_TRI,    0, 1.0f, 0.8f, 0.0f, 0.08f, 0.0f, 0.9f, 1.0f, 1.5f, 0.0f, 0.2f, 0.6f, 0.7f, 0.0f, 0, 0.83f, WT_TRI, 0.5f, 0.45f, 0.0f, 0.0f, 0},
    {"Fifths",    WT_SAW,    0, 0.0f, 0.0f, 0.0f, 0.1f, 0.02f, 1.5f, 0.4f, 99.0f, 1.0f, 0.2f, 0.7f, 0.3f, 0.0f, 0, 0.77f, WT_SAW, 1.4983f, 0.45f, 0.0f, 0.0f, 0},
    /* experimental */
    {"Laser Zap", WT_SINE,   0, 1.41f, 2.0f, 0.3f, 0.2f, 0.0f, 0.7f, 1.0f, 1.0f, 0.0f, 0.2f, 1.0f, 0.0f, 0.0f, 0, 0.66f, WT_SQR, 0.5f, 0.2f, 24.0f, 0.06f, 0},
    {"Glitch",    WT_SQR,    0, 3.17f, 2.5f, 0.8f, 0.8f, 0.0f, 1.2f, 0.5f, 2.0f, 0.4f, 0.3f, 1.0f, 0.0f, 0.6f, 40, 0.63f, WT_SAW, 1.33f, 0.3f, -7.0f, 0.4f, 7},
};
#define OM_NINST ((int)(sizeof INST / sizeof INST[0]))

/* the carriers: harmonic amplitudes (sines, cosine phase for the organ-like), peak-normalised */
static float om_wt[WT_N][100] OM_POOL;

static void wt_build(float *w, const float *amp, int nh)
{
    int i, h;
    float peak = 0.0f;
    for (i = 0; i < 100; i++) {
        float x = 0.0f;
        for (h = 0; h < nh; h++)
            if (amp[h] != 0.0f)
                x += amp[h] * fm_sinf((float)((h + 1) * i) * (FM_TWO_PI / 100.0f));
        w[i] = x;
        peak = fm_maxf(peak, fm_fabsf(x));
    }
    for (i = 0; i < 100; i++)
        w[i] /= peak;
}

static void om_wave_init(const float *omchord)
{
    float a[16];
    int h, i;
    for (h = 0; h < 16; h++)
        a[h] = 0.0f;
    a[0] = 1.0f;
    wt_build(om_wt[WT_SINE], a, 1);
    for (h = 0; h < 16; h++)                                  /* triangle: odd, 1/n^2, alternating */
        a[h] = (h & 1) ? 0.0f : ((h >> 1) & 1 ? -1.0f : 1.0f) / (float)((h + 1) * (h + 1));
    wt_build(om_wt[WT_TRI], a, 16);
    for (h = 0; h < 16; h++)                                  /* saw: all, 1/n */
        a[h] = 1.0f / (float)(h + 1);
    wt_build(om_wt[WT_SAW], a, 16);
    for (h = 0; h < 16; h++)                                  /* square: odd, 1/n */
        a[h] = (h & 1) ? 0.0f : 1.0f / (float)(h + 1);
    wt_build(om_wt[WT_SQR], a, 16);
    for (h = 0; h < 16; h++)                                  /* a 25% pulse: sin(n pi / 4) / n */
        a[h] = fm_sinf((float)(h + 1) * (FM_PI * 0.25f)) / (float)(h + 1);
    wt_build(om_wt[WT_PULSE], a, 16);
    for (h = 0; h < 16; h++)
        a[h] = 0.0f;
    a[0] = 1.0f; a[1] = 0.8f; a[2] = 0.6f; a[3] = 0.5f; a[5] = 0.3f; a[7] = 0.25f;   /* drawbars */
    wt_build(om_wt[WT_ORGAN], a, 8);
    for (h = 0; h < 16; h++)                                  /* reed: odd strong, even half */
        a[h] = ((h & 1) ? 0.5f : 1.0f) / (float)(h + 1);
    wt_build(om_wt[WT_REED], a, 12);
    for (h = 0; h < 16; h++)
        a[h] = 0.0f;
    a[0] = 1.0f; a[1] = 0.2f; a[2] = 0.1f;                   /* hollow: a flute */
    wt_build(om_wt[WT_HOLLOW], a, 3);
    {
        static const float VOX[12] = {1.0f, 0.5f, 0.8f, 0.6f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f, 0.04f, 0.03f, 0.02f};
        wt_build(om_wt[WT_VOX], VOX, 12);
    }
    for (h = 0; h < 16; h++)                                  /* bright: piano-like, 1/n^0.8 */
        a[h] = fm_powf((float)(h + 1), -0.8f) * ((h % 7 == 6) ? 0.2f : 1.0f);
    wt_build(om_wt[WT_BRIGHT], a, 16);
    for (i = 0; i < 100; i++)
        om_wt[WT_OMCHORD][i] = omchord[i];
}
