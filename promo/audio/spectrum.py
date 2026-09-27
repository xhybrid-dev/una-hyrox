"""Octave-band balance of a finished WAV, to check a mix without listening.

Octave-band power of pink noise is flat; finished mixes typically tilt down
by about 2 dB per octave, which is the reference here (0 at 250 Hz)."""
import sys
import numpy as np
from scipy.io import wavfile
from scipy import signal
sr, x = wavfile.read(sys.argv[1])
x = x.astype(np.float64).mean(axis=1) / 32768
f, P = signal.welch(x, sr, nperseg=16384)
bands = [31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
vals = []
for c in bands:
    m = (f >= c / 2 ** 0.5) & (f < c * 2 ** 0.5)
    vals.append(10 * np.log10(np.sum(P[m]) + 1e-20))
ref = vals[3]
print('band Hz   level  vs -2 dB/oct ref (0 at 250 Hz)')
for i, (c, v) in enumerate(zip(bands, vals)):
    expect = -2.0 * (i - 3)
    print(f'{c:7.0f}  {v - ref:6.1f}   {v - ref - expect:+6.1f}')
peak = np.max(np.abs(x)); rms = np.sqrt(np.mean(x ** 2))
print(f'peak {20*np.log10(peak):.1f} dBFS, rms {20*np.log10(rms):.1f} dBFS, crest {20*np.log10(peak/rms):.1f} dB')
