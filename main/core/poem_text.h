// UTF-8 helpers shared by the poetry challenge core and the LVGL UI.
// Pure C, no LVGL/ESP-IDF dependencies.
#pragma once

#include <stdbool.h>
#include <stddef.h>

// Bytes used by the first UTF-8 code point (1..4). Returns 1 for invalid input.
int poem_char_len(const char *text);

// True for Chinese/ASCII punctuation and whitespace that should not become a
// puzzle tile.
bool poem_is_noise(const char *text);

// Copy text without punctuation/whitespace into out. Always NUL-terminates when
// size > 0. Returns the number of code points copied.
int poem_strip_noise(const char *text, char *out, size_t size);

// Split text into individual code points, skipping punctuation/whitespace.
// out may be NULL when max_chars is 0. Returns the number of code points.
int poem_split_chars(const char *text, char out[][8], int max_chars);

// Number of code points in text (including punctuation).
int poem_char_count(const char *text);

// Copy one code point starting at *offset into out (max 8 bytes). Advances
// *offset. Returns false at end of string.
bool poem_next_char(const char *text, size_t *offset, char out[8]);
