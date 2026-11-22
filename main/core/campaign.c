#include "campaign.h"

#include <stddef.h>

// The first seven entries keep the original save-game indices.
static const hero_def_t HEROES[CAMPAIGN_HERO_COUNT] = {
    { .name = "赛罗奥特曼", .short_name = "赛罗", .perk_text = "必杀槽 +25%",
      .sprite_prefix = "zero", .accent_color = 0x4C9DFF, .perk = HERO_PERK_GAUGE },
    { .name = "泰罗奥特曼", .short_name = "泰罗", .perk_text = "暴击伤害 +1",
      .sprite_prefix = "taro", .accent_color = 0xFF5A5A, .perk = HERO_PERK_CRIT_DAMAGE },
    { .name = "梦比优斯奥特曼", .short_name = "梦比优斯", .perk_text = "答题时间 +0.8秒",
      .sprite_prefix = "mebius", .accent_color = 0xF2C14E, .perk = HERO_PERK_TIME },
    { .name = "泽塔奥特曼", .short_name = "泽塔", .perk_text = "暴击区 +30%",
      .sprite_prefix = "z", .accent_color = 0x7BE3FF, .perk = HERO_PERK_CRIT_ZONE },
    { .name = "迪迦奥特曼", .short_name = "迪迦", .perk_text = "每关首次受伤免疫",
      .sprite_prefix = "tiga-multi", .accent_color = 0xB07CFF, .perk = HERO_PERK_SHIELD },
    { .name = "贝利亚奥特曼", .short_name = "贝利亚", .perk_text = "暴击伤害 +1",
      .sprite_prefix = "belial", .accent_color = 0xFF3B6B, .perk = HERO_PERK_CRIT_DAMAGE },
    { .name = "捷德奥特曼·尊皇", .short_name = "捷德·尊皇", .perk_text = "每关首次受伤免疫",
      .sprite_prefix = "geed-royal-megamaster", .accent_color = 0x5F8CFF,
      .perk = HERO_PERK_SHIELD },
    { .name = "赛文奥特曼", .short_name = "赛文", .perk_text = "暴击区 +30%",
      .sprite_prefix = "seven", .accent_color = 0xFF8A3D, .perk = HERO_PERK_CRIT_ZONE },
    { .name = "银河奥特曼", .short_name = "银河", .perk_text = "必杀槽 +25%",
      .sprite_prefix = "ginga", .accent_color = 0x8FD3FF, .perk = HERO_PERK_GAUGE },
    { .name = "欧布奥特曼·斯派修姆", .short_name = "欧布·斯派修姆",
      .perk_text = "暴击伤害 +1", .sprite_prefix = "orb-spacium",
      .accent_color = 0xFF6F91, .perk = HERO_PERK_CRIT_DAMAGE },
    { .name = "欧布奥特曼·雷霆胸章", .short_name = "欧布·雷霆",
      .perk_text = "必杀槽 +25%", .sprite_prefix = "orb-thunder-breastar",
      .accent_color = 0xFF3B3B, .perk = HERO_PERK_GAUGE },
    { .name = "捷德奥特曼·银河崛起", .short_name = "捷德·银河",
      .perk_text = "暴击区 +30%", .sprite_prefix = "geed-galaxy-rising",
      .accent_color = 0x4CE0C8, .perk = HERO_PERK_CRIT_ZONE },
    { .name = "特利迦奥特曼", .short_name = "特利迦", .perk_text = "答题时间 +0.8秒",
      .sprite_prefix = "trigger", .accent_color = 0xC08CFF, .perk = HERO_PERK_TIME },
    { .name = "德凯奥特曼", .short_name = "德凯", .perk_text = "暴击区 +30%",
      .sprite_prefix = "decker", .accent_color = 0x5AD1FF, .perk = HERO_PERK_CRIT_ZONE },
    { .name = "布莱泽奥特曼", .short_name = "布莱泽", .perk_text = "每关首次受伤免疫",
      .sprite_prefix = "blazar", .accent_color = 0x6FA8FF, .perk = HERO_PERK_SHIELD },
    { .name = "究极赛罗", .short_name = "究极赛罗", .perk_text = "必杀槽 +25%",
      .sprite_prefix = "ultimate-zero", .accent_color = 0xFFD166,
      .perk = HERO_PERK_GAUGE },
    { .name = "艾克斯奥特曼", .short_name = "艾克斯", .perk_text = "暴击区 +30%",
      .sprite_prefix = "x", .accent_color = 0x38D6FF,
      .perk = HERO_PERK_CRIT_ZONE },
    { .name = "佐菲奥特曼", .short_name = "佐菲", .perk_text = "必杀槽 +25%",
      .sprite_prefix = "zoffy", .accent_color = 0xFFB347,
      .perk = HERO_PERK_GAUGE },
};

static const stage_def_t STAGES[CAMPAIGN_STAGE_COUNT] = {
    { .name = "启程星·诗光初现", .monster_name = "巴尔坦星人", .domain = "一年级 · 五言短句",
      .sprite_name = "baltan", .grade_min = 1, .grade_max = 2, .question_ms = 8000, .monster_hp = 4,
      .rules = STAGE_RULE_HINT_FREE, .boss = false },
    { .name = "月牙星·静夜拾句", .monster_name = "贝姆拉", .domain = "一~二年级 · 月夜思乡",
      .sprite_name = "bemular", .grade_min = 1, .grade_max = 2, .question_ms = 7800, .monster_hp = 4,
      .rules = 0, .boss = false },
    { .name = "春晓星·花鸟寻诗", .monster_name = "艾雷王", .domain = "二~三年级 · 春景花鸟",
      .sprite_name = "eleking", .grade_min = 2, .grade_max = 3, .question_ms = 7000, .monster_hp = 5,
      .rules = STAGE_RULE_HALF_NONCRIT, .boss = false },
    { .name = "山水星·瀑布飞流", .monster_name = "哥尔赞", .domain = "二~三年级 · 山水风光",
      .sprite_name = "golza", .grade_min = 2, .grade_max = 3, .question_ms = 6800, .monster_hp = 5,
      .rules = STAGE_RULE_FAST_GAUGE, .boss = false },
    { .name = "江雪星·孤舟独钓", .monster_name = "哥莫拉", .domain = "二~三年级 · 雪景童趣",
      .sprite_name = "gomora", .grade_min = 2, .grade_max = 3, .question_ms = 6200, .monster_hp = 6,
      .rules = 0, .boss = false },
    { .name = "云海星·千年诗魂", .monster_name = "金古桥", .domain = "三~四年级 · 情景交融",
      .sprite_name = "kingjoe", .grade_min = 3, .grade_max = 4, .question_ms = 6000, .monster_hp = 6,
      .rules = STAGE_RULE_GUARD_QTE, .boss = false },
    { .name = "星河诗卷·唐风万里", .monster_name = "雷德王", .domain = "四~五年级 · 唐诗名篇",
      .sprite_name = "redking", .grade_min = 4, .grade_max = 5, .question_ms = 5600, .monster_hp = 7,
      .rules = STAGE_RULE_ARMOR_BREAK, .boss = false },
    { .name = "银河星·宋韵长歌", .monster_name = "泰兰特", .domain = "五~六年级 · 宋诗长歌",
      .sprite_name = "tyrant", .grade_min = 5, .grade_max = 6, .question_ms = 5400, .monster_hp = 7,
      .rules = STAGE_RULE_HALF_NONCRIT, .boss = false },
    { .name = "光之诗核·遗忘终章", .monster_name = "杰顿", .domain = "四~六年级 · 光之诗库终战",
      .sprite_name = "zetton", .grade_min = 4, .grade_max = 6, .question_ms = 5200, .monster_hp = 8,
      .rules = STAGE_RULE_ENRAGE, .boss = true },
};

// Monster gallery, indexed by the visual monster id used by the battle and
// cycle planner. The first nine entries keep the original stage order.
static const monster_def_t MONSTERS[CAMPAIGN_MONSTER_COUNT] = {
    { .name = "巴尔坦星人", .domain = "一年级 · 五言短句", .sprite_name = "baltan" },
    { .name = "贝姆拉", .domain = "一~二年级 · 月夜思乡", .sprite_name = "bemular" },
    { .name = "艾雷王", .domain = "二~三年级 · 春景花鸟", .sprite_name = "eleking" },
    { .name = "哥尔赞", .domain = "二~三年级 · 山水风光", .sprite_name = "golza" },
    { .name = "哥莫拉", .domain = "二~三年级 · 雪景童趣", .sprite_name = "gomora" },
    { .name = "金古桥", .domain = "三~四年级 · 情景交融", .sprite_name = "kingjoe" },
    { .name = "雷德王", .domain = "四~五年级 · 唐诗名篇", .sprite_name = "redking" },
    { .name = "泰兰特", .domain = "五~六年级 · 宋诗长歌", .sprite_name = "tyrant" },
    { .name = "杰顿", .domain = "四~六年级 · 光之诗核", .sprite_name = "zetton" },
    { .name = "布莱克王", .domain = "一~三年级 · 光之启程", .sprite_name = "blackking" },
    { .name = "双尾怪", .domain = "四~六年级 · 星河诗卷", .sprite_name = "twintail" },
};

static const difficulty_def_t DIFFICULTIES[CAMPAIGN_DIFFICULTY_COUNT] = {
    { .name = "星光", .detail = "时间 +31.5秒 · 5颗心 · 年级 -1档", .time_bonus_ms = 31500,
      .hero_hp = 5, .monster_hp_delta = -1, .grade_delta = -1, .crit_window = 50 },
    { .name = "烈焰", .detail = "标准节奏 · 4颗心", .time_bonus_ms = 0,
      .hero_hp = 4, .monster_hp_delta = 0, .grade_delta = 0, .crit_window = 40 },
    { .name = "究极", .detail = "时间 -1秒 · 3颗心", .time_bonus_ms = -1000,
      .hero_hp = 3, .monster_hp_delta = 1, .grade_delta = 0, .crit_window = 32 },
};

const hero_def_t *campaign_hero(uint8_t index)
{
    if (index >= CAMPAIGN_HERO_COUNT) {
        index = 0;
    }
    return &HEROES[index];
}

const stage_def_t *campaign_stage(uint8_t index)
{
    if (index >= CAMPAIGN_STAGE_COUNT) {
        index = 0;
    }
    return &STAGES[index];
}

// 周目轮换只改变视觉怪兽, 名字和图鉴都按怪兽编号取, 而不是关卡编号。
const monster_def_t *campaign_monster(uint8_t index)
{
    if (index >= CAMPAIGN_MONSTER_COUNT) {
        index = 0;
    }
    return &MONSTERS[index];
}

const char *campaign_monster_name(uint8_t monster)
{
    return campaign_monster(monster)->name;
}

const difficulty_def_t *campaign_difficulty(uint8_t index)
{
    if (index >= CAMPAIGN_DIFFICULTY_COUNT) {
        index = 0;
    }
    return &DIFFICULTIES[index];
}

uint16_t campaign_question_ms(uint8_t stage, uint8_t difficulty, uint8_t hero)
{
    const stage_def_t *stage_def = campaign_stage(stage);
    const difficulty_def_t *diff = campaign_difficulty(difficulty);
    int32_t ms = (int32_t)stage_def->question_ms + diff->time_bonus_ms;
    if ((stage_def->rules & STAGE_RULE_FAST_GAUGE) != 0) {
        ms = ms * 9 / 10;
    }
    if (campaign_hero(hero)->perk == HERO_PERK_TIME) {
        ms += 800;
    }
    if (ms < 2500) {
        ms = 2500;
    }
    if (ms > 45000) {
        ms = 45000;
    }
    return (uint16_t)ms;
}

void campaign_poem_grades(uint8_t stage, uint8_t difficulty, uint8_t *grade_min,
                          uint8_t *grade_max)
{
    const stage_def_t *stage_def = campaign_stage(stage);
    int low = (int)stage_def->grade_min +
              (int)campaign_difficulty(difficulty)->grade_delta;
    int high = (int)stage_def->grade_max +
               (int)campaign_difficulty(difficulty)->grade_delta;
    if (low < 1) {
        low = 1;
    }
    if (high < low) {
        high = low;
    }
    if (high > 6) {
        high = 6;
    }
    if (low > high) {
        low = high;
    }
    if (grade_min != NULL) {
        *grade_min = (uint8_t)low;
    }
    if (grade_max != NULL) {
        *grade_max = (uint8_t)high;
    }
}

uint8_t campaign_hero_hp(uint8_t difficulty)
{
    return campaign_difficulty(difficulty)->hero_hp;
}

uint8_t campaign_monster_hp(uint8_t stage, uint8_t difficulty)
{
    const stage_def_t *stage_def = campaign_stage(stage);
    int hp = (int)stage_def->monster_hp +
             (int)campaign_difficulty(difficulty)->monster_hp_delta;
    if (hp < 2) {
        hp = 2;
    }
    return (uint8_t)hp;
}

uint8_t campaign_crit_window(uint8_t difficulty, uint8_t hero)
{
    int window = campaign_difficulty(difficulty)->crit_window;
    if (campaign_hero(hero)->perk == HERO_PERK_CRIT_ZONE) {
        window = window * 13 / 10;
    }
    if (window > 70) {
        window = 70;
    }
    return (uint8_t)window;
}

bool campaign_stage_has_rule(uint8_t stage, uint8_t rule)
{
    return (campaign_stage(stage)->rules & rule) != 0;
}
