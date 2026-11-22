// Host tests for the 光之诗库 question generator.
#include "poem_question.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "poem_library.h"

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

static bool line_index_for_next(const poem_t *poem, const char *text,
                                const char *correct)
{
    for (uint8_t i = 0; i + 1u < poem->line_count; i++) {
        if (strcmp(poem->lines[i], text) == 0 &&
            strcmp(poem->lines[i + 1u], correct) == 0) {
            return true;
        }
    }
    return false;
}

static bool line_index_for_prev(const poem_t *poem, const char *text,
                                const char *correct)
{
    for (uint8_t i = 1; i < poem->line_count; i++) {
        if (strcmp(poem->lines[i], text) == 0 &&
            strcmp(poem->lines[i - 1u], correct) == 0) {
            return true;
        }
    }
    return false;
}

static void check_question(const poem_question_t *q, uint8_t low, uint8_t high)
{
    assert(q->text[0] != '\0');
    assert(q->hint[0] != '\0');
    assert(q->kind <= POEM_Q_AUTHOR);
    assert(q->correct_index < POEM_OPTION_COUNT);
    assert(q->grade >= low && q->grade <= high);

    for (int i = 0; i < POEM_OPTION_COUNT; i++) {
        assert(q->options[i][0] != '\0');
        for (int j = i + 1; j < POEM_OPTION_COUNT; j++) {
            assert(strcmp(q->options[i], q->options[j]) != 0);
        }
    }

    const poem_t *poem = find_poem(q->poem_id);
    assert(poem != NULL);
    assert(poem->grade == q->grade);

    const char *correct = q->options[q->correct_index];
    bool matched = false;
    switch (q->kind) {
    case POEM_Q_NEXT_LINE:
        matched = line_index_for_next(poem, q->text, correct);
        break;
    case POEM_Q_PREV_LINE:
        matched = line_index_for_prev(poem, q->text, correct);
        break;
    case POEM_Q_TITLE:
        matched = strcmp(correct, poem->title) == 0;
        break;
    case POEM_Q_AUTHOR:
        matched = strcmp(correct, poem->author) == 0;
        break;
    default:
        break;
    }
    assert(matched);
}

static void test_library_integrity(void)
{
    assert(strcmp(poem_library_name(), "光之诗库") == 0);
    assert(poem_library_count() > 0);
    assert(poem_library_total_target() == 1000u);
    assert(poem_library_batch_count() >= 1);
    for (uint8_t b = 0; b < poem_library_batch_count(); b++) {
        assert(poem_library_batch_name(b)[0] != '\0');
        assert(poem_library_batch_desc(b)[0] != '\0');
    }

    bool grade_seen[7] = {false};
    for (uint16_t i = 0; i < poem_library_count(); i++) {
        const poem_t *poem = poem_library_at(i);
        assert(poem->id != 0);
        assert(poem->title[0] != '\0');
        assert(poem->author[0] != '\0');
        assert(poem->dynasty[0] != '\0');
        assert(poem->line_count >= 1 && poem->line_count <= POEM_MAX_LINES);
        assert(poem->lines != NULL);
        for (uint8_t j = 0; j < poem->line_count; j++) {
            assert(poem->lines[j] != NULL);
            assert(poem->lines[j][0] != '\0');
        }
        assert(poem->grade >= 1 && poem->grade <= 6);
        assert(poem->batch < poem_library_batch_count());
        grade_seen[poem->grade] = true;

        for (uint16_t k = i + 1; k < poem_library_count(); k++) {
            assert(poem_library_at(k)->id != poem->id);
        }
    }
    for (uint8_t grade = 1; grade <= 6; grade++) {
        assert(grade_seen[grade]);
    }
}

static void test_generation(void)
{
    static const uint8_t BANDS[][2] = {
        {1, 2}, {2, 3}, {3, 4}, {4, 6}, {1, 6},
    };
    for (uint32_t seed = 1; seed <= 20; seed++) {
        for (size_t band = 0; band < sizeof(BANDS) / sizeof(BANDS[0]); band++) {
            poem_rng_t rng;
            poem_rng_seed(&rng, seed * 7919u + (uint32_t)band);
            poem_question_t history[POEM_HISTORY];
            uint8_t history_len = 0;

            for (int n = 0; n < 30; n++) {
                poem_question_t q;
                poem_question_generate(&q, BANDS[band][0], BANDS[band][1],
                                       false, false, &rng, history,
                                       history_len);
                check_question(&q, BANDS[band][0], BANDS[band][1]);
                for (uint8_t h = 0; h < history_len; h++) {
                    assert(q.poem_id != history[h].poem_id);
                }

                if (history_len < POEM_HISTORY) {
                    history[history_len++] = q;
                } else {
                    for (int i = 1; i < POEM_HISTORY; i++) {
                        history[i - 1] = history[i];
                    }
                    history[POEM_HISTORY - 1] = q;
                }
            }
        }
    }
}

static void test_identify_only(void)
{
    poem_rng_t rng;
    poem_rng_seed(&rng, 4242);
    for (int n = 0; n < 40; n++) {
        poem_question_t q;
        poem_question_generate(&q, 1, 3, false, true, &rng, NULL, 0);
        assert(q.kind == POEM_Q_TITLE || q.kind == POEM_Q_AUTHOR);
        check_question(&q, 1, 3);
    }
}

static void test_assist_and_clamping(void)
{
    poem_rng_t rng;
    poem_rng_seed(&rng, 12345);

    poem_question_t q;
    poem_question_generate(&q, 2, 2, true, false, &rng, NULL, 0);
    check_question(&q, 1, 2);
    poem_question_generate(&q, 2, 2, false, false, &rng, NULL, 0);
    check_question(&q, 2, 2);

    poem_question_generate(&q, 0, 9, false, false, &rng, NULL, 0);
    check_question(&q, 1, 6);
    poem_question_generate(&q, 5, 2, false, false, &rng, NULL, 0);
    check_question(&q, 2, 5);
}

static void test_identity_and_names(void)
{
    poem_rng_t rng;
    poem_rng_seed(&rng, 777);
    poem_question_t a;
    poem_question_t b;
    poem_question_generate(&a, 1, 6, false, false, &rng, NULL, 0);
    b = a;
    assert(poem_question_same(&a, &b));
    b.kind = (uint8_t)((a.kind + 1u) % 4u);
    assert(poem_question_same(&a, &b));
    b.text[0] = 'x';
    assert(!poem_question_same(&a, &b));

    for (uint8_t kind = 0; kind <= POEM_Q_AUTHOR; kind++) {
        assert(poem_kind_name(kind)[0] != '\0');
    }
}

static void test_rng(void)
{
    poem_rng_t rng;
    poem_rng_seed(&rng, 0);
    const uint32_t first = poem_rng_next(&rng);
    assert(first != 0);
    for (int i = 0; i < 1000; i++) {
        assert(poem_rng_below(&rng, 7) < 7);
    }
    assert(poem_rng_below(&rng, 0) == 0);
}

int main(void)
{
    test_library_integrity();
    test_generation();
    test_identify_only();
    test_assist_and_clamping();
    test_identity_and_names();
    test_rng();
    puts("test_poem_question: PASS");
    return 0;
}
