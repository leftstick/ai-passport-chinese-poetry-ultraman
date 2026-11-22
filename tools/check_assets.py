#!/usr/bin/env python3
"""Sanity-check the generated poetry art and audio tables."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ART_C = ROOT / "main" / "assets" / "poetry_art.c"
ART_H = ROOT / "main" / "assets" / "poetry_art.h"
AUDIO_C = ROOT / "main" / "audio" / "poetry_audio.c"
AUDIO_H = ROOT / "main" / "audio" / "poetry_audio.h"
NARRATION_DIR = ROOT / "assets" / "audio" / "narration"
AUDIO_DOCS = [
    ROOT / "assets" / "audio" / "SOURCES.md",
    ROOT / "assets" / "audio" / "SOURCES.zh_CN.md",
]

SCENE_COUNT = 3
CHIBI_COUNT = 3
NARRATION_IDS = (7, 2, 16)
SFX_COUNT = 5
PCM_BUDGET_SAMPLES = 1_200_000


def check_art(errors: list[str]) -> tuple[int, int]:
    if not ART_C.is_file() or not ART_H.is_file():
        errors.append("missing main/assets/poetry_art.c or poetry_art.h")
        return 0, 0
    text = ART_C.read_text(encoding="utf-8")
    descriptors = re.findall(
        r"static const lv_image_dsc_t (\w+)_dsc = \{(.*?)\n\};", text, re.S)
    if len(descriptors) != SCENE_COUNT + CHIBI_COUNT:
        errors.append(
            f"poetry_art.c: expected {SCENE_COUNT + CHIBI_COUNT} descriptors, "
            f"found {len(descriptors)}")
    for name, body in descriptors:
        width = re.search(r"\.w = (\d+)", body)
        height = re.search(r"\.h = (\d+)", body)
        fmt = re.search(r"\.cf = (LV_COLOR_FORMAT_\w+)", body)
        if not width or not height or not fmt:
            errors.append(f"poetry_art.c: {name} is missing header fields")
            continue
        w, h = int(width.group(1)), int(height.group(1))
        expected = (240, 320, "LV_COLOR_FORMAT_RGB565") if name.startswith(
            "scene_") else (64, 84, "LV_COLOR_FORMAT_RGB565A8")
        if (w, h, fmt.group(1)) != expected:
            errors.append(
                f"poetry_art.c: {name} is {w}x{h} {fmt.group(1)}, "
                f"expected {expected[0]}x{expected[1]} {expected[2]}")
    header = ART_H.read_text(encoding="utf-8")
    for symbol in ("poetry_art_scene", "poetry_art_ultraman",
                   "POETRY_ULTRAMAN_HINT", "POETRY_ULTRAMAN_CHEER"):
        if symbol not in header:
            errors.append(f"poetry_art.h: missing {symbol}")
    return len(descriptors), sum(
        int(re.search(r"\.w = (\d+)", body).group(1)) *
        int(re.search(r"\.h = (\d+)", body).group(1)) *
        (3 if "RGB565A8" in body else 2)
        for _, body in descriptors)


def check_audio(errors: list[str]) -> int:
    if not AUDIO_C.is_file() or not AUDIO_H.is_file():
        errors.append("missing main/audio/poetry_audio.c or poetry_audio.h")
        return 0
    text = AUDIO_C.read_text(encoding="utf-8")
    header = AUDIO_H.read_text(encoding="utf-8")
    total = 0
    for match in re.finditer(r"static const int16_t (\w+)\[\] = \{(.*?)\n\};",
                             text, re.S):
        values = re.findall(r"-?\d+", match.group(2))
        total += len(values)
        if not values:
            errors.append(f"poetry_audio.c: {match.group(1)} is empty")
    if total == 0:
        errors.append("poetry_audio.c: no PCM samples found")
    elif total > PCM_BUDGET_SAMPLES:
        errors.append(
            f"poetry_audio: {total} samples exceed budget {PCM_BUDGET_SAMPLES}")
    for poem_id in NARRATION_IDS:
        if f"narration_{poem_id}" not in text:
            errors.append(f"poetry_audio.c: missing narration for poem {poem_id}")
        wav = NARRATION_DIR / f"poem_{poem_id}.wav"
        if not wav.is_file():
            errors.append(f"missing narration source {wav.relative_to(ROOT)}")
    if text.count("static const audio_clip_t NARRATION_") != len(NARRATION_IDS):
        errors.append("poetry_audio.c: narration clip table is incomplete")
    for name in ("POETRY_SFX_MOVE", "POETRY_SFX_CORRECT", "POETRY_SFX_UNLOCK"):
        if name not in header:
            errors.append(f"poetry_audio.h: missing {name}")
    if text.count(".samples = sfx_") != SFX_COUNT:
        errors.append(f"poetry_audio.c: expected {SFX_COUNT} sound cues")
    for doc in AUDIO_DOCS:
        if not doc.is_file():
            errors.append(f"missing audio documentation: {doc.relative_to(ROOT)}")
    return total


def main() -> int:
    errors: list[str] = []
    sprites, art_bytes = check_art(errors)
    samples = check_audio(errors)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Assets: {sprites} art descriptors ({art_bytes / 1024:.0f} KiB), "
          f"{samples / 16000:.2f}s PCM")
    print("Asset tables: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
