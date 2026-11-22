<p align="right">
  <a href="gameplay-difference.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Gameplay Difference from the Math Matching Game

This application reuses the AI Passport BSP, the three-button input path, the
display pipeline, and general sprite-drawing techniques from
[`leftstick/ai-passport-match-ultraman`](https://github.com/leftstick/ai-passport-match-ultraman).
Everything that decides what the player does, how a round ends, and what is
remembered between sessions has been rebuilt. The table below is the contract
for that rebuild.

## Side-by-side comparison

| Dimension | Math matching game | Ultraman poetry collection adventure |
| --- | --- | --- |
| Core goal | Clear as many equivalent-expression card pairs as possible before the timer or lives run out. | Rebuild and collect one complete poem per stage. Score is not the objective. |
| Round shape | Repeated short matching rounds inside a score-driven loop. | Intro → four different collection interactions → full-poem reward → codex entry. |
| Primary input | Move a cursor over a card grid and select two matching cards. | Move a cursor through ordered character tiles or three answer options; the board layout changes with each interaction type. |
| Question content | Arithmetic expressions and equivalent values. | Character ordering, next-line recall, picture-to-title recognition, and missing-character completion for classical poems. |
| Failure handling | Wrong matches consume time or lives; the run can end. | No lives, no timer in the main mode, and no run-ending failure. A wrong answer can be retried; the second wrong attempt reveals a poem-based hint. |
| Pace | Fast, arcade-like repetition. | Slow reading pace: scene narration, one poem per stage, optional narration playback, and no pressure to answer quickly. |
| Difficulty | Increases through card count, time limits, and score pressure. | Difficulty comes from poem length and interaction type; the same poem can be revisited without penalty. |
| Extra mode | Time attack / endless matching around the same card loop. | Optional 15-second speed quiz used only for review; the main adventure remains the primary experience. |
| Progression | High score and run statistics. | Collected-poem codex, three Ultraman micro-stories unlocked at 1/2/3 poems, and per-poem replay. |
| Visual language | Card grid, arcade HUD, monster and beam sprite effects. | Ink-wash landscape backdrops, poem panels, seal-red accents, and a chibi Ultraman guide; no card grid and no demo menu shell. |
| Audio language | Arcade beams, monster growls, and short score cues. | Full-poem Mandarin narration, a looping pentatonic guofeng bed, and five gentle interaction cues; background music ducks under narration. |
| Persistence | Best score and run-related state. | Collected-poem bitmap, unlocked story bits, and mute preference in NVS namespace `poem_adv_v1`. |
| Reuse boundary | — | BSP, key-event framework, display pipeline, and sprite drawing only. Gameplay state machines, pages, layouts, question generation, audio mixing, and assets are new. |

## What deliberately does not carry over

- No card grid, no pair matching, and no arithmetic content.
- No lives, combo meter, boss health bar, beam rhythm QTE, or score-first victory screen.
- No reuse of the upstream demo menu, page shell, or the math game's visual layout.
- No growls or arcade beam sounds as answer feedback; the new feedback is calm and
  informational so a wrong answer does not feel like a punishment.

## Why this fits the target player

The intended player is a seven-year-old who enjoys Ultraman and is starting to
learn classical poetry. The design keeps the familiar three-button control
scheme but changes the emotional loop: the child is not defending a score, but
helping Ultraman put a poem back together. Each correct step visibly advances a
story, and the reward is the complete poem plus a short explanation rather than
a number.

## Verification hooks

- `main/core/adventure.c` fixes the four-step order and the hint-on-second-wrong
  rule; `tests/test_adventure.c` asserts both.
- `main/core/poem_challenge.c` generates and validates all four interactions;
  `tests/test_poem_challenge.c` covers ordering, options, blanks, and hints.
- `main/core/collection.c` owns collection and story bitmasks;
  `tests/test_collection.c` covers serialization and story thresholds.
- `tools/check_assets.py` enforces the three scenes, three chibi poses, three
  narrations, and five cues that make the new presentation distinct.
