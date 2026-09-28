"""A small synthesiser for the films' soundtracks.

Everything is generated from oscillators and noise with numpy and scipy: no
samples, no external instruments, so the scores are reproducible and free of
third-party audio. Instruments return mono float arrays at SR; a Mix places
them on stereo buses with pan, reverb send and sidechain ducking.
"""

import numpy as np
from scipy import signal

SR = 48000
RNG = np.random.default_rng(1234)


def t_axis(dur):
    return np.arange(int(dur * SR)) / SR


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12)


NOTE = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6,
        'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}


def n(name):
    """'F3' -> MIDI number."""
    p = name[:-1]
    o = int(name[-1])
    return NOTE[p] + 12 * (o + 1)


def env_ad(length, a, d, curve=4.0):
    """Attack then exponential-ish decay over `length` samples."""
    N = length
    e = np.zeros(N)
    na = max(1, int(a * SR))
    na = min(na, N)
    e[:na] = np.linspace(0, 1, na)
    rest = N - na
    if rest > 0:
        x = np.linspace(0, 1, rest)
        e[na:] = np.exp(-curve * x * (rest / SR) / max(d, 1e-4))
    return e


def env_adsr(length, a, d, s, r):
    N = length
    na, nd, nr = int(a * SR), int(d * SR), int(r * SR)
    ns = max(0, N - na - nd - nr)
    parts = [np.linspace(0, 1, max(1, na)), np.linspace(1, s, max(1, nd)), np.full(ns, s), np.linspace(s, 0, max(1, nr))]
    e = np.concatenate(parts)[:N]
    if len(e) < N:
        e = np.pad(e, (0, N - len(e)))
    return e


def sos_filter(x, kind, f, order=2, q=None):
    nyq = SR / 2
    if kind == 'lp':
        sos = signal.butter(order, min(f / nyq, 0.99), 'low', output='sos')
    elif kind == 'hp':
        sos = signal.butter(order, min(f / nyq, 0.99), 'high', output='sos')
    else:
        lo, hi = f
        sos = signal.butter(order, [max(lo / nyq, 1e-4), min(hi / nyq, 0.99)], 'band', output='sos')
    return signal.sosfilt(sos, x)


def sweep_filter(x, kind, f_start, f_end, q=0.9, block=256, curve='exp'):
    """A biquad whose cutoff moves from f_start to f_end over the signal."""
    y = np.zeros_like(x)
    N = len(x)
    zi = np.zeros(2)
    for i in range(0, N, block):
        fr = i / max(1, N - 1)
        f = f_start * (f_end / f_start) ** fr if curve == 'exp' else f_start + (f_end - f_start) * fr
        f = min(max(f, 20.0), SR / 2 * 0.95)
        w0 = 2 * np.pi * f / SR
        alpha = np.sin(w0) / (2 * q)
        cw = np.cos(w0)
        if kind == 'lp':
            b = np.array([(1 - cw) / 2, 1 - cw, (1 - cw) / 2])
        elif kind == 'hp':
            b = np.array([(1 + cw) / 2, -(1 + cw), (1 + cw) / 2])
        else:  # band-pass, constant peak gain
            b = np.array([alpha, 0, -alpha])
        a = np.array([1 + alpha, -2 * cw, 1 - alpha])
        b, a = b / a[0], a / a[0]
        seg = x[i:i + block]
        out, zi = signal.lfilter(b, a, seg, zi=zi)
        y[i:i + block] = out
    return y


# --- Oscillators -------------------------------------------------------------

def _phase(freq, dur, phase=0.0):
    t = t_axis(dur)
    f = np.asarray(freq, dtype=np.float64)
    f = np.broadcast_to(f, t.shape) if f.ndim else np.full(t.shape, float(f))
    ph = (np.cumsum(f) / SR + phase) % 1.0
    return ph, f / SR


def _blep(ph, dt):
    """PolyBLEP residual for a unit step at phase 0 (band-limits the edges)."""
    r = np.zeros_like(ph)
    m = ph < dt
    x = ph[m] / dt[m]
    r[m] = x + x - x * x - 1.0
    m = ph > 1.0 - dt
    x = (ph[m] - 1.0) / dt[m]
    r[m] = x * x + x + x + 1.0
    return r


def saw(freq, dur, phase=0.0):
    """Band-limited sawtooth (PolyBLEP), -1..1."""
    ph, dt = _phase(freq, dur, phase)
    return (2.0 * ph - 1.0) - _blep(ph, dt)


def square(freq, dur, pw=0.5):
    """Band-limited pulse wave (PolyBLEP on both edges)."""
    ph, dt = _phase(freq, dur)
    y = np.where(ph < pw, 1.0, -1.0)
    y += _blep(ph, dt)
    y -= _blep((ph + (1.0 - pw)) % 1.0, dt)
    return y


def sine(freq, dur, phase=0.0):
    t = t_axis(dur)
    if np.ndim(freq):
        ph = np.cumsum(freq) / SR
        return np.sin(2 * np.pi * ph + phase)
    return np.sin(2 * np.pi * freq * t + phase)


def tri(freq, dur):
    s = saw(freq, dur)
    return 2 * np.abs(s) - 1


def noise(dur):
    return RNG.standard_normal(int(dur * SR))


# --- Drums ---------------------------------------------------------------------

def kick(dur=0.55, f0=160.0, f1=44.0, pitch_decay=0.045, decay=0.32, click=0.5, drive=1.6):
    t = t_axis(dur)
    f = f1 + (f0 - f1) * np.exp(-t / pitch_decay)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    body *= np.exp(-t / decay)
    body = np.tanh(body * drive) / np.tanh(drive)
    ck = sos_filter(noise(dur), 'hp', 2500) * np.exp(-t / 0.004) * click
    out = body + ck
    out[: int(0.002 * SR)] *= np.linspace(0, 1, int(0.002 * SR))
    return out * 0.9


def soft_kick(dur=0.5):
    return kick(dur, f0=110, f1=48, pitch_decay=0.03, decay=0.26, click=0.12, drive=1.1) * 0.9


def snare(dur=0.32, tone=185.0, bright=1.0):
    t = t_axis(dur)
    nz = sos_filter(noise(dur), 'band', (1200, 9000)) * np.exp(-t / 0.09) * bright
    body = np.sin(2 * np.pi * tone * t) * np.exp(-t / 0.05) * 0.7
    return (nz * 0.8 + body) * 0.7


def clap(dur=0.4):
    t = t_axis(dur)
    nz = sos_filter(noise(dur), 'band', (900, 5000))
    e = np.zeros_like(t)
    for k, off in enumerate([0.0, 0.011, 0.022, 0.034]):
        e += (t >= off) * np.exp(-np.maximum(t - off, 0) / (0.006 if k < 3 else 0.12)) * (1 if k < 3 else 0.8)
    return nz * e * 0.55


def hat(dur=0.07, open_=False, bright=1.0):
    """A hi-hat: band-passed noise with a few inharmonic partials for metal."""
    d = 0.45 if open_ else dur
    t = t_axis(d)
    partials = sum(np.sin(2 * np.pi * f * t + i) for i, f in enumerate([3140.0, 4230.0, 5870.0, 7390.0]))
    m = sos_filter(RNG.standard_normal(len(t)), 'band', (5200 * bright, 12500)) + 0.25 * partials
    dec = 0.14 if open_ else 0.02
    return sos_filter(m, 'lp', 13000) * np.exp(-t / dec) * 0.4


def shaker(dur=0.12):
    t = t_axis(dur)
    nz = sos_filter(noise(dur), 'band', (4500, 12000))
    e = np.sin(np.pi * np.minimum(t / dur, 1)) ** 2
    return nz * e * 0.3


def rim(dur=0.06):
    t = t_axis(dur)
    s = np.sin(2 * np.pi * 1650 * t) * np.exp(-t / 0.012) + sos_filter(noise(dur), 'band', (2500, 6000)) * np.exp(-t / 0.004)
    return s * 0.5


def tick(dur=0.03, f=3200.0):
    t = t_axis(dur)
    return (np.sin(2 * np.pi * f * t) * np.exp(-t / 0.006) + sos_filter(noise(dur), 'hp', 5000) * np.exp(-t / 0.002)) * 0.5


def heartbeat(dur=0.9, v=1.0):
    """Lub-dub: two low thumps."""
    out = np.zeros(int(dur * SR))
    for off, a, f0 in [(0.0, 1.0, 70.0), (0.22, 0.7, 62.0)]:
        k = kick(0.35, f0=f0 * 1.6, f1=f0 * 0.6, pitch_decay=0.02, decay=0.09, click=0.0, drive=1.0)
        i = int(off * SR)
        out[i:i + len(k)] += k[: len(out) - i] * a
    return sos_filter(out, 'lp', 180) * v * 1.4


# --- Tonal ---------------------------------------------------------------------

def supersaw(freq, dur, voices=7, detune=0.18, cutoff=2400.0, a=0.4, r=0.8, s=1.0):
    """Returns stereo (2, N): detuned saws, split across the field."""
    N = int(dur * SR)
    L = np.zeros(N)
    R = np.zeros(N)
    for v in range(voices):
        dv = (v - (voices - 1) / 2) / ((voices - 1) / 2 or 1)
        f = freq * 2 ** (dv * detune / 12)
        w = saw(f, dur, phase=RNG.random())
        pan = 0.5 + 0.45 * dv
        L += w * (1 - pan)
        R += w * pan
    e = env_adsr(N, a, 0.2, s, r)
    L = sos_filter(L, 'lp', cutoff, order=2) * e / voices
    R = sos_filter(R, 'lp', cutoff, order=2) * e / voices
    return np.vstack([L, R]) * 1.6


def pluck(freq, dur=0.5, bright=4000.0, decay=0.25, wave='saw'):
    N = int(dur * SR)
    t = t_axis(dur)
    w = saw(freq, dur) * 0.6 + square(freq * 1.002, dur, 0.35) * 0.4 if wave == 'saw' else tri(freq, dur)
    y = sweep_filter(w, 'lp', bright, max(200.0, freq * 1.5), q=1.1)
    return y * np.exp(-t / decay) * np.minimum(1, t / 0.002) * 0.8


def bass(freq, dur, cutoff=900.0, env_amt=2.5, decay=0.18, sub=0.6):
    t = t_axis(dur)
    w = saw(freq, dur) * 0.7 + square(freq, dur) * 0.3
    y = sweep_filter(w, 'lp', cutoff * env_amt, cutoff, q=1.0)
    y = y * (0.55 + 0.45 * np.exp(-t / decay))
    y += np.sin(2 * np.pi * freq * t) * sub
    rel = np.minimum(1, (dur - t) / 0.02)
    return y * np.minimum(1, t / 0.004) * np.maximum(rel, 0) * 0.55


def fm_bell(freq, dur=2.0, ratio=3.5, index=3.0, decay=0.9):
    t = t_axis(dur)
    idx = index * np.exp(-t / (decay * 0.35))
    mod = np.sin(2 * np.pi * freq * ratio * t) * idx
    car = np.sin(2 * np.pi * freq * t + mod)
    return car * np.exp(-t / decay) * np.minimum(1, t / 0.002) * 0.5


def glass(freq, dur=1.5):
    """Bright glassy tone: bell plus an octave partial."""
    return fm_bell(freq, dur, ratio=2.0, index=1.2, decay=0.7) * 0.7 + fm_bell(freq * 2, dur, ratio=3.01, index=0.8, decay=0.4) * 0.35


def sub_boom(dur=2.2, f0=80.0, f1=28.0):
    t = t_axis(dur)
    f = f1 + (f0 - f1) * np.exp(-t / 0.25)
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.7) * 0.9


def impact(dur=2.5, bright=1.0):
    t = t_axis(dur)
    boom = sub_boom(dur)
    nz = sos_filter(noise(dur), 'lp', 3000 * bright) * np.exp(-t / 0.35) * 0.35
    crack = sos_filter(noise(dur), 'hp', 3000) * np.exp(-t / 0.03) * 0.4
    return boom + nz + crack


def riser(dur, f0=300.0, f1=6000.0, tone=True):
    t = t_axis(dur)
    nz = sweep_filter(noise(dur), 'bp', f0, f1, q=2.5)
    amp = (t / dur) ** 2.2
    out = nz * amp * 0.9
    if tone:
        f = 110 * (8 ** (t / dur))
        out += saw(f, dur) * amp * 0.08
    return out


def downlifter(dur=1.2):
    t = t_axis(dur)
    nz = sweep_filter(noise(dur), 'bp', 6000, 200, q=2.0)
    return nz * np.exp(-t / (dur * 0.5)) * 0.6


def whoosh(dur=0.6, up=True):
    t = t_axis(dur)
    nz = sweep_filter(noise(dur), 'bp', 400 if up else 5000, 5000 if up else 400, q=1.8)
    e = np.sin(np.pi * np.minimum(t / dur, 1)) ** 1.5
    return nz * e * 0.8


def buzz(dur=0.3, f=172.0):
    """A watch's haptic motor: a rattly low tone with a fast wobble."""
    t = t_axis(dur)
    w = np.sign(np.sin(2 * np.pi * f * t)) * 0.6 + np.sin(2 * np.pi * f * 2 * t) * 0.3
    am = 0.75 + 0.25 * np.sin(2 * np.pi * 31 * t)
    e = np.minimum(1, t / 0.01) * np.minimum(1, (dur - t) / 0.04)
    return sos_filter(w * am * e, 'band', (120, 1400)) * 0.35


def click(dur=0.05):
    t = t_axis(dur)
    s = sos_filter(noise(dur), 'band', (1500, 7000)) * np.exp(-t / 0.004)
    s += np.sin(2 * np.pi * 2100 * t) * np.exp(-t / 0.008) * 0.4
    return s * 0.6


# --- Effects -----------------------------------------------------------------

def reverb_ir(seconds=2.8, predelay=0.02, damp=5500.0, seed=7):
    rng = np.random.default_rng(seed)
    N = int(seconds * SR)
    t = np.arange(N) / SR
    env = np.exp(-t / (seconds / 6.5))
    L = rng.standard_normal(N) * env
    R = rng.standard_normal(N) * env
    L = sos_filter(L, 'lp', damp)
    R = sos_filter(R, 'lp', damp)
    pd = int(predelay * SR)
    L = np.concatenate([np.zeros(pd), L])
    R = np.concatenate([np.zeros(pd), R])
    norm = np.sqrt(np.sum(L ** 2))
    return np.vstack([L, R]) / norm


def convolve_stereo(x, ir):
    """x: (2, N) or (N,); ir: (2, M). Returns (2, N + M - 1)."""
    if x.ndim == 1:
        x = np.vstack([x, x])
    L = signal.fftconvolve(x[0], ir[0])
    R = signal.fftconvolve(x[1], ir[1])
    return np.vstack([L, R])


def pingpong(x, delay, feedback=0.45, taps=6, wet=0.35):
    """x: (2, N). Delay in seconds; alternating left and right."""
    N = x.shape[1]
    d = int(delay * SR)
    out = x.copy()
    mono = x.mean(axis=0)
    g = wet
    for k in range(1, taps + 1):
        off = d * k
        if off >= N:
            break
        ch = k % 2
        out[ch, off:] += mono[: N - off] * g
        g *= feedback
    return out


def pan_mono(x, pan=0.0):
    """Equal-power pan, pan in -1..1."""
    a = (pan + 1) * np.pi / 4
    return np.vstack([x * np.cos(a), x * np.sin(a)])


# --- The mix -----------------------------------------------------------------

class Mix:
    """Stereo buses with reverb sends and a kick-driven sidechain."""

    def __init__(self, duration, tail=4.0):
        self.N = int((duration + tail) * SR)
        self.duration = duration
        self.buses = {}
        self.sends = {}
        self.duck_times = []

    def bus(self, name):
        if name not in self.buses:
            self.buses[name] = np.zeros((2, self.N))
        return self.buses[name]

    def add(self, name, x, at, gain=1.0, pan=0.0, send=0.0):
        """Place audio x (mono or stereo) on bus `name` at time `at` (s)."""
        if x.ndim == 1:
            x = pan_mono(x, pan)
        i = int(round(at * SR))
        if i >= self.N:
            return
        if i < 0:
            x = x[:, -i:]
            i = 0
        n_ = min(x.shape[1], self.N - i)
        self.bus(name)[:, i:i + n_] += x[:, :n_] * gain
        if send > 0:
            self.bus('_verb')[:, i:i + n_] += x[:, :n_] * gain * send

    def duck(self, at):
        self.duck_times.append(at)

    def sidechain_env(self, depth=0.6, release=0.16):
        e = np.ones(self.N)
        rel = int(release * 4 * SR)
        k = 1 - depth * np.exp(-np.arange(rel) / (release * SR))
        for tt in self.duck_times:
            i = int(tt * SR)
            if i >= self.N:
                continue
            n_ = min(rel, self.N - i)
            e[i:i + n_] = np.minimum(e[i:i + n_], k[:n_])
        return e

    def render(self, ducked=(), depth=0.6, verb_seconds=2.8, verb_gain=0.9, bus_fx=None):
        sc = self.sidechain_env(depth)
        out = np.zeros((2, self.N))
        for name, x in self.buses.items():
            if name == '_verb':
                continue
            if bus_fx and name in bus_fx:
                x = bus_fx[name](x)
            if name in ducked:
                x = x * sc
            out += x
        if '_verb' in self.buses:
            wet = convolve_stereo(self.buses['_verb'] * sc, reverb_ir(verb_seconds))[:, : self.N]
            out += wet * verb_gain
        return out


def high_shelf(x, f0=6000.0, gain_db=-4.0, q=0.7):
    """RBJ high shelf on each channel."""
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0 / SR
    alpha = np.sin(w0) / (2 * q)
    cw = np.cos(w0)
    b = np.array([A * ((A + 1) + (A - 1) * cw + 2 * np.sqrt(A) * alpha),
                  -2 * A * ((A - 1) + (A + 1) * cw),
                  A * ((A + 1) + (A - 1) * cw - 2 * np.sqrt(A) * alpha)])
    a = np.array([(A + 1) - (A - 1) * cw + 2 * np.sqrt(A) * alpha,
                  2 * ((A - 1) - (A + 1) * cw),
                  (A + 1) - (A - 1) * cw - 2 * np.sqrt(A) * alpha])
    return signal.lfilter(b / a[0], a / a[0], x, axis=-1)


def tone(x, shelf_db=-1.0):
    """The master's tone: a touch off the top, and nothing above 18 kHz."""
    x = high_shelf(x, 7000.0, shelf_db)
    sos = signal.butter(4, 18000 / (SR / 2), 'low', output='sos')
    return signal.sosfilt(sos, x, axis=-1)


def master(x, drive=1.1, ceiling=0.9, release=0.08):
    """Gentle saturation, then a look-ahead peak limiter.

    The gain needed at each sample is spread over a window around it (a
    running minimum, then a moving average inside that window), so the gain
    is already down when a peak arrives and never sits above what it needs;
    a one-pole release lets it recover smoothly afterwards.
    """
    from scipy.ndimage import minimum_filter1d
    x = tone(x)
    x = np.tanh(x * drive) / np.tanh(drive)
    peak = np.max(np.abs(x), axis=0)
    need = np.minimum(1.0, ceiling / np.maximum(peak, 1e-9))
    w = int(0.005 * SR)
    g = minimum_filter1d(need, size=2 * w + 1, mode='nearest')
    g = np.convolve(g, np.ones(w) / w, mode='same')
    # Release: the gain reduction may rise at once but falls back
    # exponentially (a peak-hold decay, computed on a decimated grid).
    red = 1.0 - g
    hop = 32
    coarse = red[: len(red) // hop * hop].reshape(-1, hop).max(axis=1)
    a = np.exp(-hop / (release * SR))
    held = np.empty_like(coarse)
    acc = 0.0
    for i, v in enumerate(coarse):
        acc = v if v > acc else acc * a
        held[i] = acc
    held_full = np.interp(np.arange(len(red)), np.arange(len(held)) * hop + hop / 2, held)
    red = np.maximum(red, held_full)
    return x * (1.0 - red)


def write_wav(path, x, duration=None):
    from scipy.io import wavfile
    if duration is not None:
        x = x[:, : int(duration * SR)]
        # Fade the very end so the file never stops on a click.
        f = int(0.05 * SR)
        x[:, -f:] *= np.linspace(1, 0, f)
    x = np.clip(x, -1, 1)
    wavfile.write(path, SR, (x.T * 32767).astype(np.int16))
