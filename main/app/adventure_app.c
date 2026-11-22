// main/app/adventure_app.c —— 奥特曼诗词收集冒险。
//
// 设计目标: 慢节奏、以“解锁诗篇”为奖励，而不是数学配对游戏那种快速判分循环。
// 四种交互(拼句/接龙/看图/填字)都围绕同一首诗展开，答错只给提示，不扣分。
//
// 线程模型:
//   * 按键回调在 esp_timer 任务里只入队;输入任务调用 adventure_app_handle_key()。
//   * 所有 LVGL 对象与游戏状态都在 bsp_lvgl_lock() 内访问。
//   * NVS 写入在锁内完成，但只在收集诗篇时触发，频率很低。
#include "adventure_app.h"

#include <stdio.h>
#include <string.h>

#include "adventure.h"
#include "audio_player.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "collection.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "poem_challenge.h"
#include "poem_library.h"
#include "poem_question.h"
#include "poetry_art.h"
#include "poetry_audio.h"

LV_FONT_DECLARE(app_font_12);
LV_FONT_DECLARE(app_font_14);
LV_FONT_DECLARE(app_font_20);
LV_FONT_DECLARE(app_font_28);

static const char *TAG = "adventure";

#define SCREEN_W 240
#define SCREEN_H 320
#define TICK_MS 40
#define NVS_NAMESPACE "poem_adv_v1"
#define NVS_KEY_CODEX "codex"
#define NVS_KEY_MUTE "mute"

#define C_PAPER      0xF7F0E1
#define C_PAPER_DARK 0xEADDC4
#define C_INK        0x2B2B2B
#define C_INK_LIGHT  0x5A5147
#define C_SEAL       0xB43A2E
#define C_JADE       0x5E8271
#define C_GOLD       0xB98A2E
#define C_MUTED      0x948B7B
#define C_GREEN      0x4F7A54
#define C_WHITE      0xFFFFFF
#define C_NIGHT      0x15213A

typedef enum {
    PAGE_HOME = 0,
    PAGE_MAP,
    PAGE_INTRO,
    PAGE_CHALLENGE,
    PAGE_REWARD,
    PAGE_CODEX,
    PAGE_CODEX_DETAIL,
    PAGE_STORY,
    PAGE_SPEED,
    PAGE_SPEED_RESULT,
} page_t;

#define HOME_ITEMS 4
#define SPEED_QUESTIONS 5
#define SPEED_SECONDS 15

typedef struct {
    page_t page;
    lv_obj_t *root;
    lv_obj_t *battery;

    uint8_t home_index;
    uint8_t map_index;
    uint8_t codex_index;

    adventure_t adv;
    poem_challenge_t challenge;
    poem_rng_t rng;
    uint8_t cursor;
    uint8_t order[CHALLENGE_MAX_TILES];
    uint8_t order_len;
    bool picked;
    uint8_t picked_pos;
    bool hint;

    collection_t collection;
    uint8_t story_bits;  // newly unlocked stories still to read
    uint8_t story_index;
    bool bgm_on;

    bool muted;
    int battery_soc;

    poem_question_t speed_q;
    uint8_t speed_index;
    uint8_t speed_score;
    uint8_t speed_cursor;
    bool speed_answered;
    bool speed_correct;
    int64_t speed_deadline_us;
    int64_t speed_next_us;
    lv_obj_t *speed_bar;
    lv_obj_t *speed_clock;
} app_t;

static app_t s;

static void render(void);
static void set_page(page_t page);
static void begin_challenge(void);

static const poem_t *poem_by_id(uint16_t id)
{
    for (uint16_t i = 0; i < poem_library_count(); i++) {
        const poem_t *poem = poem_library_at(i);
        if (poem->id == id) {
            return poem;
        }
    }
    return poem_library_at(0);
}

// ---------------------------------------------------------------------------
// 基础控件
// ---------------------------------------------------------------------------
static lv_obj_t *make_screen(uint32_t color)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    return scr;
}

static void add_scene(lv_obj_t *scr, uint8_t stage)
{
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, poetry_art_scene(stage));
    lv_obj_set_pos(img, 0, 0);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE);
}

static lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h,
                            uint32_t color, lv_opa_t opa, int radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    return obj;
}

static lv_obj_t *make_label(lv_obj_t *parent, int x, int y, int w,
                            const char *text, const lv_font_t *font,
                            uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    if (w > 0) {
        lv_obj_set_width(label, w);
    }
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *make_wrap_label(lv_obj_t *parent, int x, int y, int w,
                                 const char *text, const lv_font_t *font,
                                 uint32_t color, int line_space)
{
    lv_obj_t *label = make_label(parent, x, y, w, text, font, color);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(label, line_space, 0);
    return label;
}

static lv_obj_t *make_center(lv_obj_t *parent, int x, int y, int w,
                             const char *text, const lv_font_t *font,
                             uint32_t color)
{
    lv_obj_t *label = make_label(parent, x, y, w, text, font, color);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

static lv_obj_t *make_right_label(lv_obj_t *parent, int x, int y, int w,
                                  const char *text, const lv_font_t *font,
                                  uint32_t color)
{
    lv_obj_t *label = make_label(parent, x, y, w, text, font, color);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
    return label;
}

static void style_card(lv_obj_t *obj, bool selected, bool enabled)
{
    lv_obj_set_style_border_width(obj, selected ? 2 : 1, 0);
    lv_obj_set_style_border_color(
        obj, lv_color_hex(selected ? C_GOLD : C_MUTED), 0);
    lv_obj_set_style_bg_color(
        obj, lv_color_hex(selected ? C_PAPER_DARK : C_PAPER), 0);
    lv_obj_set_style_bg_opa(obj, enabled ? LV_OPA_COVER : LV_OPA_60, 0);
}

static void add_battery(lv_obj_t *scr)
{
    lv_obj_t *panel = make_panel(scr, 186, 2, 52, 18, C_PAPER, LV_OPA_90, 7);
    (void)panel;
    s.battery = make_label(scr, 190, 4, 46, "--", &app_font_12, C_INK_LIGHT);
    lv_obj_set_style_text_align(s.battery, LV_TEXT_ALIGN_CENTER, 0);
    if (s.battery_soc >= 0) {
        char text[12];
        snprintf(text, sizeof(text), "%d%%", s.battery_soc);
        lv_label_set_text(s.battery, text);
    }
}

static void add_footer(lv_obj_t *scr, const char *text, uint32_t color)
{
    make_center(scr, 0, 300, SCREEN_W, text, &app_font_12, color);
}

static void add_chibi(lv_obj_t *scr, int x, int y, uint8_t pose, uint16_t scale)
{
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, poetry_art_ultraman(pose));
    lv_image_set_scale(img, scale);
    lv_obj_set_pos(img, x, y);
}

// ---------------------------------------------------------------------------
// 存档
// ---------------------------------------------------------------------------
static void nvs_load(void)
{
    collection_init(&s.collection);
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        s.muted = false;
        return;
    }
    uint8_t blob[COLLECTION_BLOB_SIZE];
    size_t len = sizeof(blob);
    if (nvs_get_blob(handle, NVS_KEY_CODEX, blob, &len) == ESP_OK) {
        (void)collection_deserialize(&s.collection, blob, len);
    }
    uint8_t muted = 0;
    if (nvs_get_u8(handle, NVS_KEY_MUTE, &muted) == ESP_OK) {
        s.muted = muted != 0;
    }
    nvs_close(handle);
}

static void nvs_save(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }
    uint8_t blob[COLLECTION_BLOB_SIZE];
    const size_t len = collection_serialize(&s.collection, blob, sizeof(blob));
    if (len > 0) {
        (void)nvs_set_blob(handle, NVS_KEY_CODEX, blob, len);
    }
    (void)nvs_set_u8(handle, NVS_KEY_MUTE, s.muted ? 1 : 0);
    (void)nvs_commit(handle);
    nvs_close(handle);
}

// ---------------------------------------------------------------------------
// 音频
// ---------------------------------------------------------------------------
static void ensure_bgm(bool on)
{
    if (s.bgm_on == on) {
        return;
    }
    s.bgm_on = on;
    if (on) {
        audio_player_set_bgm(poetry_audio_bgm());
    } else {
        audio_player_stop_bgm();
    }
}

static void cue(uint8_t id)
{
    audio_player_play(poetry_audio_sfx(id));
}

// ---------------------------------------------------------------------------
// 首页
// ---------------------------------------------------------------------------
static lv_obj_t *build_home(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, 0);
    make_panel(scr, 8, 16, 224, 290, C_PAPER, LV_OPA_90, 18);
    add_chibi(scr, 88, 26, POETRY_ULTRAMAN_CHEER, 256);
    make_center(scr, 0, 116, SCREEN_W, "奥特诗词", &app_font_28, C_SEAL);
    make_center(scr, 0, 148, SCREEN_W, "收集冒险", &app_font_20, C_INK);

    static const char *const ITEMS[HOME_ITEMS] = {
        "继续冒险", "光之诗库", "速答挑战", "音效",
    };
    for (uint8_t i = 0; i < HOME_ITEMS; i++) {
        lv_obj_t *card = make_panel(scr, 28, 184 + i * 30, 184, 26,
                                    C_PAPER, LV_OPA_COVER, 10);
        style_card(card, i == s.home_index, true);
        char text[24];
        if (i == 3) {
            snprintf(text, sizeof(text), "音效 %s",
                     s.muted ? "关闭" : "开启");
        } else {
            snprintf(text, sizeof(text), "%s", ITEMS[i]);
        }
        make_center(card, 0, 4, 184, text, &app_font_14,
                    i == s.home_index ? C_SEAL : C_INK);
    }
    add_battery(scr);
    add_footer(scr, "上下选择 · 确定进入", C_MUTED);
    return scr;
}

// ---------------------------------------------------------------------------
// 关卡地图
// ---------------------------------------------------------------------------
static lv_obj_t *build_map(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, s.map_index);
    make_panel(scr, 6, 6, 228, 308, C_PAPER, LV_OPA_90, 16);
    make_label(scr, 16, 12, 160, "诗篇地图", &app_font_20, C_INK);
    char count[24];
    snprintf(count, sizeof(count), "%u / %u", collection_count(&s.collection),
             (unsigned)ADVENTURE_STAGE_COUNT);
    make_right_label(scr, 150, 22, 72, count, &app_font_12, C_JADE);
    add_battery(scr);

    for (uint8_t i = 0; i < ADVENTURE_STAGE_COUNT; i++) {
        lv_obj_t *card = make_panel(scr, 14, 48 + i * 72, 212, 64,
                                    C_PAPER, LV_OPA_COVER, 12);
        style_card(card, i == s.map_index, true);
        char number[8];
        snprintf(number, sizeof(number), "%u", (unsigned)(i + 1));
        lv_obj_t *badge = make_panel(card, 8, 16, 32, 32, C_SEAL,
                                     LV_OPA_COVER, 16);
        (void)badge;
        make_center(card, 8, 22, 32, number, &app_font_20, C_WHITE);
        make_label(card, 48, 8, 150, adventure_stage_scene(i), &app_font_12,
                   C_JADE);
        make_label(card, 48, 24, 150, adventure_stage_poem_title(i),
                   &app_font_20, C_INK);
        const bool collected =
            collection_has(&s.collection, adventure_stage_poem_id(i));
        make_right_label(card, 132, 44, 72, collected ? "已收集" : "待收集",
                         &app_font_12, collected ? C_GOLD : C_MUTED);
    }
    add_footer(scr, "确定进入 · 长按返回", C_MUTED);
    return scr;
}

// ---------------------------------------------------------------------------
// 开场
// ---------------------------------------------------------------------------
static lv_obj_t *build_intro(void)
{
    lv_obj_t *scr = make_screen(C_NIGHT);
    add_scene(scr, s.adv.stage);
    add_chibi(scr, 168, 38, POETRY_ULTRAMAN_HINT, 256);
    add_battery(scr);
    make_panel(scr, 8, 142, 224, 168, C_PAPER, LV_OPA_90, 16);
    make_label(scr, 20, 150, 200, adventure_stage_scene(s.adv.stage),
               &app_font_14, C_JADE);
    make_label(scr, 20, 170, 200, adventure_stage_poem_title(s.adv.stage),
               &app_font_28, C_INK);
    const poem_t *poem = poem_by_id(s.adv.poem_id);
    char author[40];
    snprintf(author, sizeof(author), "%s · %s", poem->dynasty, poem->author);
    make_label(scr, 20, 204, 200, author, &app_font_12, C_MUTED);
    make_wrap_label(scr, 20, 222, 196, adventure_stage_intro(s.adv.stage),
                    &app_font_14, C_INK, 6);
    make_center(scr, 0, 288, SCREEN_W, "确定 开始闯关", &app_font_12, C_SEAL);
    return scr;
}

// ---------------------------------------------------------------------------
// 挑战
// ---------------------------------------------------------------------------
static void add_progress_dots(lv_obj_t *scr)
{
    const int start = 96;
    for (uint8_t i = 0; i < ADVENTURE_CHALLENGE_COUNT; i++) {
        const bool done = i < s.adv.challenge_index;
        const bool current = i == s.adv.challenge_index;
        make_panel(scr, start + i * 14, 30, 10, 8,
                   done ? C_GOLD : (current ? C_SEAL : C_PAPER_DARK),
                   LV_OPA_COVER, 4);
    }
}

static lv_obj_t *build_challenge_header(lv_obj_t *scr)
{
    make_panel(scr, 0, 0, SCREEN_W, 24, C_PAPER, LV_OPA_90, 0);
    char title[32];
    snprintf(title, sizeof(title), "第%u关 · %s",
             (unsigned)(s.adv.stage + 1),
             adventure_stage_poem_title(s.adv.stage));
    make_label(scr, 8, 4, 150, title, &app_font_12, C_INK);
    char progress[24];
    snprintf(progress, sizeof(progress), "%u/4 %s",
             (unsigned)(s.adv.challenge_index + 1),
             challenge_kind_name(s.challenge.kind));
    make_right_label(scr, 140, 4, 92, progress, &app_font_12, C_SEAL);
    add_progress_dots(scr);
    return scr;
}

static void add_hint_or_footer(lv_obj_t *parent, const char *footer)
{
    if (s.hint) {
        lv_obj_t *box = make_panel(parent, 8, 158, 208, 30, C_PAPER_DARK,
                                   LV_OPA_COVER, 8);
        (void)box;
        make_wrap_label(parent, 14, 162, 196, s.challenge.hint, &app_font_12,
                        C_SEAL, 2);
    } else if (footer != NULL) {
        make_center(parent, 0, 160, 208, footer, &app_font_12, C_MUTED);
    }
}

static lv_obj_t *build_order_panel(lv_obj_t *scr)
{
    lv_obj_t *panel = make_panel(scr, 8, 108, 224, 190, C_PAPER,
                                 LV_OPA_90, 14);
    make_center(panel, 0, 6, 224, s.challenge.prompt, &app_font_14, C_INK);

    const int per_row = s.challenge.tile_count > 4 ? 4 : s.challenge.tile_count;
    const int rows = (s.challenge.tile_count + per_row - 1) / per_row;
    for (uint8_t i = 0; i < s.challenge.tile_count; i++) {
        const int row = i / per_row;
        const int col = i % per_row;
        const int row_count = (row == rows - 1)
                                  ? s.challenge.tile_count - row * per_row
                                  : per_row;
        const int tile_w = 38;
        const int gap = 5;
        const int row_w = row_count * tile_w + (row_count - 1) * gap;
        const int x0 = (224 - row_w) / 2;
        const int x = x0 + col * (tile_w + gap);
        const int y = 30 + row * 44;
        lv_obj_t *tile = make_panel(panel, x, y, tile_w, 38, C_WHITE,
                                    LV_OPA_COVER, 8);
        const bool selected = s.cursor == i;
        lv_obj_set_style_border_width(tile, selected ? 2 : 1, 0);
        lv_obj_set_style_border_color(
            tile, lv_color_hex(selected ? C_GOLD : C_MUTED), 0);
        if (s.picked && s.picked_pos == i) {
            lv_obj_set_style_bg_color(tile, lv_color_hex(C_PAPER_DARK), 0);
            lv_obj_set_style_border_color(tile, lv_color_hex(C_SEAL), 0);
        }
        make_center(tile, 0, 8, tile_w, s.challenge.tiles[s.order[i]],
                    &app_font_20, C_INK);
    }

    const int check_y = 30 + rows * 44 + 2;
    lv_obj_t *check = make_panel(panel, 72, check_y, 80, 26, C_PAPER_DARK,
                                 LV_OPA_COVER, 10);
    lv_obj_set_style_border_width(check, s.cursor == s.challenge.tile_count ? 2 : 1, 0);
    lv_obj_set_style_border_color(
        check, lv_color_hex(s.cursor == s.challenge.tile_count ? C_GOLD
                                                              : C_MUTED),
        0);
    make_center(check, 0, 4, 80, "检查", &app_font_14, C_SEAL);
    add_hint_or_footer(panel, s.picked ? "选另一个字块交换位置"
                                       : "确定拿起字块 · 移到检查");
    return panel;
}

static lv_obj_t *build_next_line_panel(lv_obj_t *scr)
{
    lv_obj_t *panel = make_panel(scr, 8, 108, 224, 190, C_PAPER,
                                 LV_OPA_90, 14);
    make_center(panel, 0, 6, 224, s.challenge.prompt, &app_font_14, C_INK);
    lv_obj_t *line = make_panel(panel, 12, 30, 200, 38, C_PAPER_DARK,
                                LV_OPA_COVER, 8);
    make_center(line, 0, 8, 200, s.challenge.line, &app_font_20, C_INK);
    for (uint8_t i = 0; i < s.challenge.option_count; i++) {
        lv_obj_t *card = make_panel(panel, 12, 70 + i * 28, 200, 24,
                                    C_WHITE, LV_OPA_COVER, 8);
        style_card(card, s.cursor == i, true);
        make_center(card, 0, 4, 200, s.challenge.options[i], &app_font_14,
                    C_INK);
    }
    add_hint_or_footer(panel, "上下选择 · 确定作答");
    return panel;
}

static lv_obj_t *build_picture_panel(lv_obj_t *scr)
{
    lv_obj_t *badge = make_panel(scr, 30, 40, 180, 26, C_PAPER,
                                 LV_OPA_90, 10);
    make_center(badge, 0, 4, 180, s.challenge.prompt, &app_font_14, C_SEAL);
    for (uint8_t i = 0; i < s.challenge.option_count; i++) {
        lv_obj_t *card = make_panel(scr, 20, 150 + i * 46, 200, 38,
                                    C_PAPER, LV_OPA_90, 10);
        style_card(card, s.cursor == i, true);
        make_center(card, 0, 8, 200, s.challenge.options[i], &app_font_20,
                    C_INK);
    }
    if (s.hint) {
        lv_obj_t *box = make_panel(scr, 20, 104, 200, 34, C_PAPER_DARK,
                                   LV_OPA_90, 10);
        (void)box;
        make_wrap_label(scr, 26, 108, 188, s.challenge.hint, &app_font_12,
                        C_SEAL, 2);
    } else {
        lv_obj_t *footer = make_panel(scr, 50, 288, 140, 20, C_PAPER,
                                      LV_OPA_90, 8);
        (void)footer;
        make_center(scr, 50, 292, 140, "上下选择 · 确定作答",
                    &app_font_12, C_MUTED);
    }
    return NULL;
}

static lv_obj_t *build_blank_panel(lv_obj_t *scr)
{
    lv_obj_t *panel = make_panel(scr, 8, 108, 224, 190, C_PAPER,
                                 LV_OPA_90, 14);
    make_center(panel, 0, 6, 224, s.challenge.prompt, &app_font_14, C_INK);
    lv_obj_t *line = make_panel(panel, 10, 30, 204, 54, C_PAPER_DARK,
                                LV_OPA_COVER, 10);
    make_center(line, 0, 14, 204, s.challenge.line, &app_font_20, C_INK);
    for (uint8_t i = 0; i < s.challenge.option_count; i++) {
        lv_obj_t *card = make_panel(panel, 26 + i * 62, 94, 56, 54,
                                    C_WHITE, LV_OPA_COVER, 12);
        style_card(card, s.cursor == i, true);
        make_center(card, 0, 10, 56, s.challenge.options[i], &app_font_28,
                    C_INK);
    }
    add_hint_or_footer(panel, "选一个字补全诗句");
    return panel;
}

static lv_obj_t *build_challenge(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, s.adv.stage);
    build_challenge_header(scr);
    add_chibi(scr, 170, 26, POETRY_ULTRAMAN_HINT, 190);

    switch (s.challenge.kind) {
    case CHALLENGE_ORDER:
        (void)build_order_panel(scr);
        break;
    case CHALLENGE_NEXT_LINE:
        (void)build_next_line_panel(scr);
        break;
    case CHALLENGE_PICTURE:
        (void)build_picture_panel(scr);
        break;
    case CHALLENGE_BLANK:
        (void)build_blank_panel(scr);
        break;
    default:
        break;
    }
    return scr;
}

// ---------------------------------------------------------------------------
// 奖励: 展示整首诗
// ---------------------------------------------------------------------------
static lv_obj_t *build_reward(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, s.adv.stage);
    add_chibi(scr, 10, 4, POETRY_ULTRAMAN_CHEER, 200);
    add_battery(scr);
    make_panel(scr, 6, 76, 228, 238, C_PAPER, LV_OPA_90, 16);

    const poem_t *poem = poem_by_id(s.adv.poem_id);
    make_center(scr, 0, 82, SCREEN_W, "诗篇归位 · 已收入光之诗库",
                &app_font_12, C_SEAL);
    make_center(scr, 0, 100, SCREEN_W, poem->title, &app_font_28, C_INK);
    char author[40];
    snprintf(author, sizeof(author), "%s · %s", poem->dynasty, poem->author);
    make_center(scr, 0, 134, SCREEN_W, author, &app_font_12, C_MUTED);
    for (uint8_t i = 0; i < poem->line_count && i < 4; i++) {
        make_center(scr, 0, 154 + i * 24, SCREEN_W, poem->lines[i],
                    &app_font_20, C_INK);
    }
    make_wrap_label(scr, 16, 248, 208,
                    adventure_poem_explanation(s.adv.poem_id), &app_font_12,
                    C_INK_LIGHT, 4);
    make_center(scr, 0, 300, SCREEN_W, "确定 继续", &app_font_12, C_JADE);
    return scr;
}

// ---------------------------------------------------------------------------
// 图鉴
// ---------------------------------------------------------------------------
static lv_obj_t *build_codex(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    make_label(scr, 14, 10, 160, "光之诗库", &app_font_20, C_INK);
    char count[24];
    snprintf(count, sizeof(count), "%u / %u", collection_count(&s.collection),
             (unsigned)ADVENTURE_STAGE_COUNT);
    make_right_label(scr, 150, 22, 72, count, &app_font_12, C_JADE);
    add_battery(scr);

    for (uint8_t i = 0; i < ADVENTURE_STAGE_COUNT; i++) {
        const uint16_t poem_id = adventure_stage_poem_id(i);
        const bool collected = collection_has(&s.collection, poem_id);
        lv_obj_t *card = make_panel(scr, 14, 44 + i * 80, 212, 72,
                                    C_PAPER, LV_OPA_COVER, 12);
        style_card(card, s.codex_index == i, collected);
        if (!collected) {
            make_label(card, 16, 10, 180, "？？？", &app_font_20, C_MUTED);
            make_label(card, 16, 40, 180, "继续冒险即可解锁", &app_font_12,
                       C_MUTED);
            continue;
        }
        const poem_t *poem = poem_by_id(poem_id);
        make_label(card, 16, 8, 180, poem->title, &app_font_20, C_INK);
        char author[40];
        snprintf(author, sizeof(author), "%s · %s", poem->dynasty,
                 poem->author);
        make_label(card, 16, 38, 130, author, &app_font_12, C_MUTED);
        make_right_label(card, 130, 38, 66, adventure_stage_scene(i),
                         &app_font_12, C_JADE);
    }
    add_footer(scr, "确定阅读 · 长按返回", C_MUTED);
    return scr;
}

static lv_obj_t *build_codex_detail(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, s.codex_index);
    make_panel(scr, 6, 6, 228, 308, C_PAPER, LV_OPA_90, 16);
    const uint16_t poem_id = adventure_stage_poem_id(s.codex_index);
    const poem_t *poem = poem_by_id(poem_id);
    make_center(scr, 0, 16, SCREEN_W, poem->title, &app_font_28, C_SEAL);
    char author[40];
    snprintf(author, sizeof(author), "%s · %s", poem->dynasty, poem->author);
    make_center(scr, 0, 52, SCREEN_W, author, &app_font_12, C_MUTED);
    for (uint8_t i = 0; i < poem->line_count && i < 4; i++) {
        make_center(scr, 0, 78 + i * 26, SCREEN_W, poem->lines[i],
                    &app_font_20, C_INK);
    }
    make_wrap_label(scr, 16, 190, 208,
                    adventure_poem_explanation(poem_id), &app_font_12,
                    C_INK_LIGHT, 4);
    make_center(scr, 0, 292, 150, "确定 重听朗读", &app_font_12, C_JADE);
    make_center(scr, 150, 292, 90, "长按返回", &app_font_12, C_MUTED);
    return scr;
}

// ---------------------------------------------------------------------------
// 奥特曼小故事
// ---------------------------------------------------------------------------
static lv_obj_t *build_story(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    add_scene(scr, s.story_index % ADVENTURE_STAGE_COUNT);
    make_panel(scr, 8, 10, 224, 300, C_PAPER, LV_OPA_90, 16);
    make_center(scr, 0, 20, SCREEN_W, "奥特曼小故事", &app_font_12, C_JADE);
    make_center(scr, 0, 42, SCREEN_W, adventure_story_title(s.story_index),
                &app_font_28, C_SEAL);
    make_wrap_label(scr, 22, 92, 196, adventure_story_text(s.story_index),
                    &app_font_14, C_INK, 8);
    add_chibi(scr, 88, 212, POETRY_ULTRAMAN_CHEER, 200);
    add_footer(scr, "确定 继续", C_MUTED);
    return scr;
}

// ---------------------------------------------------------------------------
// 速答挑战
// ---------------------------------------------------------------------------
static void make_speed_question(void)
{
    poem_question_generate(&s.speed_q, 1, 2, false, false, &s.rng, NULL, 0);
    s.speed_cursor = 0;
    s.speed_answered = false;
    s.speed_correct = false;
    s.speed_deadline_us = esp_timer_get_time() + SPEED_SECONDS * 1000000LL;
    s.speed_next_us = 0;
}

static lv_obj_t *build_speed(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    make_label(scr, 14, 10, 140, "速答挑战", &app_font_20, C_INK);
    char score[24];
    snprintf(score, sizeof(score), "得分 %u", (unsigned)s.speed_score);
    make_right_label(scr, 150, 16, 76, score, &app_font_12, C_SEAL);
    add_battery(scr);

    char progress[24];
    snprintf(progress, sizeof(progress), "第 %u / %u 题",
             (unsigned)(s.speed_index + 1), (unsigned)SPEED_QUESTIONS);
    make_center(scr, 0, 40, SCREEN_W, progress, &app_font_12, C_MUTED);

    make_panel(scr, 16, 62, 208, 8, C_PAPER_DARK, LV_OPA_COVER, 4);
    s.speed_bar = make_panel(scr, 16, 62, 208, 8, C_GOLD, LV_OPA_COVER, 4);
    s.speed_clock = make_center(scr, 0, 74, SCREEN_W, "", &app_font_12,
                                C_MUTED);

    make_wrap_label(scr, 16, 96, 208, s.speed_q.text, &app_font_20, C_INK, 6);

    for (uint8_t i = 0; i < POEM_OPTION_COUNT; i++) {
        lv_obj_t *card = make_panel(scr, 16, 160 + i * 44, 208, 38,
                                    C_PAPER, LV_OPA_COVER, 10);
        bool selected = s.speed_cursor == i;
        lv_obj_set_style_border_width(card, selected ? 2 : 1, 0);
        lv_obj_set_style_border_color(
            card, lv_color_hex(selected ? C_GOLD : C_MUTED), 0);
        uint32_t color = C_INK;
        if (s.speed_answered) {
            if (i == s.speed_q.correct_index) {
                lv_obj_set_style_bg_color(card, lv_color_hex(0xDCEAD9), 0);
                color = C_GREEN;
            } else if (selected && !s.speed_correct) {
                lv_obj_set_style_bg_color(card, lv_color_hex(0xF5DCD8), 0);
                color = C_SEAL;
            }
        }
        make_center(card, 0, 7, 208, s.speed_q.options[i], &app_font_14,
                    color);
    }

    if (s.speed_answered) {
        make_center(scr, 0, 292, SCREEN_W,
                    s.speed_correct ? "答对啦，继续下一题" : "没关系，记住这句诗",
                    &app_font_12, s.speed_correct ? C_GREEN : C_SEAL);
    } else {
        add_footer(scr, "上下选择 · 确定作答", C_MUTED);
    }
    return scr;
}

static lv_obj_t *build_speed_result(void)
{
    lv_obj_t *scr = make_screen(C_PAPER);
    make_center(scr, 0, 70, SCREEN_W, "速答完成", &app_font_28, C_INK);
    char score[32];
    snprintf(score, sizeof(score), "%u / %u", (unsigned)s.speed_score,
             (unsigned)SPEED_QUESTIONS);
    make_center(scr, 0, 120, SCREEN_W, score, &app_font_28, C_SEAL);
    const char *message = "再来一次，你会更快！";
    if (s.speed_score >= 5) {
        message = "全部答对，光之勇士真厉害！";
    } else if (s.speed_score >= 3) {
        message = "读得又快又准，继续保持！";
    }
    make_center(scr, 0, 180, SCREEN_W, message, &app_font_14, C_JADE);
    add_chibi(scr, 88, 208, POETRY_ULTRAMAN_CHEER, 200);
    add_footer(scr, "确定 返回首页", C_MUTED);
    return scr;
}

// ---------------------------------------------------------------------------
// 页面切换与渲染
// ---------------------------------------------------------------------------
static lv_obj_t *build_current(void)
{
    switch (s.page) {
    case PAGE_HOME:
        return build_home();
    case PAGE_MAP:
        return build_map();
    case PAGE_INTRO:
        return build_intro();
    case PAGE_CHALLENGE:
        return build_challenge();
    case PAGE_REWARD:
        return build_reward();
    case PAGE_CODEX:
        return build_codex();
    case PAGE_CODEX_DETAIL:
        return build_codex_detail();
    case PAGE_STORY:
        return build_story();
    case PAGE_SPEED:
        return build_speed();
    case PAGE_SPEED_RESULT:
        return build_speed_result();
    default:
        return build_home();
    }
}

static void render(void)
{
    lv_obj_t *old = s.root;
    s.root = NULL;
    s.battery = NULL;
    s.speed_bar = NULL;
    s.speed_clock = NULL;
    s.root = build_current();
    lv_screen_load(s.root);
    if (old != NULL) {
        lv_obj_delete(old);
    }
}

static void set_page(page_t page)
{
    const page_t old = s.page;
    s.page = page;
    const bool narration_page =
        page == PAGE_REWARD || page == PAGE_CODEX_DETAIL;
    if ((old == PAGE_REWARD || old == PAGE_CODEX_DETAIL) && !narration_page) {
        audio_player_play_voice(NULL);
    }
    // Keep the bed playing on narration pages; the mixer ducks it while a
    // narration clip is active so the poem stays intelligible.
    ensure_bgm(true);

    if (page == PAGE_REWARD && old != PAGE_REWARD) {
        const bool added = collection_add(&s.collection, s.adv.poem_id);
        const uint8_t unlocked = collection_unlock_stories(&s.collection);
        s.story_bits |= unlocked;
        if (added || unlocked != 0) {
            nvs_save();
        }
        cue(POETRY_SFX_UNLOCK);
        audio_player_play_voice(poetry_audio_narration(s.adv.poem_id));
    } else if (page == PAGE_CODEX_DETAIL && old != PAGE_CODEX_DETAIL) {
        const uint16_t id = adventure_stage_poem_id(s.codex_index);
        audio_player_play_voice(poetry_audio_narration(id));
    }
    render();
}

// ---------------------------------------------------------------------------
// 挑战流程
// ---------------------------------------------------------------------------
static void begin_challenge(void)
{
    const poem_t *poem = poem_by_id(s.adv.poem_id);
    const uint8_t kind = adventure_challenge_kind(s.adv.challenge_index);
    poem_challenge_make(&s.challenge, poem, kind, &s.rng);
    s.cursor = 0;
    s.picked = false;
    s.hint = false;
    s.order_len = s.challenge.tile_count;
    for (uint8_t i = 0; i < s.order_len; i++) {
        s.order[i] = i;
    }
}

static void next_story_or_map(void)
{
    if (s.story_bits != 0) {
        for (uint8_t i = 0; i < ADVENTURE_STORY_COUNT; i++) {
            const uint8_t bit = (uint8_t)(1u << i);
            if ((s.story_bits & bit) != 0) {
                s.story_bits &= (uint8_t)~bit;
                s.story_index = i;
                set_page(PAGE_STORY);
                return;
            }
        }
    }
    set_page(PAGE_MAP);
}

static void challenge_correct(void)
{
    cue(POETRY_SFX_CORRECT);
    adventure_correct(&s.adv);
    if (s.adv.step == ADVENTURE_STEP_REWARD) {
        set_page(PAGE_REWARD);
        return;
    }
    begin_challenge();
    render();
}

static void challenge_wrong(void)
{
    cue(POETRY_SFX_WRONG);
    if (adventure_wrong(&s.adv)) {
        s.hint = true;
    }
    render();
}

static void order_press_ok(void)
{
    if (s.cursor == s.challenge.tile_count) {
        s.picked = false;
        if (poem_challenge_order_correct(&s.challenge, s.order,
                                         s.order_len)) {
            challenge_correct();
        } else {
            challenge_wrong();
        }
        return;
    }
    if (!s.picked) {
        s.picked = true;
        s.picked_pos = s.cursor;
        cue(POETRY_SFX_MOVE);
    } else {
        const uint8_t tmp = s.order[s.picked_pos];
        s.order[s.picked_pos] = s.order[s.cursor];
        s.order[s.cursor] = tmp;
        s.picked = false;
        cue(POETRY_SFX_CONFIRM);
    }
    render();
}

// ---------------------------------------------------------------------------
// 按键处理
// ---------------------------------------------------------------------------
static void move_cursor(int delta, int count)
{
    if (count <= 1) {
        return;
    }
    int next = (int)s.cursor + delta;
    if (next < 0) {
        next = count - 1;
    } else if (next >= count) {
        next = 0;
    }
    s.cursor = (uint8_t)next;
    cue(POETRY_SFX_MOVE);
    render();
}

static void handle_home(bsp_btn_ev_t event)
{
    if (event == BSP_BTN_LONG) {
        return;
    }
    if (event != BSP_BTN_CLICK) {
        return;
    }
    switch (s.home_index) {
    case 0:
        cue(POETRY_SFX_CONFIRM);
        set_page(PAGE_MAP);
        break;
    case 1:
        s.codex_index = 0;
        cue(POETRY_SFX_CONFIRM);
        set_page(PAGE_CODEX);
        break;
    case 2:
        cue(POETRY_SFX_CONFIRM);
        s.speed_index = 0;
        s.speed_score = 0;
        make_speed_question();
        set_page(PAGE_SPEED);
        break;
    case 3:
    default:
        s.muted = !s.muted;
        audio_player_set_muted(s.muted);
        nvs_save();
        cue(POETRY_SFX_CONFIRM);
        render();
        break;
    }
}

static void handle_key(bsp_btn_t btn, bsp_btn_ev_t event)
{
    if (event != BSP_BTN_CLICK && event != BSP_BTN_LONG) {
        return;
    }
    const bool is_long = event == BSP_BTN_LONG;

    switch (s.page) {
    case PAGE_HOME:
        if (btn == BSP_BTN_UP) {
            s.home_index = (uint8_t)((s.home_index + HOME_ITEMS - 1) % HOME_ITEMS);
            cue(POETRY_SFX_MOVE);
            render();
        } else if (btn == BSP_BTN_DOWN) {
            s.home_index = (uint8_t)((s.home_index + 1) % HOME_ITEMS);
            cue(POETRY_SFX_MOVE);
            render();
        } else if (btn == BSP_BTN_OK && !is_long) {
            handle_home(event);
        }
        break;

    case PAGE_MAP:
        if (btn == BSP_BTN_UP) {
            s.map_index = (uint8_t)((s.map_index + ADVENTURE_STAGE_COUNT - 1) %
                                    ADVENTURE_STAGE_COUNT);
            cue(POETRY_SFX_MOVE);
            render();
        } else if (btn == BSP_BTN_DOWN) {
            s.map_index = (uint8_t)((s.map_index + 1) % ADVENTURE_STAGE_COUNT);
            cue(POETRY_SFX_MOVE);
            render();
        } else if (btn == BSP_BTN_OK) {
            if (is_long) {
                set_page(PAGE_HOME);
            } else {
                cue(POETRY_SFX_CONFIRM);
                adventure_begin(&s.adv, s.map_index);
                set_page(PAGE_INTRO);
            }
        }
        break;

    case PAGE_INTRO:
        if (btn == BSP_BTN_OK) {
            if (is_long) {
                set_page(PAGE_MAP);
            } else {
                cue(POETRY_SFX_CONFIRM);
                adventure_start_challenges(&s.adv);
                begin_challenge();
                set_page(PAGE_CHALLENGE);
            }
        }
        break;

    case PAGE_CHALLENGE:
        if (btn == BSP_BTN_OK && is_long) {
            set_page(PAGE_MAP);
            break;
        }
        if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
            const int delta = btn == BSP_BTN_UP ? -1 : 1;
            const int count = s.challenge.kind == CHALLENGE_ORDER
                                  ? s.challenge.tile_count + 1
                                  : s.challenge.option_count;
            move_cursor(delta, count);
        } else if (btn == BSP_BTN_OK) {
            if (s.challenge.kind == CHALLENGE_ORDER) {
                order_press_ok();
            } else if (poem_challenge_option_correct(&s.challenge, s.cursor)) {
                challenge_correct();
            } else {
                challenge_wrong();
            }
        }
        break;

    case PAGE_REWARD:
        if (btn == BSP_BTN_OK) {
            if (is_long) {
                set_page(PAGE_MAP);
            } else {
                next_story_or_map();
            }
        }
        break;

    case PAGE_CODEX:
        if (btn == BSP_BTN_OK && is_long) {
            set_page(PAGE_HOME);
        } else if (btn == BSP_BTN_OK) {
            if (collection_has(&s.collection,
                               adventure_stage_poem_id(s.codex_index))) {
                cue(POETRY_SFX_CONFIRM);
                set_page(PAGE_CODEX_DETAIL);
            } else {
                cue(POETRY_SFX_WRONG);
            }
        } else if (btn == BSP_BTN_UP) {
            s.codex_index = (uint8_t)((s.codex_index + ADVENTURE_STAGE_COUNT - 1) %
                                      ADVENTURE_STAGE_COUNT);
            cue(POETRY_SFX_MOVE);
            render();
        } else if (btn == BSP_BTN_DOWN) {
            s.codex_index = (uint8_t)((s.codex_index + 1) %
                                      ADVENTURE_STAGE_COUNT);
            cue(POETRY_SFX_MOVE);
            render();
        }
        break;

    case PAGE_CODEX_DETAIL:
        if (btn == BSP_BTN_OK) {
            if (is_long) {
                set_page(PAGE_CODEX);
            } else {
                audio_player_play_voice(poetry_audio_narration(
                    adventure_stage_poem_id(s.codex_index)));
            }
        }
        break;

    case PAGE_STORY:
        if (btn == BSP_BTN_OK) {
            if (is_long) {
                set_page(PAGE_MAP);
            } else {
                cue(POETRY_SFX_CONFIRM);
                next_story_or_map();
            }
        }
        break;

    case PAGE_SPEED:
        if (btn == BSP_BTN_OK && is_long) {
            set_page(PAGE_HOME);
            break;
        }
        if (s.speed_answered) {
            break;
        }
        if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
            const int delta = btn == BSP_BTN_UP ? -1 : 1;
            move_cursor(delta, POEM_OPTION_COUNT);
        } else if (btn == BSP_BTN_OK) {
            s.speed_answered = true;
            s.speed_correct =
                s.speed_cursor == s.speed_q.correct_index;
            if (s.speed_correct) {
                s.speed_score++;
                cue(POETRY_SFX_CORRECT);
            } else {
                cue(POETRY_SFX_WRONG);
            }
            s.speed_next_us = esp_timer_get_time() + 900000LL;
            render();
        }
        break;

    case PAGE_SPEED_RESULT:
        if (btn == BSP_BTN_OK) {
            set_page(PAGE_HOME);
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// 应用任务
// ---------------------------------------------------------------------------
static void update_battery(void)
{
    if (s.battery == NULL) {
        return;
    }
    char text[12];
    if (s.battery_soc < 0) {
        snprintf(text, sizeof(text), "--");
    } else {
        snprintf(text, sizeof(text), "%d%%", s.battery_soc);
    }
    lv_label_set_text(s.battery, text);
}

static void tick_speed(int64_t now_us)
{
    if (s.page != PAGE_SPEED) {
        return;
    }
    if (!s.speed_answered) {
        const int64_t remain = s.speed_deadline_us - now_us;
        int percent = (int)(remain * 100 / (SPEED_SECONDS * 1000000LL));
        if (percent < 0) {
            percent = 0;
        }
        if (percent > 100) {
            percent = 100;
        }
        if (s.speed_bar != NULL) {
            lv_obj_set_width(s.speed_bar, 208 * percent / 100);
        }
        if (s.speed_clock != NULL) {
            char text[24];
            int seconds = (int)((remain + 999999) / 1000000);
            if (seconds < 0) {
                seconds = 0;
            }
            if (seconds > SPEED_SECONDS) {
                seconds = SPEED_SECONDS;
            }
            snprintf(text, sizeof(text), "剩余 %d 秒", seconds);
            lv_label_set_text(s.speed_clock, text);
        }
        if (remain <= 0) {
            s.speed_answered = true;
            s.speed_correct = false;
            s.speed_next_us = now_us + 1100000LL;
            cue(POETRY_SFX_WRONG);
            render();
        }
    } else if (s.speed_next_us > 0 && now_us >= s.speed_next_us) {
        s.speed_next_us = 0;
        if (s.speed_index + 1 >= SPEED_QUESTIONS) {
            set_page(PAGE_SPEED_RESULT);
        } else {
            s.speed_index++;
            make_speed_question();
            render();
        }
    }
}

static void app_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    int battery_ticks = 0;
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(TICK_MS));
        int soc = -1;
        if (++battery_ticks >= 100) {
            battery_ticks = 0;
            soc = bsp_battery_soc();
        }
        if (!bsp_lvgl_lock(100)) {
            continue;
        }
        if (soc >= 0) {
            s.battery_soc = soc;
            update_battery();
        }
        tick_speed(esp_timer_get_time());
        bsp_lvgl_unlock();
    }
}

void adventure_app_handle_key(bsp_btn_t btn, bsp_btn_ev_t event)
{
    if (!bsp_lvgl_lock(200)) {
        return;
    }
    handle_key(btn, event);
    bsp_lvgl_unlock();
}

esp_err_t adventure_app_start(void)
{
    memset(&s, 0, sizeof(s));
    s.battery_soc = -1;
    s.page = PAGE_HOME;
    s.home_index = 0;
    s.map_index = 0;
    s.bgm_on = false;
    poem_rng_seed(&s.rng, (uint32_t)esp_timer_get_time() ^ 0x51A7E5u);
    nvs_load();
    audio_player_set_muted(s.muted);
    if (audio_player_start() != ESP_OK) {
        ESP_LOGW(TAG, "音频任务启动失败,游戏将静音运行");
    }

    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "获取 LVGL 锁失败");
        return ESP_ERR_TIMEOUT;
    }
    ensure_bgm(true);
    render();
    bsp_lvgl_unlock();

    if (xTaskCreate(app_task, "adventure_app", 4096, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "应用任务创建失败");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "奥特曼诗词收集冒险就绪: 已收集 %u 首",
             (unsigned)collection_count(&s.collection));
    return ESP_OK;
}
