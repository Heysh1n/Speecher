/**
 * @file    validator.c
 * @brief   Configuration validation - Full implementation
 * @author  SPEECHER Team
 * @date    2025
 */

#include "config/validator.h"
#include "utils/strings.h"
#include <string.h>
#include <stdio.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Valid Values
 * ══════════════════════════════════════════════════════════════════════════════ */

static const char *valid_sort_orders[] = {
    "name_asc", "name_desc", 
    "date_asc", "date_desc",
    "size_asc", "size_desc",
    NULL
};

static const char *valid_log_levels[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL",
    "debug", "info", "warn", "error", "fatal",
    NULL
};

static const char *valid_colors[] = {
    "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
    "bright_black", "bright_red", "bright_green", "bright_yellow",
    "bright_blue", "bright_magenta", "bright_cyan", "bright_white",
    NULL
};

static const char *valid_post_actions[] = {
    "keep", "move", "delete",
    NULL
};

static const char *valid_backends[] = {
    "local", "api",
    NULL
};

static const char *valid_menu_styles[] = {
    "single", "double", "rounded", "ascii",
    NULL
};

static const char *valid_progress_styles[] = {
    "ascii", "unicode", "minimal",
    NULL
};

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

static bool is_in_list(const char *value, const char **list) {
    if (!value || !list) return false;
    
    for (const char **p = list; *p; p++) {
        if (str_compare_nocase(value, *p) == 0) {
            return true;
        }
    }
    return false;
}

static int clamp_int(int value, int min, int max, int default_val, int *corrections) {
    if (value < min || value > max) {
        (*corrections)++;
        return default_val;
    }
    return value;
}

static void validate_string_option(char *value, size_t size, 
                                   const char **valid_list, 
                                   const char *default_val,
                                   int *corrections) {
    if (!is_in_list(value, valid_list)) {
        str_copy(value, default_val, size);
        (*corrections)++;
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

int config_validate(Config *config) {
    if (!config) return 0;
    
    int corrections = 0;
    
    /* [text_merger] */
    validate_string_option(config->text_sort_order, 
                          sizeof(config->text_sort_order),
                          valid_sort_orders, "name_asc", &corrections);
    
    config->text_max_depth = clamp_int(config->text_max_depth, 1, 100, 3, &corrections);
    config->text_separator_length = clamp_int(config->text_separator_length, 10, 200, 60, &corrections);
    
    validate_string_option(config->text_post_action,
                          sizeof(config->text_post_action),
                          valid_post_actions, "keep", &corrections);
    
    /* [audio_merger] */
    validate_string_option(config->audio_sort_order,
                          sizeof(config->audio_sort_order),
                          valid_sort_orders, "name_asc", &corrections);
    
    config->audio_max_depth = clamp_int(config->audio_max_depth, 1, 100, 3, &corrections);
    
    validate_string_option(config->audio_post_action,
                          sizeof(config->audio_post_action),
                          valid_post_actions, "keep", &corrections);
    
    /* [whisper] */
    validate_string_option(config->whisper_backend,
                          sizeof(config->whisper_backend),
                          valid_backends, "local", &corrections);
    
    config->whisper_threads = clamp_int(config->whisper_threads, 0, 64, 0, &corrections);
    
    /* [logging] */
    validate_string_option(config->log_level,
                          sizeof(config->log_level),
                          valid_log_levels, "INFO", &corrections);
    
    validate_string_option(config->log_console_level,
                          sizeof(config->log_console_level),
                          valid_log_levels, "INFO", &corrections);
    
    validate_string_option(config->log_color_debug,
                          sizeof(config->log_color_debug),
                          valid_colors, "cyan", &corrections);
    
    validate_string_option(config->log_color_info,
                          sizeof(config->log_color_info),
                          valid_colors, "green", &corrections);
    
    validate_string_option(config->log_color_warn,
                          sizeof(config->log_color_warn),
                          valid_colors, "yellow", &corrections);
    
    validate_string_option(config->log_color_error,
                          sizeof(config->log_color_error),
                          valid_colors, "red", &corrections);
    
    validate_string_option(config->log_color_fatal,
                          sizeof(config->log_color_fatal),
                          valid_colors, "bright_red", &corrections);
    
    config->log_max_size_mb = clamp_int(config->log_max_size_mb, 1, 1000, 10, &corrections);
    config->log_max_files = clamp_int(config->log_max_files, 0, 1000, 30, &corrections);
    
    /* [ui] */
    validate_string_option(config->ui_menu_style,
                          sizeof(config->ui_menu_style),
                          valid_menu_styles, "double", &corrections);
    
    validate_string_option(config->ui_progress_style,
                          sizeof(config->ui_progress_style),
                          valid_progress_styles, "unicode", &corrections);
    
    config->ui_progress_width = clamp_int(config->ui_progress_width, 10, 100, 40, &corrections);
    
    return corrections;
}

bool validate_sort_order(const char *value) {
    return is_in_list(value, valid_sort_orders);
}

bool validate_log_level(const char *value) {
    return is_in_list(value, valid_log_levels);
}

bool validate_color(const char *value) {
    return is_in_list(value, valid_colors);
}

bool validate_path_safe(const char *path) {
    if (!path) return false;
    
    /* Check for directory traversal */
    if (strstr(path, "..") != NULL) {
        return false;
    }
    
    /* Check for absolute paths (could be dangerous) */
    if (path[0] == '/' || (path[0] != '\0' && path[1] == ':')) {
        /* Allow, but maybe warn */
    }
    
    return true;
}