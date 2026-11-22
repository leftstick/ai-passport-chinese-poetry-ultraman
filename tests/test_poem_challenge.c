// Host tests for the four poetry interactions.
#include "poem_challenge.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "poem_library.h"
#include "poem_question.h"
#include "poem_text.h"

static const poem_t *find_poem(uint16_t id)
{
    for (uint16_t i = 0; i < poem_library_count(); i++) {
        const poem_t *poem = poem_library_at(i);
        if (poem->id == id) {
            return poem;
        }
    }
    return NULL;
}

static void check_options(const poem_challenge_t *challenge)
{
    assert(challenge->option_count == CHALLENGE_OPTION_COUNT);
    assert(challenge->correct_index < challenge->option_count);
    for (uint8_t i = 0; i < challenge->option_count; i++) {
        assert(challenge->options[i][0] != '\0');
        for (uint8_t j = i + 1; j < challenge->option_count; j++) {
            assert(strcmp(challenge->options[i], challenge->options[j]) != 0);
        }
    }
    assert(strcmp(challenge->options[challenge->correct_index],
                  challenge->answer) == 0);
}

// Build the tile order that spells the answer. Duplicate characters are
// resolved greedily, which is enough for the quatrains in the library.
static uint8_t solve_order(const poem_challenge_t *challenge, uint8_t *order)
{
    char remaining[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
    bool used[CHALLENGE_MAX_TILES] = { false };
    for (uint8_t i = 0; i < challenge->tile_count; i++) {
        memcpy(remaining[i], challenge->tiles[i], CHALLENGE_CHAR_SIZE);
    }
    char answer_chars[CHALLENGE_MAX_TILES][CHALLENGE_CHAR_SIZE];
    const int count = poem_split_chars(challenge->answer, answer_chars,
                                       CHALLENGE_MAX_TILES);
    assert(count == challenge->tile_count);
    for (int pos = 0; pos < count; pos++) {
        bool found = false;
        for (uint8_t i = 0; i < challenge->tile_count; i++) {
            if (!used[i] && strcmp(remaining[i], answer_chars[pos]) == 0) {
                order[pos] = i;
                used[i] = true;
                found = true;
                break;
            }
        }
        assert(found);
    }
    return (uint8_t)count;
}

static void test_order(void)
{
    const poem_t *poem = find_poem(7);
    assert(poem != NULL);
    poem_rng_t rng;
    poem_rng_seed(&rng, 0x1234u);
    poem_challenge_t challenge;
    poem_challenge_make(&challenge, poem, CHALLENGE_ORDER, &rng);

    assert(challenge.kind == CHALLENGE_ORDER);
    assert(challenge.tile_count >= 4);
    assert(challenge.tile_count <= CHALLENGE_MAX_TILES);
    assert(challenge.answer[0] != '\0');
    assert(challenge.prompt[0] != '\0');
    assert(challenge.hint[0] != '\0');

    uint8_t order[CHALLENGE_MAX_TILES];
    const uint8_t len = solve_order(&challenge, order);
    assert(poem_challenge_order_correct(&challenge, order, len));
    // A wrong order must be rejected.
    if (len >= 2) {
        uint8_t swapped[CHALLENGE_MAX_TILES];
        for (uint8_t i = 0; i < len; i++) {
            swapped[i] = order[i];
        }
        const uint8_t tmp = swapped[0];
        swapped[0] = swapped[1];
        swapped[1] = tmp;
        if (strcmp(challenge.tiles[swapped[0]], challenge.tiles[swapped[1]]) !=
            0) {
            assert(!poem_challenge_order_correct(&challenge, swapped, len));
        }
    }
    assert(!poem_challenge_order_correct(&challenge, order, len - 1));
}

static void test_next_line(void)
{
    const poem_t *poem = find_poem(2);
    assert(poem != NULL);
    poem_rng_t rng;
    poem_rng_seed(&rng, 0xBEEFu);
    poem_challenge_t challenge;
    poem_challenge_make(&challenge, poem, CHALLENGE_NEXT_LINE, &rng);

    assert(challenge.kind == CHALLENGE_NEXT_LINE);
    assert(challenge.line_index + 1 < poem->line_count);
    assert(strcmp(challenge.line, poem->lines[challenge.line_index]) == 0);
    assert(strcmp(challenge.answer,
                  poem->lines[challenge.line_index + 1]) == 0);
    check_options(&challenge);
    for (uint8_t i = 0; i < challenge.option_count; i++) {
        assert(poem_challenge_option_correct(&challenge, i) ==
               (i == challenge.correct_index));
    }
}

static void test_picture(void)
{
    const poem_t *poem = find_poem(16);
    assert(poem != NULL);
    poem_rng_t rng;
    poem_rng_seed(&rng, 0x7777u);
    poem_challenge_t challenge;
    poem_challenge_make(&challenge, poem, CHALLENGE_PICTURE, &rng);

    assert(challenge.kind == CHALLENGE_PICTURE);
    assert(strcmp(challenge.answer, poem->title) == 0);
    check_options(&challenge);
    bool has_title = false;
    for (uint8_t i = 0; i < challenge.option_count; i++) {
        if (strcmp(challenge.options[i], poem->title) == 0) {
            has_title = true;
        }
    }
    assert(has_title);
}

static void test_blank(void)
{
    const poem_t *poem = find_poem(7);
    assert(poem != NULL);
    poem_rng_t rng;
    poem_rng_seed(&rng, 0xCAFEu);
    poem_challenge_t challenge;
    poem_challenge_make(&challenge, poem, CHALLENGE_BLANK, &rng);

    assert(challenge.kind == CHALLENGE_BLANK);
    assert(strstr(challenge.line, "＿") != NULL);
    assert(poem_char_count(challenge.answer) == 1);
    check_options(&challenge);
}

static void test_deterministic(void)
{
    const poem_t *poem = find_poem(7);
    assert(poem != NULL);
    poem_rng_t a;
    poem_rng_t b;
    poem_rng_seed(&a, 42u);
    poem_rng_seed(&b, 42u);
    poem_challenge_t first;
    poem_challenge_t second;
    poem_challenge_make(&first, poem, CHALLENGE_PICTURE, &a);
    poem_challenge_make(&second, poem, CHALLENGE_PICTURE, &b);
    assert(strcmp(first.answer, second.answer) == 0);
    assert(first.correct_index == second.correct_index);
    for (uint8_t i = 0; i < first.option_count; i++) {
        assert(strcmp(first.options[i], second.options[i]) == 0);
    }
}

int main(void)
{
    test_order();
    test_next_line();
    test_picture();
    test_blank();
    test_deterministic();
    printf("test_poem_challenge: PASS\n");
    return 0;
}
