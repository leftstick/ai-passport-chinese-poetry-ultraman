// Slow-paced collection-adventure flow for 奥特曼诗词收集冒险.
// Pure C, no LVGL/ESP-IDF dependencies.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "poem_challenge.h"

#define ADVENTURE_STAGE_COUNT 3
#define ADVENTURE_CHALLENGE_COUNT 4
#define ADVENTURE_STORY_COUNT 3

typedef enum {
    ADVENTURE_STEP_INTRO = 0,
    ADVENTURE_STEP_CHALLENGE,
    ADVENTURE_STEP_REWARD,
    ADVENTURE_STEP_DONE,
} adventure_step_t;

typedef struct {
    uint16_t poem_id;
    uint8_t stage;
    uint8_t step;
    uint8_t challenge_index;
    uint8_t attempts;  // wrong attempts on the current challenge
    uint8_t hints;
} adventure_t;

// Stage catalogue (vertical slice: three poems, four interactions each).
const char *adventure_stage_poem_title(uint8_t stage);
const char *adventure_stage_scene(uint8_t stage);
const char *adventure_stage_intro(uint8_t stage);
const char *adventure_stage_reward_line(uint8_t stage);
uint16_t adventure_stage_poem_id(uint8_t stage);
uint8_t adventure_stage_for_poem(uint16_t poem_id);
const char *adventure_poem_explanation(uint16_t poem_id);

// Fixed challenge order: order -> next line -> picture -> blank.
uint8_t adventure_challenge_kind(uint8_t index);

// Adventure state machine.
void adventure_begin(adventure_t *adventure, uint8_t stage);
void adventure_start_challenges(adventure_t *adventure);
// Returns true when the gentle hint should be shown (second wrong attempt).
bool adventure_wrong(adventure_t *adventure);
// Advances to the next challenge, or to the reward step after the last one.
void adventure_correct(adventure_t *adventure);
bool adventure_in_reward(const adventure_t *adventure);
void adventure_skip_to_reward(adventure_t *adventure);

// Ultraman micro-stories, unlocked by the number of collected poems.
const char *adventure_story_title(uint8_t index);
const char *adventure_story_text(uint8_t index);
uint8_t adventure_story_threshold(uint8_t index);
