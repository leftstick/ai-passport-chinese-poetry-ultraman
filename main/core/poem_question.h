// Pure poem-question generation for 奥特诗词闯关.
// No LVGL/ESP-IDF dependencies so it can be covered by host tests.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define POEM_OPTION_COUNT 3
// A UI line is at most 12 characters; UTF-8 uses at most 3 bytes per character.
#define POEM_TEXT_SIZE 40
#define POEM_HINT_SIZE 64
#define POEM_OPTION_SIZE 40
#define POEM_HISTORY 4

typedef struct {
    uint32_t state;
} poem_rng_t;

typedef enum {
    POEM_Q_NEXT_LINE = 0,  // 给出当前句，选下一句
    POEM_Q_PREV_LINE,      // 给出当前句，选上一句
    POEM_Q_TITLE,          // 给出诗句，选篇名
    POEM_Q_AUTHOR,         // 给出诗句，选作者
} poem_kind_t;

typedef struct {
    char text[POEM_TEXT_SIZE];
    char hint[POEM_HINT_SIZE];
    char options[POEM_OPTION_COUNT][POEM_OPTION_SIZE];
    uint16_t poem_id;
    uint8_t correct_index;
    uint8_t kind;
    uint8_t grade;
} poem_question_t;

void poem_rng_seed(poem_rng_t *rng, uint32_t seed);
uint32_t poem_rng_next(poem_rng_t *rng);
uint32_t poem_rng_below(poem_rng_t *rng, uint32_t bound);

// grade_min/grade_max: inclusive 1..6 grade band. assist widens the band one
// grade down so a struggling player sees easier poems.
// identify_only restricts the run to title/author questions (专项模式).
// history/history_len: recent questions used to avoid immediate repeats.
void poem_question_generate(poem_question_t *question, uint8_t grade_min,
                            uint8_t grade_max, bool assist, bool identify_only,
                            poem_rng_t *rng,
                            const poem_question_t *history,
                            uint8_t history_len);

// Review-pool identity: a repeat means the same poem and the same shown line.
bool poem_question_same(const poem_question_t *a, const poem_question_t *b);

const char *poem_kind_name(uint8_t kind);
