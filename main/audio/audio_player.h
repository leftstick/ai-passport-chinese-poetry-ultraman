// 16 kHz mono PCM mixer: one narration voice, one cue voice, one looping BGM.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    const int16_t *samples;
    uint32_t count;
} audio_clip_t;

esp_err_t audio_player_start(void);

// Short interaction cue. Replaces the previous cue.
void audio_player_play(const audio_clip_t *clip);

// Poem narration. Replaces the previous narration and ducks the BGM.
void audio_player_play_voice(const audio_clip_t *clip);

// Looping background music. Pass NULL to stop it.
void audio_player_set_bgm(const audio_clip_t *clip);
void audio_player_stop_bgm(void);

void audio_player_set_muted(bool muted);
bool audio_player_is_muted(void);
