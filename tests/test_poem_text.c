// Host tests for the shared UTF-8 poem helpers.
#include "poem_text.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_counts(void)
{
    assert(poem_char_count("春眠不觉晓，") == 6);
    assert(poem_char_count("") == 0);
    assert(poem_char_len("春") == 3);
    assert(poem_char_len("a") == 1);
}

static void test_noise(void)
{
    assert(poem_is_noise("，"));
    assert(poem_is_noise("。"));
    assert(poem_is_noise(" "));
    assert(!poem_is_noise("春"));

    char out[64];
    const int count = poem_strip_noise("春眠不觉晓，", out, sizeof(out));
    assert(count == 5);
    assert(strcmp(out, "春眠不觉晓") == 0);

    char chars[8][8];
    const int split = poem_split_chars("夜来风雨声，", chars, 8);
    assert(split == 5);
    assert(strcmp(chars[0], "夜") == 0);
    assert(strcmp(chars[4], "声") == 0);
}

static void test_next_char(void)
{
    const char *text = "春a";
    size_t offset = 0;
    char ch[8];
    assert(poem_next_char(text, &offset, ch));
    assert(strcmp(ch, "春") == 0);
    assert(offset == 3);
    assert(poem_next_char(text, &offset, ch));
    assert(strcmp(ch, "a") == 0);
    assert(!poem_next_char(text, &offset, ch));
}

int main(void)
{
    test_counts();
    test_noise();
    test_next_char();
    printf("test_poem_text: PASS\n");
    return 0;
}
