/**
 * @file    colors.c
 * @brief   Console colors - STUB
 */

#include "ui/colors.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static bool g_colors_enabled = true;

void colors_init(bool enabled) { g_colors_enabled = enabled; }
bool colors_enabled(void) { return g_colors_enabled; }

const char *color_by_name(const char *name) {
    if (!g_colors_enabled || !name) return "";
    if (strcmp(name, "red") == 0) return "\033[31m";
    if (strcmp(name, "green") == 0) return "\033[32m";
    if (strcmp(name, "yellow") == 0) return "\033[33m";
    if (strcmp(name, "blue") == 0) return "\033[34m";
    if (strcmp(name, "cyan") == 0) return "\033[36m";
    return "";
}

void color_print(const char *color, const char *fmt, ...) {
    if (g_colors_enabled && color) printf("%s", color);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    if (g_colors_enabled) printf("\033[0m");
}

void color_println(const char *color, const char *fmt, ...) {
    if (g_colors_enabled && color) printf("%s", color);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    if (g_colors_enabled) printf("\033[0m");
    printf("\n");
}
