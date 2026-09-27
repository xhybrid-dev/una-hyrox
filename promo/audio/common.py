"""Pieces every film's score shares: the sonic logo, the split sound, cue
loading and the final write-out with loudness normalisation."""

import json
import os
import subprocess
import sys

import numpy as np

from synth import (SR, Mix, midi, glass, fm_bell, impact, sub_boom, whoosh, riser, buzz, click, tick,
                   sweep_filter, noise, t_axis, master, write_wav)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def load_cues(film):
    """Ask the film itself for its cue sheet, so picture and sound agree."""
    out = subprocess.check_output(['node', os.path.join(ROOT, 'render.mjs'), film, '--cues'], cwd=ROOT)
    return json.loads(out)


def draw_sound(mix, at, dur=0.95):
    """The chevron being drawn on: a rising, filtered zip."""
    t = t_axis(dur)
    z = sweep_filter(noise(dur), 'bp', 500, 7000, q=3.0) * (t / dur) ** 1.4 * 0.5
    mix.add('fx', z, at, pan=-0.3, send=0.25)


def sonic_logo(mix, at, root, gain=1.0, big=True):
    """The lock: the arms land (two glass notes a fifth apart, one per arm),
    then the octave rings out. `root` is a MIDI note in the film's key."""
    if big:
        mix.add('fx', impact(3.0) * 0.55 * gain, at, send=0.25)
    else:
        mix.add('fx', sub_boom(2.0) * 0.35 * gain, at, send=0.2)
    mix.add('logo', glass(midi(root), 2.6) * 0.55 * gain, at, pan=-0.35, send=0.55)
    mix.add('logo', glass(midi(root + 7), 2.6) * 0.5 * gain, at + 0.075, pan=0.35, send=0.55)
    mix.add('logo', fm_bell(midi(root + 12), 3.5, ratio=2.0, index=0.8, decay=1.6) * 0.35 * gain, at + 0.15, send=0.8)
    mix.add('logo', fm_bell(midi(root + 19), 3.0, ratio=3.0, index=0.5, decay=1.2) * 0.12 * gain, at + 0.23, pan=0.1, send=0.9)


def split_sound(mix, at, strong=False):
    """A press on R2 and the watch's buzz."""
    mix.add('fx', click() * (0.55 if strong else 0.4), at, pan=0.35)
    mix.add('fx', buzz(0.28 if strong else 0.2) * (0.8 if strong else 0.55), at + 0.02, pan=0.25)


def finish(mix, path, duration, lufs=-14.0):
    """Master, normalise to `lufs` integrated with ffmpeg's two-pass loudnorm,
    and write a 48 kHz WAV."""
    raw = path.replace('.wav', '.raw.wav')
    y = mix
    write_wav(raw, y, duration)
    ff = os.environ.get('FFMPEG') or '/usr/local/lib/python3.11/dist-packages/imageio_ffmpeg/binaries/ffmpeg-linux-x86_64-v7.0.2'
    meas = subprocess.run([ff, '-hide_banner', '-nostats', '-i', raw, '-af',
                           f'loudnorm=I={lufs}:TP=-1.0:LRA=11:print_format=json', '-f', 'null', '-'],
                          capture_output=True, text=True).stderr
    j = json.loads(meas[meas.rindex('{'):meas.rindex('}') + 1])
    af = (f"loudnorm=I={lufs}:TP=-1.0:LRA=11:measured_I={j['input_i']}:measured_TP={j['input_tp']}:"
          f"measured_LRA={j['input_lra']}:measured_thresh={j['input_thresh']}:offset={j['target_offset']}:linear=true")
    subprocess.run([ff, '-y', '-loglevel', 'error', '-i', raw, '-af', af, '-ar', str(SR), path], check=True)
    os.remove(raw)
    print(f"{os.path.basename(path)}: input {j['input_i']} LUFS, TP {j['input_tp']} -> {lufs} LUFS")


def report(mix_obj):
    """Print each bus's RMS in dBFS, to check the balance without ears."""
    for k, v in mix_obj.buses.items():
        r = np.sqrt(np.mean(v ** 2)) + 1e-12
        print(f"  bus {k:8s} {20 * np.log10(r):6.1f} dBFS rms   peak {20 * np.log10(np.max(np.abs(v)) + 1e-12):6.1f}")
