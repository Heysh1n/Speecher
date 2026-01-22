/**
 * @file    strings.h
 * @brief   String utility functions
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UTILS_STRINGS_H
#define UTILS_STRINGS_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Safe string copy with null termination guarantee
 * @param dest Destination buffer
 * @param src Source string
 * @param size Destination buffer size
 * @return Number of characters copied (excluding null terminator)
 */
size_t str_copy(char *dest, const char *src, size_t size);

/**
 * @brief Safe string concatenation
 * @param dest Destination buffer
 * @param src Source string to append
 * @param size Destination buffer size
 * @return Total length (may exceed size if truncated)
 */
size_t str_concat(char *dest, const char *src, size_t size);

/**
 * @brief Trim whitespace from both ends (in-place)
 * @param str String to trim
 * @return Pointer to trimmed string (same as input)
 */
char *str_trim(char *str);

/**
 * @brief Check if string starts with prefix
 */
bool str_starts_with(const char *str, const char *prefix);

/**
 * @brief Check if string ends with suffix
 */
bool str_ends_with(const char *str, const char *suffix);

/**
 * @brief Case-insensitive string comparison
 * @return 0 if equal, <0 if a<b, >0 if a>b
 */
int str_compare_nocase(const char *a, const char *b);

/**
 * @brief Convert string to lowercase (in-place)
 */
char *str_to_lower(char *str);

/**
 * @brief Convert string to uppercase (in-place)
 */
char *str_to_upper(char *str);

/**
 * @brief Duplicate string (caller must free)
 */
char *str_duplicate(const char *str);

/**
 * @brief Check if string is empty or NULL
 */
bool str_is_empty(const char *str);

/**
 * @brief Replace all occurrences of 'old' with 'new' in string
 * @param dest Destination buffer
 * @param size Destination buffer size
 * @param src Source string
 * @param old_str String to replace
 * @param new_str Replacement string
 * @return true on success
 */
bool str_replace(char *dest, size_t size, const char *src,
                 const char *old_str, const char *new_str);

#endif /* UTILS_STRINGS_H */