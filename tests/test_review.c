// Host tests for the wrong-answer review pool.
#include "review.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static poem_question_t make_question(uint16_t id, const char *text, uint8_t grade)
{
    poem_question_t question;
    memset(&question, 0, sizeof(question));
    snprintf(question.text, sizeof(question.text), "%s", text);
    snprintf(question.hint, sizeof(question.hint), "回顾 %s", text);
    snprintf(question.options[0], sizeof(question.options[0]), "%s", text);
    snprintf(question.options[1], sizeof(question.options[1]), "干扰一");
    snprintf(question.options[2], sizeof(question.options[2]), "干扰二");
    question.poem_id = id;
    question.correct_index = 0;
    question.kind = POEM_Q_NEXT_LINE;
    question.grade = grade;
    return question;
}

static void test_record_and_collect(void)
{
    review_store_t store;
    review_store_init(&store);
    assert(store.count == 0);

    const poem_question_t a = make_question(1, "3+4", 1);
    const poem_question_t b = make_question(2, "8+2", 2);
    const poem_question_t c = make_question(3, "9-3", 1);
    assert(review_store_record(&store, &a, 1));
    assert(review_store_record(&store, &b, 2));
    assert(review_store_record(&store, &c, 1));
    assert(store.count == 3);

    review_record_t out[3];
    uint8_t n = review_store_collect(&store, 1, 0, out, 3);
    assert(n == 3);
    // Newest same-stage question first, then the older one, then other stages.
    assert(strcmp(out[0].question.text, "9-3") == 0);
    assert(strcmp(out[1].question.text, "3+4") == 0);
    assert(strcmp(out[2].question.text, "8+2") == 0);
    assert(out[0].stage == 1);
    assert(out[1].mistakes == 1);

    // A limit smaller than the pool truncates from the front.
    n = review_store_collect(&store, 1, 0, out, 1);
    assert(n == 1);
    assert(strcmp(out[0].question.text, "9-3") == 0);
}

static void test_duplicate_moves_to_newest(void)
{
    review_store_t store;
    review_store_init(&store);
    const poem_question_t a = make_question(11, "2+2", 1);
    const poem_question_t b = make_question(12, "5+5", 2);

    assert(review_store_record(&store, &a, 1));
    assert(review_store_record(&store, &b, 2));
    assert(review_store_record(&store, &a, 1));
    assert(store.count == 2);

    review_record_t out[2];
    const uint8_t n = review_store_collect(&store, 1, 0, out, 2);
    assert(n == 2);
    assert(strcmp(out[0].question.text, "2+2") == 0);
    assert(out[0].mistakes == 2);
    assert(strcmp(out[1].question.text, "5+5") == 0);
}

static void test_overflow_drops_oldest(void)
{
    review_store_t store;
    review_store_init(&store);

    for (int i = 0; i < REVIEW_CAPACITY + 1; i++) {
        char text[16];
        snprintf(text, sizeof(text), "1+%d", i);
        const poem_question_t q = make_question((uint16_t)(100 + i), text, 1);
        assert(review_store_record(&store, &q, 1));
    }
    assert(store.count == REVIEW_CAPACITY);

    // The very first record has been evicted; the newest is still present.
    review_record_t out[REVIEW_CAPACITY];
    const uint8_t n =
        review_store_collect(&store, 1, 0, out, REVIEW_CAPACITY);
    assert(n == REVIEW_CAPACITY);
    assert(strcmp(out[0].question.text, "1+40") == 0);
    bool found_first = false;
    for (int i = 0; i < n; i++) {
        if (strcmp(out[i].question.text, "1+0") == 0) {
            found_first = true;
        }
    }
    assert(!found_first);
}

static void test_clear(void)
{
    review_store_t store;
    review_store_init(&store);
    const poem_question_t q = make_question(21, "6+6", 3);
    assert(review_store_record(&store, &q, 3));
    assert(store.count == 1);
    review_store_clear(&store);
    assert(store.count == 0);

    review_record_t out[1];
    assert(review_store_collect(&store, 3, 0, out, 1) == 0);
}

static void test_collect_respects_grade_cap(void)
{
    review_store_t store;
    review_store_init(&store);
    poem_question_t easy = make_question(31, "2+3", 1);
    poem_question_t mid = make_question(32, "12-7", 2);
    poem_question_t hard = make_question(33, "3*8+4", 4);
    assert(review_store_record(&store, &easy, 1));
    assert(review_store_record(&store, &mid, 2));
    assert(review_store_record(&store, &hard, 7));

    review_record_t out[3];
    uint8_t n = review_store_collect(&store, 1, 2, out, 3);
    assert(n == 2);
    assert(strcmp(out[0].question.text, "2+3") == 0);
    assert(strcmp(out[1].question.text, "12-7") == 0);

    // 年级上限 1 会同时挡掉跨关卡题和高年级题。
    n = review_store_collect(&store, 1, 1, out, 3);
    assert(n == 1);
    assert(strcmp(out[0].question.text, "2+3") == 0);
}

static void test_mastered_removes_record(void)
{
    review_store_t store;
    review_store_init(&store);
    const poem_question_t q = make_question(41, "7+8", 2);
    assert(review_store_record(&store, &q, 3));
    assert(review_store_record(&store, &q, 3));
    assert(store.count == 1);
    assert(store.items[0].mistakes == 2);

    // The first correct answer lowers the count, the second clears it.
    assert(!review_store_mastered(&store, &q));
    assert(store.count == 1);
    assert(store.items[0].mistakes == 1);
    assert(review_store_mastered(&store, &q));
    assert(store.count == 0);

    // Mastering an unknown question is a safe no-op.
    assert(!review_store_mastered(&store, &q));
    assert(store.count == 0);
}

int main(void)
{
    test_record_and_collect();
    test_duplicate_moves_to_newest();
    test_overflow_drops_oldest();
    test_clear();
    test_collect_respects_grade_cap();
    test_mastered_removes_record();
    puts("test_review: PASS");
    return 0;
}