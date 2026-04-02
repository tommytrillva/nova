#!/usr/bin/env python3
"""Procedurally generate placeholder audio resources for a VST3 synthesizer.

Uses only standard library modules: wave, struct, math, random, os, sys, argparse.
"""

import argparse
import math
import os
import random
import struct
import sys
import wave

SAMPLE_RATE = 44100
WAVETABLE_LEN = 2048
TWO_PI = 2.0 * math.pi

random.seed(42)

# ---------------------------------------------------------------------------
# Utility helpers
# ---------------------------------------------------------------------------

def clamp(x, lo=-1.0, hi=1.0):
    return max(lo, min(hi, x))


def normalize(samples, peak=0.95):
    mx = max(abs(s) for s in samples) or 1.0
    scale = peak / mx
    return [s * scale for s in samples]


def write_wav(path, samples, sample_rate=SAMPLE_RATE):
    """Write mono 16-bit PCM WAV."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "w") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        data = b"".join(
            struct.pack("<h", int(clamp(s, -1, 1) * 32767)) for s in samples
        )
        wf.writeframes(data)
    print(os.path.basename(path))


# ---------------------------------------------------------------------------
# Biquad filter (Audio EQ Cookbook)
# ---------------------------------------------------------------------------

class Biquad:
    """Simple biquad filter: lowpass, highpass, bandpass."""

    def __init__(self, ftype, freq, sample_rate=SAMPLE_RATE, Q=0.707):
        w0 = TWO_PI * freq / sample_rate
        alpha = math.sin(w0) / (2.0 * Q)
        cos_w0 = math.cos(w0)

        if ftype == "lowpass":
            b0 = (1.0 - cos_w0) / 2.0
            b1 = 1.0 - cos_w0
            b2 = (1.0 - cos_w0) / 2.0
            a0 = 1.0 + alpha
            a1 = -2.0 * cos_w0
            a2 = 1.0 - alpha
        elif ftype == "highpass":
            b0 = (1.0 + cos_w0) / 2.0
            b1 = -(1.0 + cos_w0)
            b2 = (1.0 + cos_w0) / 2.0
            a0 = 1.0 + alpha
            a1 = -2.0 * cos_w0
            a2 = 1.0 - alpha
        elif ftype == "bandpass":
            b0 = alpha
            b1 = 0.0
            b2 = -alpha
            a0 = 1.0 + alpha
            a1 = -2.0 * cos_w0
            a2 = 1.0 - alpha
        else:
            raise ValueError(f"Unknown filter type: {ftype}")

        self.b = (b0 / a0, b1 / a0, b2 / a0)
        self.a = (a1 / a0, a2 / a0)
        self.x1 = self.x2 = 0.0
        self.y1 = self.y2 = 0.0

    def process(self, x):
        y = self.b[0] * x + self.b[1] * self.x1 + self.b[2] * self.x2 \
            - self.a[0] * self.y1 - self.a[1] * self.y2
        self.x2 = self.x1
        self.x1 = x
        self.y2 = self.y1
        self.y1 = y
        return y

    def process_block(self, samples):
        return [self.process(s) for s in samples]


def apply_filter_chain(samples, filters):
    """Apply a list of Biquad filters in series."""
    out = list(samples)
    for f in filters:
        out = f.process_block(out)
    return out


# ---------------------------------------------------------------------------
# Wavetable generators (2048 samples each)
# ---------------------------------------------------------------------------

def gen_additive_saw():
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 65):
        amp = 1.0 / k
        for i in range(WAVETABLE_LEN):
            phase = TWO_PI * k * i / WAVETABLE_LEN
            buf[i] += amp * math.sin(phase)
    return normalize(buf)


def gen_additive_square():
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 64, 2):
        amp = 1.0 / k
        for i in range(WAVETABLE_LEN):
            phase = TWO_PI * k * i / WAVETABLE_LEN
            buf[i] += amp * math.sin(phase)
    return normalize(buf)


def gen_supersaw():
    ratios = [0.89, 0.94, 0.98, 1.0, 1.02, 1.06, 1.11]
    buf = [0.0] * WAVETABLE_LEN
    for r in ratios:
        for i in range(WAVETABLE_LEN):
            phase = (r * i / WAVETABLE_LEN) % 1.0
            buf[i] += 2.0 * phase - 1.0
    return normalize(buf)


def gen_digital01():
    """Hard-sync saw, ratio 1.5."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        master = i / WAVETABLE_LEN
        slave_phase = (1.5 * master) % 1.0
        buf[i] = 2.0 * slave_phase - 1.0
    return normalize(buf)


def gen_digital02():
    """Phase distortion sine."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        t = i / WAVETABLE_LEN
        if t < 0.5:
            pd = t * 1.5
        else:
            pd = 0.75 + (t - 0.5) * 0.5
        buf[i] = math.sin(TWO_PI * pd)
    return normalize(buf)


def gen_digital03():
    """Wavefolded saw: sin(3*x) waveshaper."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        saw = 2.0 * (i / WAVETABLE_LEN) - 1.0
        buf[i] = math.sin(3.0 * saw * math.pi)
    return normalize(buf)


def gen_digital04():
    """4-bit quantized saw."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        saw = 2.0 * (i / WAVETABLE_LEN) - 1.0
        buf[i] = round(saw * 8.0) / 8.0
    return normalize(buf)


def gen_digital05():
    """Ring mod: sin * sin at golden ratio."""
    phi = (1.0 + math.sqrt(5.0)) / 2.0
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        t = TWO_PI * i / WAVETABLE_LEN
        buf[i] = math.sin(t) * math.sin(phi * t)
    return normalize(buf)


def gen_digital06():
    """25% pulse wave."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        buf[i] = 1.0 if (i / WAVETABLE_LEN) < 0.25 else -1.0
    return normalize(buf)


def gen_digital07():
    """FM synthesis, mod index 3, ratio 2:1."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        t = TWO_PI * i / WAVETABLE_LEN
        buf[i] = math.sin(t + 3.0 * math.sin(2.0 * t))
    return normalize(buf)


def gen_digital08():
    """Prime harmonics only."""
    primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61]
    buf = [0.0] * WAVETABLE_LEN
    for p in primes:
        amp = 1.0 / p
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * p * i / WAVETABLE_LEN)
    return normalize(buf)


def gen_formant(formant_freqs, bandwidths=None):
    """Additive with resonant peaks at given formant frequencies.

    We treat the wavetable fundamental as ~43 Hz (44100/2048 ≈ 21.5 Hz)
    and boost harmonics near the formant centres.
    """
    fund = SAMPLE_RATE / WAVETABLE_LEN  # ~21.5 Hz
    if bandwidths is None:
        bandwidths = [120.0] * len(formant_freqs)
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 129):
        freq_k = fund * k
        amp = 0.0
        for fc, bw in zip(formant_freqs, bandwidths):
            amp += math.exp(-0.5 * ((freq_k - fc) / (bw * 0.5)) ** 2)
        amp *= 1.0 / k  # natural roll-off
        if amp < 1e-6:
            continue
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * k * i / WAVETABLE_LEN)
    return normalize(buf)


# Vowel formant frequencies (F1, F2, F3)
VOWEL_FORMANTS = {
    "ah": ([730, 1090, 2440], [100, 110, 120]),
    "ee": ([270, 2290, 3010], [80, 100, 120]),
    "oh": ([570, 840, 2410], [100, 110, 120]),
    "oo": ([300, 870, 2240], [80, 100, 110]),
}


def gen_growl01():
    """Detuned + clipped."""
    ratios = [0.99, 1.0, 1.01, 1.5, 2.01]
    buf = [0.0] * WAVETABLE_LEN
    for r in ratios:
        for i in range(WAVETABLE_LEN):
            phase = (r * i / WAVETABLE_LEN) % 1.0
            buf[i] += 2.0 * phase - 1.0
    buf = [clamp(s * 2.0) for s in buf]
    return normalize(buf)


def gen_growl02():
    """Noise + squares."""
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 32, 2):
        amp = 1.0 / k
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * k * i / WAVETABLE_LEN)
    rng = random.Random(42)
    for i in range(WAVETABLE_LEN):
        buf[i] += 0.3 * (rng.random() * 2.0 - 1.0)
    return normalize(buf)


def gen_growl03():
    """FM high modulation index."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        t = TWO_PI * i / WAVETABLE_LEN
        buf[i] = math.sin(t + 8.0 * math.sin(3.0 * t) + 2.0 * math.sin(7.0 * t))
    return normalize(buf)


def gen_growl04():
    """Comb-filter effect."""
    buf = [0.0] * WAVETABLE_LEN
    delay = 37  # samples
    for i in range(WAVETABLE_LEN):
        saw = 2.0 * (i / WAVETABLE_LEN) - 1.0
        delayed = 2.0 * (((i - delay) % WAVETABLE_LEN) / WAVETABLE_LEN) - 1.0
        buf[i] = saw + 0.7 * delayed
    return normalize(buf)


def gen_texture01():
    """Random harmonic amplitudes."""
    rng = random.Random(101)
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 65):
        amp = rng.random() / k
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * k * i / WAVETABLE_LEN)
    return normalize(buf)


def gen_texture02():
    """Stretched harmonics (f_k = k^1.1)."""
    buf = [0.0] * WAVETABLE_LEN
    for k in range(1, 33):
        ratio = k ** 1.1
        amp = 1.0 / k
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * ratio * i / WAVETABLE_LEN)
    return normalize(buf)


def gen_texture03():
    """AM — amplitude modulated sine."""
    buf = [0.0] * WAVETABLE_LEN
    for i in range(WAVETABLE_LEN):
        t = TWO_PI * i / WAVETABLE_LEN
        carrier = math.sin(t)
        mod = 0.5 + 0.5 * math.sin(5.0 * t)
        buf[i] = carrier * mod
    return normalize(buf)


def gen_texture04():
    """Bandpass noise — filtered random."""
    rng = random.Random(202)
    raw = [rng.random() * 2.0 - 1.0 for _ in range(WAVETABLE_LEN)]
    bp = Biquad("bandpass", 1200.0, SAMPLE_RATE, Q=5.0)
    buf = bp.process_block(raw)
    return normalize(buf)


def gen_texture05():
    """Non-integer harmonic ratios."""
    ratios = [1.0, 1.414, 2.236, 3.317, 4.583, 5.831]
    buf = [0.0] * WAVETABLE_LEN
    for idx, r in enumerate(ratios):
        amp = 1.0 / (idx + 1)
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * r * i / WAVETABLE_LEN)
    return normalize(buf)


def gen_texture06():
    """Spectral cloud — many close partials."""
    rng = random.Random(303)
    buf = [0.0] * WAVETABLE_LEN
    for _ in range(40):
        ratio = 1.0 + rng.random() * 15.0
        amp = rng.random() * 0.3
        phase_off = rng.random() * TWO_PI
        for i in range(WAVETABLE_LEN):
            buf[i] += amp * math.sin(TWO_PI * ratio * i / WAVETABLE_LEN + phase_off)
    return normalize(buf)


# ---------------------------------------------------------------------------
# Grain source generators (variable length)
# ---------------------------------------------------------------------------

def seconds_to_samples(sec):
    return int(SAMPLE_RATE * sec)


def gen_vox_chop(index):
    """Formant synthesis vocal sound, 0.5-1.5 s."""
    rng = random.Random(500 + index)
    dur = 0.5 + rng.random() * 1.0
    n = seconds_to_samples(dur)
    vowels = list(VOWEL_FORMANTS.keys())
    v = vowels[index % len(vowels)]
    freqs, bws = VOWEL_FORMANTS[v]

    # Generate a pulse train
    f0 = 100.0 + rng.random() * 80.0  # fundamental 100-180 Hz
    vibrato_rate = 4.0 + rng.random() * 3.0
    vibrato_depth = 0.02 + rng.random() * 0.03

    pulse = [0.0] * n
    phase = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        vib = 1.0 + vibrato_depth * math.sin(TWO_PI * vibrato_rate * t)
        freq = f0 * vib
        # Sweep f0 a bit for variety
        if index >= 4:
            freq *= 1.0 + 0.3 * math.sin(TWO_PI * 0.8 * t)
        phase += freq / SAMPLE_RATE
        if phase >= 1.0:
            phase -= 1.0
        # Pulse-like wave
        pulse[i] = 1.0 if phase < 0.1 else -0.05

    # Apply resonant bandpass filters at formant frequencies
    filters = [Biquad("bandpass", f, SAMPLE_RATE, Q=8.0) for f in freqs]
    result = [0.0] * n
    for filt in filters:
        filtered = filt.process_block(pulse)
        for i in range(n):
            result[i] += filtered[i]

    # Tremolo for some
    if index in (2, 5, 7):
        trem_rate = 5.0 + rng.random() * 4.0
        for i in range(n):
            t = i / SAMPLE_RATE
            result[i] *= 0.6 + 0.4 * math.sin(TWO_PI * trem_rate * t)

    # Amplitude envelope: quick attack, sustain, short release
    env_attack = int(0.01 * SAMPLE_RATE)
    env_release = int(0.05 * SAMPLE_RATE)
    for i in range(min(env_attack, n)):
        result[i] *= i / env_attack
    for i in range(min(env_release, n)):
        idx = n - 1 - i
        result[idx] *= i / env_release

    return normalize(result)


def gen_atmosphere(index):
    """Ambient texture, 2.5-4 s."""
    rng = random.Random(600 + index)
    dur = 2.5 + rng.random() * 1.5
    n = seconds_to_samples(dur)
    buf = [0.0] * n

    if index == 0:
        # Filtered noise sweep
        raw = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        for step in range(8):
            fc = 200.0 + step * 400.0
            lp = Biquad("lowpass", min(fc, 18000), SAMPLE_RATE, Q=1.5)
            chunk_size = n // 8
            start = step * chunk_size
            end = min(start + chunk_size, n)
            seg = lp.process_block(raw[start:end])
            for i, s in enumerate(seg):
                buf[start + i] = s
    elif index == 1:
        # Sine drone chord
        freqs = [110.0, 165.0, 220.0, 330.0, 440.0]
        for f in freqs:
            amp = 0.2
            for i in range(n):
                t = i / SAMPLE_RATE
                buf[i] += amp * math.sin(TWO_PI * f * t + 0.3 * math.sin(TWO_PI * 0.1 * t))
    elif index == 2:
        # Granular-style bursts
        grain_len = int(0.03 * SAMPLE_RATE)
        for _ in range(200):
            pos = rng.randint(0, max(0, n - grain_len - 1))
            freq = 200.0 + rng.random() * 2000.0
            amp = rng.random() * 0.3
            for j in range(grain_len):
                env = 0.5 - 0.5 * math.cos(TWO_PI * j / grain_len)
                if pos + j < n:
                    buf[pos + j] += amp * env * math.sin(TWO_PI * freq * j / SAMPLE_RATE)
    elif index == 3:
        # Shimmer clusters
        for k in range(12):
            f = 440.0 * (2.0 ** (rng.random() * 2.0 - 0.5))
            amp = 0.15
            phase_off = rng.random() * TWO_PI
            for i in range(n):
                t = i / SAMPLE_RATE
                a = amp * (0.5 + 0.5 * math.sin(TWO_PI * (0.2 + rng.random() * 0.3) * t))
                buf[i] += a * math.sin(TWO_PI * f * t + phase_off)
    elif index == 4:
        # Dark pad — low sines with slow LFO
        for k in range(6):
            f = 55.0 * (k + 1)
            amp = 0.3 / (k + 1)
            lfo_rate = 0.05 + rng.random() * 0.2
            for i in range(n):
                t = i / SAMPLE_RATE
                mod = 0.5 + 0.5 * math.sin(TWO_PI * lfo_rate * t)
                buf[i] += amp * mod * math.sin(TWO_PI * f * t)
    else:
        # FM drone
        fc = 80.0
        fm = 120.0
        mod_idx = 2.0
        for i in range(n):
            t = i / SAMPLE_RATE
            mi = mod_idx * (0.5 + 0.5 * math.sin(TWO_PI * 0.15 * t))
            buf[i] = 0.5 * math.sin(TWO_PI * fc * t + mi * math.sin(TWO_PI * fm * t))

    # Fade in/out
    fade = int(0.1 * SAMPLE_RATE)
    for i in range(min(fade, n)):
        buf[i] *= i / fade
    for i in range(min(fade, n)):
        buf[n - 1 - i] *= i / fade

    return normalize(buf)


def gen_perc(index):
    """Percussive sound, 0.5-0.8 s."""
    rng = random.Random(700 + index)
    dur = 0.5 + rng.random() * 0.3
    n = seconds_to_samples(dur)
    buf = [0.0] * n

    if index == 0:
        # Kick: pitch-dropping sine
        for i in range(n):
            t = i / SAMPLE_RATE
            freq = 150.0 * math.exp(-8.0 * t) + 40.0
            env = math.exp(-5.0 * t)
            buf[i] = env * math.sin(TWO_PI * freq * t)
    elif index == 1:
        # Snare: noise + sine
        for i in range(n):
            t = i / SAMPLE_RATE
            env_tone = math.exp(-20.0 * t)
            env_noise = math.exp(-10.0 * t)
            tone = env_tone * math.sin(TWO_PI * 180.0 * t)
            noise = env_noise * (rng.random() * 2.0 - 1.0)
            buf[i] = 0.5 * tone + 0.5 * noise
    elif index == 2:
        # Hihat: HP noise
        raw = [rng.random() * 2.0 - 1.0 for _ in range(n)]
        hp = Biquad("highpass", 6000.0, SAMPLE_RATE, Q=1.0)
        filtered = hp.process_block(raw)
        for i in range(n):
            t = i / SAMPLE_RATE
            env = math.exp(-15.0 * t)
            buf[i] = env * filtered[i]
    else:
        # Metallic hit: inharmonic sines
        partials = [1.0, 1.6, 2.3, 3.7, 5.1, 6.8]
        for p in partials:
            freq = 300.0 * p
            amp = 0.3 / p
            decay = 6.0 + p * 2.0
            for i in range(n):
                t = i / SAMPLE_RATE
                buf[i] += amp * math.exp(-decay * t) * math.sin(TWO_PI * freq * t)

    return normalize(buf)


def gen_noise(index):
    """Noise texture, 1.5-2 s."""
    rng = random.Random(800 + index)
    dur = 1.5 + rng.random() * 0.5
    n = seconds_to_samples(dur)
    buf = [0.0] * n

    if index == 0:
        # White noise
        for i in range(n):
            buf[i] = rng.random() * 2.0 - 1.0
    elif index == 1:
        # Pink-ish noise (simple filter approximation)
        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0
        for i in range(n):
            white = rng.random() * 2.0 - 1.0
            b0 = 0.99886 * b0 + white * 0.0555179
            b1 = 0.99332 * b1 + white * 0.0750759
            b2 = 0.96900 * b2 + white * 0.1538520
            b3 = 0.86650 * b3 + white * 0.3104856
            b4 = 0.55000 * b4 + white * 0.5329522
            b5 = -0.7616 * b5 - white * 0.0168980
            buf[i] = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362
            b6 = white * 0.115926
    elif index == 2:
        # Crackle: sparse impulses
        for i in range(n):
            if rng.random() < 0.005:
                buf[i] = (rng.random() * 2.0 - 1.0) * 0.9
            else:
                buf[i] = 0.0
        # Slight LP to smooth
        lp = Biquad("lowpass", 8000.0, SAMPLE_RATE, Q=0.5)
        buf = lp.process_block(buf)
    else:
        # Digital noise: sample & hold
        hold_len = 10
        val = 0.0
        for i in range(n):
            if i % hold_len == 0:
                val = rng.random() * 2.0 - 1.0
            buf[i] = val

    # Fade in/out
    fade = int(0.02 * SAMPLE_RATE)
    for i in range(min(fade, n)):
        buf[i] *= i / fade
    for i in range(min(fade, n)):
        buf[n - 1 - i] *= i / fade

    return normalize(buf)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate placeholder audio resources for a VST3 synthesizer."
    )
    script_dir = os.path.dirname(os.path.abspath(__file__))
    default_output = os.path.join(os.path.dirname(script_dir), "Resources")
    parser.add_argument(
        "--output-dir", default=default_output,
        help="Output directory (default: Resources/ relative to script's parent dir)"
    )
    args = parser.parse_args()

    wt_dir = os.path.join(args.output_dir, "Wavetables")
    gs_dir = os.path.join(args.output_dir, "GrainSources")

    # --- Wavetables ---
    print("=== Wavetables ===")
    write_wav(os.path.join(wt_dir, "BasicSaw.wav"), gen_additive_saw())
    write_wav(os.path.join(wt_dir, "BasicSquare.wav"), gen_additive_square())
    write_wav(os.path.join(wt_dir, "SuperSaw.wav"), gen_supersaw())

    digital_gens = [
        gen_digital01, gen_digital02, gen_digital03, gen_digital04,
        gen_digital05, gen_digital06, gen_digital07, gen_digital08,
    ]
    for idx, gen in enumerate(digital_gens, 1):
        write_wav(os.path.join(wt_dir, f"Digital{idx:02d}.wav"), gen())

    vowel_keys = ["ah", "ee", "oh", "oo"]
    for idx, vk in enumerate(vowel_keys, 1):
        freqs, bws = VOWEL_FORMANTS[vk]
        write_wav(os.path.join(wt_dir, f"Formant{idx:02d}.wav"), gen_formant(freqs, bws))

    growl_gens = [gen_growl01, gen_growl02, gen_growl03, gen_growl04]
    for idx, gen in enumerate(growl_gens, 1):
        write_wav(os.path.join(wt_dir, f"Growl{idx:02d}.wav"), gen())

    texture_gens = [
        gen_texture01, gen_texture02, gen_texture03,
        gen_texture04, gen_texture05, gen_texture06,
    ]
    for idx, gen in enumerate(texture_gens, 1):
        write_wav(os.path.join(wt_dir, f"Texture{idx:02d}.wav"), gen())

    # --- Grain Sources ---
    print("=== Grain Sources ===")

    for idx in range(8):
        write_wav(os.path.join(gs_dir, f"VoxChop{idx+1:02d}.wav"), gen_vox_chop(idx))

    for idx in range(6):
        write_wav(os.path.join(gs_dir, f"Atmosphere{idx+1:02d}.wav"), gen_atmosphere(idx))

    for idx in range(4):
        write_wav(os.path.join(gs_dir, f"Perc{idx+1:02d}.wav"), gen_perc(idx))

    for idx in range(4):
        write_wav(os.path.join(gs_dir, f"Noise{idx+1:02d}.wav"), gen_noise(idx))

    print("=== Done ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
