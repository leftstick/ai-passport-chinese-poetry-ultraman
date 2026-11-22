#include "audio_player.h"

#include <string.h>

#include "bsp_audio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "audio";

#define AUDIO_RATE 16000
#define AUDIO_BLOCK_SAMPLES 160  // 10 ms
#define AUDIO_QUEUE_DEPTH 8
#define AUDIO_VOLUME 70

#define AUDIO_VOICE_GAIN 100
#define AUDIO_CUE_GAIN 88
#define AUDIO_BGM_GAIN 34
#define AUDIO_BGM_DUCK_GAIN 14

typedef enum {
    VOICE_NARRATION = 0,
    VOICE_CUE,
    VOICE_BGM,
    VOICE_COUNT,
} voice_slot_t;

typedef enum {
    CMD_PLAY,
    CMD_VOICE,
    CMD_BGM,
} cmd_kind_t;

typedef struct {
    cmd_kind_t kind;
    const audio_clip_t *clip;
} audio_cmd_t;

typedef struct {
    const audio_clip_t *clip;
    uint32_t position;
    bool active;
} voice_t;

static QueueHandle_t s_queue;
static TaskHandle_t s_task;
static volatile bool s_ready;
static volatile bool s_muted;
static voice_t s_voices[VOICE_COUNT];

static bool narration_active(void)
{
    return s_voices[VOICE_NARRATION].active;
}

static void start_voice(voice_slot_t slot, const audio_clip_t *clip)
{
    if (clip == NULL || clip->samples == NULL || clip->count == 0) {
        s_voices[slot].active = false;
        s_voices[slot].clip = NULL;
        s_voices[slot].position = 0;
        return;
    }
    s_voices[slot].clip = clip;
    s_voices[slot].position = 0;
    s_voices[slot].active = true;
}

static void mix_block(int16_t *out)
{
    int32_t mix[AUDIO_BLOCK_SAMPLES];
    memset(mix, 0, sizeof(mix));
    const int bgm_gain =
        narration_active() ? AUDIO_BGM_DUCK_GAIN : AUDIO_BGM_GAIN;
    bool any = false;

    for (int v = 0; v < VOICE_COUNT; v++) {
        voice_t *voice = &s_voices[v];
        if (!voice->active || voice->clip == NULL) {
            continue;
        }
        any = true;
        int gain = AUDIO_CUE_GAIN;
        if (v == VOICE_NARRATION) {
            gain = AUDIO_VOICE_GAIN;
        } else if (v == VOICE_BGM) {
            gain = bgm_gain;
        }
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            if (!voice->active) {
                break;
            }
            mix[i] += (int32_t)voice->clip->samples[voice->position] * gain / 100;
            voice->position++;
            if (voice->position >= voice->clip->count) {
                if (v == VOICE_BGM) {
                    voice->position = 0;
                } else {
                    voice->active = false;
                    voice->clip = NULL;
                }
            }
        }
    }

    if (!any) {
        return;
    }

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        int32_t sample = mix[i];
        if (sample > 32767) {
            sample = 32767;
        } else if (sample < -32768) {
            sample = -32768;
        }
        out[i] = (int16_t)sample;
    }
    (void)bsp_audio_write(out, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
}

static void handle_command(const audio_cmd_t *cmd)
{
    switch (cmd->kind) {
    case CMD_VOICE:
        start_voice(VOICE_NARRATION, cmd->clip);
        break;
    case CMD_BGM:
        start_voice(VOICE_BGM, cmd->clip);
        break;
    case CMD_PLAY:
    default:
        if (!s_muted) {
            start_voice(VOICE_CUE, cmd->clip);
        }
        break;
    }
}

static void audio_task(void *arg)
{
    (void)arg;
    const esp_err_t init_err = bsp_audio_init();
    esp_err_t format_err = ESP_FAIL;
    if (init_err == ESP_OK) {
        format_err = bsp_audio_set_format(AUDIO_RATE, 16, 1);
        if (format_err == ESP_OK) {
            bsp_audio_set_volume(AUDIO_VOLUME);
            s_ready = true;
            ESP_LOGI(TAG, "音频就绪: %d Hz mono", AUDIO_RATE);
        }
    }
    if (!s_ready) {
        ESP_LOGW(TAG, "音频不可用,游戏将静音运行 (init=%s format=%s)",
                 esp_err_to_name(init_err), esp_err_to_name(format_err));
    }

    int16_t block[AUDIO_BLOCK_SAMPLES];
    for (;;) {
        audio_cmd_t cmd;
        while (xQueueReceive(s_queue, &cmd, 0) == pdTRUE) {
            handle_command(&cmd);
        }
        if (!s_ready || s_muted) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        bool active = false;
        for (int v = 0; v < VOICE_COUNT; v++) {
            active = active || s_voices[v].active;
        }
        if (active) {
            mix_block(block);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

esp_err_t audio_player_start(void)
{
    if (s_task != NULL) {
        return ESP_OK;
    }
    s_queue = xQueueCreate(AUDIO_QUEUE_DEPTH, sizeof(audio_cmd_t));
    if (s_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(audio_task, "poetry_audio", 4096, NULL, 4, &s_task) !=
        pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        s_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void send(cmd_kind_t kind, const audio_clip_t *clip)
{
    if (s_queue == NULL) {
        return;
    }
    const audio_cmd_t cmd = { .kind = kind, .clip = clip };
    (void)xQueueSend(s_queue, &cmd, 0);
}

void audio_player_play(const audio_clip_t *clip)
{
    if (clip == NULL || s_muted) {
        return;
    }
    send(CMD_PLAY, clip);
}

void audio_player_play_voice(const audio_clip_t *clip)
{
    send(CMD_VOICE, clip);
}

void audio_player_set_bgm(const audio_clip_t *clip)
{
    send(CMD_BGM, clip);
}

void audio_player_stop_bgm(void)
{
    send(CMD_BGM, NULL);
}

void audio_player_set_muted(bool muted)
{
    s_muted = muted;
}

bool audio_player_is_muted(void)
{
    return s_muted;
}
