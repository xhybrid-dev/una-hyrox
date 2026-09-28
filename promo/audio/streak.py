"""HybridX Streak: the score for "The climb".

D major, 112 BPM, 70 bars. Warm and patient: a pad under the stars, soft
plucks for every session found, a groove that grows with the climb, a
quiet minor turn for the missed week, a sunrise swell for the fresh start,
and the montage's 104 weeks as a rising cascade of notes.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (SR, Mix, n, midi, soft_kick, kick, clap, hat, shaker, rim, tick, supersaw, pluck, bass, fm_bell,
                   glass, impact, riser, downlifter, whoosh, click, buzz, sub_boom, sos_filter, noise, t_axis, sine)
from common import load_cues, draw_sound, sonic_logo, split_sound, finish, report

BPM = 112
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('streak')
DUR = cues['duration']
mix = Mix(DUR, tail=5.0)
CUES = cues['cues']


def cue(kind):
    return [c for c in CUES if c['type'] == kind]


# I - V - vi - IV, two bars each, with added ninths for warmth.
CHORDS = [
    dict(root='D2', pad=['D3', 'A3', 'E4', 'F#4'], arp=['D5', 'F#5', 'A5', 'E5']),
    dict(root='A1', pad=['A2', 'E3', 'B3', 'C#4'], arp=['C#5', 'E5', 'A5', 'B4']),
    dict(root='B1', pad=['B2', 'F#3', 'C#4', 'D4'], arp=['B4', 'D5', 'F#5', 'C#5']),
    dict(root='G1', pad=['G2', 'D3', 'A3', 'B3'], arp=['B4', 'D5', 'G5', 'A5']),
]
PENTA = ['D5', 'E5', 'F#5', 'A5', 'B5', 'D6', 'E6', 'F#6', 'A6', 'B6', 'D7']


def chord_at(bar, minor_turn=False):
    c = CHORDS[(int(bar) // 2) % 4]
    if minor_turn:
        c = CHORDS[2] if (int(bar) // 2) % 2 == 0 else CHORDS[3]
    return c


KS = soft_kick()
KB = kick(0.5, f0=130, f1=46, decay=0.3, click=0.2, drive=1.2)
CL = clap()
HH = hat()


def drums(b0, b1, level=1.0, clap_on=True, shaker_on=True, hats_on=False, big=False):
    for b in range(b0, b1):
        for beat in range(4):
            t0 = at(b, beat)
            mix.add('drums', KB if big else KS, t0, gain=0.9 * level)
            mix.duck(t0)
            if clap_on and beat in (1, 3):
                mix.add('drums', CL, t0, gain=0.42 * level, send=0.25)
            if shaker_on:
                for s in range(4):
                    mix.add('hats', shaker(0.1), t0 + s * BEAT / 4, gain=[0.8, 0.4, 0.64, 0.4][s] * level, pan=-0.3)
            if hats_on:
                mix.add('hats', HH, t0 + BEAT / 2, gain=0.42 * level, pan=0.3)


def pads(b0, b1, level=1.0, cutoff=1800.0, attack=0.6, minor=False):
    b = b0
    while b < b1:
        c = chord_at(b, minor)
        length = min(2 - (b % 2), b1 - b)
        for nm in c['pad']:
            x = supersaw(midi(n(nm)), length * BAR + 0.8, cutoff=cutoff, a=attack, r=0.8, detune=0.14)
            x = np.vstack([sos_filter(x[0], 'hp', 160), sos_filter(x[1], 'hp', 160)])
            mix.add('pad', x, at(b), gain=0.28 * level, send=0.45)
        b += length


def bassline(b0, b1, level=1.0, minor=False):
    for b in range(b0, b1):
        r = n(chord_at(b, minor)['root']) + 12
        for beat, ln in ((0, 1.4), (1.5, 0.4), (2, 1.4), (3.5, 0.4)):
            mix.add('bass', bass(midi(r), ln * BEAT, cutoff=520, env_amt=2.0, sub=0.9), at(b, beat), gain=0.62 * level)


def plucks(b0, b1, level=1.0, minor=False, rate=2):
    """Marimba-ish triangle plucks, eighths (rate 2) or sixteenths (rate 4)."""
    for b in range(b0, b1):
        c = chord_at(b, minor)
        for s in range(4 * rate):
            nm = c['arp'][s % 4]
            mix.add('arp', pluck(midi(n(nm)), 0.4, bright=2600, decay=0.16, wave='tri'), at(b, s / rate),
                    gain=0.34 * level * (1.0 if s % 4 == 0 else 0.75), pan=(-0.3 if s % 2 else 0.3), send=0.3)


def melody(b0, b1, level=1.0):
    """A hopeful four-bar hook, on a bell."""
    motif = [(0, 'A5', 1), (1, 'F#5', 1), (2, 'E5', 0.5), (2.5, 'D5', 1.5), (4, 'E5', 1), (5, 'F#5', 1), (6, 'A5', 2),
             (8, 'B5', 1), (9, 'A5', 1), (10, 'F#5', 2), (12, 'G5', 1), (13, 'F#5', 1), (14, 'E5', 2)]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            mix.add('lead', fm_bell(midi(n(nm)), ln * BEAT + 1.0, ratio=2.0, index=1.2, decay=0.6), t0,
                    gain=0.3 * level, pan=0.1, send=0.45)


# --- Arrangement ---------------------------------------------------------------

# Intro under the stars: a pad, sparse glass pings, the words, the pips.
pads(0, 6, cutoff=900, attack=2.5, level=0.9)
rng = np.random.default_rng(5)
for i in range(18):
    tt = rng.uniform(0.5, at(5.5))
    nm = PENTA[int(rng.integers(4, len(PENTA)))]
    mix.add('lead', glass(midi(n(nm)), 1.5) * 0.12, tt, pan=float(rng.uniform(-0.7, 0.7)), send=0.8)
for b in (1, 2):
    mix.add('fx', riser(1.0, 300, 3000, tone=False) * 0.12, at(b) - 1.0, send=0.4)
    mix.add('pad', supersaw(midi(n('D4')), 2.5, cutoff=2400, a=0.02, r=1.5), at(b), gain=0.12, send=0.6)
for i in range(3):
    mix.add('fx', tick(0.03, 3000 + i * 400), at(4) + i * 0.18, gain=0.35)
mix.add('fx', riser(at(6) - at(5), 250, 6000) * 0.4, at(5), send=0.3)
for c in cue('draw'):
    draw_sound(mix, c['t'])
for c in cue('lock'):
    sonic_logo(mix, c['t'], n('D5'))
    mix.add('fx', whoosh(0.9) * 0.4, c['t'] + 0.05, send=0.3)

# Title: pad and a pulse.
pads(6, 9, cutoff=1600)
bassline(7, 9, level=0.7)

# Counts & coach: the groove arrives.
drums(9, 21, clap_on=True, shaker_on=True)
bassline(9, 21)
pads(9, 21, cutoff=1900)
plucks(9, 21, rate=2)
plucks(13, 21, level=0.5, rate=4)

# Week complete: a bigger beat, the hook.
drums(21, 25, big=True, hats_on=True)
bassline(21, 25)
pads(21, 25, cutoff=2600, level=1.1)
plucks(21, 25, rate=4)
melody(21, 25)

# The ladder: build, then the summit.
drums(25, 31, big=True, hats_on=True)
bassline(25, 31)
pads(25, 31, cutoff=2400)
plucks(25, 31, rate=4)
melody(27, 31, level=0.9)

# Life happens: the beat drops away; the minor turn.
pads(31, 36, cutoff=1200, attack=1.2, minor=True, level=1.1)
plucks(31, 36, level=0.55, minor=True, rate=2)
for b in range(31, 36):
    mix.add('drums', KS, at(b), gain=0.5)
    mix.duck(at(b))

# Fresh start: the sun comes up.
pads(36, 41, cutoff=2800, attack=2.0, level=1.2)
bassline(38, 41, level=0.7)
plucks(38, 41, level=0.7, rate=2)
for c in cue('sunrise'):
    mix.add('fx', riser(c['until'] - c['t'], 150, 4500, tone=False) * 0.35, c['t'], send=0.5)
    mix.add('lead', glass(midi(n('A5')), 4.0) * 0.3, c['until'], send=0.7)
    mix.add('lead', glass(midi(n('D6')), 4.0) * 0.25, c['until'] + 0.1, send=0.7)

# The montage: 104 weeks as a rising cascade, then the climb, layer on layer.
pads(41, 45, cutoff=1500, level=0.9)
for c in cue('week'):
    w = c['w']
    nm = PENTA[min(len(PENTA) - 1, w * len(PENTA) // 104)]
    mix.add('arp', pluck(midi(n(nm)), 0.3, bright=4200, decay=0.1, wave='tri'), c['t'], gain=0.24,
            pan=float(np.sin(w * 0.7) * 0.6), send=0.35)
for c in cue('fly'):
    mix.add('fx', whoosh(c['until'] - c['t'], up=True) * 0.55, c['t'], send=0.3)
drums(45, 53, big=True, hats_on=True)
bassline(45, 53)
pads(45, 53, cutoff=2800, level=1.1)
plucks(45, 53, rate=4)
melody(45, 53, level=1.0)
for c in cue('passSummit'):
    mix.add('fx', impact(1.8, bright=0.6) * 0.25, c['t'], send=0.3)
    mix.add('lead', glass(midi(n(PENTA[3 + c['i']])), 2.5) * 0.3, c['t'], send=0.6)

# Trophy case and the glance: lighter.
drums(53, 59, clap_on=True, shaker_on=True, level=0.75)
bassline(53, 59, level=0.8)
pads(53, 59, cutoff=2000)
plucks(53, 59, rate=2, level=0.8)
for i, c in enumerate(cue('badge')):
    mix.add('lead', fm_bell(midi(n(['D6', 'F#6', 'A6', 'D7'][i])), 1.5, ratio=3.0, index=1.0, decay=0.6) * 0.3, c['t'], send=0.5)

# Everest: the peak.
for c in cue('everest'):
    mix.add('fx', impact(3.5) * 0.7, c['t'], send=0.35)
    mix.add('fx', riser(1.6, 400, 8000, tone=False) * 0.3, c['t'] - 1.6)
drums(59, 65, big=True, hats_on=True, level=1.05)
bassline(59, 65)
pads(59, 65, cutoff=3200, level=1.2)
plucks(59, 65, rate=4, level=1.1)
melody(59, 65, level=1.2)
for c in cue('haptic'):
    mix.add('fx', buzz(0.3) * 0.7, c['t'], pan=0.1)

# End card.
for c in cue('logo'):
    sonic_logo(mix, c['t'], n('D5'), gain=0.9, big=False)
    for nm in ['D3', 'A3', 'E4', 'F#4']:
        mix.add('pad', supersaw(midi(n(nm)), DUR - c['t'] + 2, cutoff=1500, a=1.0, r=3.0), c['t'] - 0.6, gain=0.14, send=0.55)

# Picture-locked effects.
for c in CUES:
    ty, t0 = c['type'], c['t']
    if ty == 'whoosh':
        mix.add('fx', whoosh(0.65) * 0.45, t0, send=0.2)
    elif ty == 'session':
        v = c.get('v', 1)
        mix.add('fx', tick(0.03, 4200), t0, gain=0.4 * v, pan=0.2)
        mix.add('fx', buzz(0.12) * 0.45 * v, t0 + 0.01)
        mix.add('lead', glass(midi(n('A5')), 1.2) * 0.22 * v, t0 + 0.02, send=0.5)
    elif ty == 'coach':
        mood = c['mood']
        notes = {'soft': ['F#5'], 'risk': ['G5', 'C#6'], 'win': ['D6', 'A6']}[mood]
        for i, nm in enumerate(notes):
            mix.add('lead', fm_bell(midi(n(nm)), 1.6, ratio=2.0, index=1.0, decay=0.6) * 0.22, t0 + i * 0.08, send=0.5)
    elif ty == 'weekComplete':
        split_sound(mix, t0, strong=True)
        mix.add('fx', buzz(0.2) * 0.7, t0 + 0.18)
        for i, nm in enumerate(['D5', 'F#5', 'A5', 'D6']):
            mix.add('lead', glass(midi(n(nm)), 2.0) * 0.28, t0 + 0.05 + i * 0.07, pan=-0.3 + i * 0.2, send=0.55)
    elif ty == 'burst':
        mix.add('fx', whoosh(0.7) * 0.5, t0 - 0.05, send=0.4)
        mix.add('fx', sub_boom(1.5) * 0.35, t0)
    elif ty == 'summit':
        mix.add('fx', impact(2.8) * 0.55, t0, send=0.3)
        for i, nm in enumerate(['A5', 'D6', 'F#6', 'A6']):
            mix.add('lead', glass(midi(n(nm)), 2.5) * 0.25, t0 + i * 0.09, pan=-0.4 + i * 0.27, send=0.6)
        for k in range(22):
            tt = t0 + 0.2 + rng.uniform(0, 2.2)
            mix.add('lead', glass(midi(n(PENTA[int(rng.integers(5, len(PENTA)))])), 0.8) * 0.06, tt,
                    pan=float(rng.uniform(-0.8, 0.8)), send=0.7)
    elif ty == 'shieldPress':
        mix.add('fx', click() * 0.5, t0, pan=0.3)
        mix.add('fx', sub_boom(1.2, f0=70, f1=40) * 0.3, t0 + 0.05)
        mix.add('lead', glass(midi(n('D6')), 2.5) * 0.3, t0 + 0.1, send=0.6)
    elif ty == 'reset':
        mix.add('fx', tick(0.04, 1800), t0, gain=0.4)

report(mix)
y = mix.render(ducked=('pad', 'bass', 'arp'), depth=0.45)
from synth import master
y = master(y)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'streak.wav'), DUR)
