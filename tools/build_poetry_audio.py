#!/usr/bin/env python3
"""Build the poetry narration, guofeng background loop, and gentle cues.

Narration is spoken with the Windows zh-CN voice on first run and cached as
16 kHz mono WAV under assets/audio/narration. The background loop and cues are
synthesised from scratch, so the generated C tables are reproducible.

    python tools/build_poetry_audio.py          # use cached narration
    python tools/build_poetry_audio.py --tts    # re-record narration first
"""
from __future__ import annotations

import argparse
import array
import math
import random
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NARRATION_DIR = ROOT / "assets" / "audio" / "narration"
OUT_C = ROOT / "main" / "audio" / "poetry_audio.c"
OUT_H = ROOT / "main" / "audio" / "poetry_audio.h"
RATE = 16000

POEMS = {
    7: "春晓。孟浩然。春眠不觉晓，处处闻啼鸟。夜来风雨声，花落知多少。",
    2: "静夜思。李白。床前明月光，疑是地上霜。举头望明月，低头思故乡。",
    16: "登鹳雀楼。王之涣。白日依山尽，黄河入海流。欲穷千里目，更上一层楼。",
}


def clamp_sample(value: float) -> int:
    return max(-32768, min(32767, int(round(value))))


def write_wav(path: Path, samples: list[int]) -> None:
    data = array.array("h", (max(-32768, min(32767, s)) for s in samples))
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(data.tobytes())


def read_wav(path: Path) -> tuple[list[float], int]:
    with wave.open(str(path), "rb") as src:
        channels = src.getnchannels()
        width = src.getsampwidth()
        rate = src.getframerate()
        frames = src.readframes(src.getnframes())
    if width != 2:
        raise SystemExit(f"{path}: only 16-bit WAV is supported")
    values = array.array("h")
    values.frombytes(frames)
    if sys.byteorder == "big":
        values.byteswap()
    if channels > 1:
        mono = [sum(values[i:i + channels]) / channels
                for i in range(0, len(values), channels)]
    else:
        mono = list(values)
    return mono, rate


def resample(samples: list[float], source_rate: int) -> list[float]:
    if source_rate == RATE or not samples:
        return samples
    ratio = source_rate / RATE
    out_len = int(len(samples) / ratio)
    out: list[float] = []
    for i in range(out_len):
        pos = i * ratio
        left = int(pos)
        right = min(left + 1, len(samples) - 1)
        frac = pos - left
        out.append(samples[left] * (1 - frac) + samples[right] * frac)
    return out


def trim_and_normalize(samples: list[float]) -> list[int]:
    if not samples:
        return [0] * 400
    peak = max(abs(v) for v in samples) or 1.0
    scaled = [v / peak * 0.86 for v in samples]
    threshold = 0.015
    start = 0
    while start < len(scaled) and abs(scaled[start]) < threshold:
        start += 1
    end = len(scaled)
    while end > start and abs(scaled[end - 1]) < threshold:
        end -= 1
    pad = RATE // 20
    start = max(0, start - pad)
    end = min(len(scaled), end + pad)
    return [clamp_sample(v * 32767) for v in scaled[start:end]]


def synthesize_tts(text: str, target: Path) -> None:
    if sys.platform != "win32":
        raise SystemExit("Narration recording requires Windows SAPI")
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        text_file = tmp_path / "text.txt"
        text_file.write_text(text, encoding="utf-8")
        raw = tmp_path / "raw.wav"
        script = f"""
Add-Type -AssemblyName System.Speech
$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
$voice = $synth.GetInstalledVoices() | Where-Object {{ $_.VoiceInfo.Culture.Name -eq 'zh-CN' }} | Select-Object -First 1
if ($voice) {{ $synth.SelectVoice($voice.VoiceInfo.Name) }}
$synth.Rate = -2
$synth.Volume = 100
$synth.SetOutputToWaveFile('{raw.as_posix()}')
$text = Get-Content -Raw -Encoding UTF8 '{text_file.as_posix()}'
$synth.Speak($text)
$synth.Dispose()
"""
        subprocess.run(["powershell", "-NoProfile", "-Command", script],
                       check=True)
        samples, rate = read_wav(raw)
        done = trim_and_normalize(resample(samples, rate))
        write_wav(target, done)
    print(f"Recorded {target.relative_to(ROOT)} ({len(done) / RATE:.1f}s)")


def ensure_narration(force: bool) -> dict[int, list[int]]:
    NARRATION_DIR.mkdir(parents=True, exist_ok=True)
    clips: dict[int, list[int]] = {}
    for poem_id, text in POEMS.items():
        path = NARRATION_DIR / f"poem_{poem_id}.wav"
        if force or not path.is_file():
            synthesize_tts(text, path)
        samples, rate = read_wav(path)
        clips[poem_id] = trim_and_normalize(resample(samples, rate))
        print(f"Narration {poem_id}: {len(clips[poem_id]) / RATE:.1f}s")
    return clips


def pluck(freq: float, seconds: float, decay: float = 3.2) -> list[float]:
    count = int(seconds * RATE)
    attack = max(1, int(0.008 * RATE))
    out = [0.0] * count
    for i in range(count):
        t = i / RATE
        env = math.exp(-decay * t)
        if i < attack:
            env *= i / attack
        value = (math.sin(2 * math.pi * freq * t) * 0.62 +
                 math.sin(4 * math.pi * freq * t) * 0.22 +
                 math.sin(6 * math.pi * freq * t) * 0.09)
        out[i] = value * env
    return out


def make_bgm() -> list[int]:
    seconds = 12.8
    total = int(seconds * RATE)
    mix = [0.0] * total
    # Pentatonic C major: C D E G A
    notes = [261.63, 293.66, 329.63, 392.00, 440.00]
    melody = [0, 2, 4, 2, 3, 1, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0]
    beat = 0.8
    for index, step in enumerate(melody):
        start = int(index * beat * RATE)
        freq = notes[step % len(notes)]
        if index % 4 == 3:
            freq /= 2.0
        voice = pluck(freq, 1.6, decay=3.0)
        for i, value in enumerate(voice):
            pos = start + i
            if pos >= total:
                break
            mix[pos] += value * 0.16
    # Slow low pad
    for freq, gain in ((130.81, 0.035), (196.00, 0.025)):
        for i in range(total):
            t = i / RATE
            swell = 0.7 + 0.3 * math.sin(2 * math.pi * 0.12 * t)
            mix[i] += math.sin(2 * math.pi * freq * t) * gain * swell
    # Subtle water-like shimmer
    rng = random.Random(20261122)
    shimmer = [0.0] * total
    for _ in range(70):
        start = rng.randrange(0, max(1, total - RATE // 8))
        freq = rng.choice([880.0, 1174.66, 1318.51, 1567.98])
        voice = pluck(freq, 0.16, decay=8.0)
        for i, value in enumerate(voice):
            if start + i < total:
                shimmer[start + i] += value * 0.012
    for i in range(total):
        mix[i] += shimmer[i]
    # Fade the edges so the loop restart does not click.
    fade = RATE // 50
    for i in range(fade):
        mix[i] *= i / fade
        mix[total - 1 - i] *= i / fade
    peak = max(abs(v) for v in mix) or 1.0
    return [clamp_sample(v / peak * 0.5 * 32767) for v in mix]


def tone(freq: float, seconds: float, gain: float = 0.5,
         decay: float = 6.0, start_phase: float = 0.0) -> list[float]:
    count = int(seconds * RATE)
    attack = max(1, int(0.01 * RATE))
    out = [0.0] * count
    for i in range(count):
        t = i / RATE
        env = math.exp(-decay * t)
        if i < attack:
            env *= i / attack
        out[i] = math.sin(2 * math.pi * freq * t + start_phase) * env * gain
    return out


def concat(parts: list[list[float]], gaps: list[float]) -> list[float]:
    out: list[float] = []
    for index, part in enumerate(parts):
        out.extend(part)
        if index < len(gaps):
            out.extend([0.0] * int(gaps[index] * RATE))
    return out


def make_sfx() -> list[list[int]]:
    move = tone(1046.5, 0.06, 0.35, decay=28.0)
    confirm = concat([tone(659.25, 0.08, 0.4, 12.0),
                      tone(880.0, 0.12, 0.4, 10.0)], [0.02])
    correct = concat([tone(523.25, 0.11, 0.4, 9.0),
                      tone(659.25, 0.11, 0.4, 9.0),
                      tone(783.99, 0.22, 0.45, 6.0)], [0.015, 0.015])
    wrong = concat([tone(392.0, 0.16, 0.32, 8.0),
                    tone(329.63, 0.24, 0.3, 6.0)], [0.02])
    unlock = concat([tone(523.25, 0.12, 0.42, 8.0),
                     tone(659.25, 0.12, 0.42, 8.0),
                     tone(783.99, 0.12, 0.42, 8.0),
                     tone(1046.5, 0.46, 0.5, 4.0)], [0.015, 0.015, 0.015])
    clips = [move, confirm, correct, wrong, unlock]
    normalized: list[list[int]] = []
    for clip in clips:
        peak = max(abs(v) for v in clip) or 1.0
        normalized.append([clamp_sample(v / peak * 0.8 * 32767) for v in clip])
    return normalized


def emit_array(name: str, samples: list[int]) -> str:
    lines = [f"static const int16_t {name}[] = {{"]
    for i in range(0, len(samples), 12):
        chunk = ", ".join(str(v) for v in samples[i:i + 12])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tts", action="store_true",
                        help="re-record narration with the Windows zh-CN voice")
    args = parser.parse_args()

    narration = ensure_narration(args.tts)
    bgm = make_bgm()
    sfx = make_sfx()

    parts = [
        "// Generated by tools/build_poetry_audio.py. Do not edit by hand.",
        '#include "poetry_audio.h"',
        "",
        emit_array("bgm_samples", bgm),
        "",
    ]
    for poem_id in sorted(narration):
        parts.append(emit_array(f"narration_{poem_id}", narration[poem_id]))
        parts.append("")
    for index, clip in enumerate(sfx):
        parts.append(emit_array(f"sfx_{index}", clip))
        parts.append("")

    parts.append("""static const audio_clip_t BGM_CLIP = {
    .samples = bgm_samples,
    .count = sizeof(bgm_samples) / sizeof(bgm_samples[0]),
};
""")
    for poem_id in sorted(narration):
        parts.append(f"""static const audio_clip_t NARRATION_{poem_id} = {{
    .samples = narration_{poem_id},
    .count = sizeof(narration_{poem_id}) / sizeof(narration_{poem_id}[0]),
}};
""")
    parts.append("static const audio_clip_t *const SFX_CLIPS[] = {")
    for index in range(len(sfx)):
        parts.append(f"    &(const audio_clip_t){{ .samples = sfx_{index}, "
                     f".count = sizeof(sfx_{index}) / sizeof(sfx_{index}[0]) }},")
    parts.append("};")
    parts.append("""
const audio_clip_t *poetry_audio_bgm(void)
{
    return &BGM_CLIP;
}

const audio_clip_t *poetry_audio_narration(uint16_t poem_id)
{
    switch (poem_id) {
    case 7:
        return &NARRATION_7;
    case 2:
        return &NARRATION_2;
    case 16:
        return &NARRATION_16;
    default:
        return NULL;
    }
}

const audio_clip_t *poetry_audio_sfx(uint8_t id)
{
    if (id >= POETRY_SFX_COUNT) {
        return NULL;
    }
    return SFX_CLIPS[id];
}
""")
    OUT_C.write_text("\n".join(parts), encoding="utf-8")
    OUT_H.write_text("""// Generated by tools/build_poetry_audio.py. Do not edit by hand.
#pragma once

#include <stdint.h>

#include "audio_player.h"

typedef enum {
    POETRY_SFX_MOVE = 0,
    POETRY_SFX_CONFIRM,
    POETRY_SFX_CORRECT,
    POETRY_SFX_WRONG,
    POETRY_SFX_UNLOCK,
    POETRY_SFX_COUNT,
} poetry_sfx_t;

const audio_clip_t *poetry_audio_bgm(void);
const audio_clip_t *poetry_audio_narration(uint16_t poem_id);
const audio_clip_t *poetry_audio_sfx(uint8_t id);
""", encoding="utf-8")

    print(f"Generated {OUT_C.relative_to(ROOT)}: BGM {len(bgm) / RATE:.1f}s, "
          f"{len(narration)} narrations, {len(sfx)} cues")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
