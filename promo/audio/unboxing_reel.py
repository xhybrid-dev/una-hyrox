"""HybridX, the unboxing reel: "Box to start line" in 32 seconds.

F minor, 120 BPM, 16 bars, in the Race reel's key so "Now we race" lands
home. A hit on the finished watch and the clock ticking; the tape rewinding
to the empty table; the drop on GO as the box slides in; a groove through the
stations that steps back for the knife and the boot; a breakdown for the
telestrator on the lid, one glass note per mark; a snare build to the finish;
the open platform; the sonic logo on the end card.

Under the music, the clips' own sound: the box sliding in, the knife through
the seal, the lid, the strap box, the cable, the watch booting. Each shot's
sound follows its picture (sped up with it), placed from the film's own shot
list so sound and picture agree.
"""

import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from synth import (SR, Mix, n, midi, kick, clap, snare, hat, shaker, tick, supersaw, pluck, bass, fm_bell, glass,
                   impact, sub_boom, riser, downlifter, whoosh, click, buzz, noise, sweep_filter, saw, t_axis,
                   sos_filter, master)
from common import load_cues, sonic_logo, split_sound, finish, report

BPM = 120
BEAT = 60 / BPM
BAR = BEAT * 4


def at(bar, beat=0.0):
    return bar * BAR + beat * BEAT


cues = load_cues('unboxing-reel')
DUR = cues['duration']
CUES = cues['cues']
EDL = cues['edl']
music = Mix(DUR, tail=4.0)
nat = Mix(DUR, tail=4.0)


def cue(kind):
    return [c for c in CUES if c['type'] == kind]


def cue_t(kind):
    return cue(kind)[0]['t']


CHORDS = [
    dict(root=n('F2'), pad=['F3', 'Ab3', 'C4', 'F4'], arp=['F5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Db2'), pad=['Db3', 'F3', 'Ab3', 'Db4'], arp=['F5', 'Ab5', 'Db6', 'Ab5']),
    dict(root=n('Ab2'), pad=['Ab3', 'C4', 'Eb4', 'Ab4'], arp=['Eb5', 'Ab5', 'C6', 'Ab5']),
    dict(root=n('Eb2'), pad=['Eb3', 'G3', 'Bb3', 'Eb4'], arp=['Eb5', 'G5', 'Bb5', 'G5']),
]


def chord_at(bar):
    return CHORDS[int(bar) % 4]


K, CL, HH, OH, SN = kick(), clap(), hat(), hat(open_=True), snare()


def drums(t0, t1, clap_on=True, hats=2, shaker_on=False, level=1.0, kick_on=True):
    """Four to the floor between two times (on the beat grid)."""
    b = int(round(t0 / BEAT))
    while b * BEAT < t1 - 1e-6:
        t = b * BEAT
        beat = b % 4
        if kick_on:
            music.add('drums', K, t, gain=0.8 * level)
            music.duck(t)
        if clap_on and beat in (1, 3):
            music.add('drums', CL, t, gain=0.5 * level, send=0.18)
        steps = 4 if hats == 2 else 2
        for s in range(steps):
            acc = [0.62, 0.3, 0.48, 0.3][s] if steps == 4 else [0.55, 0.38][s]
            music.add('hats', HH, t + s * BEAT / steps, gain=acc * level, pan=0.25)
        music.add('hats', OH, t + BEAT / 2, gain=0.14 * level, pan=-0.2, send=0.1)
        if shaker_on:
            music.add('hats', shaker(), t + BEAT * 0.75, gain=0.3 * level, pan=-0.4)
        b += 1


def bassline(b0, b1, level=1.0, cutoff=700.0):
    for b in range(b0, b1):
        r = chord_at(b)['root']
        for beat in range(4):
            note = r + (12 if beat == 3 and b % 2 == 1 else 0)
            music.add('bass', bass(midi(note), BEAT * 0.45, cutoff=cutoff), at(b, beat + 0.5), gain=0.8 * level)


def pads(t0, t1, level=1.0, cutoff=2200.0, attack=0.3):
    b = int(t0 // BAR)
    while at(b) < t1 - 1e-6:
        s = max(t0, at(b))
        e = min(t1, at(b + 1))
        for nm in chord_at(b)['pad']:
            music.add('pad', supersaw(midi(n(nm)), e - s + 0.6, cutoff=cutoff, a=attack, r=0.6), s, gain=0.28 * level, send=0.35)
        b += 1


def arps(b0, b1, level=1.0, bright=3800.0, octave=0):
    for b in range(b0, b1):
        c = chord_at(b)
        for s in range(16):
            f = midi(n(c['arp'][s % 4]) + 12 * octave)
            music.add('arp', pluck(f, 0.32, bright=bright, decay=0.12), at(b, s / 4), gain=0.4 * level * (1.0 if s % 4 == 0 else 0.7),
                      pan=(-0.35 if s % 2 else 0.35), send=0.2)


def lead(b0, b1, level=1.0):
    motif = [(0, 'C6', 1.5), (1.5, 'Ab5', 0.5), (2, 'F5', 2), (4, 'Db6', 1.5), (5.5, 'C6', 0.5), (6, 'Ab5', 2),
             (8, 'Eb6', 1.5), (9.5, 'C6', 0.5), (10, 'Ab5', 2), (12, 'G5', 1), (13, 'Bb5', 1), (14, 'Eb6', 2)]
    for b in range(b0, b1, 4):
        for off, nm, ln in motif:
            t0 = at(b, off)
            if t0 >= at(b1):
                break
            music.add('lead', fm_bell(midi(n(nm)), ln * BEAT + 0.8, ratio=2.0, index=1.4, decay=0.5), t0, gain=0.3 * level, pan=0.1, send=0.4)


def rewind_sound(dur):
    """A tape spooling back: a warbling whine climbing in pitch, with hiss."""
    t = t_axis(dur)
    f = 260 * (7 ** (t / dur)) * (1 + 0.05 * np.sin(2 * np.pi * 23 * t))
    ph = np.cumsum(f) / SR
    w = 2 * (ph - np.floor(ph + 0.5))
    w = sos_filter(w, 'band', (300, 5000))
    hiss = sweep_filter(noise(dur), 'bp', 900, 7000, q=1.5) * 0.5
    e = np.minimum(1, t / 0.03) * np.minimum(1, (dur - t) / 0.02)
    return (w * 0.35 + hiss) * e


def pen_sound(dur=0.3):
    """A marker on card: a short, bright, scratchy swipe."""
    t = t_axis(dur)
    nz = sweep_filter(noise(dur), 'bp', 2500, 6000, q=3.0)
    grain = 0.6 + 0.4 * np.sign(np.sin(2 * np.pi * 70 * t))
    e = np.sin(np.pi * np.minimum(t / dur, 1)) ** 0.8
    return nz * grain * e * 0.35


# --- The clips' own sound --------------------------------------------------------

def load_clip(path):
    """The clip's sound as 48 kHz mono, denoised, with the room's rumble cut."""
    ff = os.environ.get('FFMPEG') or 'ffmpeg'
    raw = subprocess.run([ff, '-v', 'error', '-i', path, '-vn', '-ac', '1', '-ar', str(SR),
                          '-af', 'highpass=f=90,afftdn=nf=-32', '-f', 'f32le', '-'], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype=np.float32).astype(np.float64)
    # Bring the loud moments (the top half-percent of 50 ms windows) to -16 dBFS.
    w = int(0.05 * SR)
    r = np.sqrt(np.mean(x[: len(x) // w * w].reshape(-1, w) ** 2, axis=1))
    loud = np.percentile(r, 99.5)
    return x * (10 ** (-16 / 20) / max(loud, 1e-6))


CLIPS = {k: load_clip(v) for k, v in EDL['sources'].items()}


def place_shot(sh):
    """Lay a shot's own sound where its picture plays, at its picture's speed."""
    x = CLIPS[sh['src']]
    N = int((sh['t1'] - sh['t0']) * SR)
    p = np.arange(N) / N
    q = p * (1 - sh['ease']) + sh['ease'] * (-(np.cos(np.pi * p) - 1) / 2)
    s = (sh['a'] + (sh['b'] - sh['a']) * q) * SR
    y = np.interp(s, np.arange(len(x)), x)
    f = int(0.012 * SR)
    y[:f] *= np.linspace(0, 1, f)
    y[-f:] *= np.linspace(1, 0, f)
    nat.add('nat', y * sh['nat'], sh['t0'], send=0.04)


for sh in EDL['shots']:
    if sh['nat'] > 0 and not sh['hold']:
        place_shot(sh)

# --- The score -------------------------------------------------------------------

REW = cue_t('rewind')
GO = cue_t('go')
FREEZE = cue_t('freeze')
FIN = cue_t('finish')
BOOT = cue_t('boot')
END = cue_t('whoosh') + 0.3

# The hook: a hit on the finished watch, a dark chord, the clock ticking.
music.add('fx', impact(1.8, bright=0.7) * 0.55, 0, send=0.2)
music.add('drums', K, 0, gain=1.0)
for nm in CHORDS[0]['pad']:
    music.add('stab', supersaw(midi(n(nm) + 12), 0.9, cutoff=5000, a=0.005, r=0.5), 0, gain=0.36, send=0.45)
for nm in ['F2', 'C3', 'F3', 'Ab3']:
    music.add('pad', supersaw(midi(n(nm)), REW + 0.3, cutoff=700, a=0.1, r=0.3, detune=0.12), 0, gain=0.2, send=0.4)
for k in range(int(REW / (BEAT / 2)) + 1):
    music.add('fx', tick(0.03, 3600), k * BEAT / 2, gain=0.26 if k % 2 == 0 else 0.16, pan=0.2)
# The rewind, and a riser into GO.
for c in cue('rewind'):
    music.add('fx', rewind_sound(c['until'] - c['t']), c['t'], gain=0.55, send=0.1)
    music.add('fx', riser(c['until'] - c['t'], 400, 9000, tone=False) * 0.45, c['t'], send=0.2)

# GO: the drop, and the first split.
music.add('fx', impact(2.5) * 0.7, GO, send=0.3)
split_sound(music, GO, strong=True)
for nm in ['F3', 'C4', 'F4', 'Ab4']:
    music.add('stab', supersaw(midi(n(nm) + 12), 0.6, cutoff=6000, a=0.003, r=0.4), GO, gain=0.34, send=0.4)

# The stations. Bars 1 to 3: the groove, bare. Kick only under the knife.
drums(GO, 3.8, shaker_on=True)
drums(3.8, FREEZE, clap_on=False, hats=1, level=0.7)
bassline(1, 4)
pads(GO, FREEZE, cutoff=1800)

# The lid, held: the drums stop, the pad opens, one glass note per mark.
music.add('fx', downlifter(0.6) * 0.5, FREEZE - 0.05)
music.add('fx', sub_boom(1.6) * 0.4, FREEZE, send=0.2)
pads(FREEZE, at(4, 2), level=1.1, cutoff=3200, attack=0.05)
for c in cue('pen'):
    k = c['k']
    music.add('fx', pen_sound(0.28), c['t'], gain=0.9, pan=(-0.3 + 0.15 * k))
    note = ['F5', 'Ab5', 'C6', 'Eb6', 'F6', 'C6'][k]
    music.add('lead', glass(midi(n(note)), 1.6) * 0.3, c['t'] + 0.05, pan=(-0.3 + 0.15 * k), send=0.6)
music.add('fx', riser(at(4, 2) - FREEZE - 0.2, 300, 6000) * 0.4, FREEZE + 0.2, send=0.3)

# Back in on the watch: full groove, arps, then the hook line.
drums(at(4, 2), BOOT - 0.5, shaker_on=True)
drums(BOOT - 0.5, BOOT + 0.5, clap_on=False, hats=1, level=0.6)  # under the boot
drums(BOOT + 0.5, FIN, shaker_on=True)
bassline(4, 12)
pads(at(4, 2), FIN, cutoff=2200)
arps(5, 12, bright=3600)
arps(9, 12, level=0.5, octave=1, bright=5000)
lead(6, 12)
# Into the power-on: a riser, then the screen lights with a glass chord.
music.add('fx', riser(1.0, 300, 7000) * 0.4, BOOT - 1.0, send=0.3)
for c in cue('boot'):
    music.add('fx', sub_boom(2.0) * 0.45, c['t'], send=0.3)
    for i, nm in enumerate(['F5', 'C6', 'F6']):
        music.add('lead', glass(midi(n(nm)), 2.2) * 0.32, c['t'] + i * 0.05, pan=(-0.3 + 0.3 * i), send=0.6)
# The build to the finish.
music.add('fx', riser(at(12) - at(10, 2), 200, 9000) * 0.5, at(10, 2), send=0.3)
for s in range(16):
    music.add('drums', SN, at(11, s / 4), gain=0.12 + 0.02 * s, send=0.12)

# The finish: the last split, an impact, a long chord.
split_sound(music, FIN, strong=True)
music.add('fx', impact(4.0) * 0.8, FIN, send=0.35)
for nm in ['F2', 'C3', 'F3', 'Ab3', 'C4', 'G4']:
    music.add('pad', supersaw(midi(n(nm)), at(13) - FIN + 1.5, cutoff=2600, a=0.02, r=1.5), FIN, gain=0.2, send=0.5)
music.add('lead', glass(midi(n('C6')), 3.0) * 0.4, FIN + 0.05, send=0.6)
for s in range(8):
    f = midi(n(['F5', 'C6', 'Ab5', 'G5'][s % 4]))
    music.add('arp', pluck(f, 0.5, bright=2400, decay=0.25), FIN + 0.5 + s * BEAT / 2, gain=0.12 * (1 - s / 10),
              pan=(-0.4 if s % 2 else 0.4), send=0.5)

# The open platform: a lighter groove, and the lift into the end card.
drums(at(13), END - 0.3, clap_on=False, hats=2, level=0.65)
bassline(13, 14, level=0.7)
pads(at(13), END, cutoff=2600)
arps(13, 14, level=0.7, bright=4200)
music.add('fx', riser(END - 0.3 - at(13), 300, 8000) * 0.4, at(13), send=0.3)
for c in cue('whoosh'):
    music.add('fx', whoosh(0.6), c['t'], gain=0.55, pan=-0.2, send=0.15)

# The end card: the sonic logo, and a pad to close on.
for c in cue('logo'):
    sonic_logo(music, c['t'], n('F5'), gain=0.95, big=True)
    for nm in ['F3', 'C4', 'Ab4', 'C5']:
        music.add('pad', supersaw(midi(n(nm)), DUR - c['t'] + 2.5, cutoff=1400, a=0.6, r=2.0), c['t'] - 0.8, gain=0.13, send=0.5)

# Every station's split, and a swish on the fastest cuts.
for c in cue('split'):
    split_sound(music, c['t'], strong=False)
for c in cue('swish'):
    music.add('fx', whoosh(0.3), c['t'] - 0.12, gain=0.35, pan=0.2, send=0.1)

# --- Mix ---------------------------------------------------------------------------

print('music')
report(music)
print('natural')
report(nat)
m = music.render(ducked=('pad', 'bass', 'arp', 'stab'), depth=0.5)
s = nat.render(verb_seconds=1.2, verb_gain=0.5)
# Lift the quiet handling sounds and hold the peaks down: +12 dB into a soft
# limit at -6 dBFS, so the knife and the boot sit forward of the music.
s = np.tanh(s * 4.0 / 0.5) * 0.5
# The music steps back where the clips' own sound leads.
env = np.ones(m.shape[1])
tt = np.arange(m.shape[1]) / SR
for a, b in EDL['duck']:
    ramp = np.clip(np.minimum((tt - a) / 0.08 + 1, (b - tt) / 0.15 + 1), 0, 1)
    env = np.minimum(env, 1 - 0.7 * ramp)
y = master(m * env + s * 1.0)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')
os.makedirs(out, exist_ok=True)
finish(y, os.path.join(out, 'unboxing-reel.wav'), DUR)

if os.environ.get('LEVELS'):
    # Where the clips' own sound should lead: its level against the music's.
    for name, a, b in [('box', 2.0, 3.5), ('knife', 3.8, 5.0), ('lid', 5.0, 6.2), ('cable', 12.0, 14.0), ('strap', 14.0, 15.0),
                       ('plugIn', 16.0, 17.5), ('boot', 17.9, 18.8), ('menu', 20.0, 24.0)]:
        i, j = int(a * SR), int(b * SR)
        ms = 20 * np.log10(np.sqrt(np.mean((m * env)[:, i:j] ** 2)) + 1e-9)
        ns = 20 * np.log10(np.sqrt(np.mean(s[:, i:j] ** 2)) + 1e-9)
        npk = 20 * np.log10(np.max(np.abs(s[:, i:j])) + 1e-9)
        print(f'  {name:7s} music {ms:6.1f}  natural {ns:6.1f} (peak {npk:6.1f}) dBFS')
