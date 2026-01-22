/**
 * @file    utf8.h
 * @brief   UTF-8 string utilities for display width calculation
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UTILS_UTF8_H
#define UTILS_UTF8_H

#include <stddef.h>

/**
 * @brief Calculate display width of UTF-8 string in terminal columns
 * 
 * Correctly handles:
 * - ASCII characters (1 column)
 * - Cyrillic, Latin extended (1 column)  
 * - CJK characters (2 columns)
 * - Emoji (2 columns)
 * 
 * @param str UTF-8 encoded string
 * @return Number of terminal columns
 */
size_t utf8_display_width(const char *str);

/**
 * @brief Count UTF-8 characters (code points), not bytes
 * @param str UTF-8 encoded string
 * @return Number of Unicode characters
 */
size_t utf8_strlen(const char *str);

#endif /* UTILS_UTF8_H */