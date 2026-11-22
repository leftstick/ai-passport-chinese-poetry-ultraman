<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, generated poetry art, poetry audio, and battery profiles, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Poetry art

The adventure scenes and the chibi Ultraman sprites are generated, not hand-drawn
binaries. [`../tools/build_poetry_art.py`](../tools/build_poetry_art.py) draws the
three 240 × 320 RGB565 backdrops (spring blossoms, moonlit window, tower and river
at sunset) and the three 64 × 84 RGB565A8 Ultraman poses (idle, hint, cheer), then
writes `main/assets/poetry_art.c` and `main/assets/poetry_art.h`.

- Re-run the generator after changing composition, palette, or pose geometry; do
  not edit the generated C by hand.
- Keep the display contract in `tools/check_assets.py`: three scenes at 240 × 320
  and three 64 × 84 sprites with exported accessor functions.
- The generator uses only Pillow and the Python standard library, so it runs
  offline and adds no runtime dependency to the firmware.

## Poetry audio

Text-to-speech narration and the generated guofeng background music and cues live
in [`audio/`](audio/). Provenance is documented in
[`audio/SOURCES.md`](audio/SOURCES.md).

| Path | Content |
| --- | --- |
| [`audio/narration/poem_7.wav`](audio/narration/poem_7.wav) | Mandarin narration of *Chunxiao* (Spring Dawn), 16 kHz 16-bit mono PCM. |
| [`audio/narration/poem_2.wav`](audio/narration/poem_2.wav) | Mandarin narration of *Jingye Si* (Quiet Night Thoughts). |
| [`audio/narration/poem_16.wav`](audio/narration/poem_16.wav) | Mandarin narration of *Deng Guanque Lou* (Climbing Stork Tower). |

[`../tools/build_poetry_audio.py`](../tools/build_poetry_audio.py) normalizes the
committed narration recordings, synthesizes the 12.8 s pentatonic background loop
and the five short cues (move, confirm, correct, wrong, unlock), and writes
`main/audio/poetry_audio.c` plus `main/audio/poetry_audio.h`.

- Keep the clip budget enforceable: `tools/check_assets.py` caps the embedded
  poetry audio at 1,200,000 samples so the factory partition still fits.
- Narration is regenerated with Windows SAPI (`Microsoft Huihui Desktop`, Chinese
  mainland voice) at rate −2; the committed WAVs are the source of truth so other
  platforms can rebuild the firmware without SAPI.
- Do not commit media without redistribution permission.

## Battery profiles

[`battery/wanli-500mah-profile2-xil40x-20261020.txt`](battery/wanli-500mah-profile2-xil40x-20261020.txt)
contains the supplied CW2017 profile for the Wanli New Energy 500 mAh cell
(profile2, XIL40X, 2026-10-20). It is an unchanged copy of the project-supplied
file, retained at the maintainer's request for this firmware update. Its
original filename is recorded in the paired Chinese document. No separate
license was included in the supplied file.

The format is 80 lines of two hexadecimal digits, one byte per line in register
order. The BSP embeds those bytes in `components/bsp/src/bsp_battery.c`; the
host gate checks the source file and initializer for an exact match. Text-file
SHA-256: `7ea96eba4634f4495a37c25ffacd54c1deaa0693b162c892d671a2ba68460919`.
Decoded 80-byte SHA-256:
`fa7365671093ef449c5bdcfb4adb279e3d275f888109f50b6b2f21e9b96220d5`.
This is a fuel-gauge profile, not a charging configuration or a generic profile
for every 500 mAh cell; see the [hardware guide](../docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md#9-cw2017-fuel-gauge).
