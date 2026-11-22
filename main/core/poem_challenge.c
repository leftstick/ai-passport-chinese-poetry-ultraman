#include "poem_challenge.h"

#include <stdio.h>
#include <string.h>

#include "poem_text.h"

#define DISTRACTOR_TRIES 48

static void copy_text(char *out, size_t size, const char *text)
{
    if (size == 0) {
        return;
    }
    snprintf(out, size, "%s", text != NULL ? text : "");
}

static int cleaned_count(const char *line)
{
    return poem_split_chars(line, NULL, 0);
}

static int line_chars(const char *line, char out[][CHALLENGE_CHAR_SIZE],
                      int max_chars)
{
    return poem_split_chars(line, out, max_chars);
}

static uint8_t pick_line(const poem_t *poem, uint8_t min_chars, uint8_t max_chars,
                         poem_rng_t *rng)
{
    uint8_t candidates[POEM_MAX_LINES];
    uint8_t count = 0;
    for (uint8_t i = 0; i < poem->line_count && i < POEM_MAX_LINES; i++) {
        const int chars = cleaned_count(poem->lines[i]);
        if (chars >= min_chars && chars <= max_chars) {
            candidates[count++] = i;
        }
    }
    if (count == 0) {
        return 0;
    }
    return candidates[poem_rng_below(rng, count)];
}

static bool option_has(const poem_challenge_t *out, const char *text)
{
    for (uint8_t i = 0; i < out->option_count; i++) {
        if (strcmp(out->options[i], text) == 0) {
            return true;
        }
    }
    return false;
}

static void shuffle_options(poem_challenge_t *out, poem_rng_t *rng)
{
    for (uint8_t i = out->option_count; i > 1; i--) {
        const uint8_t j = (uint8_t)poem_rng_below(rng, i);
        if (j == i - 1) {
            continue;
        }
        char tmp[CHALLENGE_LINE_SIZE];
        memcpy(tmp, out->options[i - 1], sizeof(tmp));
        memcpy(out->options[i - 1], out->options[j], sizeof(tmp));
        memcpy(out->options[j], tmp, sizeof(tmp));
        if (out->correct_index == i - 1) {
            out->correct_index = j;
        } else if (out->correct_index == j) {
            out->correct_index = i - 1;
        }
    }
}

static void add_line_options(poem_challenge_t *out, const poem_t *poem,
                             uint8_t line_index, poem_rng_t *rng)
{
    out->option_count = 1;
    out->correct_index = 0;
    copy_text(out->options[0], CHALLENGE_LINE_SIZE, poem->lines[line_index]);
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES &&
                            out->option_count < CHALLENGE_OPTION_COUNT;
         attempt++) {
        const poem_t *other = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (other->id == poem->id || other->line_count == 0) {
            continue;
        }
        const char *line = other->lines[poem_rng_below(rng, other->line_count)];
        if (line == NULL || option_has(out, line)) {
            continue;
        }
        copy_text(out->options[out->option_count], CHALLENGE_LINE_SIZE, line);
        out->option_count++;
    }
    // Deterministic fallback: walk the library in order until two distractors
    // are found.
    for (uint16_t i = 0; i < total &&
                        out->option_count < CHALLENGE_OPTION_COUNT; i++) {
        const poem_t *other = poem_library_at(i);
        if (other->id == poem->id) {
            continue;
        }
        for (uint8_t j = 0; j < other->line_count &&
                            out->option_count < CHALLENGE_OPTION_COUNT; j++) {
            const char *line = other->lines[j];
            if (line != NULL && !option_has(out, line)) {
                copy_text(out->options[out->option_count],
                          CHALLENGE_LINE_SIZE, line);
                out->option_count++;
            }
        }
    }
    shuffle_options(out, rng);
}

static void add_title_options(poem_challenge_t *out, const poem_t *poem,
                              poem_rng_t *rng)
{
    out->option_count = 1;
    out->correct_index = 0;
    copy_text(out->options[0], CHALLENGE_LINE_SIZE, poem->title);
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES &&
                            out->option_count < CHALLENGE_OPTION_COUNT;
         attempt++) {
        const poem_t *other = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (other->id == poem->id ||
            option_has(out, other->title) ||
            strcmp(other->title, poem->title) == 0) {
            continue;
        }
        copy_text(out->options[out->option_count], CHALLENGE_LINE_SIZE,
                  other->title);
        out->option_count++;
    }
    for (uint16_t i = 0; i < total &&
                        out->option_count < CHALLENGE_OPTION_COUNT; i++) {
        const poem_t *other = poem_library_at(i);
        if (other->id == poem->id || option_has(out, other->title)) {
            continue;
        }
        copy_text(out->options[out->option_count], CHALLENGE_LINE_SIZE,
                  other->title);
        out->option_count++;
    }
    shuffle_options(out, rng);
}

static void add_char_options(poem_challenge_t *out, const char *correct,
                             poem_rng_t *rng)
{
    out->option_count = 1;
    out->correct_index = 0;
    copy_text(out->options[0], CHALLENGE_LINE_SIZE, correct);
    const uint16_t total = poem_library_count();
    for (int attempt = 0; attempt < DISTRACTOR_TRIES &&
                            out->option_count < CHALLENGE_OPTION_COUNT;
         attempt++) {
        const poem_t *other = poem_library_at(
            (uint16_t)poem_rng_below(rng, total));
        if (other->line_count == 0) {
            continue;
        }
        char chars[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
        const int count = line_chars(
            other->lines[poem_rng_below(rng, other->line_count)], chars,
            CHALLENGE_MAX_TILES);
        if (count <= 0) {
            continue;
        }
        const char *candidate = chars[poem_rng_below(rng, (uint32_t)count)];
        if (strcmp(candidate, correct) == 0 || option_has(out, candidate)) {
            continue;
        }
        copy_text(out->options[out->option_count], CHALLENGE_LINE_SIZE,
                  candidate);
        out->option_count++;
    }
    for (uint16_t i = 0; i < total &&
                        out->option_count < CHALLENGE_OPTION_COUNT; i++) {
        const poem_t *other = poem_library_at(i);
        for (uint8_t j = 0; j < other->line_count &&
                            out->option_count < CHALLENGE_OPTION_COUNT; j++) {
            char chars[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
            const int count =
                line_chars(other->lines[j], chars, CHALLENGE_MAX_TILES);
            for (int k = 0; k < count &&
                            out->option_count < CHALLENGE_OPTION_COUNT; k++) {
                if (strcmp(chars[k], correct) != 0 &&
                    !option_has(out, chars[k])) {
                    copy_text(out->options[out->option_count],
                              CHALLENGE_LINE_SIZE, chars[k]);
                    out->option_count++;
                }
            }
        }
    }
    shuffle_options(out, rng);
}

static void make_order(poem_challenge_t *out, const poem_t *poem,
                       poem_rng_t *rng)
{
    out->line_index = pick_line(poem, 4, CHALLENGE_MAX_TILES, rng);
    const char *line = poem->lines[out->line_index];
    copy_text(out->line, CHALLENGE_LINE_SIZE, line);
    poem_strip_noise(line, out->answer, CHALLENGE_LINE_SIZE);
    out->tile_count = (uint8_t)line_chars(line, out->tiles,
                                          CHALLENGE_MAX_TILES);
    if (out->tile_count < 2) {
        out->tile_count = 2;
    }
    for (uint8_t i = out->tile_count; i > 1; i--) {
        const uint8_t j = (uint8_t)poem_rng_below(rng, i);
        char tmp[CHALLENGE_CHAR_SIZE];
        memcpy(tmp, out->tiles[i - 1], sizeof(tmp));
        memcpy(out->tiles[i - 1], out->tiles[j], sizeof(tmp));
        memcpy(out->tiles[j], tmp, sizeof(tmp));
    }
    // Never hand the player an already-solved puzzle: compare the shuffled
    // tiles with the identity order.
    uint8_t identity[CHALLENGE_MAX_TILES];
    for (uint8_t i = 0; i < out->tile_count; i++) {
        identity[i] = i;
    }
    char ordered[CHALLENGE_LINE_SIZE] = "";
    size_t used = 0;
    for (uint8_t i = 0; i < out->tile_count; i++) {
        const size_t len = strlen(out->tiles[identity[i]]);
        memcpy(&ordered[used], out->tiles[identity[i]], len);
        used += len;
        ordered[used] = '\0';
    }
    if (strcmp(ordered, out->answer) == 0 && out->tile_count >= 2) {
        char tmp[CHALLENGE_CHAR_SIZE];
        memcpy(tmp, out->tiles[0], sizeof(tmp));
        memcpy(out->tiles[0], out->tiles[1], sizeof(tmp));
        memcpy(out->tiles[1], tmp, sizeof(tmp));
    }
    copy_text(out->prompt, CHALLENGE_TEXT_SIZE, "把散落的字块排成完整诗句");
    char first[CHALLENGE_CHAR_SIZE] = "";
    if (out->tile_count > 0) {
        // answer starts with the original first character
        size_t offset = 0;
        char ch[8];
        if (poem_next_char(out->answer, &offset, ch) && !poem_is_noise(ch)) {
            copy_text(first, sizeof(first), ch);
        }
    }
    snprintf(out->hint, CHALLENGE_TEXT_SIZE,
             "这句诗一共 %u 个字，开头是「%s」。", out->tile_count, first);
}

static void make_next_line(poem_challenge_t *out, const poem_t *poem,
                           poem_rng_t *rng)
{
    uint8_t index = 0;
    if (poem->line_count > 1) {
        index = (uint8_t)poem_rng_below(rng, poem->line_count - 1);
    }
    out->line_index = index;
    copy_text(out->line, CHALLENGE_LINE_SIZE, poem->lines[index]);
    copy_text(out->answer, CHALLENGE_LINE_SIZE, poem->lines[index + 1]);
    add_line_options(out, poem, index + 1, rng);
    copy_text(out->prompt, CHALLENGE_TEXT_SIZE, "下一句是什么？");
    char first[CHALLENGE_CHAR_SIZE] = "";
    size_t offset = 0;
    char ch[8];
    while (poem_next_char(out->answer, &offset, ch)) {
        if (!poem_is_noise(ch)) {
            copy_text(first, sizeof(first), ch);
            break;
        }
    }
    snprintf(out->hint, CHALLENGE_TEXT_SIZE, "下一句以「%s」开头。", first);
}

static void make_picture(poem_challenge_t *out, const poem_t *poem,
                         poem_rng_t *rng)
{
    out->line_index = 0;
    copy_text(out->line, CHALLENGE_LINE_SIZE, poem->lines[0]);
    copy_text(out->answer, CHALLENGE_LINE_SIZE, poem->title);
    add_title_options(out, poem, rng);
    copy_text(out->prompt, CHALLENGE_TEXT_SIZE, "这幅国风小画是哪一首诗？");
    char tag[24] = "";
    if (poem->tags != NULL) {
        const char *space = strchr(poem->tags, ' ');
        const size_t len = space != NULL ? (size_t)(space - poem->tags)
                                         : strlen(poem->tags);
        const size_t copy = len < sizeof(tag) - 1 ? len : sizeof(tag) - 1;
        memcpy(tag, poem->tags, copy);
        tag[copy] = '\0';
    }
    snprintf(out->hint, CHALLENGE_TEXT_SIZE, "画面里藏着「%s」的线索。", tag);
}

static void make_blank(poem_challenge_t *out, const poem_t *poem,
                       poem_rng_t *rng)
{
    out->line_index = pick_line(poem, 5, CHALLENGE_MAX_TILES, rng);
    const char *line = poem->lines[out->line_index];
    copy_text(out->line, CHALLENGE_LINE_SIZE, line);

    char chars[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
    const int count = line_chars(line, chars, CHALLENGE_MAX_TILES);
    uint8_t target = count > 0 ? (uint8_t)(count / 2) : 0;
    if (target >= CHALLENGE_MAX_TILES) {
        target = CHALLENGE_MAX_TILES - 1;
    }
    copy_text(out->answer, CHALLENGE_LINE_SIZE,
              count > 0 ? chars[target] : "");
    add_char_options(out, out->answer, rng);

    // Build the prompt with the chosen character replaced by a blank tile while
    // preserving punctuation and the original order.
    char prompt[CHALLENGE_TEXT_SIZE] = "";
    size_t used = 0;
    size_t offset = 0;
    int ordinal = -1;
    char ch[8];
    while (poem_next_char(line, &offset, ch)) {
        if (!poem_is_noise(ch)) {
            ordinal++;
        }
        const char *piece = ch;
        if (ordinal == (int)target) {
            piece = "＿";
        }
        const size_t len = strlen(piece);
        if (used + len + 1 > sizeof(prompt)) {
            break;
        }
        memcpy(&prompt[used], piece, len);
        used += len;
        prompt[used] = '\0';
    }
    copy_text(out->line, CHALLENGE_LINE_SIZE, prompt);
    copy_text(out->prompt, CHALLENGE_TEXT_SIZE, "选一个字，把诗句补完整");

    char neighbor[CHALLENGE_CHAR_SIZE] = "";
    if (count > 0) {
        const int near = target > 0 ? target - 1 : target + 1;
        if (near >= 0 && near < count) {
            copy_text(neighbor, sizeof(neighbor), chars[near]);
        }
    }
    if (neighbor[0] != '\0') {
        snprintf(out->hint, CHALLENGE_TEXT_SIZE,
                 "这个字在「%s」的旁边，读一读试试。", neighbor);
    } else {
        copy_text(out->hint, CHALLENGE_TEXT_SIZE, "先读一读前半句，想想押韵。");
    }
}

const char *challenge_kind_name(uint8_t kind)
{
    switch (kind) {
    case CHALLENGE_ORDER:
        return "拼句";
    case CHALLENGE_NEXT_LINE:
        return "接龙";
    case CHALLENGE_PICTURE:
        return "看图";
    case CHALLENGE_BLANK:
        return "填字";
    default:
        return "闯关";
    }
}

void poem_challenge_make(poem_challenge_t *out, const poem_t *poem,
                         uint8_t kind, poem_rng_t *rng)
{
    if (out == NULL || poem == NULL || rng == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->kind = kind < CHALLENGE_KIND_COUNT ? kind : CHALLENGE_ORDER;
    out->poem_id = poem->id;
    out->option_count = 0;
    switch (out->kind) {
    case CHALLENGE_NEXT_LINE:
        make_next_line(out, poem, rng);
        break;
    case CHALLENGE_PICTURE:
        make_picture(out, poem, rng);
        break;
    case CHALLENGE_BLANK:
        make_blank(out, poem, rng);
        break;
    case CHALLENGE_ORDER:
    default:
        make_order(out, poem, rng);
        break;
    }
}

bool poem_challenge_order_correct(const poem_challenge_t *challenge,
                                  const uint8_t *order, uint8_t order_len)
{
    if (challenge == NULL || order == NULL ||
        order_len != challenge->tile_count) {
        return false;
    }
    char text[CHALLENGE_LINE_SIZE] = "";
    size_t used = 0;
    for (uint8_t i = 0; i < order_len; i++) {
        if (order[i] >= challenge->tile_count) {
            return false;
        }
        const size_t len = strlen(challenge->tiles[order[i]]);
        if (used + len + 1 > sizeof(text)) {
            return false;
        }
        memcpy(&text[used], challenge->tiles[order[i]], len);
        used += len;
        text[used] = '\0';
    }
    return strcmp(text, challenge->answer) == 0;
}

bool poem_challenge_option_correct(const poem_challenge_t *challenge,
                                   uint8_t option_index)
{
    return challenge != NULL && option_index < challenge->option_count &&
           option_index == challenge->correct_index;
}
