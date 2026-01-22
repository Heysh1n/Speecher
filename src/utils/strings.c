/**
 * @file    strings.c
 * @brief   String utility functions implementation
 * @author  SPEECHER Team
 * @date    2025
 */

#include "utils/strings.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

size_t str_copy(char *dest, const char *src, size_t size) {
    if (dest == NULL || size == 0) return 0;
    if (src == NULL) { dest[0] = '\0'; return 0; }
    
    size_t i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return i;
}

size_t str_concat(char *dest, const char *src, size_t size) {
    if (dest == NULL || size == 0) return 0;
    size_t dest_len = strlen(dest);
    if (dest_len >= size - 1) return dest_len;
    return dest_len + str_copy(dest + dest_len, src, size - dest_len);
}

char *str_trim(char *str) {
    if (str == NULL) return NULL;
    
    /* Trim leading */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') return str;
    
    /* Trim trailing */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    
    return str;
}

bool str_starts_with(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool str_ends_with(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int str_compare_nocase(const char *a, const char *b) {
    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    
    while (*a && *b) {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

char *str_to_lower(char *str) {
    if (str == NULL) return NULL;
    for (char *p = str; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
    return str;
}

char *str_to_upper(char *str) {
    if (str == NULL) return NULL;
    for (char *p = str; *p; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return str;
}

char *str_duplicate(const char *str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, str, len);
    return copy;
}

bool str_is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

bool str_replace(char *dest, size_t size, const char *src,
                 const char *old_str, const char *new_str) {
    /* TODO: Implement full replace */
    if (dest == NULL || src == NULL) return false;
    str_copy(dest, src, size);
    return true;
}
