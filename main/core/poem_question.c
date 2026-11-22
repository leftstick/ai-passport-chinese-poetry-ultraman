#include "poem_question.h"

#include <stdio.h>
#include <string.h>

#include "poem_library.h"

#define DISTRACTOR_TRIES 48

static int utf8_chars(const char *text)
{
    int count = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++) {
        if ((*p & 0xC0u) != 0x80u) {
            count++;
        }
    }
    return count;
}

static void copy_text(char *out, size_t size, const char *text)
{
    if (size == 0) {
        return;
    }
    snprintf(out, size, "%s", text);
}

void poem_rng_seed(poem_rng_t *rng, uint32_t seed)
{
    rng->state = seed == 0 ? 0x9E3779B9u : seed;
}

uint32_t poem_rng_next(poem_rng_t *rng)
{
    uint32_t x = rng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (x == 0) {
        x = 0x9E3779B9u;
    }
    rng->state = x;
    return x;
}

uint32_t poem_rng_below(poem_rng_t *rng, uint32_t bound)
{
    return bound == 0 ? 0 : poem_rng_next(rng) % bound;
}

static bool grade_in_band(uint8_t grade, uint8_t low, uint8_t high)
{
    return grade >= low && grade <= high;
}

static uint16_t count_eligible(uint8_t low, uint8_t high)
{
    uint16_t count = 0;
    const uint16_t total = poem_library_count();
    for (uint16_t i = 0; i < total; i++) {
        if (grade_in_band(poem_library_at(i)->grade, low, high)) {
            count++;
        }
    }
    return count;
}

static const poem_t *eligible_at(uint16_t ordinal, uint8_t low, uint8_t high)
{
    const uint16_t total = poem_library_count();
    for (uint16_t i = 0; i < total; i++) {
        const poem_t *poem = poem_library_at(i);
        if (!grade_in_band(poem->grade, low, high)) {
            continue;
        }
        if (ordinal == 0) {
            return poem;
        }
        ordinal--;
    }
    return poem_library_at(0);
}

static bool history_has(const poem_question_t *history, uint8_t history_len,
                        const poem_t *poem, const char *text)
{
    for (uint8_t i = 0; i < history_len; i++) {
        if (history[i].poem_id == poem->id ||
            strcmp(history[i].text, text) == 0) {
            return true;
        }
    }
    return false;
}

static uint8_t choose_kind(const poem_t *poem, bool identify_only,
                           poem_rng_t *rng)
{
    if (identify_only) {
        return poem_rng_below(rng, 2) == 0 ? POEM_Q_TITLE : POEM_Q_AUTHOR;
    }
    const uint32_t roll = poem_rng_below(rng, 100);
    if (poem->line_count >= 2 && roll < 55) {
        return POEM_Q_NEXT_LINE;
    }
    if (poem->line_count >= 2 && roll < 75) {
        return POEM_Q_PREV_LINE;
    }
    if (roll < 90) {
        return POEM_Q_TITLE;
    }
    return POEM_Q_AUTHOR;
}

static bool duplicate_option(const poem_question_t *q, int slots,
                             const char *text)
{
    for (int i = 0; i < slots; i++) {
        if (strcmp(q->options[i], text) == 0) {
            return true;
        }
    }
    return false;
}

static void add_line_distractor(poem_question_t *q, int slot,
                                const poem_t *avoid, const char *correct,
                                poem_rng_t *rng)
{
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES; attempt++) {
        const poem_t *poem = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (poem->id == avoid->id || poem->line_count == 0) {
            continue;
        }
        const char *line =
            poem->lines[poem_rng_below(rng, poem->line_count)];
        if (line == NULL || strcmp(line, correct) == 0 ||
            duplicate_option(q, slot, line)) {
            continue;
        }
        copy_text(q->options[slot], POEM_OPTION_SIZE, line);
        return;
    }
    for (uint16_t i = 0; i < total; i++) {
        const poem_t *poem = poem_library_at(i);
        if (poem->id == avoid->id) {
            continue;
        }
        for (uint8_t j = 0; j < poem->line_count; j++) {
            const char *line = poem->lines[j];
            if (line != NULL && strcmp(line, correct) != 0 &&
                !duplicate_option(q, slot, line)) {
                copy_text(q->options[slot], POEM_OPTION_SIZE, line);
                return;
            }
        }
    }
}

static void add_title_distractor(poem_question_t *q, int slot,
                                 const poem_t *avoid, const char *correct,
                                 poem_rng_t *rng)
{
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES; attempt++) {
        const poem_t *poem = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (poem->id == avoid->id || strcmp(poem->title, correct) == 0 ||
            duplicate_option(q, slot, poem->title)) {
            continue;
        }
        copy_text(q->options[slot], POEM_OPTION_SIZE, poem->title);
        return;
    }
    for (uint16_t i = 0; i < total; i++) {
        const poem_t *poem = poem_library_at(i);
        if (poem->id != avoid->id && strcmp(poem->title, correct) != 0 &&
            !duplicate_option(q, slot, poem->title)) {
            copy_text(q->options[slot], POEM_OPTION_SIZE, poem->title);
            return;
        }
    }
}

static void add_author_distractor(poem_question_t *q, int slot,
                                  const poem_t *avoid, const char *correct,
                                  poem_rng_t *rng)
{
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES; attempt++) {
        const poem_t *poem = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (poem->id == avoid->id || strcmp(poem->author, correct) == 0 ||
            duplicate_option(q, slot, poem->author)) {
            continue;
        }
        copy_text(q->options[slot], POEM_OPTION_SIZE, poem->author);
        return;
    }
    for (uint16_t i = 0; i < total; i++) {
        const poem_t *poem = poem_library_at(i);
        if (poem->id != avoid->id && strcmp(poem->author, correct) != 0 &&
            !duplicate_option(q, slot, poem->author)) {
            copy_text(q->options[slot], POEM_OPTION_SIZE, poem->author);
            return;
        }
    }
}

static void shuffle_options(poem_question_t *q, poem_rng_t *rng)
{
    for (int i = POEM_OPTION_COUNT - 1; i > 0; i--) {
        const int j = (int)poem_rng_below(rng, (uint32_t)i + 1u);
        char tmp[POEM_OPTION_SIZE];
        memcpy(tmp, q->options[i], POEM_OPTION_SIZE);
        memcpy(q->options[i], q->options[j], POEM_OPTION_SIZE);
        memcpy(q->options[j], tmp, POEM_OPTION_SIZE);
    }
}

static void set_hint(poem_question_t *q, const poem_t *poem, uint8_t kind)
{
    switch (kind) {
    case POEM_Q_NEXT_LINE:
        if (utf8_chars(poem->title) <= 8) {
            snprintf(q->hint, sizeof(q->hint), "《%s》下一句？", poem->title);
        } else {
            snprintf(q->hint, sizeof(q->hint), "补全下一句 · %s", poem->author);
        }
        break;
    case POEM_Q_PREV_LINE:
        if (utf8_chars(poem->title) <= 8) {
            snprintf(q->hint, sizeof(q->hint), "《%s》上一句？", poem->title);
        } else {
            snprintf(q->hint, sizeof(q->hint), "补全上一句 · %s", poem->author);
        }
        break;
    case POEM_Q_TITLE:
        snprintf(q->hint, sizeof(q->hint), "这句出自哪首诗？");
        break;
    case POEM_Q_AUTHOR:
        if (utf8_chars(poem->title) <= 8) {
            snprintf(q->hint, sizeof(q->hint), "《%s》是谁写的？", poem->title);
        } else {
            snprintf(q->hint, sizeof(q->hint), "这句诗是谁写的？");
        }
        break;
    default:
        q->hint[0] = '\0';
        break;
    }
}

static void build_question(poem_question_t *q, const poem_t *poem,
                           uint8_t kind, poem_rng_t *rng)
{
    q->poem_id = poem->id;
    q->kind = kind;
    q->grade = poem->grade;

    uint8_t line_index = 0;
    const char *correct = NULL;
    switch (kind) {
    case POEM_Q_NEXT_LINE:
        line_index = (uint8_t)poem_rng_below(rng, poem->line_count - 1u);
        correct = poem->lines[line_index + 1u];
        break;
    case POEM_Q_PREV_LINE:
        line_index = (uint8_t)(1u + poem_rng_below(rng, poem->line_count - 1u));
        correct = poem->lines[line_index - 1u];
        break;
    case POEM_Q_TITLE:
        line_index = (uint8_t)poem_rng_below(rng, poem->line_count);
        correct = poem->title;
        break;
    case POEM_Q_AUTHOR:
        line_index = (uint8_t)poem_rng_below(rng, poem->line_count);
        correct = poem->author;
        break;
    default:
        correct = poem->lines[0];
        break;
    }

    copy_text(q->text, POEM_TEXT_SIZE, poem->lines[line_index]);
    set_hint(q, poem, kind);
    copy_text(q->options[0], POEM_OPTION_SIZE, correct);

    if (kind == POEM_Q_TITLE) {
        add_title_distractor(q, 1, poem, correct, rng);
        add_title_distractor(q, 2, poem, correct, rng);
    } else if (kind == POEM_Q_AUTHOR) {
        add_author_distractor(q, 1, poem, correct, rng);
        add_author_distractor(q, 2, poem, correct, rng);
    } else {
        add_line_distractor(q, 1, poem, correct, rng);
        add_line_distractor(q, 2, poem, correct, rng);
    }

    shuffle_options(q, rng);
    q->correct_index = 0;
    for (uint8_t i = 0; i < POEM_OPTION_COUNT; i++) {
        if (strcmp(q->options[i], correct) == 0) {
            q->correct_index = i;
            break;
        }
    }
}

void poem_question_generate(poem_question_t *question, uint8_t grade_min,
                            uint8_t grade_max, bool assist, bool identify_only,
                            poem_rng_t *rng,
                            const poem_question_t *history,
                            uint8_t history_len)
{
    memset(question, 0, sizeof(*question));
    if (grade_min < 1) {
        grade_min = 1;
    }
    if (grade_max > 6) {
        grade_max = 6;
    }
    if (grade_min > grade_max) {
        const uint8_t tmp = grade_min;
        grade_min = grade_max;
        grade_max = tmp;
    }
    if (assist && grade_min > 1) {
        grade_min--;
    }
    if (count_eligible(grade_min, grade_max) == 0) {
        grade_min = 1;
        grade_max = 6;
    }

    const uint16_t eligible = count_eligible(grade_min, grade_max);
    const poem_t *poem = NULL;
    uint8_t kind = POEM_Q_NEXT_LINE;
    uint8_t line_count = 0;

    for (int attempt = 0; attempt < 32; attempt++) {
        const poem_t *candidate =
            eligible_at((uint16_t)poem_rng_below(rng, eligible), grade_min,
                        grade_max);
        kind = choose_kind(candidate, identify_only, rng);
        uint8_t index = 0;
        const char *shown = candidate->lines[0];
        if (kind == POEM_Q_NEXT_LINE) {
            index = (uint8_t)poem_rng_below(rng, candidate->line_count - 1u);
            shown = candidate->lines[index];
        } else if (kind == POEM_Q_PREV_LINE) {
            index = (uint8_t)(1u + poem_rng_below(
                rng, candidate->line_count - 1u));
            shown = candidate->lines[index];
        } else {
            index = (uint8_t)poem_rng_below(rng, candidate->line_count);
            shown = candidate->lines[index];
        }
        if (history != NULL && history_has(history, history_len, candidate,
                                          shown)) {
            continue;
        }
        poem = candidate;
        line_count = index;
        break;
    }

    if (poem == NULL) {
        poem = eligible_at((uint16_t)poem_rng_below(rng, eligible), grade_min,
                           grade_max);
        kind = choose_kind(poem, identify_only, rng);
    }
    (void)line_count;
    build_question(question, poem, kind, rng);
}

bool poem_question_same(const poem_question_t *a, const poem_question_t *b)
{
    return a->poem_id == b->poem_id && strcmp(a->text, b->text) == 0;
}

const char *poem_kind_name(uint8_t kind)
{
    switch (kind) {
    case POEM_Q_NEXT_LINE:
        return "补全下句";
    case POEM_Q_PREV_LINE:
        return "补全上句";
    case POEM_Q_TITLE:
        return "认出篇名";
    case POEM_Q_AUTHOR:
        return "认出作者";
    default:
        return "";
    }
}
