#include "collection.h"

#include <string.h>

#define COLLECTION_MAGIC 0x504D4131u  // "PMA1"
#define COLLECTION_VERSION 1u
#define COLLECTION_THRESHOLDS_INIT \
    { 1, 2, 3 }

void collection_init(collection_t *collection)
{
    if (collection == NULL) {
        return;
    }
    memset(collection, 0, sizeof(*collection));
}

uint8_t collection_story_threshold(uint8_t index)
{
    static const uint8_t thresholds[COLLECTION_STORY_COUNT] =
        COLLECTION_THRESHOLDS_INIT;
    return index < COLLECTION_STORY_COUNT ? thresholds[index] : 0;
}

static uint16_t popcount_masks(const uint32_t mask[3])
{
    uint16_t count = 0;
    for (int word = 0; word < 3; word++) {
        uint32_t bits = mask[word];
        while (bits != 0) {
            count += (uint16_t)(bits & 1u);
            bits >>= 1;
        }
    }
    return count;
}

bool collection_has(const collection_t *collection, uint16_t poem_id)
{
    if (collection == NULL || poem_id >= COLLECTION_MAX_POEMS) {
        return false;
    }
    return (collection->mask[poem_id / 32] & (1u << (poem_id % 32))) != 0;
}

bool collection_add(collection_t *collection, uint16_t poem_id)
{
    if (collection == NULL || poem_id >= COLLECTION_MAX_POEMS) {
        return false;
    }
    const uint32_t bit = 1u << (poem_id % 32);
    if ((collection->mask[poem_id / 32] & bit) != 0) {
        return false;
    }
    collection->mask[poem_id / 32] |= bit;
    collection->count = popcount_masks(collection->mask);
    return true;
}

uint16_t collection_count(const collection_t *collection)
{
    return collection != NULL ? collection->count : 0;
}

bool collection_story_unlocked(const collection_t *collection, uint8_t index)
{
    return collection != NULL && index < COLLECTION_STORY_COUNT &&
           (collection->stories & (1u << index)) != 0;
}

uint8_t collection_unlock_stories(collection_t *collection)
{
    if (collection == NULL) {
        return 0;
    }
    uint8_t unlocked = 0;
    for (uint8_t i = 0; i < COLLECTION_STORY_COUNT; i++) {
        const uint8_t threshold = collection_story_threshold(i);
        if (threshold == 0 || collection->count < threshold) {
            continue;
        }
        const uint8_t bit = (uint8_t)(1u << i);
        if ((collection->stories & bit) == 0) {
            collection->stories |= bit;
            unlocked |= bit;
        }
    }
    return unlocked;
}

static void put_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)(value >> 8);
}

static uint16_t get_u16(const uint8_t *in)
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static void put_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8) & 0xFFu);
    out[2] = (uint8_t)((value >> 16) & 0xFFu);
    out[3] = (uint8_t)((value >> 24) & 0xFFu);
}

static uint32_t get_u32(const uint8_t *in)
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static uint8_t checksum(const uint8_t *blob, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (uint8_t)(sum + blob[i]);
    }
    return sum;
}

size_t collection_serialize(const collection_t *collection, uint8_t *out,
                            size_t size)
{
    if (collection == NULL || out == NULL || size < COLLECTION_BLOB_SIZE) {
        return 0;
    }
    memset(out, 0, COLLECTION_BLOB_SIZE);
    put_u32(&out[0], COLLECTION_MAGIC);
    out[4] = COLLECTION_VERSION;
    out[5] = collection->stories;
    put_u16(&out[6], collection->count);
    put_u32(&out[8], collection->mask[0]);
    put_u32(&out[12], collection->mask[1]);
    put_u32(&out[16], collection->mask[2]);
    out[20] = checksum(out, 20);
    return COLLECTION_BLOB_SIZE;
}

bool collection_deserialize(collection_t *collection, const uint8_t *blob,
                            size_t size)
{
    if (collection == NULL || blob == NULL || size < COLLECTION_BLOB_SIZE) {
        return false;
    }
    if (get_u32(&blob[0]) != COLLECTION_MAGIC ||
        blob[4] != COLLECTION_VERSION ||
        blob[20] != checksum(blob, 20)) {
        return false;
    }
    collection_t parsed;
    memset(&parsed, 0, sizeof(parsed));
    parsed.stories = blob[5];
    parsed.count = get_u16(&blob[6]);
    parsed.mask[0] = get_u32(&blob[8]);
    parsed.mask[1] = get_u32(&blob[12]);
    parsed.mask[2] = get_u32(&blob[16]);
    if (parsed.count != popcount_masks(parsed.mask)) {
        return false;
    }
    *collection = parsed;
    return true;
}
