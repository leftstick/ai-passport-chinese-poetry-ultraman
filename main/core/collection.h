// Collected-poem codex and Ultraman story unlocks.
// Pure C; serialize() produces the NVS blob used by the application.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COLLECTION_MAX_POEMS 96
#define COLLECTION_STORY_COUNT 3
#define COLLECTION_BLOB_SIZE 21

typedef struct {
    uint32_t mask[3];
    uint8_t stories;  // bit i set when story i is unlocked
    uint16_t count;
} collection_t;

void collection_init(collection_t *collection);
bool collection_has(const collection_t *collection, uint16_t poem_id);
// Returns true when this poem is newly collected.
bool collection_add(collection_t *collection, uint16_t poem_id);
uint16_t collection_count(const collection_t *collection);
bool collection_story_unlocked(const collection_t *collection, uint8_t index);
// Unlocks every story whose threshold is reached. Returns the bitmask of
// stories unlocked by this call (0 when nothing changed).
uint8_t collection_unlock_stories(collection_t *collection);
uint8_t collection_story_threshold(uint8_t index);

size_t collection_serialize(const collection_t *collection, uint8_t *out,
                            size_t size);
bool collection_deserialize(collection_t *collection, const uint8_t *blob,
                            size_t size);
