// Host tests for the collected-poem codex and NVS blob.
#include "collection.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_add(void)
{
    collection_t collection;
    collection_init(&collection);
    assert(collection_count(&collection) == 0);
    assert(!collection_has(&collection, 7));
    assert(collection_add(&collection, 7));
    assert(collection_has(&collection, 7));
    assert(collection_count(&collection) == 1);
    assert(!collection_add(&collection, 7));
    assert(collection_count(&collection) == 1);
    assert(collection_add(&collection, 95));
    assert(collection_count(&collection) == 2);
    assert(!collection_add(&collection, 96));
}

static void test_serialize(void)
{
    collection_t collection;
    collection_init(&collection);
    collection_add(&collection, 7);
    collection_add(&collection, 16);
    const uint8_t unlocked = collection_unlock_stories(&collection);
    assert((unlocked & 0x3u) == 0x3u);

    uint8_t blob[COLLECTION_BLOB_SIZE];
    assert(collection_serialize(&collection, blob, sizeof(blob)) ==
           COLLECTION_BLOB_SIZE);

    collection_t restored;
    assert(collection_deserialize(&restored, blob, sizeof(blob)));
    assert(collection_has(&restored, 7));
    assert(collection_has(&restored, 16));
    assert(collection_count(&restored) == 2);
    assert(collection_story_unlocked(&restored, 0));
    assert(collection_story_unlocked(&restored, 1));
    assert(!collection_story_unlocked(&restored, 2));

    // Corrupt the checksum and the payload.
    uint8_t broken[COLLECTION_BLOB_SIZE];
    memcpy(broken, blob, sizeof(broken));
    broken[20] ^= 0xFFu;
    assert(!collection_deserialize(&restored, broken, sizeof(broken)));
    memcpy(broken, blob, sizeof(broken));
    broken[8] ^= 0x04u;
    assert(!collection_deserialize(&restored, broken, sizeof(broken)));
    assert(!collection_deserialize(&restored, blob, 4));
}

static void test_stories(void)
{
    collection_t collection;
    collection_init(&collection);
    collection_add(&collection, 7);
    assert((collection_unlock_stories(&collection) & 0x1u) != 0);
    assert(collection_story_unlocked(&collection, 0));
    assert(!collection_story_unlocked(&collection, 1));
    assert((collection_unlock_stories(&collection) & 0x1u) == 0);

    collection_add(&collection, 2);
    assert((collection_unlock_stories(&collection) & 0x2u) != 0);
    assert(collection_story_unlocked(&collection, 1));

    collection_add(&collection, 16);
    assert((collection_unlock_stories(&collection) & 0x4u) != 0);
    assert(collection_story_unlocked(&collection, 2));
    assert(collection_unlock_stories(&collection) == 0);
}

int main(void)
{
    test_add();
    test_serialize();
    test_stories();
    printf("test_collection: PASS\n");
    return 0;
}
