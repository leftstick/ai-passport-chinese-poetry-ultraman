// Four poetry interactions for the collection adventure.
// Pure C, no LVGL/ESP-IDF dependencies.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "poem_library.h"
#include "poem_question.h"  // poem_rng_t

#define CHALLENGE_MAX_TILES 8
#define CHALLENGE_OPTION_COUNT 3
#define CHALLENGE_TEXT_SIZE 64
#define CHALLENGE_LINE_SIZE 48
#define CHALLENGE_CHAR_SIZE 8

typedef enum {
    CHALLENGE_ORDER = 0,   // 诗句拼图：重排字块
    CHALLENGE_NEXT_LINE,   // 诗句接龙：选下一句
    CHALLENGE_PICTURE,     // 看图猜诗：选篇名
    CHALLENGE_BLANK,       // 缺字填空：选关键字
    CHALLENGE_KIND_COUNT,
} challenge_kind_t;

typedef struct {
    uint8_t kind;
    uint16_t poem_id;
    uint8_t line_index;
    char prompt[CHALLENGE_TEXT_SIZE];
    char hint[CHALLENGE_TEXT_SIZE];
    char line[CHALLENGE_LINE_SIZE];
    uint8_t tile_count;
    char tiles[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
    uint8_t option_count;
    char options[CHALLENGE_OPTION_COUNT][CHALLENGE_LINE_SIZE];
    uint8_t correct_index;
    char answer[CHALLENGE_LINE_SIZE];
} poem_challenge_t;

const char *challenge_kind_name(uint8_t kind);

// Build one challenge for the given poem. The same poem and kind with the same
// RNG state produce the same challenge, which keeps host tests deterministic.
void poem_challenge_make(poem_challenge_t *out, const poem_t *poem,
                         uint8_t kind, poem_rng_t *rng);

// ORDER: compare the selected tile sequence against the target line.
bool poem_challenge_order_correct(const poem_challenge_t *challenge,
                                  const uint8_t *order, uint8_t order_len);

// NEXT_LINE / PICTURE / BLANK.
bool poem_challenge_option_correct(const poem_challenge_t *challenge,
                                   uint8_t option_index);
