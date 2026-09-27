"""HybridX Race, the reel: "Sixteen" in 34 seconds.

F minor, 128 BPM, 18 bars, the film's score cut to its hooks: a hit per word
of the count-in, a snare roll to the gun, the drop on the lock, the driving
groove with the hook over it, a build into the last press, the finish, and
the sonic logo on the end card.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (Mix, n, midi, kick, clap, snare, hat, shaker, tick, supersaw, pluck, bass, fm_bell, glass,
                   impact, riser, downlifter, whoosh, click, buzz, sos_filter, master)
from common import load_cues, draw_sound, sonic_logo, split_sound, finish, report

BPM = 128
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('race-reel')
DUR = cues['duration']
CUES = cues['cues']
mix = Mix(DUR, tail=4.0)


def cue(kind):
    return [c for c in CUES if c['type'] == kind]


CHORDS = [
    dict(root=n('F2'), pad=['F3', 'Ab3', 'C4', 'F4'], arp=['F5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Db2'), pad=['Db3', 'F3', 'Ab3', 'Db4'], arp=['F5', 'Ab5', 'Db6', 'Ab5']),
    dict(root=n('Ab2'), pad=['Ab3', 'C4', 'Eb4', 'Ab4'], arp=['Eb5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Eb2'), pad=['Eb3', 'G3', 'Bb3', 'Eb4'], arp=['Eb5', 'G5', 'Bb5', 'G5']),
]


def chord_at(bar):
    return CHORDS[(int(bar) // 2) % 4]


K, CL, HH, OH, SN = kick(), clap(), hat(), hat(open_=True), snare()


def drums(b0, b1, clap_on=True, hats=2, shaker_on=False, level=1.0):
    for b in range(b0, b1):
        for beat in range(4):
            t0 = at(b, beat)
            mix.add('drums', K, t0, gain=0.8 * level)
            mix.duck(t0)
            if clap_on and beat in (1, 3):
                mix.add('drums', CL, t0, gain=0.5 * level, send=0.18)
            steps = 4 if hats == 2 else 2
            for s in range(steps):
                acc = [0.62, 0.3, 0.48, 0.3][s] if steps == 4 else [0.55, 0.38][s]
                mix.add('hats', HH, t0 + s * BEAT / steps, gain=acc * level, pan=0.25)
            mix.add('hats', OH, t0 + BEAT / 2, gain=0.16 * level, pan=-0.2, send=0.1)
            if shaker_on:
                mix.add('hats', shaker(), t0 + BEAT * 0.75, gain=0.35 * level, pan=-0.4)


def bassline(b0, b1, level=1.0, cutoff=700.0):
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
        for nm in c['pad']:
            mix.add('pad', supersaw(midi(n(nm)), length * BAR + 0.6, cutoff=cutoff, a=attack, r=0.6), at(b), gain=0.30 * level, send=0.35)
        b += length


def arps(b0, b1, level=1.0, bright=3800.0, octave=0):
    for b in range(b0, b1):
        c = chord_at(b)
        for s in range(16):
            f = midi(n(c['arp'][s % 4]) + 12 * octave)
            mix.add('arp', pluck(f, 0.32, bright=bright, decay=0.12), at(b, s / 4), gain=0.42 * level * (1.0 if s % 4 == 0 else 0.7),
                    pan=(-0.35 if s % 2 else 0.35), send=0.2)


def lead(b0, b1, level=1.0):
    motif = [(0, 'C6', 1.5), (1.5, 'Ab5', 0.5), (2, 'F5', 2), (4, 'Db6', 1.5), (5.5, 'C6', 0.5), (6, 'Ab5', 2),
             (8, 'Eb6', 1.5), (9.5, 'C6', 0.5), (10, 'Ab5', 2), (12, 'G5', 1), (13, 'Bb5', 1), (14, 'Eb6', 2)]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            mix.add('lead', fm_bell(midi(n(nm)), ln * BEAT + 0.8, ratio=2.0, index=1.4, decay=0.5), t0, gain=0.34 * level, pan=0.1, send=0.4)


# Count-in: a hit and a stab per word, a dark pad, the heartbeat of the clock.
for nm in ['F2', 'C3', 'F3', 'Ab3']:
    mix.add('pad', supersaw(midi(n(nm)), at(2) + 0.5, cutoff=600, a=0.4, r=0.6, detune=0.12), 0, gain=0.2, send=0.4)
for i, c in enumerate(cue('hit')):
    mix.add('fx', impact(1.6, bright=0.7) * 0.5 * c.get('v', 1), c['t'], send=0.2)
    mix.add('drums', K, c['t'], gain=1.0)
    for nm in CHORDS[i]['pad']:
        mix.add('stab', supersaw(midi(n(nm) + 12), 0.8, cutoff=5200, a=0.005, r=0.4), c['t'], gain=0.4, send=0.45)
for beat in range(8):
    mix.add('fx', tick(0.03, 3600), at(0, beat), gain=0.22, pan=0.2)
for c in cue('press'):
    split_sound(mix, c['t'], strong=True)
for s in range(8):
    mix.add('drums', SN, at(1, 2 + s / 4), gain=0.2 + 0.04 * s, send=0.15)
for c in cue('riser'):
    mix.add('fx', riser(c['until'] - c['t'], 250, 7000) * 0.5, c['t'], send=0.3)
for c in cue('draw'):
    draw_sound(mix, c['t'])
for c in cue('lock'):
    sonic_logo(mix, c['t'], n('F5'))

# The race.
drums(2, 4, clap_on=False, hats=1)
bassline(2, 14)
pads(2, 14, cutoff=2000)
drums(4, 14, shaker_on=True)
arps(4, 14, bright=3600)
arps(8, 14, level=0.55, octave=1, bright=5000)
lead(6, 14)
# The build into the last press.
mix.add('fx', riser(at(13, 3.6) - at(12), 200, 9000) * 0.5, at(12), send=0.3)
for s in range(16):
    mix.add('drums', SN, at(13, s / 4), gain=0.14 + 0.02 * s, send=0.12)

# The finish: an impact, a long chord.
for c in cue('finish'):
    FIN = c['t']
    mix.add('fx', impact(4.0) * 0.8, FIN, send=0.35)
    for nm in ['F2', 'C3', 'F3', 'Ab3', 'C4', 'G4']:
        mix.add('pad', supersaw(midi(n(nm)), at(16) - FIN + 2.0, cutoff=2600, a=0.02, r=2.0), FIN, gain=0.2, send=0.5)
    mix.add('lead', glass(midi(n('C6')), 3.0) * 0.4, FIN + 0.05, send=0.6)
    for s in range(12):
        f = midi(n(['F5', 'C6', 'Ab5', 'G5'][s % 4]))
        mix.add('arp', pluck(f, 0.5, bright=2400, decay=0.25), FIN + 0.9 + s * BEAT / 2, gain=0.12 * (1 - s / 14),
                pan=(-0.4 if s % 2 else 0.4), send=0.5)
drums(15, 16, clap_on=False, hats=1, level=0.6)

# The end card.
for c in cue('logo'):
    sonic_logo(mix, c['t'], n('F5'), gain=0.95, big=False)
    for nm in ['F3', 'C4', 'Ab4']:
        mix.add('pad', supersaw(midi(n(nm)), DUR - c['t'] + 2, cutoff=1400, a=0.8, r=2.0), c['t'] - 0.5, gain=0.13, send=0.5)

for c in CUES:
    ty, t0 = c['type'], c['t']
    if ty == 'split':
        split_sound(mix, t0, strong=True)
    elif ty == 'whoosh':
        mix.add('fx', whoosh(0.6), t0, gain=0.5, pan=-0.2, send=0.15)
    elif ty == 'click':
        mix.add('fx', click(), t0, gain=0.5, pan=0.1)
        mix.add('fx', buzz(0.18), t0 + 0.02, gain=0.4)
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
y = master(mix.render(ducked=('pad', 'bass', 'arp', 'stab'), depth=0.55))
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'race-reel.wav'), DUR)
