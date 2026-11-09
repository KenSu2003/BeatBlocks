#!/usr/bin/env python3
"""Generate higher-quality kick/snare PCM headers for the ESP32 drum sketch."""

from __future__ import annotations

from pathlib import Path

import numpy as np

SR = 44100
ROOT = Path(__file__).resolve().parent
OUT_DIRS = (
    ROOT / "drum_samples",
    ROOT / "pattern_test",
    ROOT / "reed_bridge",
)


def soft_limit(x: np.ndarray, drive: float = 1.35) -> np.ndarray:
    y = np.tanh(x * drive)
    return y / (np.max(np.abs(y)) or 1.0)


def to_i16(x: np.ndarray, peak: float = 0.82) -> np.ndarray:
    x = soft_limit(np.asarray(x, dtype=np.float64))
    x = x * peak
    return np.clip(x * 32767.0, -32768, 32767).astype(np.int16)


def one_pole_lowpass(x: np.ndarray, cutoff_hz: float, sr: int) -> np.ndarray:
    # Simple smoothing filter to tame harsh highs on a small speaker.
    a = np.exp(-2.0 * np.pi * cutoff_hz / sr)
    y = np.empty_like(x)
    acc = 0.0
    for i, v in enumerate(x):
        acc = (1.0 - a) * v + a * acc
        y[i] = acc
    return y


def one_pole_highpass(x: np.ndarray, cutoff_hz: float, sr: int) -> np.ndarray:
    a = np.exp(-2.0 * np.pi * cutoff_hz / sr)
    y = np.empty_like(x)
    prev_x = 0.0
    prev_y = 0.0
    for i, v in enumerate(x):
        prev_y = a * (prev_y + v - prev_x)
        prev_x = v
        y[i] = prev_y
    return y


def kick(sr: int = SR, dur: float = 0.38) -> np.ndarray:
    n = int(sr * dur)
    t = np.arange(n) / sr

    # Fast pitch drop for the punchy body.
    f_start, f_end = 195.0, 38.0
    freq = f_end + (f_start - f_end) * np.exp(-t * 22.0)
    phase = 2.0 * np.pi * np.cumsum(freq) / sr
    body = np.sin(phase) * np.exp(-t * 7.5)

    # Sub layer for speaker thump.
    sub = np.sin(2.0 * np.pi * 48.0 * t) * np.exp(-t * 5.5) * 0.55

    # Soft beater click (filtered, short).
    rng = np.random.default_rng(3)
    click_noise = rng.standard_normal(n) * np.exp(-t * 140.0)
    click_noise = one_pole_highpass(click_noise, 900.0, sr)
    click_noise = one_pole_lowpass(click_noise, 4500.0, sr) * 0.22
    click_tone = np.sin(2.0 * np.pi * 1400.0 * t) * np.exp(-t * 95.0) * 0.18

    mix = body + sub + click_noise + click_tone
    mix = one_pole_lowpass(mix, 6500.0, sr)
    return to_i16(mix, peak=0.88)


def snare(sr: int = SR, dur: float = 0.30) -> np.ndarray:
    n = int(sr * dur)
    t = np.arange(n) / sr
    rng = np.random.default_rng(11)

    noise = rng.standard_normal(n)
    noise = one_pole_highpass(noise, 700.0, sr)
    noise = one_pole_lowpass(noise, 7000.0, sr)
    noise_env = 0.65 * np.exp(-t * 18.0) + 0.35 * np.exp(-t * 55.0)
    snares = noise * noise_env

    # Drum shell / tone body.
    body = (
        np.sin(2.0 * np.pi * 185.0 * t) * np.exp(-t * 16.0) * 0.45
        + np.sin(2.0 * np.pi * 330.0 * t) * np.exp(-t * 24.0) * 0.22
    )

    # Transient snap.
    snap = np.sin(2.0 * np.pi * 2500.0 * t) * np.exp(-t * 110.0) * 0.25
    snap += one_pole_highpass(rng.standard_normal(n), 2000.0, sr) * np.exp(-t * 130.0) * 0.12

    mix = snares + body + snap
    mix = one_pole_lowpass(mix, 9000.0, sr)
    return to_i16(mix, peak=0.84)


def write_header(path: Path, name: str, data: np.ndarray, sr: int) -> None:
    uname = name.upper()
    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        f"// Auto-generated {name} sample @ {sr} Hz, mono int16",
        f"static const int {uname}_SAMPLE_RATE = {sr};",
        f"static const size_t {uname}_NUM_SAMPLES = {len(data)};",
        f"static const int16_t {uname}_PCM[{len(data)}] PROGMEM = {{",
    ]
    chunk: list[str] = []
    for v in data:
        chunk.append(str(int(v)))
        if len(chunk) == 16:
            lines.append("  " + ", ".join(chunk) + ",")
            chunk = []
    if chunk:
        lines.append("  " + ", ".join(chunk) + ",")
    lines.append("};")
    lines.append("")
    path.write_text("\n".join(lines))
    print(f"wrote {path} ({len(data)} samples, {len(data) / sr:.3f}s)")


def main() -> None:
    kick_pcm = kick()
    snare_pcm = snare()
    for out in OUT_DIRS:
        out.mkdir(parents=True, exist_ok=True)
        write_header(out / "kick_pcm.h", "kick", kick_pcm, SR)
        write_header(out / "snare_pcm.h", "snare", snare_pcm, SR)


if __name__ == "__main__":
    main()
