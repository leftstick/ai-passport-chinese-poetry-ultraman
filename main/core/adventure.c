#include "adventure.h"

#include <stddef.h>

#include "collection.h"

typedef struct {
    uint16_t poem_id;
    const char *title;
    const char *scene;
    const char *intro;
    const char *reward;
    const char *explanation;
} stage_info_t;

// Poem ids come from assets/poems/poem_library.json.
static const stage_info_t STAGES[ADVENTURE_STAGE_COUNT] = {
    {
        .poem_id = 7,  // 春晓
        .title = "春晓",
        .scene = "春山花林",
        .intro = "春天醒来了，花瓣落在小溪上。光之勇士，一起收集这首诗吧。",
        .reward = "你听见小鸟的叫声了吗？这首诗已经被光之诗库收藏啦。",
        .explanation =
            "春天睡得真香，不知不觉天就亮了。到处都是小鸟的叫声，"
            "昨夜的风雨吹落了好多花瓣。",
    },
    {
        .poem_id = 2,  // 静夜思
        .title = "静夜思",
        .scene = "月夜窗前",
        .intro = "月光像薄薄的霜。李白正在想家，我们轻轻读一读。",
        .reward = "月亮把光借给了我们，这首诗也亮晶晶地收进图鉴了。",
        .explanation =
            "床前洒满明亮的月光，好像地上结了白霜。抬头看看月亮，"
            "低下头就想起了远方的家。",
    },
    {
        .poem_id = 16,  // 登鹳雀楼
        .title = "登鹳雀楼",
        .scene = "江楼落日",
        .intro = "太阳落山，黄河入海。站得高才看得远，一起登楼吧！",
        .reward = "再登一层楼，就能看见更远的风景。这首诗已经归位！",
        .explanation =
            "太阳靠着山慢慢落下，黄河向大海流去。想看得更远，"
            "就要再登上一层楼。",
    },
};

static const uint8_t CHALLENGE_SEQUENCE[ADVENTURE_CHALLENGE_COUNT] = {
    CHALLENGE_ORDER,
    CHALLENGE_NEXT_LINE,
    CHALLENGE_PICTURE,
    CHALLENGE_BLANK,
};

typedef struct {
    const char *title;
    const char *text;
} story_info_t;

static const story_info_t STORIES[ADVENTURE_STORY_COUNT] = {
    {
        .title = "光之信物",
        .text =
            "奥特曼把一枚小小的光之徽章放在你手心。"
            "“每读懂一首诗，徽章就会亮一点。这是守护诗篇的证明。”",
    },
    {
        .title = "月下的伙伴",
        .text =
            "夜里，奥特曼坐在窗边陪你读诗。"
            "“我也常常在很远的地方看星星。想家的时候，就抬头看看月亮。”",
    },
    {
        .title = "诗篇守护者",
        .text =
            "三首诗篇亮起，光之诗库的大门打开了。"
            "“从今天起，你就是诗篇守护者。还有更多古诗，在等我们一起冒险。”",
    },
};

static const char *const DEFAULT_EXPLANATION =
    "把诗句大声读一遍，想象诗里的画面，就更容易记住啦。";

const char *adventure_stage_poem_title(uint8_t stage)
{
    return stage < ADVENTURE_STAGE_COUNT ? STAGES[stage].title : "古诗";
}

const char *adventure_stage_scene(uint8_t stage)
{
    return stage < ADVENTURE_STAGE_COUNT ? STAGES[stage].scene : "诗境";
}

const char *adventure_stage_intro(uint8_t stage)
{
    return stage < ADVENTURE_STAGE_COUNT ? STAGES[stage].intro
                                         : "光之勇士，我们出发吧。";
}

const char *adventure_stage_reward_line(uint8_t stage)
{
    return stage < ADVENTURE_STAGE_COUNT ? STAGES[stage].reward
                                         : "这首诗已经收进光之诗库。";
}

uint16_t adventure_stage_poem_id(uint8_t stage)
{
    return stage < ADVENTURE_STAGE_COUNT ? STAGES[stage].poem_id : 1;
}

uint8_t adventure_stage_for_poem(uint16_t poem_id)
{
    for (uint8_t i = 0; i < ADVENTURE_STAGE_COUNT; i++) {
        if (STAGES[i].poem_id == poem_id) {
            return i;
        }
    }
    return 0xFF;
}

const char *adventure_poem_explanation(uint16_t poem_id)
{
    for (uint8_t i = 0; i < ADVENTURE_STAGE_COUNT; i++) {
        if (STAGES[i].poem_id == poem_id) {
            return STAGES[i].explanation;
        }
    }
    return DEFAULT_EXPLANATION;
}

uint8_t adventure_challenge_kind(uint8_t index)
{
    return index < ADVENTURE_CHALLENGE_COUNT ? CHALLENGE_SEQUENCE[index]
                                             : CHALLENGE_SEQUENCE[0];
}

void adventure_begin(adventure_t *adventure, uint8_t stage)
{
    if (adventure == NULL) {
        return;
    }
    adventure->stage = stage < ADVENTURE_STAGE_COUNT ? stage : 0;
    adventure->poem_id = adventure_stage_poem_id(adventure->stage);
    adventure->step = ADVENTURE_STEP_INTRO;
    adventure->challenge_index = 0;
    adventure->attempts = 0;
    adventure->hints = 0;
}

void adventure_start_challenges(adventure_t *adventure)
{
    if (adventure == NULL) {
        return;
    }
    adventure->step = ADVENTURE_STEP_CHALLENGE;
    adventure->challenge_index = 0;
    adventure->attempts = 0;
    adventure->hints = 0;
}

bool adventure_wrong(adventure_t *adventure)
{
    if (adventure == NULL) {
        return false;
    }
    adventure->attempts++;
    if (adventure->attempts >= 2) {
        adventure->hints++;
        return true;
    }
    return false;
}

void adventure_correct(adventure_t *adventure)
{
    if (adventure == NULL) {
        return;
    }
    adventure->attempts = 0;
    if (adventure->challenge_index + 1 < ADVENTURE_CHALLENGE_COUNT) {
        adventure->challenge_index++;
    } else {
        adventure->step = ADVENTURE_STEP_REWARD;
    }
}

bool adventure_in_reward(const adventure_t *adventure)
{
    return adventure != NULL && adventure->step == ADVENTURE_STEP_REWARD;
}

void adventure_skip_to_reward(adventure_t *adventure)
{
    if (adventure == NULL) {
        return;
    }
    adventure->step = ADVENTURE_STEP_DONE;
    adventure->challenge_index = ADVENTURE_CHALLENGE_COUNT;
}

const char *adventure_story_title(uint8_t index)
{
    return index < ADVENTURE_STORY_COUNT ? STORIES[index].title : "";
}

const char *adventure_story_text(uint8_t index)
{
    return index < ADVENTURE_STORY_COUNT ? STORIES[index].text : "";
}

uint8_t adventure_story_threshold(uint8_t index)
{
    return collection_story_threshold(index);
}
