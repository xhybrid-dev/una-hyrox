"""HybridX Race: the score for "Sixteen".

F minor, 128 BPM, 80 bars. A heartbeat and a ticking clock for the cold
open, a hit per word of the count-in, the drop on the gun, a driving groove
for the race, a breakdown where the film turns to heart rate, and a build
into the last press. Every split in the picture is a click and a buzz here.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (SR, Mix, n, midi, kick, clap, snare, hat, shaker, rim, tick, heartbeat, supersaw, pluck,
                   bass, fm_bell, glass, impact, riser, downlifter, whoosh, click, buzz, sub_boom, sos_filter,
                   pingpong, master, noise, t_axis)
from common import load_cues, draw_sound, sonic_logo, split_sound, finish, report

BPM = 128
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('race')
DUR = cues['duration']
mix = Mix(DUR, tail=5.0)

# i - VI - III - VII, two bars each.
CHORDS = [
    dict(root=n('F2'), pad=['F3', 'Ab3', 'C4', 'F4'], arp=['F5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Db2'), pad=['Db3', 'F3', 'Ab3', 'Db4'], arp=['F5', 'Ab5', 'Db6', 'Ab5']),
    dict(root=n('Ab2'), pad=['Ab3', 'C4', 'Eb4', 'Ab4'], arp=['Eb5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Eb2'), pad=['Eb3', 'G3', 'Bb3', 'Eb4'], arp=['Eb5', 'G5', 'Bb5', 'G5']),
]


def chord_at(bar):
    return CHORDS[(int(bar) // 2) % 4]


# Cached one-shots.
K = kick()
CL = clap()
HH = hat()
OH = hat(open_=True)
SN = snare()
RIM = rim()


def drums(b0, b1, kick_on=True, clap_on=True, hats=2, open_hats=True, shaker_on=False, level=1.0):
    for b in range(b0, b1):
        for beat in range(4):
            t0 = at(b, beat)
            if kick_on:
                mix.add('drums', K, t0, gain=0.8 * level)
                mix.duck(t0)
            if clap_on and beat in (1, 3):
                mix.add('drums', CL, t0, gain=0.5 * level, send=0.18)
            if hats:
                steps = 4 if hats == 2 else 2
                for s in range(steps):
                    acc = [0.62, 0.3, 0.48, 0.3][s] if steps == 4 else [0.55, 0.38][s]
                    mix.add('hats', HH, t0 + s * BEAT / steps, gain=acc * level, pan=0.25)
            if open_hats:
                mix.add('hats', OH, t0 + BEAT / 2, gain=0.16 * level, pan=-0.2, send=0.1)
            if shaker_on:
                mix.add('hats', shaker(), t0 + BEAT * 0.75, gain=0.35 * level, pan=-0.4)


def bassline(b0, b1, level=1.0, cutoff=700.0):
    """Off-beat eighths on the root, pumping against the kick."""
    for b in range(b0, b1):
        r = chord_at(b)['root']
        for beat in range(4):
            note = r + (12 if beat == 3 and b % 2 == 1 else 0)
            mix.add('bass', bass(midi(note), BEAT * 0.45, cutoff=cutoff), at(b, beat + 0.5), gain=0.8 * level)


def pads(b0, b1, level=1.0, cutoff=2200.0, attack=0.3):
    b = b0
    while b < b1:
        c = chord_at(b)
        length = min(2 - (b % 2), b1 - b)
        dur = length * BAR
        for i, nm in enumerate(c['pad']):
            x = supersaw(midi(n(nm)), dur + 0.6, cutoff=cutoff, a=attack, r=0.6)
            mix.add('pad', x, at(b), gain=0.30 * level, send=0.35)
        b += length


def arps(b0, b1, level=1.0, bright=3800.0, octave=0):
    for b in range(b0, b1):
        c = chord_at(b)
        for s in range(16):
            nm = c['arp'][s % 4]
            f = midi(n(nm) + 12 * octave)
            acc = 1.0 if s % 4 == 0 else 0.7
            mix.add('arp', pluck(f, 0.32, bright=bright, decay=0.12), at(b, s / 4), gain=0.42 * level * acc,
                    pan=(-0.35 if s % 2 else 0.35), send=0.2)


def lead(b0, b1, level=1.0):
    """A four-bar hook over the progression, on a bell."""
    motif = [  # (beat offset within 4 bars, note, length in beats)
        (0, 'C6', 1.5), (1.5, 'Ab5', 0.5), (2, 'F5', 2), (4, 'Db6', 1.5), (5.5, 'C6', 0.5), (6, 'Ab5', 2),
        (8, 'Eb6', 1.5), (9.5, 'C6', 0.5), (10, 'Ab5', 2), (12, 'G5', 1), (13, 'Bb5', 1), (14, 'Eb6', 2),
    ]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            mix.add('lead', fm_bell(midi(n(nm)), ln * BEAT + 0.8, ratio=2.0, index=1.4, decay=0.5), t0,
                    gain=0.34 * level, pan=0.1, send=0.4)


# --- Arrangement ----------------------------------------------------------------

# Intro: heartbeat at 64 bpm, a clock tick on every beat, a dark pad.
for b in range(0, 4):
    for beat in (0, 2):
        mix.add('fx', heartbeat(0.8, v=0.55 + 0.12 * b), at(b, beat))
    for beat in range(4):
        mix.add('fx', tick(0.03, 3600), at(b, beat), gain=0.22 + 0.04 * b, pan=0.2)
for nm in ['F2', 'C3', 'F3', 'Ab3']:
    x = supersaw(midi(n(nm)), at(4) + 1.0, cutoff=520, a=2.5, r=1.0, detune=0.12)
    mix.add('pad', x, 0, gain=0.22, send=0.4)

# Count-in: a hit per word, the heartbeat quickening, a snare roll to the gun.
for i, cue in enumerate([c for c in cues['cues'] if c['type'] == 'hit']):
    t0 = cue['t']
    mix.add('fx', impact(2.0, bright=0.7) * 0.5 * cue.get('v', 1), t0, send=0.2)
    mix.add('drums', K, t0, gain=1.0)
    c = CHORDS[i]
    for nm in c['pad']:
        mix.add('stab', supersaw(midi(n(nm) + 12), 0.9, cutoff=5200, a=0.005, r=0.5), t0, gain=0.4, send=0.45)
for b in range(4, 8):
    for beat in range(4):
        mix.add('fx', heartbeat(0.6, v=0.5 + 0.1 * (b - 4)), at(b, beat))
for b in range(5, 7):
    for beat in range(4):
        mix.add('hats', HH, at(b, beat + 0.5), gain=0.3, pan=0.25)
# Snare roll in bar 7: sixteenths, then thirty-seconds, then a breath.
for s in range(8):
    mix.add('drums', SN, at(7, s / 4), gain=0.18 + 0.03 * s, send=0.15)
for s in range(12):
    mix.add('drums', SN, at(7, 2 + s / 8), gain=0.3 + 0.03 * s, send=0.15)
for cue in cues['cues']:
    if cue['type'] == 'riser':
        d = cue['until'] - cue['t']
        mix.add('fx', riser(d, 250, 7000) * 0.5, cue['t'], send=0.3)
    if cue['type'] == 'draw':
        draw_sound(mix, cue['t'])
    if cue['type'] == 'lock':
        sonic_logo(mix, cue['t'], n('F5'))

# The race.
drums(8, 12, clap_on=False, hats=1, open_hats=False)          # title: kick and hats
bassline(8, 44)
pads(8, 44, cutoff=1800)
drums(12, 44, hats=2, open_hats=True)
arps(12, 44, bright=3200)
arps(28, 44, level=0.6, octave=1, bright=5000)                # a second layer from bar 28

# Breakdown: the heart takes over the kick; the pad opens; the arp recedes.
for b in range(44, 50):
    for beat in range(4):
        mix.add('fx', heartbeat(0.5, v=0.9), at(b, beat))
        mix.duck(at(b, beat))
pads(44, 50, cutoff=3200, attack=1.2, level=1.1)
arps(44, 50, level=0.55, bright=1500)
mix.add('fx', riser(at(50) - at(48), 200, 6000) * 0.45, at(48), send=0.3)
for s in range(16):
    mix.add('drums', SN, at(49, s / 4), gain=0.12 + 0.02 * s, send=0.1)

# Rebuild, with the hook.
drums(50, 66, hats=2, open_hats=True, shaker_on=True)
bassline(50, 66)
pads(50, 66, cutoff=2400)
arps(50, 66, bright=4200)
lead(54, 66)

# Final segment: everything, and a build to the last press.
drums(66, 71, hats=2, open_hats=True, shaker_on=True, level=1.05)
for beat in range(3):
    mix.add('drums', K, at(71, beat), gain=1.0)
    mix.duck(at(71, beat))
bassline(66, 71, cutoff=1200)
pads(66, 72, cutoff=3000, level=1.1)
arps(66, 72, bright=5200, level=1.1)
lead(66, 70, level=1.1)
mix.add('fx', riser(at(71, 3.6) - at(69), 200, 9000) * 0.6, at(69), send=0.3)
for s in range(16):
    mix.add('drums', SN, at(70, s / 4), gain=0.14 + 0.015 * s, send=0.12)
for s in range(24):
    mix.add('drums', SN, at(71, s / 8), gain=0.3 + 0.012 * s, send=0.12)

# The finish: an impact, a long chord, the heart slowing.
FIN = [c['t'] for c in cues['cues'] if c['type'] == 'finish'][0]
mix.add('fx', impact(4.0) * 0.8, FIN, send=0.35)
for nm in ['F2', 'C3', 'F3', 'Ab3', 'C4', 'G4']:
    mix.add('pad', supersaw(midi(n(nm)), 12.0, cutoff=2600, a=0.02, r=4.0), FIN, gain=0.2, send=0.5)
mix.add('lead', glass(midi(n('C6')), 4.0) * 0.4, FIN + 0.05, send=0.6)
for i, dt in enumerate([1.0, 2.2, 3.6, 5.2]):
    mix.add('fx', heartbeat(0.9, v=0.45 - 0.07 * i), FIN + dt)
for s in range(24):
    f = midi(n(['F5', 'C6', 'Ab5', 'G5'][s % 4]))
    mix.add('arp', pluck(f, 0.5, bright=2400, decay=0.25), FIN + 1.5 + s * BEAT / 2, gain=0.1 * (1 - s / 26), pan=(-0.4 if s % 2 else 0.4), send=0.5)

# The end card.
for c in cues['cues']:
    if c['type'] == 'logo':
        sonic_logo(mix, c['t'], n('F5'), gain=0.9, big=False)
        for nm in ['F3', 'C4', 'Ab4']:
            mix.add('pad', supersaw(midi(n(nm)), DUR - c['t'] + 2, cutoff=1400, a=1.0, r=3.0), c['t'] - 0.5, gain=0.12, send=0.5)

# Picture-locked effects.
for c in cues['cues']:
    ty, t0 = c['type'], c['t']
    if ty == 'split':
        split_sound(mix, t0, strong=c.get('station', False))
    elif ty == 'whoosh':
        mix.add('fx', whoosh(0.62), t0, gain=0.5, pan=-0.2, send=0.15)
    elif ty == 'click':
        mix.add('fx', click(), t0, gain=0.5, pan=0.1)
        mix.add('fx', buzz(0.18), t0 + 0.02, gain=0.4)
    elif ty == 'thud':
        mix.add('fx', sos_filter(click(), 'lp', 900), t0, gain=0.6)
    elif ty == 'undo':
        mix.add('fx', downlifter(0.8) * 0.5, t0)
        mix.add('fx', tick(0.04, 2400), t0 + 0.3, gain=0.5)
    elif ty == 'toggle':
        mix.add('fx', click(), t0, gain=0.5)
        mix.add('fx', tick(0.03, 4200), t0 + 0.12, gain=0.4)
    elif ty == 'swell':
        mix.add('fx', riser(c['until'] - t0, 400, 5000, tone=False) * 0.35, t0, send=0.3)
    elif ty == 'tick':
        mix.add('fx', tick(0.025, 5200), t0, gain=0.25 * c.get('v', 1), pan=0.3)
    elif ty == 'finishPress':
        split_sound(mix, t0, strong=True)
    elif ty == 'saved':
        mix.add('lead', glass(midi(n('C6')), 2.0) * 0.35, t0, pan=0.2, send=0.5)
        mix.add('lead', glass(midi(n('F6')), 2.0) * 0.3, t0 + 0.12, pan=-0.2, send=0.5)

report(mix)
y = mix.render(ducked=('pad', 'bass', 'arp', 'stab'), depth=0.55)
y = master(y)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'race.wav'), DUR)
