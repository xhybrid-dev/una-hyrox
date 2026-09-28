"""HybridX Trail: the score for "Follow the line".

E minor, 96 BPM, 60 bars. Spacious: wind, a deep pulse, a plucked arpeggio
with a long ping-pong delay that stands for the line, soft half-time drums.
Going off course turns it: a low drone a tritone away and a pulsing alarm,
until the line is found again and the chord resolves.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (SR, Mix, n, midi, soft_kick, kick, clap, hat, shaker, rim, tick, supersaw, pluck, bass, fm_bell,
                   glass, impact, riser, downlifter, whoosh, click, buzz, sub_boom, sos_filter, sweep_filter, noise,
                   t_axis, sine, pingpong, master)
from common import load_cues, draw_sound, sonic_logo, split_sound, finish, report

BPM = 96
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('trail')
DUR = cues['duration']
mix = Mix(DUR, tail=6.0)
CUES = cues['cues']


def cue(kind):
    return [c for c in CUES if c['type'] == kind]


CHORDS = [
    dict(root='E1', pad=['E3', 'B3', 'D4', 'F#4', 'G4'], arp=['E5', 'B5', 'F#5', 'G5', 'D6', 'B5']),
    dict(root='C2', pad=['C3', 'G3', 'B3', 'E4'], arp=['E5', 'G5', 'B5', 'C6', 'G5', 'E5']),
    dict(root='G1', pad=['G2', 'D3', 'B3', 'E4'], arp=['D5', 'G5', 'B5', 'E6', 'B5', 'G5']),
    dict(root='D2', pad=['D3', 'A3', 'F#4', 'G4'], arp=['D5', 'F#5', 'A5', 'D6', 'A5', 'F#5']),
]


def chord_at(bar):
    return CHORDS[(int(bar) // 2) % 4]


KS = soft_kick()
KD = kick(0.6, f0=120, f1=40, decay=0.4, click=0.1, drive=1.1)
RIM = rim()


def hp(x, f=180):
    return np.vstack([sos_filter(x[0], 'hp', f), sos_filter(x[1], 'hp', f)])


def pads(b0, b1, level=1.0, cutoff=1500.0, attack=1.5):
    b = b0
    while b < b1:
        c = chord_at(b)
        length = min(2 - (b % 2), b1 - b)
        for nm in c['pad']:
            mix.add('pad', hp(supersaw(midi(n(nm)), length * BAR + 1.2, cutoff=cutoff, a=attack, r=1.2, detune=0.1)),
                    at(b), gain=0.22 * level, send=0.6)
        b += length


def sub(b0, b1, level=1.0):
    """A deep pulse on the root, twice a bar."""
    for b in range(b0, b1):
        r = midi(n(chord_at(b)['root']) + 12)
        for beat in (0, 2.5):
            t = t_axis(BEAT * 1.4)
            x = np.sin(2 * np.pi * r * t) * np.minimum(1, t / 0.01) * np.exp(-t / 0.7) * 0.8
            mix.add('bass', x, at(b, beat), gain=0.58 * level)


def line_arp(b0, b1, level=1.0, octave=0, bright=2400.0, detune_off=False):
    """The line: plucked eighths through a dotted-eighth ping-pong delay."""
    for b in range(b0, b1):
        c = chord_at(b)
        for s in range(8):
            nm = c['arp'][s % 6]
            f = midi(n(nm) + 12 * octave)
            if detune_off:
                f *= 2 ** (0.5 / 12)  # a quarter-tone sour, for off course
            x = pluck(f, 0.5, bright=bright, decay=0.18, wave='tri')
            mix.add('arp', x, at(b, s / 2), gain=0.26 * level * (1 if s % 2 == 0 else 0.7), pan=(-0.25 if s % 2 else 0.25), send=0.35)


def drums(b0, b1, level=1.0, rim_on=True, shaker_on=True):
    """Half-time: kick on 1 and the and of 2, a rim on 3."""
    for b in range(b0, b1):
        for beat in (0, 1.5):
            mix.add('drums', KS, at(b, beat), gain=0.85 * level)
            mix.duck(at(b, beat))
        if rim_on:
            mix.add('drums', RIM, at(b, 2), gain=0.4 * level, pan=0.2, send=0.3)
        if shaker_on:
            for s in range(8):
                mix.add('hats', shaker(0.12), at(b, s / 2), gain=(0.95 if s % 2 else 0.55) * level, pan=-0.35)


def wind(t0, t1, level=1.0):
    d = t1 - t0
    x = sweep_filter(noise(d), 'bp', 300, 900, q=0.8) * 0.5 + sweep_filter(noise(d), 'bp', 900, 350, q=0.8) * 0.4
    t = t_axis(d)
    env = np.minimum(1, t / 2.0) * np.minimum(1, (d - t) / 2.0) * (0.6 + 0.4 * np.sin(2 * np.pi * t / 7.3))
    L = x * env
    R = np.roll(L, int(0.013 * SR))
    mix.add('fx', np.vstack([L, R]), t0, gain=0.35 * level, send=0.4)


def melody(b0, b1, level=1.0):
    motif = [(0, 'B5', 2), (2, 'G5', 1), (3, 'F#5', 1), (4, 'E5', 3), (8, 'D6', 2), (10, 'B5', 1), (11, 'A5', 1),
             (12, 'G5', 3)]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            mix.add('lead', glass(midi(n(nm)), ln * BEAT + 1.5) * 0.9, t0, gain=0.3 * level, pan=0.15, send=0.6)


# --- Arrangement --------------------------------------------------------------

# Intro: wind over the fells, a pad, the words typed, then the line.
wind(0, at(10), level=1.0)
pads(0, 6, cutoff=900, attack=3.0, level=0.9)
for c in cue('type'):
    for i in range(c.get('n', 12)):
        mix.add('fx', tick(0.02, 2600 + (i % 3) * 300), c['t'] + i * 0.055, gain=0.18, pan=-0.3)
for c in cue('line'):
    if c['t'] < 30:
        mix.add('fx', riser(4.0, 200, 2500, tone=False) * 0.18, c['t'], send=0.5)
line_arp(4, 6, level=0.6, bright=1600)
mix.add('fx', riser(at(6) - at(5), 250, 6000) * 0.35, at(5), send=0.4)
for c in cue('draw'):
    draw_sound(mix, c['t'])
for c in cue('lock'):
    sonic_logo(mix, c['t'], n('E5'))

# Title and the file.
pads(6, 19, cutoff=1500)
sub(6, 19)
line_arp(6, 19, level=0.9)
drums(9, 19, level=0.8)
mix.add('fx', whoosh(1.0) * 0.3, cue('thin')[0]['t'] if cue('thin') else at(16), send=0.4)
for c in cue('thin'):
    mix.add('fx', downlifter(c['until'] - c['t']) * 0.35, c['t'], send=0.4)
for c in cue('copy'):
    mix.add('fx', riser(c['until'] - c['t'], 400, 3000, tone=False) * 0.2, c['t'])
    mix.add('lead', glass(midi(n('B5')), 1.8) * 0.3, c['until'], send=0.6)

# USB and the list: lighter.
pads(19, 25, cutoff=1300)
sub(19, 25, level=0.8)
line_arp(19, 25, level=0.7)
drums(19, 25, level=0.55, shaker_on=True, rim_on=False)

# Zoom and heading: the full groove, the line an octave up.
pads(25, 35, cutoff=2000)
sub(25, 35)
line_arp(25, 35, level=0.9)
line_arp(29, 35, level=0.5, octave=1, bright=3600)
drums(25, 35, level=1.0)
melody(29, 35, level=0.8)

# Off course: a low drone a tritone from home, the line gone sour.
off = cue('offCourse')[0]['t']
back = cue('backOnCourse')[0]['t']
pads(35, 43, cutoff=1100, attack=2.0, level=0.8)
sub(35, 43, level=0.7)
drums(35, 43, level=0.7, shaker_on=False)
for b in range(35, 43):
    sour = at(b) >= off - 0.5 and at(b) < back
    line_arp(b, b + 1, level=0.8, bright=1200 if sour else 2200, detune_off=sour)
for c in cue('tension'):
    d = c['until'] - c['t']
    t = t_axis(d + 1.0)
    drone = (np.sin(2 * np.pi * midi(n('A#1')) * t) + 0.5 * np.sin(2 * np.pi * midi(n('A#2')) * t)) * np.minimum(1, t / d) ** 2
    mix.add('fx', drone * 0.25, c['t'], send=0.3)
mix.add('fx', impact(2.5, bright=0.6) * 0.45, off, send=0.3)
mix.add('fx', buzz(0.5) * 1.0, off + 0.02, pan=0.2)
mix.add('fx', buzz(0.3) * 0.8, off + 0.62, pan=0.2)
# The alarm: a pulsing two-note figure while you're off the line.
k = 0
tt = off + 0.2
while tt < back - 0.1:
    mix.add('lead', fm_bell(midi(n('A#5' if k % 2 else 'E6')), 0.5, ratio=3.5, index=2.0, decay=0.2) * 0.25, tt, pan=0.3, send=0.3)
    tt += BEAT / 2
    k += 1
# Back on the line: resolve.
mix.add('fx', buzz(0.2) * 0.6, back + 0.02)
for i, nm in enumerate(['E5', 'G5', 'B5', 'E6']):
    mix.add('lead', glass(midi(n(nm)), 3.0) * 0.28, back + 0.05 + i * 0.08, pan=-0.3 + i * 0.2, send=0.6)

# Along the line, and the run.
pads(43, 51, cutoff=1800)
sub(43, 51)
line_arp(43, 51, level=0.9)
drums(43, 51, level=0.95)
for c in cue('lap'):
    split_sound(mix, c['t'], strong=False)

# Finale: wide, the melody, the loop completed.
pads(51, 57, cutoff=2600, level=1.2)
sub(51, 57)
line_arp(51, 57, level=1.0)
line_arp(51, 57, level=0.55, octave=1, bright=4200)
drums(51, 56, level=1.05)
melody(51, 57, level=1.1)
for c in cue('complete'):
    mix.add('fx', impact(3.0, bright=0.7) * 0.5, c['t'], send=0.4)
    for i, nm in enumerate(['E6', 'B5', 'G5', 'E5']):
        mix.add('lead', glass(midi(n(nm)), 2.5) * 0.25, c['t'] + i * 0.12, pan=0.3 - i * 0.2, send=0.7)
wind(at(54), DUR + 2, level=0.8)

# End card.
for c in cue('logo'):
    sonic_logo(mix, c['t'], n('E5'), gain=0.9, big=False)
    for nm in ['E3', 'B3', 'F#4', 'G4']:
        mix.add('pad', hp(supersaw(midi(n(nm)), DUR - c['t'] + 2, cutoff=1300, a=1.0, r=3.0)), c['t'] - 0.6, gain=0.14, send=0.6)

# Picture-locked effects.
for c in CUES:
    ty, t0 = c['type'], c['t']
    if ty == 'whoosh':
        mix.add('fx', whoosh(1.0) * 0.45, t0, send=0.3)
    elif ty == 'click':
        mix.add('fx', click() * 0.4, t0, pan=0.3)
    elif ty == 'select':
        mix.add('fx', click() * 0.5, t0, pan=0.3)
        mix.add('lead', glass(midi(n('E6')), 1.5) * 0.25, t0 + 0.05, send=0.5)
    elif ty == 'zoom':
        mix.add('fx', click() * 0.4, t0, pan=-0.3)
        mix.add('fx', whoosh(0.5, up=c['i'] % 2 == 0) * 0.25, t0 + 0.02, send=0.3)
    elif ty == 'toggle':
        mix.add('fx', click() * 0.45, t0)
        mix.add('fx', tick(0.03, 3800), t0 + 0.1, gain=0.35)

report(mix)
# The line's delay: a dotted-eighth ping-pong on the arp bus only.
y = mix.render(ducked=('pad', 'bass', 'arp'), depth=0.35, verb_seconds=3.6,
               bus_fx={'arp': lambda x: pingpong(x, BEAT * 0.75, feedback=0.5, taps=6, wet=0.45)})
y = master(y)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'trail.wav'), DUR)
