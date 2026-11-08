/* SPDX-License-Identifier: GPL-3.0-only */
/* The instrument (omni.h). One compilation unit, -O2. Waves, envelope shapes, voicing and rhythms
 * after Chordian (Jan125/pb.chordian, CC0 1.0); its rates were written for 96 kHz and are converted
 * here where they are per sample (the filters), not where they are per second (the envelopes).
 *
 * After the Suzuki Q Chord: the strum plate, the chord and the chord plus each take one of the voices
 * in om_voices.h; EZ PLAY (no rhythm: the chord button and the strum plate), AUTO CHORD (the chord
 * section on / off), CHORD PLUS (a third part: with the rhythm, a figure of its own; held, the chord an
 * octave up and the bass), VIBRATO and REVERB on the strum plate, and a pitch bend on the strum plate. */
#include "omni.h"
#include "fastmath.h"
#include "om_data.h"
#include "om_drums.h"

#ifdef OM_HOST
#define OM_POOL
#else
#define OM_POOL __attribute__((section(".pool")))
#endif
#define BARRIER() __asm__ volatile("" ::: "memory")

#include "om_voices.h"

const char *const OM_TYPE_NAME[CH_NTYPES] = {"", "m", "7", "M7", "m7", "dim", "aug", "sus4", "add9"};
const char *const OM_NOTE_NAME[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
/* the three tones (semitones over the root); the third tone of a 7th is the 7th, as on the OM */
const int8_t OM_TYPE_TONES[CH_NTYPES][3] = {
    {0, 4, 7}, {0, 3, 7}, {0, 4, 10}, {0, 4, 11}, {0, 3, 10}, {0, 3, 6}, {0, 4, 8}, {0, 5, 7}, {0, 4, 2}};

static const om_param_t PARAMS[P_NPARAMS] = {
    {"Strum", 0, OM_NINST - 1, 0}, {"Chord", 0, OM_NINST - 1, 0}, {"Plus", 0, OM_NINST - 1, 2}, {"Sustain", 0, 100, 50},
    {"Strum vol", 0, 100, 80}, {"Chord vol", 0, 100, 60}, {"Drum vol", 0, 100, 70}, {"Bass vol", 0, 100, 60},
    {"Rhythm", 0, OM_NRHYTHM - 1, 0}, {"Tempo", 60, 240, 116}, {"Bass", 0, 1, 1},
    {"Rev depth", 0, 100, 35}, {"Space", 0, 100, 55}, {"Vib depth", 0, 100, 40}, {"Width", 0, 100, 60},
    {"Transpose", -6, 6, 0}, {"Octave", -1, 1, 0},
    {"Tune", -50, 50, 0}, {"MIDI out", 0, 1, 1}, {"Bend", 1, 12, 2},
    {"EZ Play", 0, 1, 1}, {"Auto Chord", 0, 1, 1}, {"Chord Plus", 0, 1, 0}, {"Vibrato", 0, 1, 0}, {"Reverb", 0, 1, 1},
};

/* CHORD PLUS with the rhythm: a figure for each rhythm, a step a character, as long as the rhythm:
 * '1' '2' '3' the chord's tones (low to high) an octave over the chord, '4' its lowest an octave more */
static const char *const PLUS_PAT[OM_NRHYTHM] = {
    "1-2-3-2-1-2-3-2-1-2-3-2-1-2-3-2-",            /* Rock 1 */
    "1---3---2---3---1---3---2---4---",            /* Rock 2 */
    "--4---4---4---4---4---4---4---4-",            /* Disco: the off-beat octave */
    "1--2--3-4--3--2-1--2--3-4--3--2-",            /* Latin */
    "1-3-2-3-1-3-2-3-1-3-2-3-1-3-2-3-",            /* Country */
    "4-------3-------4-------3-------",            /* March */
    "1--1--1-4-3-2-1-1--1--1-4-3-2-1-",            /* Tango */
    "1---2-3---2-1---2-3---2-",                    /* Blues */
    "4--3----2--1----4--3----2--1----",            /* Swing */
    "1---2---3---4---3---2---",                    /* Waltz */
};
const char *om_plus_pattern(int i) { return PLUS_PAT[(unsigned)i < OM_NRHYTHM ? i : 0]; }

int om_ninst(void) { return OM_NINST; }
const char *om_inst_name(int i) { return INST[(unsigned)i < (unsigned)OM_NINST ? i : 0].name; }

const om_rhythm_t *om_rhythm(int i) { return &OM_RHYTHM[(unsigned)i < OM_NRHYTHM ? i : 0]; }

const om_param_t *om_param_info(int i) { return (i >= 0 && i < P_NPARAMS) ? &PARAMS[i] : &PARAMS[0]; }

static void itoa_s(int v, char *b)
{
    char t[8];
    int n = 0, neg = v < 0;
    if (neg)
        v = -v;
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v && n < 6);
    if (neg)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

void om_param_text(int i, int v, char *b)
{
    static const char *const ONOFF[2] = {"Off", "On"};
    const char *s = 0;
    if (i == P_RHYTHM)
        s = OM_RHYTHM[v < 0 ? 0 : v % OM_NRHYTHM].name;
    else if (i == P_SVOICE || i == P_CVOICE || i == P_PVOICE)
        s = om_inst_name(v);
    else if (i == P_BASS || i == P_MIDI || i >= P_EZ)
        s = ONOFF[v ? 1 : 0];
    if (s) {
        int k = 0;
        while (s[k] && k < 9) {
            b[k] = s[k];
            k++;
        }
        b[k] = 0;
        return;
    }
    if ((i == P_TRANSPOSE || i == P_OCTAVE || i == P_TUNE) && v > 0) {
        *b++ = '+';
    }
    itoa_s(v, b);
}

/* ------------------------------------------------------------- voicing --- */
static int fold(int pc, int lo) { return lo + (((pc - lo) % 12) + 12) % 12; }

void om_voicing(int root, int type, int transpose, int octave, uint8_t harp[OM_NSTR], uint8_t chord[OM_NCHORD],
                uint8_t *bass)
{
    const int8_t *t = OM_TYPE_TONES[(unsigned)type < CH_NTYPES ? type : 0];
    int r = root + transpose, k, i;
    for (k = 0; k < 5; k++)                       /* windows F#2..F3, F#3..F4, ...: root, 2nd, 3rd tone */
        for (i = 0; i < 3; i++)
            harp[3 * k + i] = (uint8_t)fold(r + t[i], 42 + 12 * k + 12 * octave);
    harp[15] = (uint8_t)fold(r, 42 + 60 + 12 * octave);
    for (i = 0; i < 3; i++)
        chord[i] = (uint8_t)fold(r + t[i], 54);  /* F#3..F4, the OM's chord register */
    *bass = (uint8_t)fold(r, 36);                 /* C2..B2 */
}

/* ------------------------------------------------------------ commands --- */
enum { C_SET, C_CHORD, C_GATE, C_STRUM, C_PLAY, C_PANIC, C_CLOCK, C_BEND };
#define QN 64u
static uint32_t q[QN];
static volatile uint32_t q_w, q_r;
static void post(uint32_t c, int a, int b)
{
    uint32_t w = q_w;
    if (w - q_r >= QN)
        return;                                   /* full: dropped (never, at the UI's rate) */
    q[w % QN] = c << 24 | ((uint32_t)a & 0xFFu) << 16 | ((uint32_t)b & 0xFFFFu);
    BARRIER();
    q_w = w + 1;
}
void om_set(int p, int v) { post(C_SET, p, v); }
void om_chord(int root, int type) { post(C_CHORD, root, type); }
void om_gate(int on) { post(C_GATE, on, 0); }
void om_strum(int s) { post(C_STRUM, s, 0); }
void om_play(int on) { post(C_PLAY, on, 0); }
void om_panic(void) { post(C_PANIC, 0, 0); }
void om_clock(int msg) { post(C_CLOCK, msg, 0); }
void om_bend(int pos) { post(C_BEND, 0, pos < -100 ? -100 : pos > 100 ? 100 : pos); }

/* -------------------------------------------------------------- state --- */
volatile float om_str_level[OM_NSTR];
volatile uint8_t om_str_note[OM_NSTR];        /* the note a pluck plays now */
volatile float om_chord_level, om_bass_level;
volatile uint8_t om_playing, om_step, om_steps = 32;
volatile uint8_t om_drum_hit;
volatile uint8_t om_ext;

static int16_t par[P_NPARAMS];
static float tunefac = 1.0f, svol, cvol, bvol, rvol, rsend, csend, sus_rate, vib_depth;
static float pan_l[OM_NSTR], pan_r[OM_NSTR];
static int root, type;
static uint8_t harp_n[OM_NSTR], chord_n[OM_NCHORD], bass_n, plus_n[4];
static int gate;

/* the voices' rates, per sample (from om_voices.h, at om_init) */
static float i_idk[OM_NINST], i_ar[OM_NINST], i_sdk[OM_NINST], i_cdk[OM_NINST], i_rk[OM_NINST];

/* the LFO (vibrato, tremolo) and the bend: once a chunk */
#define LFO_HZ 5.5f
static float lfo_ph, lfo_val, bend_target, bend_c, bend_fac = 1.0f;

static float sin3[360];                       /* Chordian's shimmer shape: |sin|^0.6 */
static float shim_ph, shim_rate;
/* the shimmer's rate follows the chord's root (Chordian, Hz, C..B) */
static const float SHIM_HZ[12] = {3.761f, 2.910f, 4.275f, 4.565f, 5.050f, 6.333f, 2.843f, 3.125f, 3.162f, 3.419f,
                                  3.667f, 3.953f};

static float note_inc(int n)                   /* wave positions (100 per cycle) per sample */
{
    return 440.0f * tunefac * fm_exp2f((float)(n - 69) * (1.0f / 12.0f)) * (100.0f / OM_SR);
}

static inline float wave(const float *w, float pos)
{
    int i = (int)pos;
    float f = pos - (float)i, a = w[i], b = w[i + 1 < 100 ? i + 1 : 0];
    return a + (b - a) * f;
}
static inline float adv(float pos, float inc)
{
    pos += inc;
    while (pos >= 100.0f)
        pos -= 100.0f;
    return pos;
}

static int clamp_note(int n)                   /* a voice's octave shift kept inside what sounds */
{
    while (n > 108)
        n -= 12;
    while (n < 24)
        n += 12;
    return n;
}

static void midi(uint32_t st, uint32_t d1, uint32_t d2)
{
    if (par[P_MIDI])
        om_midi_out(st, d1, d2);
}

/* ---- harp strings */
typedef struct {
    float pos, inc, mpos, v, g, y1, y2, k, c, m, modamt, idec, vg;
    uint16_t shim_off;
    uint8_t on, note, inst, atk;
} str_t;
static str_t str[OM_NSTR];

static void strum(int s)
{
    str_t *v = &str[s];
    int ii = par[P_SVOICE], n = harp_n[s];
    const om_inst_t *in = &INST[ii];
    if (ii >= 2)
        n = clamp_note(n + in->shift);
    if (v->on)
        midi(0x80, v->note, 0);
    midi(0x90, (uint32_t)n, 100);
    if (!v->on) {
        v->y1 = v->y2 = 0.0f;
        v->g = 0.0f;
        v->v = 0.0f;
    }
    v->on = 1;
    v->note = (uint8_t)n;
    v->inst = (uint8_t)ii;
    v->inc = note_inc(n);
    v->idec = 1.0f;
    v->vg = 0.0f;
    if (ii < 2) {                                /* the OM-84's harp */
        float u = fm_exp2f((float)(n - 69) * (1.0f / 12.0f)) * tunefac, base;
        v->v = 1.0f;
        v->atk = 0;
        /* the voice filter: Chordian's per-note brightness, darker as the note decays */
        base = fm_maxf(((18.0f - u) / 18.0f) * 0.31f + 0.34f, 0.05f);
        v->k = fm_powf(base, 2.85f);
        {   /* the shimmer: less on the higher strings; each string its own phase (Chordian's offsets) */
            int i = s * 12 / 15;
            v->modamt = 1.0f - (float)i / 17.0f;
            v->shim_off = (uint16_t)(((i & 1) ? 27 * i : 14 * i) % 360);
        }
    } else if (i_ar[ii] > 0.0f) {
        v->atk = 1;                              /* from where it is: a re-pluck does not click */
    } else {
        v->v = 1.0f;
        v->atk = 0;
    }
}

/* ---- chord, chord plus: voices of om_voices.h with a gate */
enum { A_OFF, A_ATK, A_DEC, A_REL, A_CUT };
typedef struct {
    float pos, inc, mpos, v, g, y, idec;
    uint8_t st, base, note, os, ch;              /* os: 0 held, 1 a stab (released at once), 2 rings out */
} av_t;
#define OM_NPLUS 4
static av_t chv[OM_NCHORD], plv[OM_NPLUS];
static int plus_rr;

static void av_note(av_t *v, int base, int ii)
{
    int n = ii >= 2 ? clamp_note(base + INST[ii].shift) : base;
    v->base = (uint8_t)base;
    v->note = (uint8_t)n;
    v->inc = note_inc(n);
}

static void av_trigger(av_t *v, int base, int ii, int os)
{
    if (v->st != A_OFF)
        midi(0x80u | v->ch, v->note, 0);
    if (v->st == A_OFF) {
        v->v = v->g = v->y = 0.0f;
    }
    av_note(v, base, ii);
    v->idec = 1.0f;
    v->os = (uint8_t)os;
    if (i_ar[ii] > 0.0f) {
        v->st = A_ATK;
    } else {
        v->v = 1.0f;
        v->st = os == 1 ? A_REL : A_DEC;
    }
    midi(0x90u | v->ch, v->note, 90);
}

static void av_retune(av_t *v, int base, int ii)
{
    midi(0x80u | v->ch, v->note, 0);
    av_note(v, base, ii);
    midi(0x90u | v->ch, v->note, 90);
}

static int av_held(const av_t *v, int n)        /* a held part sounding (not a stab, not released) */
{
    int i;
    for (i = 0; i < n; i++)
        if (v[i].st == A_ATK || (v[i].st == A_DEC && !v[i].os))
            return 1;
    return 0;
}

static void chord_cmd(char c)
{
    int i;
    if (c == 'T' || c == 'O') {
        for (i = 0; i < OM_NCHORD; i++)
            av_trigger(&chv[i], chord_n[i], par[P_CVOICE], c == 'O');
    } else if (c == 'R' || c == 'N') {
        for (i = 0; i < OM_NCHORD; i++)
            if (chv[i].st != A_OFF)
                chv[i].st = c == 'R' ? A_REL : A_CUT;
    }
}

static void plus_hold(int on)                   /* the held CHORD PLUS: the chord an octave up */
{
    int i;
    for (i = 0; i < OM_NPLUS; i++)
        if (on && i < 3)
            av_trigger(&plv[i], plus_n[i], par[P_PVOICE], 0);
        else if (plv[i].st != A_OFF && plv[i].st != A_CUT)
            plv[i].st = A_REL;
    plus_rr = 0;
}

/* ---- bass (the OM's own) */
enum { E_OFF, E_DEC, E_SUS, E_REL, E_BDEC, E_CUT };
typedef struct {
    float pos, inc, v, g;
    uint8_t st, note;
} cv_t;
static cv_t bsv;
static uint8_t bass_kind;                      /* 0 root, 1 fifth, 2 third */

static int bass_note(void)
{
    const int8_t *t = OM_TYPE_TONES[type];
    return bass_n + (bass_kind == 1 ? 7 : bass_kind == 2 ? t[1] : 0);
}

static void bass_cmd(char c, char which)
{
    if (c == 'T' || c == 'O') {
        bass_kind = which == '5' ? 1 : which == '3' ? 2 : 0;
        if (bsv.st != E_OFF)
            midi(0x82, bsv.note, 0);
        bsv.note = (uint8_t)bass_note();
        bsv.inc = note_inc(bsv.note);
        bsv.v = 1.0f;
        bsv.st = c == 'T' ? E_SUS : E_BDEC;
        midi(0x92, bsv.note, 100);
    } else if ((c == 'R' || c == 'N') && bsv.st != E_OFF) {
        bsv.st = c == 'R' ? E_REL : E_CUT;
    }
}

/* ---- the accompaniment by hand (no rhythm running): what the buttons say sounds while a chord
 * button is down. AUTO CHORD: the chord. CHORD PLUS: the chord an octave up, and the bass. BASS: the
 * bass (not in EZ PLAY: there only the chord and the strum plate, unless CHORD PLUS). retrig: a new
 * chord button, every part starts again; else (a button changed) only what changed. */
static void held_apply(int retrig)
{
    int wc = par[P_AUTOCH], wp = par[P_PLUS], wb = wp || (par[P_BASS] && !par[P_EZ]);
    if (wc && (retrig || !av_held(chv, OM_NCHORD)))
        chord_cmd('T');
    else if (!wc)
        chord_cmd('R');
    if (wb && (retrig || bsv.st != E_SUS)) {
        bass_kind = 0;
        bass_cmd('T', 'R');
    } else if (!wb && bsv.st == E_SUS) {
        bass_cmd('R', 'R');
    }
    if (wp && (retrig || !av_held(plv, OM_NPLUS)))
        plus_hold(1);
    else if (!wp && av_held(plv, OM_NPLUS))
        plus_hold(0);
}

static void accomp_release(void)
{
    chord_cmd('R');
    bass_cmd('R', 'R');
    plus_hold(0);
}

/* the chord changed: what sounds moves to it (the OM's chord and bass follow the buttons) */
static void retune(void)
{
    int i;
    om_voicing(root, type, par[P_TRANSPOSE], par[P_OCTAVE], harp_n, chord_n, &bass_n);
    for (i = 0; i < OM_NSTR; i++)                 /* what each string plays when plucked next */
        om_str_note[i] = harp_n[i];
    {   /* CHORD PLUS: the chord's tones, low to high, an octave up; then the lowest an octave more */
        uint8_t a = chord_n[0], b = chord_n[1], c = chord_n[2], t;
        if (a > b) { t = a; a = b; b = t; }
        if (b > c) { t = b; b = c; c = t; }
        if (a > b) { t = a; a = b; b = t; }
        plus_n[0] = (uint8_t)(a + 12);
        plus_n[1] = (uint8_t)(b + 12);
        plus_n[2] = (uint8_t)(c + 12);
        plus_n[3] = (uint8_t)(a + 24);
    }
    for (i = 0; i < OM_NCHORD; i++)
        if (chv[i].st != A_OFF && chv[i].base != chord_n[i])
            av_retune(&chv[i], chord_n[i], par[P_CVOICE]);
    for (i = 0; i < 3; i++)                       /* the held plus layer follows; the figure's notes ring out */
        if (plv[i].st != A_OFF && !plv[i].os && plv[i].base != plus_n[i])
            av_retune(&plv[i], plus_n[i], par[P_PVOICE]);
    if (bsv.st != E_OFF && bsv.note != bass_note()) {
        midi(0x82, bsv.note, 0);
        bsv.note = (uint8_t)bass_note();
        bsv.inc = note_inc(bsv.note);
        midi(0x92, bsv.note, 100);
    }
    shim_rate = SHIM_HZ[((root + par[P_TRANSPOSE]) % 12 + 12) % 12] * 360.0f / OM_SR;
}

/* ---- rhythm */
typedef struct {
    uint32_t pos;
    uint8_t on;
} drum_t;
static drum_t drum[OM_NDRUM];
static int rh, rh_next, step, pulse;            /* pulse: MIDI clock, 0..5 within a step */
static float pulse_left, step_len;
static uint32_t ext_age;                         /* render calls since the last clock tick */
static const uint8_t GM_DRUM[OM_NDRUM] = {36, 75, 42, 51, 38};   /* BD, claves, closed hat, ride, snare */

static void tempo_set(void) { step_len = OM_SR * 60.0f / ((float)par[P_TEMPO] * 4.0f); }

static void do_step(void)
{
    const om_rhythm_t *r = &OM_RHYTHM[rh];
    uint8_t d = r->drum[step];
    int i;
    for (i = 0; i < OM_NDRUM; i++)
        if (d >> i & 1u) {
            drum[i].pos = 0;
            drum[i].on = 1;
            midi(0x99, GM_DRUM[i], 100);
            midi(0x89, GM_DRUM[i], 0);
        }
    om_drum_hit |= d;
    if (gate) {                                  /* the accompaniment, in time */
        if (par[P_BASS] && r->bass[step] != '-')
            bass_cmd(r->bass[step], r->note[step]);
        if (par[P_AUTOCH] && r->chord[step] != '-')
            chord_cmd(r->chord[step]);
        if (par[P_PLUS]) {
            char c = PLUS_PAT[rh][step];
            if (c >= '1' && c <= '4') {
                av_trigger(&plv[plus_rr], plus_n[c - '1'], par[P_PVOICE], 2);
                plus_rr = (plus_rr + 1) % OM_NPLUS;
            }
        }
    }
    om_step = (uint8_t)step;
    if (++step >= r->len) {
        step = 0;
        if (rh_next != rh) {                     /* a new rhythm starts on the bar */
            rh = rh_next;
            om_steps = OM_RHYTHM[rh].len;
        }
    }
}

/* ---- reverb: Dattorro's plate (J. Dattorro, "Effect Design, Part 1", JAES 1997): a predelay, four
 * input diffusers, then a figure-eight tank of two halves, each a modulated allpass, a delay, a damping
 * lowpass, an allpass and a delay, feeding the other half. Stereo out from seven taps a side. The
 * paper's lengths are for 29761 Hz; DS() scales them to 44.1 kHz. */
#define DS(n) (((n) * 14818 + 5000) / 10000)          /* 44100 / 29761 */
#define PRE_N 442                                    /* 10 ms */
#define EXC DS(16)                                   /* the tank allpasses' modulation, samples */
enum { L_PRE, L_IN1, L_IN2, L_IN3, L_IN4, L_APL, L_D1L, L_AP2L, L_D2L, L_APR, L_D1R, L_AP2R, L_D2R, L_N };
static const int DL_LEN[L_N] = {PRE_N, DS(142), DS(107), DS(379), DS(277), DS(672) + EXC + 2, DS(4453), DS(1800),
                                DS(3720), DS(908) + EXC + 2, DS(4217), DS(2656), DS(3163)};
#define DL_TOTAL (PRE_N + DS(142) + DS(107) + DS(379) + DS(277) + DS(672) + DS(4453) + DS(1800) + DS(3720) + DS(908) + \
                  DS(4217) + DS(2656) + DS(3163) + 2 * (EXC + 2))
static float pl_buf[DL_TOTAL] OM_POOL;
typedef struct {
    float *b;
    int n, i;
} dl_t;
static dl_t dl[L_N];
static float pl_bw, pl_damp_l, pl_damp_r, pl_decay = 0.5f, pl_dd2 = 0.5f, lfo_s, lfo_c = 1.0f;

static inline float tap(const dl_t *d, int k)          /* the value written k samples ago, 1..n */
{
    int j = d->i - k;
    return d->b[j < 0 ? j + d->n : j];
}
static inline void push(dl_t *d, float x)
{
    d->b[d->i] = x;
    if (++d->i >= d->n)
        d->i = 0;
}
static inline float allpass(dl_t *d, int len, float x, float g)
{
    float z = tap(d, len), v = x - g * z;
    push(d, fm_flush(v));
    return z + g * v;
}
static inline float allpass_mod(dl_t *d, float len, float x, float g)   /* a fractional, moving length */
{
    int k = (int)len;
    float f = len - (float)k, z = tap(d, k) + (tap(d, k + 1) - tap(d, k)) * f, v = x - g * z;
    push(d, fm_flush(v));
    return z + g * v;
}

static inline void reverb(float in, float *ol, float *or_)
{
    float x, a, b, fb_l = tap(&dl[L_D2R], DS(3163)), fb_r = tap(&dl[L_D2L], DS(3720));
    push(&dl[L_PRE], in);
    x = tap(&dl[L_PRE], PRE_N);
    pl_bw += 0.7f * (x - pl_bw);                     /* input bandwidth */
    x = allpass(&dl[L_IN1], DS(142), pl_bw, 0.75f);
    x = allpass(&dl[L_IN2], DS(107), x, 0.75f);
    x = allpass(&dl[L_IN3], DS(379), x, 0.625f);
    x = allpass(&dl[L_IN4], DS(277), x, 0.625f);
    {   /* the modulation: ~0.9 Hz, sine and cosine for the two halves */
        float s = lfo_s + 0.000128f * lfo_c, c = lfo_c - 0.000128f * s;
        lfo_s = s;
        lfo_c = c;
    }
    a = allpass_mod(&dl[L_APL], (float)DS(672) + (float)EXC * lfo_s, x + pl_decay * fb_l, -0.7f);
    push(&dl[L_D1L], a);
    b = tap(&dl[L_D1L], DS(4453));
    pl_damp_l += 0.62f * (b - pl_damp_l);
    b = allpass(&dl[L_AP2L], DS(1800), pl_damp_l * pl_decay, pl_dd2);
    push(&dl[L_D2L], b);
    a = allpass_mod(&dl[L_APR], (float)DS(908) + (float)EXC * lfo_c, x + pl_decay * fb_r, -0.7f);
    push(&dl[L_D1R], a);
    b = tap(&dl[L_D1R], DS(4217));
    pl_damp_r += 0.62f * (b - pl_damp_r);
    b = allpass(&dl[L_AP2R], DS(2656), pl_damp_r * pl_decay, pl_dd2);
    push(&dl[L_D2R], b);
    *ol = 0.6f * (tap(&dl[L_D1R], DS(266)) + tap(&dl[L_D1R], DS(2974)) - tap(&dl[L_AP2R], DS(1913)) +
                  tap(&dl[L_D2R], DS(1996)) - tap(&dl[L_D1L], DS(1990)) - tap(&dl[L_AP2L], DS(187)) -
                  tap(&dl[L_D2L], DS(1066)));
    *or_ = 0.6f * (tap(&dl[L_D1L], DS(353)) + tap(&dl[L_D1L], DS(3627)) - tap(&dl[L_AP2L], DS(1228)) +
                   tap(&dl[L_D2L], DS(2673)) - tap(&dl[L_D1R], DS(2111)) - tap(&dl[L_AP2R], DS(335)) -
                   tap(&dl[L_D2R], DS(121)));
}

static void plate_init(void)
{
    float *p = pl_buf;
    int k, i;
    for (k = 0; k < L_N; k++) {
        dl[k].b = p;
        dl[k].n = DL_LEN[k];
        dl[k].i = 0;
        p += DL_LEN[k];
    }
    for (i = 0; i < DL_TOTAL; i++)
        pl_buf[i] = 0.0f;
    pl_bw = pl_damp_l = pl_damp_r = 0.0f;
    lfo_s = 0.0f;
    lfo_c = 1.0f;
}

/* ---- parameters */
static void sends_set(void)                      /* REVERB: the strum plate's (and, less, the chord's) */
{
    float d = (float)par[P_REVERB] * 0.01f;
    rsend = par[P_REVON] ? d * d : 0.0f;
    csend = rsend * 0.5f;
}

static void play_stop(void);

static void apply(int p, int v)
{
    const om_param_t *pi = &PARAMS[p];
    int i;
    if (v < pi->lo)
        v = pi->lo;
    if (v > pi->hi)
        v = pi->hi;
    par[p] = (int16_t)v;
    switch (p) {
    case P_SVOL: svol = (float)v * (1.0f / 80.0f); break;
    case P_SUSTAIN:                               /* 0.25 s .. 12 s: the fade's time scale */
        sus_rate = (1.0f / OM_SR) / (0.25f * fm_powf(48.0f, (float)v * 0.01f));
        break;
    case P_CHORD: cvol = (float)v * 0.01f; break;
    case P_BASSVOL: bvol = (float)v * 0.01f; break;
    case P_RHYVOL: rvol = (float)v * 0.01f; break;
    case P_REVERB: case P_REVON: sends_set(); break;
    case P_VIBDEPTH: vib_depth = (float)v * 0.5f; break;    /* up to 50 cents */
    case P_SPACE:                                 /* the plate's decay: a small room to a long hall */
        pl_decay = 0.35f + 0.55f * (float)v * 0.01f;
        pl_dd2 = fm_clampf(pl_decay + 0.15f, 0.25f, 0.5f);
        break;
    case P_TEMPO: tempo_set(); break;
    case P_RHYTHM:
        rh_next = v;
        if (!om_playing) {
            rh = v;
            om_steps = OM_RHYTHM[rh].len;
        }
        break;
    case P_TUNE:
        tunefac = fm_exp2f((float)v * (1.0f / 1200.0f));
        retune();
        break;
    case P_TRANSPOSE: case P_OCTAVE: retune(); break;
    case P_WIDTH:
        for (i = 0; i < OM_NSTR; i++) {           /* low strings left, high right */
            float p0 = ((float)i / 15.0f * 2.0f - 1.0f) * (float)v * 0.01f, a = (p0 + 1.0f) * (FM_PI / 4.0f);
            pan_l[i] = fm_cosf(a);
            pan_r[i] = fm_sinf(a);
        }
        break;
    case P_MIDI:
        if (!v)
            om_midi_out(0xB0, 123, 0);            /* all notes off, as it goes quiet */
        break;
    case P_CVOICE:                                /* what sounds takes the new voice (its octave too) */
        for (i = 0; i < OM_NCHORD; i++)
            if (chv[i].st != A_OFF)
                av_retune(&chv[i], chv[i].base, v);
        break;
    case P_PVOICE:
        for (i = 0; i < OM_NPLUS; i++)
            if (plv[i].st != A_OFF)
                av_retune(&plv[i], plv[i].base, v);
        break;
    case P_EZ:                                    /* EZ PLAY: no rhythm */
        if (v && om_playing)
            play_stop();
        else if (gate && !om_playing)
            held_apply(0);
        break;
    case P_AUTOCH: case P_PLUS: case P_BASS:
        if (gate && !om_playing) {
            held_apply(0);
        } else if (om_playing) {                  /* with the rhythm: a part turned off stops now */
            if (!par[P_AUTOCH])
                chord_cmd('R');
            if (!par[P_PLUS])
                plus_hold(0);
            if (!par[P_BASS])
                bass_cmd('R', 'R');
        }
        break;
    default: break;
    }
}

static void quiet(void)
{
    int i;
    for (i = 0; i < OM_NSTR; i++)
        str[i].on = 0;
    for (i = 0; i < OM_NCHORD; i++)
        chv[i].st = A_OFF;
    for (i = 0; i < OM_NPLUS; i++)
        plv[i].st = A_OFF;
    bsv.st = E_OFF;
    for (i = 0; i < OM_NDRUM; i++)
        drum[i].on = 0;
    for (i = 0; i < 4; i++)
        om_midi_out(0xB0 | (uint32_t)i, 123, 0);
}

static void inst_init(void)
{
    int i;
    for (i = 0; i < OM_NINST; i++) {
        const om_inst_t *in = &INST[i];
        i_idk[i] = fm_expf(-16.0f / (OM_SR * fm_maxf(in->itime, 0.001f)));   /* a tick is 16 samples */
        i_ar[i] = in->atk > 0.002f ? 1.0f / (OM_SR * in->atk) : 0.0f;
        i_sdk[i] = 1.0f / fm_maxf(in->sdec, 0.05f);
        i_cdk[i] = 1.0f / (OM_SR * fm_maxf(in->cdec, 0.01f));
        i_rk[i] = 1.0f / (OM_SR * fm_maxf(in->rel, 0.004f));
    }
}

void om_init(void)
{
    int i;
    for (i = 0; i < 360; i++) {
        float s = fm_sinf((float)(i + 125) * (FM_PI / 180.0f));
        sin3[i] = s == 0.0f ? 0.0f : (s > 0.0f ? 1.0f : -1.0f) * fm_powf(fm_fabsf(s), 0.6f);
    }
    om_wave_init(OM_WAVE_CHORD);
    inst_init();
    plate_init();
    q_r = q_w = 0;
    root = 0;
    type = CH_MAJ;
    gate = 0;
    for (i = 0; i < OM_NCHORD; i++)
        chv[i].ch = 1;
    for (i = 0; i < OM_NPLUS; i++)
        plv[i].ch = 3;
    bend_target = bend_c = 0.0f;
    bend_fac = 1.0f;
    for (i = 0; i < P_NPARAMS; i++)
        apply(i, PARAMS[i].def);
    retune();
}

/* the rhythm's clock: a step every 6 pulses. From its own tempo (pulse_left) or from MIDI clock in */
static void clock_pulse(void)
{
    if (pulse == 0)
        do_step();
    if (!om_ext)
        midi(0xF8, 0, 0);
    if (++pulse >= 6)
        pulse = 0;
}

static void play_start(int from_top)
{
    if (par[P_EZ])                              /* EZ PLAY: the rhythm stays out */
        return;
    if (from_top) {
        rh = rh_next;
        om_steps = OM_RHYTHM[rh].len;
        step = 0;
        pulse = 0;
    }
    pulse_left = 0.0f;
    om_playing = 1;
    plus_hold(0);                               /* the figure takes over from the held layer */
    if (!om_ext)
        midi(from_top ? 0xFA : 0xFB, 0, 0);
}

static void play_stop(void)
{
    om_playing = 0;
    if (!om_ext)
        midi(0xFC, 0, 0);
    if (gate)
        held_apply(1);                          /* a button still down: its chord, by hand */
    else
        accomp_release();
}

static void drain(void)
{
    while (q_r != q_w) {
        uint32_t c = q[q_r % QN];
        int a = (int)(c >> 16 & 0xFFu), b = (int)(int16_t)(c & 0xFFFFu);
        BARRIER();
        q_r++;
        switch (c >> 24) {
        case C_SET:
            if (a < P_NPARAMS)
                apply(a, b);
            break;
        case C_CHORD:
            root = a % 12;
            type = b < CH_NTYPES ? b : 0;
            retune();
            break;
        case C_GATE:
            gate = a;
            if (!om_playing) {                      /* by hand: the chord sounds while the pad is down */
                if (a)
                    held_apply(1);
                else
                    accomp_release();
            } else if (!a) {
                accomp_release();
            }
            break;
        case C_STRUM:
            if (a < OM_NSTR)
                strum(a);
            break;
        case C_PLAY:
            if (a && !om_playing)
                play_start(1);
            else if (!a && om_playing)
                play_stop();
            break;
        case C_BEND:
            bend_target = (float)b;
            break;
        case C_CLOCK:
            om_ext = 1;                             /* a clock message: someone else leads */
            ext_age = 0;
            if (a == OM_CLK_TICK) {
                if (om_playing)
                    clock_pulse();
            } else if (a == OM_CLK_START) {
                play_start(1);
            } else if (a == OM_CLK_CONTINUE) {
                if (!om_playing)
                    play_start(0);
            } else if (om_playing) {
                play_stop();
            }
            break;
        case C_PANIC:
            quiet();
            break;
        }
    }
}

/* ------------------------------------------------------------- render --- */
#define BLK 256
static float mixl[BLK], mixr[BLK], send[BLK];

/* the OM-84's harp: two voices, the shimmer (VOICE 1) and the plain (VOICE 2) */
static void render_classic(str_t *v, int s, uint32_t n, float inc, float g1, float g2)
{
    float pos = v->pos, y1 = v->y1, y2 = v->y2, e = v->v, g = v->g, c = v->c, m = v->m;
    const float gl = pan_l[s], gr = pan_r[s], sr = sus_rate;
    uint32_t j;
    for (j = 0; j < n; j++) {
        float x1, x2, o;
        if (!(j & 15u)) {                       /* every 16 samples: the filter and the shimmer */
            float c96 = (0.15f + 0.85f * e) * v->k;
            int ph = (int)shim_ph + v->shim_off;
            c = 1.0f - fm_exp2f(2.1768707f * fm_log2f(1.0f - c96));   /* 96 kHz coefficient at 44.1 */
            m = (0.61f + 0.61f * sin3[ph % 360]) * v->modamt;
        }
        x1 = wave(OM_WAVE_HARP_BASE, pos) + wave(OM_WAVE_HARP_MOD, pos) * m;
        x2 = wave(OM_WAVE_HARP, pos);
        y1 += c * (x1 - y1);
        y2 += c * (x2 - y2);
        g += 0.05f * (e - g);
        o = (y1 * g1 + y2 * g2) * g;
        mixl[j] += o * gl;
        mixr[j] += o * gr;
        send[j] += o * rsend;
        pos = adv(pos, inc);
        e -= sr * (0.34f + 8.5f * e * e * e);
        if (e <= 0.0f) {
            e = 0.0f;
            if (g < 1e-4f) {
                v->on = 0;
                midi(0x80, v->note, 0);
                break;
            }
        }
    }
    v->pos = pos;
    v->y1 = fm_flush(y1);
    v->y2 = fm_flush(y2);
    v->v = e;
    v->g = g;
    v->c = c;
    v->m = m;
}

/* one of om_voices.h's on a string */
static void render_voice(str_t *v, int s, uint32_t n, float inc, float gain)
{
    const om_inst_t *in = &INST[v->inst];
    const float *W = om_wt[in->wave], *S = om_wt[WT_SINE];
    const float gl = pan_l[s] * gain, gr = pan_r[s] * gain, sr = sus_rate * i_sdk[v->inst], ar = i_ar[v->inst],
                minc = inc * in->ratio, idk = i_idk[v->inst], curve = 8.5f * in->curve, rs = rsend * gain;
    float pos = v->pos, mpos = v->mpos, y = v->y1, e = v->v, g = v->g, c = v->c, I = 0.0f, idec = v->idec;
    int atk = v->atk;
    uint32_t j;
    for (j = 0; j < n; j++) {
        float x, o;
        if (!(j & 15u)) {                       /* every 16 samples: the filter and the FM depth */
            float b = in->bright * (1.0f - in->track * (1.0f - e));
            c = fm_minf(1.0f, inc * 0.0628f * (1.0f + 40.0f * b * b));
            I = in->index * (in->ifloor + (1.0f - in->ifloor) * idec) * 15.915f;
            idec *= idk;
        }
        if (I > 0.0f) {
            float ph;
            mpos = adv(mpos, minc);
            ph = pos + I * wave(S, mpos);
            if (ph < 0.0f)
                ph += 100.0f;
            else if (ph >= 100.0f)
                ph -= 100.0f;
            x = wave(W, ph);
        } else {
            x = wave(W, pos);
        }
        y += c * (x - y);
        g += 0.05f * (e - g);
        o = y * g;
        mixl[j] += o * gl;
        mixr[j] += o * gr;
        send[j] += o * rs;
        pos = adv(pos, inc);
        if (atk) {
            e += ar;
            if (e >= 1.0f) {
                e = 1.0f;
                atk = 0;
            }
        } else {
            e -= sr * (0.34f + curve * e * e * e);
            if (e <= 0.0f) {
                e = 0.0f;
                if (g < 1e-4f) {
                    v->on = 0;
                    midi(0x80, v->note, 0);
                    break;
                }
            }
        }
    }
    v->pos = pos;
    v->mpos = mpos;
    v->y1 = fm_flush(y);
    v->v = e;
    v->g = g;
    v->c = c;
    v->idec = idec;
    v->atk = (uint8_t)atk;
}

static void render_strings(uint32_t n)
{
    /* the OM's two voices: VOICE 1 at 80 and VOICE 2 at 45 (Omni 1), or VOICE 2 alone (Omni 2) */
    const float g1h = 0.8f * (1.0f - 0.5f * 0.45f) * 1.2f * svol, g2h = 0.45f * (1.0f - 0.5f * 0.8f) * 0.85f * 1.2f * svol,
                g2p = 0.85f * 1.2f * svol, vg_step = (float)n / (0.25f * OM_SR);
    int s;
    for (s = 0; s < OM_NSTR; s++) {
        str_t *v = &str[s];
        const om_inst_t *in;
        float cents, inc, tr;
        if (!v->on) {
            om_str_level[s] = 0.0f;
            continue;
        }
        in = &INST[v->inst];
        /* the pitch, this chunk: the bend, VIBRATO (it comes in over a quarter second) and the voice's own */
        cents = (par[P_VIB] ? vib_depth : 0.0f) * v->vg + in->vib;
        v->vg = fm_minf(1.0f, v->vg + vg_step);
        inc = v->inc * bend_fac * (1.0f + cents * 0.000578f * lfo_val);
        tr = 1.0f - in->trem * (0.5f + 0.5f * lfo_val);
        if (v->inst < 2)
            render_classic(v, s, n, inc, v->inst ? 0.0f : g1h, v->inst ? g2p : g2h);
        else
            render_voice(v, s, n, inc, in->gain * svol * tr);
        om_str_level[s] = v->on ? v->v : 0.0f;
    }
    shim_ph += shim_rate * (float)n;
    while (shim_ph >= 360.0f)
        shim_ph -= 360.0f;
}

/* a chord or chord plus voice: the same engine, with a gate */
static float render_av(av_t *v, int ii, uint32_t n, float gain, float sendg)
{
    const om_inst_t *in = &INST[ii];
    const float *W = om_wt[in->wave], *S = om_wt[WT_SINE];
    const float inc = v->inc * (1.0f + in->vib * 0.000578f * lfo_val), minc = inc * in->ratio,
                ar = i_ar[ii], idk = i_idk[ii], gt = gain * in->gain * (1.0f - in->trem * (0.5f + 0.5f * lfo_val)),
                rk = v->os == 1 ? fm_minf(i_rk[ii], 1.0f / (0.18f * OM_SR)) : i_rk[ii],
                dk = v->os == 2 ? fm_maxf(i_cdk[ii], 1.0f / (1.0f * OM_SR)) : i_cdk[ii],
                tgt = v->os ? 0.0f : in->sus, ck = 1.0f / (0.004f * OM_SR);
    float pos = v->pos, mpos = v->mpos, y = v->y, e = v->v, g = v->g, c = 1.0f, I = 0.0f, idec = v->idec;
    uint32_t j;
    for (j = 0; j < n; j++) {
        float x, o;
        if (!(j & 15u)) {
            float b = in->bright * (1.0f - in->track * (1.0f - e));
            c = fm_minf(1.0f, inc * 0.0628f * (1.0f + 40.0f * b * b));
            I = in->index * (in->ifloor + (1.0f - in->ifloor) * idec) * 15.915f;
            idec *= idk;
        }
        switch (v->st) {
        case A_ATK:
            e += ar;
            if (e >= 1.0f) {
                e = 1.0f;
                v->st = v->os == 1 ? A_REL : A_DEC;
            }
            break;
        case A_DEC:
            e += (tgt - e) * dk;
            if (tgt == 0.0f && e < 0.001f)
                e = 0.0f;
            break;
        case A_REL: e -= rk; break;
        case A_CUT: e -= ck; break;
        default: break;
        }
        if (e <= 0.0f) {
            e = 0.0f;
            if (g < 1e-4f) {
                v->st = A_OFF;
                midi(0x80u | v->ch, v->note, 0);
                break;
            }
        }
        if (I > 0.0f) {
            float ph;
            mpos = adv(mpos, minc);
            ph = pos + I * wave(S, mpos);
            if (ph < 0.0f)
                ph += 100.0f;
            else if (ph >= 100.0f)
                ph -= 100.0f;
            x = wave(W, ph);
        } else {
            x = wave(W, pos);
        }
        y += c * (x - y);
        g += 0.05f * (e - g);
        o = y * g * gt;
        mixl[j] += o;
        mixr[j] += o;
        send[j] += o * sendg;
        pos = adv(pos, inc);
    }
    v->pos = pos;
    v->mpos = mpos;
    v->y = fm_flush(y);
    v->v = e;
    v->g = g;
    v->idec = idec;
    return v->st == A_OFF ? 0.0f : e;
}

static float env_step(cv_t *v)                 /* the OM's bass */
{
    switch (v->st) {
    case E_DEC:
        v->v -= (1.0f / OM_SR) / 0.333f;
        if (v->v <= 0.95f) {
            v->v = 0.95f;
            v->st = E_SUS;
        }
        break;
    case E_BDEC:
        v->v -= (1.0f / OM_SR) / 1.222f;
        if (v->v <= 0.95f) {
            v->v = 0.95f;
            v->st = E_REL;
        }
        break;
    case E_REL:
        v->v -= (1.0f / OM_SR) / 0.333f * (0.5f + v->v);
        break;
    case E_CUT:
        v->v -= (1.0f / OM_SR) / 0.004f;
        break;
    default:
        break;
    }
    if (v->v <= 0.0f) {
        v->v = 0.0f;
        if (v->g < 1e-4f) {
            v->st = E_OFF;
            midi(0x82, v->note, 0);
        }
    }
    return v->v;
}

static void render_chord(uint32_t n)
{
    int i;
    uint32_t j;
    /* the voices are levelled on the strum plate; held, the steady ones sit a little under the OM's organ */
    const float gc = 0.4f * cvol * 0.7071f, gb = bvol * 0.7071f, gcv = par[P_CVOICE] < 2 ? gc : gc * 0.65f,
                gpv = (par[P_PVOICE] < 2 ? gc : gc * 0.65f) * 0.8f;
    float lvl = 0.0f;
    for (i = 0; i < OM_NCHORD; i++)
        if (chv[i].st != A_OFF)
            lvl = fm_maxf(lvl, render_av(&chv[i], par[P_CVOICE], n, gcv, csend));
    om_chord_level = lvl;
    for (i = 0; i < OM_NPLUS; i++)
        if (plv[i].st != A_OFF)
            render_av(&plv[i], par[P_PVOICE], n, gpv, csend);
    if (bsv.st != E_OFF) {
        cv_t *v = &bsv;
        for (j = 0; j < n && v->st != E_OFF; j++) {
            float e = env_step(v), o;
            v->g += 0.05f * (e - v->g);
            o = (wave(OM_WAVE_BASS_BASE, v->pos) + wave(OM_WAVE_BASS_MOD, v->pos) * (1.0f - e * e)) * v->g * gb;
            mixl[j] += o;
            mixr[j] += o;
            send[j] += o * csend * 0.5f;               /* a little: a wet bass muddies */
            v->pos = adv(v->pos, v->inc);
        }
    }
    om_bass_level = bsv.st != E_OFF ? bsv.v : 0.0f;
}

static void render_drums(uint32_t n)
{
    int i;
    uint32_t j;
    const float g = rvol * (1.0f / 32768.0f) * 0.7071f;
    for (i = 0; i < OM_NDRUM; i++) {
        drum_t *d = &drum[i];
        const int16_t *w = OM_DRUM[i];
        uint32_t len = OM_DRUM_LEN[i];
        if (!d->on)
            continue;
        for (j = 0; j < n; j++) {
            float o;
            if (d->pos >= len) {
                d->on = 0;
                break;
            }
            o = (float)w[d->pos++] * g;
            mixl[j] += o;
            mixr[j] += o;
        }
    }
}

static void render_chunk(int32_t *out, uint32_t n, float gain)
{
    uint32_t j;
    {   /* the LFO and the bend, once a chunk (a chunk is at most 256 samples, 6 ms) */
        float t = bend_target * (float)par[P_BENDRANGE], a = fm_minf(1.0f, (float)n * (1.0f / 350.0f));
        lfo_ph = adv(lfo_ph, LFO_HZ * 100.0f / OM_SR * (float)n);
        lfo_val = wave(om_wt[WT_SINE], lfo_ph);
        if (bend_c != t || bend_fac != 1.0f) {
            bend_c += (t - bend_c) * a;
            if (fm_fabsf(t - bend_c) < 0.05f)
                bend_c = t;
            bend_fac = fm_exp2f(bend_c * (1.0f / 1200.0f));
        }
    }
    for (j = 0; j < n; j++)
        mixl[j] = mixr[j] = send[j] = 0.0f;
    render_strings(n);
    render_chord(n);
    render_drums(n);
    for (j = 0; j < n; j++) {
        float wl, wr, l, r;
        reverb(send[j] * 0.5f, &wl, &wr);
        l = (mixl[j] + wl) * 0.42f * gain;
        r = (mixr[j] + wr) * 0.42f * gain;
        out[2 * j] = (int32_t)(fm_tanhf(l) * 8300000.0f);
        out[2 * j + 1] = (int32_t)(fm_tanhf(r) * 8300000.0f);
    }
}

void om_render(int32_t *out, uint32_t n, uint32_t gain_q12)
{
    const float gain = (float)gain_q12 * (1.0f / 4096.0f) * 2.0f;
    drain();
    if (om_ext && ++ext_age > 100u)                /* ~0.6 s without a tick: back to its own tempo */
        om_ext = 0;
    while (n) {
        uint32_t k = n > BLK ? BLK : n;
        if (om_playing && !om_ext) {
            if (pulse_left <= 0.0f) {
                clock_pulse();
                pulse_left += step_len * (1.0f / 6.0f);
            }
            if ((float)k > pulse_left)
                k = (uint32_t)pulse_left + 1u;
            if (k > n)
                k = n;
            pulse_left -= (float)k;
        }
        render_chunk(out, k, gain);
        out += 2 * k;
        n -= k;
    }
}
