#include "poem_text.h"

#include <string.h>

static unsigned decode(const char *text, int *bytes)
{
    const unsigned char *p = (const unsigned char *)text;
    if (p[0] < 0x80u) {
        *bytes = 1;
        return p[0];
    }
    if ((p[0] & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u) {
        *bytes = 2;
        return ((unsigned)(p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu);
    }
    if ((p[0] & 0xF0u) == 0xE0u && (p[1] & 0xC0u) == 0x80u &&
        (p[2] & 0xC0u) == 0x80u) {
        *bytes = 3;
        return ((unsigned)(p[0] & 0x0Fu) << 12) |
               ((unsigned)(p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
    }
    if ((p[0] & 0xF8u) == 0xF0u && (p[1] & 0xC0u) == 0x80u &&
        (p[2] & 0xC0u) == 0x80u && (p[3] & 0xC0u) == 0x80u) {
        *bytes = 4;
        return ((unsigned)(p[0] & 0x07u) << 18) |
               ((unsigned)(p[1] & 0x3Fu) << 12) |
               ((unsigned)(p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
    }
    *bytes = 1;
    return p[0];
}

int poem_char_len(const char *text)
{
    int bytes = 1;
    (void)decode(text, &bytes);
    return bytes;
}

bool poem_is_noise(const char *text)
{
    int bytes = 1;
    const unsigned cp = decode(text, &bytes);
    if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') {
        return true;
    }
    if (cp < 0x80u) {
        return cp < 0x30u && cp != '\0';
    }
    switch (cp) {
    case 0x3002:  // 。
    case 0xFF0C:  // ，
    case 0xFF01:  // ！
    case 0xFF1F:  // ？
    case 0x3001:  // 、
    case 0xFF1B:  // ；
    case 0xFF1A:  // ：
    case 0x201C:  // “
    case 0x201D:  // ”
    case 0x2018:  // ‘
    case 0x2019:  // ’
    case 0x300A:  // 《
    case 0x300B:  // 》
    case 0x3008:  // 〈
    case 0x3009:  // 〉
    case 0xFF08:  // （
    case 0xFF09:  // ）
    case 0x2026:  // …
    case 0x2014:  // —
    case 0x00B7:  // ·
    case 0x300C:  // 「
    case 0x300D:  // 」
    case 0x300E:  // 『
    case 0x300F:  // 』
    case 0x3010:  // 【
    case 0x3011:  // 】
        return true;
    default:
        return false;
    }
}

int poem_char_count(const char *text)
{
    if (text == NULL) {
        return 0;
    }
    int count = 0;
    size_t offset = 0;
    char ch[8];
    while (poem_next_char(text, &offset, ch)) {
        count++;
    }
    return count;
}

bool poem_next_char(const char *text, size_t *offset, char out[8])
{
    if (text == NULL || offset == NULL) {
        return false;
    }
    size_t pos = *offset;
    if (text[pos] == '\0') {
        return false;
    }
    const int len = poem_char_len(&text[pos]);
    for (int i = 0; i < len && i < 7; i++) {
        out[i] = text[pos + (size_t)i];
    }
    out[len > 7 ? 7 : len] = '\0';
    *offset = pos + (size_t)len;
    return true;
}

int poem_split_chars(const char *text, char out[][8], int max_chars)
{
    if (text == NULL) {
        return 0;
    }
    int count = 0;
    size_t offset = 0;
    char ch[8];
    while (poem_next_char(text, &offset, ch)) {
        if (poem_is_noise(ch)) {
            continue;
        }
        if (out != NULL && count < max_chars) {
            memcpy(out[count], ch, sizeof(ch));
        }
        count++;
    }
    return count;
}

int poem_strip_noise(const char *text, char *out, size_t size)
{
    if (out == NULL || size == 0) {
        return 0;
    }
    out[0] = '\0';
    if (text == NULL) {
        return 0;
    }
    size_t used = 0;
    size_t offset = 0;
    char ch[8];
    int count = 0;
    while (poem_next_char(text, &offset, ch)) {
        if (poem_is_noise(ch)) {
            continue;
        }
        const size_t len = strlen(ch);
        if (used + len + 1 > size) {
            break;
        }
        memcpy(&out[used], ch, len);
        used += len;
        out[used] = '\0';
        count++;
    }
    return count;
}
