// Host tests for the collection-adventure state machine.
#include "adventure.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_catalogue(void)
{
    assert(strcmp(adventure_stage_poem_title(0), "春晓") == 0);
    assert(strcmp(adventure_stage_poem_title(1), "静夜思") == 0);
    assert(strcmp(adventure_stage_poem_title(2), "登鹳雀楼") == 0);
    assert(adventure_stage_poem_id(0) == 7);
    assert(adventure_stage_poem_id(1) == 2);
    assert(adventure_stage_poem_id(2) == 16);
    assert(adventure_stage_for_poem(2) == 1);
    assert(adventure_stage_for_poem(999) == 0xFF);
    assert(adventure_stage_scene(0)[0] != '\0');
    assert(adventure_stage_intro(0)[0] != '\0');
    assert(adventure_poem_explanation(7)[0] != '\0');
    assert(adventure_challenge_kind(0) == CHALLENGE_ORDER);
    assert(adventure_challenge_kind(1) == CHALLENGE_NEXT_LINE);
    assert(adventure_challenge_kind(2) == CHALLENGE_PICTURE);
    assert(adventure_challenge_kind(3) == CHALLENGE_BLANK);
}

static void test_flow(void)
{
    adventure_t adventure;
    adventure_begin(&adventure, 0);
    assert(adventure.poem_id == 7);
    assert(adventure.step == ADVENTURE_STEP_INTRO);
    assert(adventure.challenge_index == 0);

    adventure_start_challenges(&adventure);
    assert(adventure.step == ADVENTURE_STEP_CHALLENGE);

    // The first wrong answer is gentle; the second brings the hint.
    assert(!adventure_wrong(&adventure));
    assert(adventure.attempts == 1);
    assert(adventure_wrong(&adventure));
    assert(adventure.hints == 1);

    // A correct answer resets attempts but keeps the hint count.
    adventure_correct(&adventure);
    assert(adventure.challenge_index == 1);
    assert(adventure.attempts == 0);
    assert(adventure.hints == 1);

    adventure_correct(&adventure);
    adventure_correct(&adventure);
    assert(adventure.challenge_index == 3);
    assert(adventure.step == ADVENTURE_STEP_CHALLENGE);
    adventure_correct(&adventure);
    assert(adventure.step == ADVENTURE_STEP_REWARD);
    assert(adventure_in_reward(&adventure));
}

static void test_stories(void)
{
    for (uint8_t i = 0; i < ADVENTURE_STORY_COUNT; i++) {
        assert(adventure_story_title(i)[0] != '\0');
        assert(adventure_story_text(i)[0] != '\0');
        assert(adventure_story_threshold(i) >= 1);
    }
}

int main(void)
{
    test_catalogue();
    test_flow();
    test_stories();
    printf("test_adventure: PASS\n");
    return 0;
}
