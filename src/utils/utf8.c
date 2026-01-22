/**
 * @file    utf8.c
 * @brief   UTF-8 string utilities implementation
 * @author  SPEECHER Team
 * @date    2025
 */

#include "utils/utf8.h"
#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Wide Character Ranges
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Unicode ranges for wide characters (2 columns in terminal)
 * Format: { start, end } (inclusive)
 */
static const uint32_t g_wide_ranges[][2] = {
    /* CJK */
    { 0x1100,  0x115F  },  /* Hangul Jamo */
    { 0x2E80,  0x2EFF  },  /* CJK Radicals */
    { 0x2F00,  0x2FDF  },  /* Kangxi Radicals */
    { 0x3000,  0x303F  },  /* CJK Symbols */
    { 0x3040,  0x309F  },  /* Hiragana */
    { 0x30A0,  0x30FF  },  /* Katakana */
    { 0x3100,  0x312F  },  /* Bopomofo */
    { 0x3130,  0x318F  },  /* Hangul Compatibility Jamo */
    { 0x3190,  0x31FF  },  /* Kanbun + Katakana Ext */
    { 0x3200,  0x32FF  },  /* Enclosed CJK */
    { 0x3300,  0x33FF  },  /* CJK Compatibility */
    { 0x3400,  0x4DBF  },  /* CJK Extension A */
    { 0x4E00,  0x9FFF  },  /* CJK Unified Ideographs */
    { 0xA000,  0xA4CF  },  /* Yi */
    { 0xAC00,  0xD7AF  },  /* Hangul Syllables */
    { 0xF900,  0xFAFF  },  /* CJK Compatibility Ideographs */
    { 0xFE10,  0xFE1F  },  /* Vertical Forms */
    { 0xFE30,  0xFE6F  },  /* CJK Compatibility Forms */
    { 0xFF00,  0xFF60  },  /* Fullwidth ASCII */
    { 0xFFE0,  0xFFE6  },  /* Fullwidth Symbols */
    { 0x20000, 0x2FFFF },  /* CJK Extension B-F */
    { 0x30000, 0x3FFFF },  /* CJK Extension G+ */
    
    /* Emoji (most are wide) */
    { 0x1F300, 0x1F9FF },  /* Misc Symbols & Pictographs, Emoticons, etc. */
    { 0x1FA00, 0x1FAFF },  /* Chess, Symbols */
    { 0x2600,  0x26FF  },  /* Misc Symbols */
    { 0x2700,  0x27BF  },  /* Dingbats */
    { 0x231A,  0x231B  },  /* Watch, Hourglass */
    { 0x23E9,  0x23F3  },  /* Media controls */
    { 0x23F8,  0x23FA  },  /* More media */
    { 0x25AA,  0x25AB  },  /* Squares */
    { 0x25B6,  0x25B6  },  /* Play button */
    { 0x25C0,  0x25C0  },  /* Reverse */
    { 0x25FB,  0x25FE  },  /* Squares */
    
    /* Terminator */
    { 0, 0 }
};

/**
 * @brief Check if Unicode code point is wide (2 columns)
 */
static int is_wide_char(uint32_t cp) {
    for (int i = 0; g_wide_ranges[i][0] != 0; i++) {
        if (cp >= g_wide_ranges[i][0] && cp <= g_wide_ranges[i][1]) {
            return 1;
        }
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              UTF-8 Decoding
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Decode one UTF-8 character
 * @param s Pointer to UTF-8 bytes
 * @param cp Output: decoded code point
 * @return Number of bytes consumed (1-4) or 0 on error
 */
static int utf8_decode(const unsigned char *s, uint32_t *cp) {
    if (s[0] < 0x80) {
        /* ASCII */
        *cp = s[0];
        return 1;
    }
    
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        /* 2 bytes: 110xxxxx 10xxxxxx */
        *cp = ((uint32_t)(s[0] & 0x1F) << 6) |
              ((uint32_t)(s[1] & 0x3F));
        return 2;
    }
    
    if ((s[0] & 0xF0) == 0xE0 && 
        (s[1] & 0xC0) == 0x80 && 
        (s[2] & 0xC0) == 0x80) {
        /* 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx */
        *cp = ((uint32_t)(s[0] & 0x0F) << 12) |
              ((uint32_t)(s[1] & 0x3F) << 6)  |
              ((uint32_t)(s[2] & 0x3F));
        return 3;
    }
    
    if ((s[0] & 0xF8) == 0xF0 && 
        (s[1] & 0xC0) == 0x80 && 
        (s[2] & 0xC0) == 0x80 && 
        (s[3] & 0xC0) == 0x80) {
        /* 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        *cp = ((uint32_t)(s[0] & 0x07) << 18) |
              ((uint32_t)(s[1] & 0x3F) << 12) |
              ((uint32_t)(s[2] & 0x3F) << 6)  |
              ((uint32_t)(s[3] & 0x3F));
        return 4;
    }
    
    /* Invalid UTF-8 */
    *cp = 0xFFFD;  /* Replacement character */
    return 1;      /* Skip one byte */
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public Functions
 * ══════════════════════════════════════════════════════════════════════════════ */

size_t utf8_strlen(const char *str) {
    if (!str) return 0;
    
    size_t len = 0;
    const unsigned char *s = (const unsigned char *)str;
    
    while (*s) {
        /* Count only leading bytes (not continuation 10xxxxxx) */
        if ((*s & 0xC0) != 0x80) {
            len++;
        }
        s++;
    }
    
    return len;
}

size_t utf8_display_width(const char *str) {
    if (!str) return 0;
    
    size_t width = 0;
    const unsigned char *s = (const unsigned char *)str;
    
    while (*s) {
        uint32_t cp;
        int bytes = utf8_decode(s, &cp);
        
        if (bytes > 0) {
            width += is_wide_char(cp) ? 2 : 1;
            s += bytes;
        } else {
            s++;  /* Skip invalid byte */
        }
    }
    
    return width;
}