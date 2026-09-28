"""HybridX Streak, the reel: "The climb" in 34 seconds.

D major, 112 BPM, 16 bars. A pad under the stars and a pluck for each
session that lands, the sonic logo, a groove for the week banked, a hush for
the missed week, then the climb: 104 weeks as a rising cascade and the full
beat to Everest's summit.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (Mix, n, midi, soft_kick, kick, clap, hat, shaker, tick, supersaw, pluck, bass, fm_bell, glass,
                   impact, riser, whoosh, click, buzz, sub_boom, sos_filter, master)
from common import load_cues, draw_sound, sonic_logo, split_sound, finish, report

BPM = 112
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('streak-reel')
DUR = cues['duration']
CUES = cues['cues']
mix = Mix(DUR, tail=4.0)
rng = np.random.default_rng(5)


def cue(kind):
    return [c for c in CUES if c['type'] == kind]


CHORDS = [
    dict(root='D2', pad=['D3', 'A3', 'E4', 'F#4'], arp=['D5', 'F#5', 'A5', 'E5']),
    dict(root='A1', pad=['A2', 'E3', 'B3', 'C#4'], arp=['C#5', 'E5', 'A5', 'B4']),
    dict(root='B1', pad=['B2', 'F#3', 'C#4', 'D4'], arp=['B4', 'D5', 'F#5', 'C#5']),
    dict(root='G1', pad=['G2', 'D3', 'A3', 'B3'], arp=['B4', 'D5', 'G5', 'A5']),
]
PENTA = ['D5', 'E5', 'F#5', 'A5', 'B5', 'D6', 'E6', 'F#6', 'A6', 'B6', 'D7']


def chord_at(bar, minor=False):
    c = CHORDS[(int(bar) // 2) % 4]
    if minor:
        c = CHORDS[2] if (int(bar) // 2) % 2 == 0 else CHORDS[3]
    return c


KS = soft_kick()
KB = kick(0.5, f0=130, f1=46, decay=0.3, click=0.2, drive=1.2)
CL = clap()
HH = hat()


def drums(b0, b1, level=1.0, big=False, hats_on=False):
    for b in range(b0, b1):
        for beat in range(4):
            t0 = at(b, beat)
            mix.add('drums', KB if big else KS, t0, gain=0.9 * level)
            mix.duck(t0)
            if beat in (1, 3):
                mix.add('drums', CL, t0, gain=0.42 * level, send=0.25)
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
    for b in range(b0, b1):
        c = chord_at(b, minor)
        for s in range(4 * rate):
            mix.add('arp', pluck(midi(n(c['arp'][s % 4])), 0.4, bright=2600, decay=0.16, wave='tri'), at(b, s / rate),
                    gain=0.34 * level * (1.0 if s % 4 == 0 else 0.75), pan=(-0.3 if s % 2 else 0.3), send=0.3)


def melody(b0, b1, level=1.0):
    motif = [(0, 'A5', 1), (1, 'F#5', 1), (2, 'E5', 0.5), (2.5, 'D5', 1.5), (4, 'E5', 1), (5, 'F#5', 1), (6, 'A5', 2),
             (8, 'B5', 1), (9, 'A5', 1), (10, 'F#5', 2), (12, 'G5', 1), (13, 'F#5', 1), (14, 'E5', 2)]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            mix.add('lead', fm_bell(midi(n(nm)), ln * BEAT + 1.0, ratio=2.0, index=1.2, decay=0.6), t0, gain=0.3 * level, pan=0.1, send=0.45)


# Hook: under the stars, the plucks already moving so the first frame has sound.
pads(0, 2, cutoff=1100, attack=0.3, level=0.9)
plucks(0, 2, level=0.6, rate=2)
for b in range(0, 2):
    for beat in range(4):
        mix.add('drums', KS, at(b, beat), gain=0.45)
mix.add('fx', riser(at(2) - at(1), 250, 6000) * 0.4, at(1), send=0.3)
for c in cue('draw'):
    draw_sound(mix, c['t'])
for c in cue('lock'):
    sonic_logo(mix, c['t'], n('D5'))
    mix.add('fx', whoosh(0.9) * 0.4, c['t'] + 0.05, send=0.3)

# Title, then the week banked.
pads(2, 4, cutoff=1600)
bassline(3, 4, level=0.7)
drums(4, 6, big=True, hats_on=True)
bassline(4, 6)
pads(4, 6, cutoff=2600, level=1.1)
plucks(4, 6, rate=4)
melody(4, 6)

# Life happens: the beat drops away, the minor turn.
pads(6, 8, cutoff=1200, attack=0.8, minor=True, level=1.1)
plucks(6, 8, level=0.55, minor=True, rate=2)
for b in range(6, 8):
    for beat in (0, 2):
        mix.add('drums', KS, at(b, beat), gain=0.5)
        mix.duck(at(b, beat))

# The climb: the cascade, then everything.
pads(8, 10, cutoff=1500, level=0.9)
for c in cue('week'):
    w = c['w']
    nm = PENTA[min(len(PENTA) - 1, w * len(PENTA) // 104)]
    mix.add('arp', pluck(midi(n(nm)), 0.3, bright=4200, decay=0.1, wave='tri'), c['t'], gain=0.22, pan=float(np.sin(w * 0.7) * 0.6), send=0.35)
for c in cue('fly'):
    mix.add('fx', whoosh(c['until'] - c['t'], up=True) * 0.55, c['t'], send=0.3)
drums(9, 12, big=True, hats_on=True)
bassline(9, 12)
pads(10, 12, cutoff=2800, level=1.1)
plucks(9, 12, rate=4)
melody(9, 12, level=1.0)
for c in cue('passSummit'):
    mix.add('fx', impact(1.6, bright=0.6) * 0.22, c['t'], send=0.3)
    mix.add('lead', glass(midi(n(PENTA[3 + c['i']])), 2.0) * 0.28, c['t'], send=0.6)
mix.add('fx', riser(at(12) - at(11), 400, 8000, tone=False) * 0.35, at(11))

# Everest.
for c in cue('everest'):
    mix.add('fx', impact(3.5) * 0.7, c['t'], send=0.35)
    for i, nm in enumerate(['A5', 'D6', 'F#6', 'A6']):
        mix.add('lead', glass(midi(n(nm)), 2.5) * 0.25, c['t'] + i * 0.09, pan=-0.4 + i * 0.27, send=0.6)
    for k in range(22):
        mix.add('lead', glass(midi(n(PENTA[int(rng.integers(5, len(PENTA)))])), 0.8) * 0.06, c['t'] + 0.2 + rng.uniform(0, 2.4),
                pan=float(rng.uniform(-0.8, 0.8)), send=0.7)
drums(12, 14, big=True, hats_on=True, level=1.05)
bassline(12, 14)
pads(12, 14, cutoff=3200, level=1.2)
plucks(12, 14, rate=4, level=1.1)
melody(12, 14, level=1.2)
for c in cue('haptic'):
    mix.add('fx', buzz(0.3) * 0.7, c['t'], pan=0.1)

# End card.
for c in cue('logo'):
    sonic_logo(mix, c['t'], n('D5'), gain=0.95, big=False)
    for nm in ['D3', 'A3', 'E4', 'F#4']:
        mix.add('pad', supersaw(midi(n(nm)), DUR - c['t'] + 2, cutoff=1500, a=0.8, r=2.0), c['t'] - 0.6, gain=0.14, send=0.55)

for c in CUES:
    ty, t0 = c['type'], c['t']
    if ty == 'whoosh':
        mix.add('fx', whoosh(0.65) * 0.45, t0, send=0.2)
    elif ty == 'session':
        v = c.get('v', 1)
        mix.add('fx', tick(0.03, 4200), t0, gain=0.4 * v, pan=0.2)
        mix.add('fx', buzz(0.12) * 0.45 * v, t0 + 0.01)
        mix.add('lead', glass(midi(n('A5')), 1.2) * 0.22 * v, t0 + 0.02, send=0.5)
    elif ty == 'weekComplete':
        split_sound(mix, t0, strong=True)
        for i, nm in enumerate(['D5', 'F#5', 'A5', 'D6']):
            mix.add('lead', glass(midi(n(nm)), 2.0) * 0.28, t0 + 0.05 + i * 0.07, pan=-0.3 + i * 0.2, send=0.55)
    elif ty == 'burst':
        mix.add('fx', whoosh(0.7) * 0.5, t0 - 0.05, send=0.4)
        mix.add('fx', sub_boom(1.5) * 0.35, t0)
    elif ty == 'shieldPress':
        mix.add('fx', click() * 0.5, t0, pan=0.3)
        mix.add('fx', sub_boom(1.2, f0=70, f1=40) * 0.3, t0 + 0.05)
        mix.add('lead', glass(midi(n('D6')), 2.5) * 0.3, t0 + 0.1, send=0.6)

report(mix)
y = master(mix.render(ducked=('pad', 'bass', 'arp'), depth=0.45))
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'streak-reel.wav'), DUR)
