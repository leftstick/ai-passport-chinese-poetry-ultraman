<p align="right">
  <a href="device-test-plan.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# On-Device Test Plan

Use this checklist to validate the poetry collection adventure on real hardware.
Flashing is a destructive action: obtain explicit approval, then write the merged
image from `0x0` as described in the root README. Do not perform a full-chip erase
unless separately approved.

## Preconditions

- ESP32-C3 AI Passport with a working 240 × 320 display, three buttons, speaker,
  and CW2017 battery gauge.
- ESP-IDF 5.5.3 build of this repository, or the released
  `FoloToy-AI-Passport-full.bin`.
- Stable power: charged battery or a data-capable USB connection.
- Keep notes of each step so failures can be reproduced.

## Test cases

| ID | Area | Steps | Expected result |
| --- | --- | --- | --- |
| D01 | Boot | Power on and wait for the first screen. | The home screen shows the ink landscape, chibi Ultraman, the adventure title, four menu cards, a battery percentage, and the footer hint. No blank screen, flicker loop, or reset loop. |
| D02 | Menu navigation | Press UP and DOWN repeatedly on the home screen. | The highlight wraps through the four items in both directions and each move plays a soft cue. |
| D03 | Map entry | Select the first menu item and press OK. | The poem map opens with three numbered cards, the scene name, poem title, collection count, and the correct collected/locked labels. Long OK returns home. |
| D04 | Stage intro | Open stage 1 and read the intro. | The stage shows its landscape, chibi Ultraman, scene name, poem title, dynasty and author, and the intro text without clipping or overlapping. OK starts the challenges; long OK returns to the map. |
| D05 | Order interaction | In a stage, move to the first challenge. Pick a tile with OK, move, press OK again, then move to the check tile and press OK. | The first OK highlights a picked tile, the second OK swaps it with the cursor tile, and check submits the order. A correct order advances to challenge 2/4. |
| D06 | Gentle hint | Submit a deliberately wrong order twice. | The first wrong attempt only plays a soft cue and keeps the board editable. The second wrong attempt shows a poem-based hint in the hint bar. No life, timer, or game-over appears. |
| D07 | Next-line interaction | Complete challenge 1, then answer challenge 2. | The header reads 2/4 and the matching interaction name. The current line, three options, and the hint bar are readable. A wrong option is retryable; a correct option advances to 3/4. |
| D08 | Picture interaction | Open challenge 3 and inspect the illustration. | The full-screen scene is visible above the three title options, the chibi guide does not cover the options, and the footer or hint remains readable. |
| D09 | Blank interaction | Answer challenge 4. | The poem line contains a visible blank, the three character options are large enough to read, and a wrong choice remains retryable. |
| D10 | Reward and narration | Finish challenge 4. | The reward screen shows the complete poem, dynasty/author, explanation, chibi Ultraman, and the collection message. Full-poem Mandarin narration starts and the unlock cue plays. |
| D11 | Codex entry | Leave the reward screen, return home, and open the second menu item. | The collected poem appears with its title, author, and scene; uncollected poems show a locked placeholder and cannot be opened. |
| D12 | Codex replay | Open a collected poem from the codex. | The detail page shows the full poem and explanation. OK replays the narration; long OK returns to the codex. |
| D13 | Persistence | Power off, wait, and power on again. | The collected count and codex entries are preserved. The story is not replayed for an already-unlocked poem. |
| D14 | Story unlock | Collect the first poem, then the second, then the third. | A short Ultraman story appears after each newly collected poem (thresholds 1, 2, and 3). OK advances; long OK returns to the map. |
| D15 | Speed mode | Open the third menu item. | Five questions appear one at a time with a 15-second countdown bar, score, and progress. After answering, the next question appears automatically; the result screen appears after question 5. Long OK exits early. |
| D16 | Speed timeout | In speed mode, wait for the countdown to reach zero. | The question is marked wrong, the gentle wrong cue plays, and the next question appears without freezing. |
| D17 | Mute | Toggle the fourth home item, collect or answer something, then power cycle. | The label switches between enabled and disabled, cues are silenced when disabled, and the setting survives a reboot. |
| D18 | Audio mix | Start narration on the reward or codex detail page and listen. | Narration is intelligible, the background loop is audibly quieter while narration plays, and interaction cues do not cut the narration. |
| D19 | Long-press back | From map, intro, challenge, reward, codex, codex detail, story, and speed pages, long-press OK. | Each page returns to its documented parent page without leaving audio playing or leaving the UI unresponsive. |
| D20 | Battery | Observe the battery percentage on the home, map, reward, codex, and speed pages. | The value updates within a few seconds and does not overlap titles, score text, or progress dots. |
| D21 | Stability | Play all three stages, the speed mode, and several codex replays for at least 10 minutes. | No watchdog reset, memory exhaustion, audio lock-up, or screen corruption. Button input remains responsive. |

## Pass criteria

Record the result of every case. A release candidate passes when all cases are
PASS, or when the owner explicitly accepts a documented deviation. Report the
outcome separately as:

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / NOT RUN
Unverified: remaining board, instrument, or user checks
```

## Reporting a failure

Include the test ID, the exact button sequence, what appeared on screen, whether
audio was involved, and whether the issue survives a power cycle. If the device
prints logs, capture only the relevant lines and remove any personal data or
credentials before sharing.
