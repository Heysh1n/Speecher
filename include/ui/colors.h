/**
 * @file    colors.h
 * @brief   Console colors
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UI_COLORS_H
#define UI_COLORS_H

#include <stdbool.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              ANSI Color Codes
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Reset */
#define COLOR_RESET         "\033[0m"

/* Regular colors */
#define COLOR_BLACK         "\033[0;30m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_YELLOW        "\033[0;33m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_MAGENTA       "\033[0;35m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_WHITE         "\033[0;37m"

/* Bright colors */
#define COLOR_BRIGHT_BLACK   "\033[0;90m"
#define COLOR_BRIGHT_RED     "\033[0;91m"
#define COLOR_BRIGHT_GREEN   "\033[0;92m"
#define COLOR_BRIGHT_YELLOW  "\033[0;93m"
#define COLOR_BRIGHT_BLUE    "\033[0;94m"
#define COLOR_BRIGHT_MAGENTA "\033[0;95m"
#define COLOR_BRIGHT_CYAN    "\033[0;96m"
#define COLOR_BRIGHT_WHITE   "\033[0;97m"

/* Bold */
#define COLOR_BOLD          "\033[1m"

/* ══════════════════════════════════════════════════════════════════════════════
 *                              API
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize color system
 * @param enabled Enable colors
 */
void colors_init(bool enabled);

/**
 * @brief Check if colors are enabled
 */
bool colors_enabled(void);

/**
 * @brief Get color code by name
 * @param name Color name (e.g., "red", "bright_green")
 * @return ANSI code or empty string if colors disabled
 */
const char *color_by_name(const char *name);

/**
 * @brief Print colored text
 * @param color Color code
 * @param fmt Printf format
 */
void color_print(const char *color, const char *fmt, ...);

/**
 * @brief Print colored line (with newline)
 */
void color_println(const char *color, const char *fmt, ...);

#endif /* UI_COLORS_H */