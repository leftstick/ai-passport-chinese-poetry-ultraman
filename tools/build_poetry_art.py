#!/usr/bin/env python3
"""Generate the ink-style scene backdrops and chibi Ultraman illustrations.

Every scene is 240x320 so it can fill the AI Passport panel. The chibi
Ultraman is 64x84 and stored as RGB565A8 so it can sit on top of any scene.
Run from the repository root:

    python tools/build_poetry_art.py

The output C tables are committed; Pillow is only needed when regenerating.
"""
from __future__ import annotations

import math
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    raise SystemExit("Pillow is required: python -m pip install pillow")

ROOT = Path(__file__).resolve().parent.parent
OUT_C = ROOT / "main" / "assets" / "poetry_art.c"
OUT_H = ROOT / "main" / "assets" / "poetry_art.h"

W, H = 240, 320
CHIBI_W, CHIBI_H = 64, 84
SUPERSAMPLE = 2


def lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def vertical_gradient(draw: ImageDraw.ImageDraw, size: tuple[int, int],
                      top: tuple[int, int, int],
                      bottom: tuple[int, int, int]) -> None:
    width, height = size
    for y in range(height):
        t = y / max(1, height - 1)
        color = (lerp(top[0], bottom[0], t), lerp(top[1], bottom[1], t),
                 lerp(top[2], bottom[2], t))
        draw.line([(0, y), (width, y)], fill=color)


def mountain(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]],
             color: tuple[int, int, int]) -> None:
    draw.polygon(points, fill=color)


def blossom_cluster(draw: ImageDraw.ImageDraw, cx: int, cy: int, radius: int,
                    color: tuple[int, int, int]) -> None:
    draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius],
                 fill=color)
    for angle in range(0, 360, 60):
        rad = math.radians(angle)
        px = cx + int(math.cos(rad) * radius * 0.7)
        py = cy + int(math.sin(rad) * radius * 0.7)
        r = max(2, radius // 2)
        draw.ellipse([px - r, py - r, px + r, py + r], fill=color)


def scene_spring() -> Image.Image:
    scale = SUPERSAMPLE
    img = Image.new("RGB", (W * scale, H * scale))
    draw = ImageDraw.Draw(img)
    vertical_gradient(draw, (W * scale, H * scale), (250, 238, 210),
                      (201, 226, 202))
    # Sun
    draw.ellipse([178 * scale, 28 * scale, 214 * scale, 64 * scale],
                 fill=(247, 214, 150))
    # Far and near mountains
    mountain(draw, [(0, 190 * scale), (52 * scale, 118 * scale),
                    (104 * scale, 186 * scale), (150 * scale, 126 * scale),
                    (208 * scale, 190 * scale), (240 * scale, 160 * scale),
                    (240 * scale, 240 * scale), (0, 240 * scale)],
             (154, 186, 162))
    mountain(draw, [(0, 218 * scale), (44 * scale, 168 * scale),
                    (92 * scale, 220 * scale), (142 * scale, 174 * scale),
                    (196 * scale, 224 * scale), (240 * scale, 198 * scale),
                    (240 * scale, 262 * scale), (0, 262 * scale)],
             (112, 150, 124))
    # River
    draw.polygon([(0, 250 * scale), (240 * scale, 250 * scale),
                  (240 * scale, 320 * scale), (0, 320 * scale)],
                 fill=(176, 210, 208))
    for i in range(7):
        y = (262 + i * 8) * scale
        draw.line([(14 * scale, y), (100 * scale, y)],
                  fill=(214, 236, 232), width=2 * scale)
        draw.line([(130 * scale, y + 3 * scale), (222 * scale, y + 3 * scale)],
                  fill=(214, 236, 232), width=2 * scale)
    # Foreground hill
    mountain(draw, [(0, 300 * scale), (70 * scale, 268 * scale),
                    (150 * scale, 306 * scale), (240 * scale, 274 * scale),
                    (240 * scale, 320 * scale), (0, 320 * scale)],
             (90, 128, 100))
    # Blossom trees: trunk + branches + pink clusters
    trunk = (104, 74, 58)
    draw.line([(36 * scale, 318 * scale), (40 * scale, 226 * scale)],
              fill=trunk, width=7 * scale)
    draw.line([(40 * scale, 250 * scale), (16 * scale, 210 * scale)],
              fill=trunk, width=5 * scale)
    draw.line([(40 * scale, 238 * scale), (70 * scale, 204 * scale)],
              fill=trunk, width=5 * scale)
    for cx, cy, r in ((20, 196, 20), (46, 186, 22), (72, 198, 18),
                      (32, 220, 15)):
        blossom_cluster(draw, cx * scale, cy * scale, r * scale, (240, 176, 190))
    draw.line([(204 * scale, 318 * scale), (198 * scale, 236 * scale)],
              fill=trunk, width=6 * scale)
    draw.line([(198 * scale, 258 * scale), (222 * scale, 224 * scale)],
              fill=trunk, width=4 * scale)
    for cx, cy, r in ((222, 216, 18), (196, 206, 21), (214, 236, 14)):
        blossom_cluster(draw, cx * scale, cy * scale, r * scale, (244, 186, 198))
    # Birds
    for bx, by in ((120, 74), (138, 62), (158, 80)):
        draw.arc([bx * scale, by * scale, (bx + 12) * scale, (by + 10) * scale],
                 200, 340, fill=(90, 96, 90), width=2 * scale)
    return img.resize((W, H), Image.LANCZOS)


def scene_moon() -> Image.Image:
    scale = SUPERSAMPLE
    img = Image.new("RGB", (W * scale, H * scale))
    draw = ImageDraw.Draw(img)
    vertical_gradient(draw, (W * scale, H * scale), (14, 23, 46),
                      (38, 54, 86))
    # Stars
    for x, y in ((24, 26), (52, 48), (92, 22), (146, 36), (198, 24),
                 (216, 62), (74, 78), (172, 84), (120, 96)):
        draw.ellipse([x * scale, y * scale, (x + 3) * scale, (y + 3) * scale],
                     fill=(226, 232, 220))
    # Moon
    draw.ellipse([150 * scale, 40 * scale, 206 * scale, 96 * scale],
                 fill=(246, 236, 198))
    draw.ellipse([166 * scale, 58 * scale, 176 * scale, 68 * scale],
                 fill=(232, 220, 184))
    draw.ellipse([184 * scale, 76 * scale, 192 * scale, 84 * scale],
                 fill=(232, 220, 184))
    # Window lattice
    frame = (48, 34, 30)
    draw.rectangle([22 * scale, 24 * scale, 218 * scale, 236 * scale],
                   outline=frame, width=10 * scale)
    for x in (72, 122, 172):
        draw.line([(x * scale, 30 * scale), (x * scale, 230 * scale)],
                  fill=frame, width=5 * scale)
    for y in (82, 140, 198):
        draw.line([(28 * scale, y * scale), (212 * scale, y * scale)],
                  fill=frame, width=5 * scale)
    # Moonlight falling on the floor
    draw.polygon([(70 * scale, 236 * scale), (170 * scale, 236 * scale),
                  (220 * scale, 320 * scale), (20 * scale, 320 * scale)],
                 fill=(52, 68, 100))
    draw.polygon([(96 * scale, 236 * scale), (144 * scale, 236 * scale),
                  (176 * scale, 320 * scale), (64 * scale, 320 * scale)],
                 fill=(74, 92, 124))
    # Frosty floor strokes
    for i in range(5):
        y = (262 + i * 12) * scale
        draw.line([(36 * scale, y), (204 * scale, y)],
                  fill=(104, 122, 150), width=1 * scale)
    return img.resize((W, H), Image.LANCZOS)


def scene_tower() -> Image.Image:
    scale = SUPERSAMPLE
    img = Image.new("RGB", (W * scale, H * scale))
    draw = ImageDraw.Draw(img)
    vertical_gradient(draw, (W * scale, H * scale), (250, 206, 150),
                      (122, 66, 82))
    # Sun low on the horizon
    draw.ellipse([168 * scale, 128 * scale, 214 * scale, 174 * scale],
                 fill=(230, 120, 78))
    # Far mountains
    mountain(draw, [(0, 190 * scale), (60 * scale, 140 * scale),
                    (118 * scale, 194 * scale), (176 * scale, 152 * scale),
                    (240 * scale, 196 * scale), (240 * scale, 250 * scale),
                    (0, 250 * scale)],
             (96, 70, 86))
    mountain(draw, [(0, 214 * scale), (58 * scale, 178 * scale),
                    (116 * scale, 220 * scale), (170 * scale, 186 * scale),
                    (240 * scale, 224 * scale), (240 * scale, 274 * scale),
                    (0, 274 * scale)],
             (70, 50, 66))
    # River
    draw.rectangle([0, 246 * scale, 240 * scale, 320 * scale],
                   fill=(196, 138, 96))
    for i in range(6):
        y = (256 + i * 11) * scale
        draw.line([(20 * scale, y), (220 * scale, y)],
                  fill=(228, 178, 132), width=2 * scale)
    # Pagoda tower on the left hill
    tower = (54, 40, 52)
    base_y = 250
    for tier, (cx, half) in enumerate(((56, 30), (56, 25), (56, 20))):
        y = base_y - tier * 34
        draw.polygon([((cx - half) * scale, y * scale),
                      ((cx + half) * scale, y * scale),
                      ((cx + half - 6) * scale, (y - 10) * scale),
                      ((cx - half + 6) * scale, (y - 10) * scale)],
                     fill=tower)
        draw.rectangle([(cx - half + 8) * scale, (y - 30) * scale,
                        (cx + half - 8) * scale, (y - 10) * scale],
                       fill=tower)
    draw.line([(56 * scale, 148 * scale), (56 * scale, 132 * scale)],
              fill=tower, width=3 * scale)
    # Birds
    for bx, by in ((132, 92), (150, 80), (170, 96)):
        draw.arc([bx * scale, by * scale, (bx + 12) * scale, (by + 10) * scale],
                 200, 340, fill=(64, 46, 52), width=2 * scale)
    # Foreground bank
    mountain(draw, [(0, 306 * scale), (80 * scale, 288 * scale),
                    (160 * scale, 312 * scale), (240 * scale, 292 * scale),
                    (240 * scale, 320 * scale), (0, 320 * scale)],
             (48, 34, 44))
    return img.resize((W, H), Image.LANCZOS)


def draw_chibi(pose: str) -> Image.Image:
    scale = 4
    w, h = CHIBI_W * scale, CHIBI_H * scale
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    silver = (214, 220, 230, 255)
    silver_dark = (172, 180, 194, 255)
    red = (216, 67, 59, 255)
    red_dark = (168, 44, 40, 255)
    yellow = (248, 214, 92, 255)
    blue = (74, 196, 232, 255)
    outline = (38, 42, 54, 255)

    def box(x0, y0, x1, y1, color, radius=0):
        draw.rounded_rectangle([x0 * scale, y0 * scale, x1 * scale, y1 * scale],
                               radius=radius * scale, fill=color)

    # Legs and boots
    box(21, 64, 30, 82, silver, 3)
    box(34, 64, 43, 82, silver, 3)
    box(20, 76, 31, 84, red, 2)
    box(33, 76, 44, 84, red, 2)
    # Body
    box(20, 38, 44, 68, silver, 6)
    # Chest stripes
    draw.polygon([(20 * scale, 42 * scale), (28 * scale, 38 * scale),
                  (28 * scale, 52 * scale), (20 * scale, 58 * scale)],
                 fill=red)
    draw.polygon([(44 * scale, 42 * scale), (36 * scale, 38 * scale),
                  (36 * scale, 52 * scale), (44 * scale, 58 * scale)],
                 fill=red)
    box(28, 48, 36, 66, red_dark, 2)
    # Color timer
    draw.ellipse([29 * scale, 44 * scale, 35 * scale, 50 * scale], fill=blue)
    draw.ellipse([30 * scale, 45 * scale, 33 * scale, 48 * scale],
                 fill=(212, 248, 255, 255))
    # Arms
    if pose == "hint":
        draw.line([(21 * scale, 48 * scale), (12 * scale, 40 * scale),
                   (10 * scale, 30 * scale)], fill=silver, width=7 * scale,
                  joint="curve")
        draw.line([(10 * scale, 30 * scale), (10 * scale, 26 * scale)],
                  fill=silver, width=5 * scale)
        draw.line([(9 * scale, 26 * scale), (9 * scale, 22 * scale)],
                  fill=red, width=3 * scale)
        draw.line([(43 * scale, 48 * scale), (48 * scale, 60 * scale)],
                  fill=silver, width=7 * scale, joint="curve")
    elif pose == "cheer":
        draw.line([(21 * scale, 48 * scale), (12 * scale, 38 * scale),
                   (8 * scale, 26 * scale)], fill=silver, width=7 * scale,
                  joint="curve")
        draw.line([(43 * scale, 48 * scale), (52 * scale, 38 * scale),
                   (56 * scale, 26 * scale)], fill=silver, width=7 * scale,
                  joint="curve")
        box(5, 22, 12, 28, red, 2)
        box(52, 22, 59, 28, red, 2)
    else:
        draw.line([(21 * scale, 48 * scale), (14 * scale, 58 * scale)],
                  fill=silver, width=7 * scale, joint="curve")
        draw.line([(43 * scale, 48 * scale), (50 * scale, 58 * scale)],
                  fill=silver, width=7 * scale, joint="curve")
        box(11, 56, 17, 62, red, 2)
        box(47, 56, 53, 62, red, 2)
    # Head
    draw.ellipse([14 * scale, 4 * scale, 50 * scale, 40 * scale], fill=silver,
                 outline=outline, width=2 * scale)
    # Crest
    draw.polygon([(32 * scale, 4 * scale), (26 * scale, 12 * scale),
                  (32 * scale, 10 * scale), (38 * scale, 12 * scale)],
                 fill=red)
    draw.line([(32 * scale, 6 * scale), (32 * scale, 18 * scale)],
              fill=red_dark, width=3 * scale)
    # Eyes
    draw.polygon([(19 * scale, 20 * scale), (29 * scale, 17 * scale),
                  (27 * scale, 26 * scale), (20 * scale, 26 * scale)],
                 fill=yellow)
    draw.polygon([(45 * scale, 20 * scale), (35 * scale, 17 * scale),
                  (37 * scale, 26 * scale), (44 * scale, 26 * scale)],
                 fill=yellow)
    # Cheek highlights
    draw.ellipse([18 * scale, 30 * scale, 24 * scale, 34 * scale],
                 fill=(244, 176, 164, 255))
    draw.ellipse([40 * scale, 30 * scale, 46 * scale, 34 * scale],
                 fill=(244, 176, 164, 255))
    # Soft outline around the body
    draw.rounded_rectangle([20 * scale, 38 * scale, 44 * scale, 68 * scale],
                           radius=6 * scale, outline=outline, width=2 * scale)
    return img.resize((CHIBI_W, CHIBI_H), Image.LANCZOS)


def rgb565_bytes(img: Image.Image) -> bytes:
    out = bytearray()
    for r, g, b, *_ in img.convert("RGBA").getdata():
        value = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
        out.append(value & 0xFF)
        out.append(value >> 8)
    return bytes(out)


def rgb565a8_bytes(img: Image.Image) -> bytes:
    rgba = img.convert("RGBA")
    color = rgb565_bytes(rgba)
    alpha = bytes(pixel[3] for pixel in rgba.getdata())
    return color + alpha


def emit_array(name: str, data: bytes) -> str:
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 16):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i:i + 16])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def emit_descriptor(name: str, data_name: str, width: int, height: int,
                    fmt: str) -> str:
    stride = width * (2 if fmt == "rgb565" else 3)
    cf = "LV_COLOR_FORMAT_RGB565" if fmt == "rgb565" else "LV_COLOR_FORMAT_RGB565A8"
    return f"""static const lv_image_dsc_t {name} = {{
    .header = {{
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = {cf},
        .flags = 0,
        .w = {width},
        .h = {height},
        .stride = {stride},
        .reserved_2 = 0,
    }},
    .data_size = sizeof({data_name}),
    .data = {data_name},
    .reserved = NULL,
}};"""


def main() -> int:
    scenes = [scene_spring(), scene_moon(), scene_tower()]
    chibis = [draw_chibi("idle"), draw_chibi("hint"), draw_chibi("cheer")]

    parts = [
        "// Generated by tools/build_poetry_art.py. Do not edit by hand.",
        '#include "poetry_art.h"',
        "",
    ]
    for index, img in enumerate(scenes):
        data = rgb565_bytes(img)
        parts.append(emit_array(f"scene_{index}", data))
        parts.append(emit_descriptor(f"scene_{index}_dsc", f"scene_{index}",
                                     W, H, "rgb565"))
        parts.append("")
    for index, img in enumerate(chibis):
        data = rgb565a8_bytes(img)
        parts.append(emit_array(f"chibi_{index}", data))
        parts.append(emit_descriptor(f"chibi_{index}_dsc", f"chibi_{index}",
                                     CHIBI_W, CHIBI_H, "rgb565a8"))
        parts.append("")

    parts.append("""const lv_image_dsc_t *poetry_art_scene(uint8_t stage)
{
    switch (stage) {
    case 0:
        return &scene_0_dsc;
    case 1:
        return &scene_1_dsc;
    case 2:
        return &scene_2_dsc;
    default:
        return &scene_0_dsc;
    }
}

const lv_image_dsc_t *poetry_art_ultraman(uint8_t pose)
{
    switch (pose) {
    case POETRY_ULTRAMAN_HINT:
        return &chibi_1_dsc;
    case POETRY_ULTRAMAN_CHEER:
        return &chibi_2_dsc;
    case POETRY_ULTRAMAN_IDLE:
    default:
        return &chibi_0_dsc;
    }
}
""")
    OUT_C.write_text("\n".join(parts), encoding="utf-8")

    OUT_H.write_text("""// Generated by tools/build_poetry_art.py. Do not edit by hand.
#pragma once

#include <stdint.h>

#include "lvgl.h"

typedef enum {
    POETRY_ULTRAMAN_IDLE = 0,
    POETRY_ULTRAMAN_HINT,
    POETRY_ULTRAMAN_CHEER,
    POETRY_ULTRAMAN_POSE_COUNT,
} poetry_ultraman_pose_t;

const lv_image_dsc_t *poetry_art_scene(uint8_t stage);
const lv_image_dsc_t *poetry_art_ultraman(uint8_t pose);
""", encoding="utf-8")

    print(f"Generated {OUT_C.relative_to(ROOT)} ({len(scenes)} scenes, "
          f"{len(chibis)} chibi poses, {W}x{H})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
