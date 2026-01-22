/**
 * @file    menu.c
 * @brief   Interactive menu - Full implementation with i18n and UTF-8 support
 * @author  SPEECHER Team
 * @date    2025
 */

#include "ui/menu.h"
#include "ui/colors.h"
#include "config/config.h"
#include "utils/platform.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "utils/input.h"
#include "utils/utf8.h"
#include "logging/logger.h"
#include "i18n/lang.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Box Drawing Characters
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *top_left;
    const char *top_right;
    const char *bottom_left;
    const char *bottom_right;
    const char *horizontal;
    const char *vertical;
} BoxChars;

static const BoxChars BOX_DOUBLE = {
    .top_left = "╔", .top_right = "╗",
    .bottom_left = "╚", .bottom_right = "╝",
    .horizontal = "═", .vertical = "║"
};

static const BoxChars BOX_SINGLE = {
    .top_left = "┌", .top_right = "┐",
    .bottom_left = "└", .bottom_right = "┘",
    .horizontal = "─", .vertical = "│"
};

static const BoxChars BOX_ROUNDED = {
    .top_left = "╭", .top_right = "╮",
    .bottom_left = "╰", .bottom_right = "╯",
    .horizontal = "─", .vertical = "│"
};

static const BoxChars BOX_ASCII = {
    .top_left = "+", .top_right = "+",
    .bottom_left = "+", .bottom_right = "+",
    .horizontal = "-", .vertical = "|"
};

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

static const BoxChars *get_box_style(void) {
    Config *cfg = config_get();
    
    if (strcmp(cfg->ui_menu_style, "single") == 0) return &BOX_SINGLE;
    if (strcmp(cfg->ui_menu_style, "rounded") == 0) return &BOX_ROUNDED;
    if (strcmp(cfg->ui_menu_style, "ascii") == 0) return &BOX_ASCII;
    
    return &BOX_DOUBLE;
}

static void print_box_line(const BoxChars *box, int width, bool is_top) {
    printf("%s", is_top ? box->top_left : box->bottom_left);
    for (int i = 0; i < width - 2; i++) {
        printf("%s", box->horizontal);
    }
    printf("%s\n", is_top ? box->top_right : box->bottom_right);
}

static void print_centered_text(const BoxChars *box, int width, const char *text) {
    int text_display_width = (int)utf8_display_width(text);
    int inner_width = width - 2;
    int padding_total = inner_width - text_display_width;
    int padding_left = padding_total / 2;
    int padding_right = padding_total - padding_left;
    
    if (padding_left < 0) padding_left = 0;
    if (padding_right < 0) padding_right = 0;
    
    printf("%s", box->vertical);
    for (int i = 0; i < padding_left; i++) printf(" ");
    printf("%s", text);
    for (int i = 0; i < padding_right; i++) printf(" ");
    printf("%s\n", box->vertical);
}

static void print_header(const char *title) {
    const BoxChars *box = get_box_style();
    Config *cfg = config_get();
    
    int width = (cfg->ui_menu_width > 0) ? cfg->ui_menu_width : 48;
    
    int title_width = (int)utf8_display_width(title);
    if (title_width + 4 > width) {
        width = title_width + 6;
    }
    
    printf("\n");
    print_box_line(box, width, true);
    print_centered_text(box, width, title);
    print_box_line(box, width, false);
    printf("\n");
}

/**
 * @brief Get localized Yes/No string
 */
static const char* bool_str(bool value) {
    return value ? lang_msg("yes") : lang_msg("no");
}

/**
 * @brief Get select prompt
 */
static void print_select_prompt(char *buffer, size_t size) {
    snprintf(buffer, size, "  %s: ", lang_msg("select"));
}

static int read_int_option(const char *prompt, int min, int max, int current) {
    char full_prompt[256];
    snprintf(full_prompt, sizeof(full_prompt), "  %s [%d-%d] (%s: %d): ", 
             prompt, min, max, lang_msg("current"), current);
    
    int value = input_read_int(full_prompt, current);
    
    if (value < min || value > max) {
        color_println(COLOR_YELLOW, "  %s", lang_err("invalid_value"));
        return current;
    }
    
    return value;
}

static bool read_bool_option(const char *prompt, bool current) {
    Config *cfg = config_get();
    char full_prompt[256];
    snprintf(full_prompt, sizeof(full_prompt), "  %s %s [%s: %s]: ", 
             prompt, 
             input_get_yesno_hint(cfg->language),
             lang_msg("current"), 
             bool_str(current));
    
    return input_read_bool_lang(full_prompt, cfg->language, current);
}

static void read_string_option(const char *prompt, char *dest, size_t size, const char *current) {
    printf("  %s (%s: %s)\n", prompt, lang_msg("current"), current);
    
    char buffer[512];
    char line_prompt[128];
    snprintf(line_prompt, sizeof(line_prompt), "  %s: ", lang_msg("enter_to_keep"));
    
    if (input_read_line(line_prompt, buffer, sizeof(buffer))) {
        if (buffer[0] != '\0') {
            str_copy(dest, buffer, size);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Settings Submenus
 * ══════════════════════════════════════════════════════════════════════════════ */

static void settings_general(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("general"));
    
    printf("  1. %s:    %s\n", lang_settings("language"), cfg->language);
    printf("  2. %s:  %s\n", lang_settings("show_emoji"), bool_str(cfg->show_emoji));
    printf("  0. %s\n\n", lang_msg("back"));
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    switch (choice) {
        case 1:
            printf("\n  %s: en, ru, tr, ja\n", lang_msg("available"));
            read_string_option(lang_settings("language"), cfg->language, 
                              sizeof(cfg->language), cfg->language);
             lang_init(cfg->language);
            break;
        case 2:
            cfg->show_emoji = read_bool_option(lang_settings("show_emoji"), cfg->show_emoji);
            break;
        case 0:
            return;
    }
    
 LOG_INFO("Settings changed: general%s", "");
}

static void settings_text_merger(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("text_merger"));
    
    printf("  1. %s:        %s\n", lang_settings("extensions"), cfg->text_extensions);
    printf("  2. %s:        %s\n", lang_settings("sort_order"), cfg->text_sort_order);
    printf("  3. %s:         %s\n", lang_settings("recursive"), bool_str(cfg->text_recursive));
    printf("  4. %s:         %d\n", lang_settings("max_depth"), cfg->text_max_depth);
    printf("  5. %s:         %s\n", lang_settings("separator"), bool_str(cfg->text_separator_enabled));
    printf("  6. %s:  %d\n", lang_settings("separator_length"), cfg->text_separator_length);
    printf("  7. %s:   %s\n", lang_settings("output_template"), cfg->text_output_template);
    printf("  0. %s\n\n", lang_msg("back"));
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    switch (choice) {
        case 1:
            read_string_option(lang_settings("extensions"), cfg->text_extensions, 
                              sizeof(cfg->text_extensions), cfg->text_extensions);
            break;
        case 2:
            printf("\n  %s: name_asc, name_desc, date_asc, date_desc, size_asc, size_desc\n", 
                   lang_msg("options"));
            read_string_option(lang_settings("sort_order"), cfg->text_sort_order, 
                              sizeof(cfg->text_sort_order), cfg->text_sort_order);
            break;
        case 3:
            cfg->text_recursive = read_bool_option(lang_settings("recursive"), cfg->text_recursive);
            break;
        case 4:
            cfg->text_max_depth = read_int_option(lang_settings("max_depth"), 1, 100, cfg->text_max_depth);
            break;
        case 5:
            cfg->text_separator_enabled = read_bool_option(lang_settings("separator"), cfg->text_separator_enabled);
            break;
        case 6:
            cfg->text_separator_length = read_int_option(lang_settings("separator_length"), 10, 200, cfg->text_separator_length);
            break;
        case 7:
            printf("\n  %s: {date}, {time}\n", lang_msg("placeholders"));
            read_string_option(lang_settings("output_template"), cfg->text_output_template, 
                              sizeof(cfg->text_output_template), cfg->text_output_template);
            break;
        case 0:
            return;
    }
    
LOG_INFO("Settings changed: text_merger%s", "");
}

static void settings_audio_merger(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("audio_merger"));
    
    printf("  1. %s:      %s\n", lang_settings("extensions"), cfg->audio_extensions);
    printf("  2. %s:      %s\n", lang_settings("sort_order"), cfg->audio_sort_order);
    printf("  3. %s:       %s\n", lang_settings("recursive"), bool_str(cfg->audio_recursive));
    printf("  4. %s:       %d\n", lang_settings("max_depth"), cfg->audio_max_depth);
    printf("  5. %s:      %s\n", lang_settings("timestamps"), bool_str(cfg->audio_timestamps));
    printf("  6. %s:   %s\n", lang_settings("whisper_model"), cfg->whisper_model);
    printf("  7. %s: %s\n", lang_settings("whisper_backend"), cfg->whisper_backend);
    printf("  0. %s\n\n", lang_msg("back"));
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    switch (choice) {
        case 1:
            read_string_option(lang_settings("extensions"), cfg->audio_extensions, 
                              sizeof(cfg->audio_extensions), cfg->audio_extensions);
            break;
        case 2:
            printf("\n  %s: name_asc, name_desc, date_asc, date_desc, size_asc, size_desc\n", 
                   lang_msg("options"));
            read_string_option(lang_settings("sort_order"), cfg->audio_sort_order, 
                              sizeof(cfg->audio_sort_order), cfg->audio_sort_order);
            break;
        case 3:
            cfg->audio_recursive = read_bool_option(lang_settings("recursive"), cfg->audio_recursive);
            break;
        case 4:
            cfg->audio_max_depth = read_int_option(lang_settings("max_depth"), 1, 100, cfg->audio_max_depth);
            break;
        case 5:
            cfg->audio_timestamps = read_bool_option(lang_settings("timestamps"), cfg->audio_timestamps);
            break;
        case 6:
            printf("\n  %s: tiny, base, small, medium, large\n", lang_msg("models"));
            read_string_option(lang_settings("whisper_model"), cfg->whisper_model, 
                              sizeof(cfg->whisper_model), cfg->whisper_model);
            break;
        case 7:
            printf("\n  %s: local, api\n", lang_msg("backends"));
            read_string_option(lang_settings("whisper_backend"), cfg->whisper_backend, 
                              sizeof(cfg->whisper_backend), cfg->whisper_backend);
            break;
        case 0:
            return;
    }
    
    LOG_INFO("Settings changed: audio_merger%s", "");
}

static void settings_ui(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("ui"));
    
    printf("  1. %s:    %s\n", lang_settings("progress_bar"), bool_str(cfg->ui_show_progress));
    printf("  2. %s:  %s\n", lang_settings("progress_style"), cfg->ui_progress_style);
    printf("  3. %s:  %d\n", lang_settings("progress_width"), cfg->ui_progress_width);
    printf("  4. %s:    %s\n", lang_settings("clear_screen"), bool_str(cfg->ui_clear_screen));
    printf("  5. %s:      %s\n", lang_settings("menu_style"), cfg->ui_menu_style);
    printf("  6. %s:      %d\n", lang_settings("menu_width"), cfg->ui_menu_width);
    printf("  7. %s:  %s\n", lang_settings("colored_output"), bool_str(cfg->log_colored));
    printf("  0. %s\n\n", lang_msg("back"));
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    switch (choice) {
        case 1:
            cfg->ui_show_progress = read_bool_option(lang_settings("progress_bar"), cfg->ui_show_progress);
            break;
        case 2:
            printf("\n  %s: ascii, unicode, minimal\n", lang_msg("styles"));
            read_string_option(lang_settings("progress_style"), cfg->ui_progress_style, 
                              sizeof(cfg->ui_progress_style), cfg->ui_progress_style);
            break;
        case 3:
            cfg->ui_progress_width = read_int_option(lang_settings("progress_width"), 10, 100, cfg->ui_progress_width);
            break;
        case 4:
            cfg->ui_clear_screen = read_bool_option(lang_settings("clear_screen"), cfg->ui_clear_screen);
            break;
        case 5:
            printf("\n  %s: single, double, rounded, ascii\n", lang_msg("styles"));
            read_string_option(lang_settings("menu_style"), cfg->ui_menu_style, 
                              sizeof(cfg->ui_menu_style), cfg->ui_menu_style);
            break;
        case 6:
            cfg->ui_menu_width = read_int_option(lang_settings("menu_width"), 30, 120, cfg->ui_menu_width);
            break;
        case 7:
            cfg->log_colored = read_bool_option(lang_settings("colored_output"), cfg->log_colored);
            colors_init(cfg->log_colored);
            break;
        case 0:
            return;
    }
    
    LOG_INFO("Settings changed: ui%s", "");
}

static void settings_logging(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("logging"));
    
    printf("  1. %s:       %s\n", lang_settings("enabled"), bool_str(cfg->log_enabled));
    printf("  2. %s:     %s\n", lang_settings("log_level"), cfg->log_level);
    printf("  3. %s:       %s\n", lang_settings("console"), bool_str(cfg->log_console_output));
    printf("  4. %s: %s\n", lang_settings("console_level"), cfg->log_console_level);
    printf("  5. %s:     %d\n", lang_settings("max_files"), cfg->log_max_files);
    printf("  6. %s:   %d\n", lang_settings("max_size"), cfg->log_max_size_mb);
    printf("  0. %s\n\n", lang_msg("back"));
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    switch (choice) {
        case 1:
            cfg->log_enabled = read_bool_option(lang_settings("enabled"), cfg->log_enabled);
            break;
        case 2:
            printf("\n  %s: DEBUG, INFO, WARN, ERROR, FATAL\n", lang_msg("levels"));
            read_string_option(lang_settings("log_level"), cfg->log_level, 
                              sizeof(cfg->log_level), cfg->log_level);
            break;
        case 3:
            cfg->log_console_output = read_bool_option(lang_settings("console"), cfg->log_console_output);
            break;
        case 4:
            read_string_option(lang_settings("console_level"), cfg->log_console_level, 
                              sizeof(cfg->log_console_level), cfg->log_console_level);
            break;
        case 5:
            cfg->log_max_files = read_int_option(lang_settings("max_files"), 0, 1000, cfg->log_max_files);
            break;
        case 6:
            cfg->log_max_size_mb = read_int_option(lang_settings("max_size"), 1, 1000, cfg->log_max_size_mb);
            break;
        case 0:
            return;
    }
    
    LOG_INFO("Settings changed: logging%s", "");
}

static void settings_paths(void) {
    Config *cfg = config_get();
    
    print_header(lang_settings("paths"));
    
    printf("  1. %s:  %s\n", lang_settings("input_path"), cfg->input_dir);
    printf("  2. %s: %s\n", lang_settings("output_path"), cfg->output_dir);
    printf("  3. %s:   %s\n", lang_settings("logs_path"), cfg->logs_dir);
    printf("  4. %s:    %s\n", lang_settings("lib_path"), cfg->lib_dir);
    printf("  0. %s\n\n", lang_msg("back"));
    
    color_println(COLOR_YELLOW, "  ⚠ %s", lang_settings("requires_restart"));
    printf("\n");
    
    char prompt[64];
    print_select_prompt(prompt, sizeof(prompt));
    int choice = input_read_choice(prompt);
    
    /* Сохраняем старые пути */
    char old_path[SPEECHER_PATH_MAX];
    char full_old_path[SPEECHER_PATH_MAX];
    char full_new_path[SPEECHER_PATH_MAX];
    const char *base_dir = app_get_base_dir();
    bool path_changed = false;
    
    switch (choice) {
        case 1:
            strncpy(old_path, cfg->input_dir, sizeof(old_path) - 1);
            old_path[sizeof(old_path) - 1] = '\0';
            read_string_option(lang_settings("input_path"), cfg->input_dir, 
                              sizeof(cfg->input_dir), cfg->input_dir);
            path_changed = (strcmp(old_path, cfg->input_dir) != 0);
            if (path_changed) {
                fs_path_join(full_old_path, sizeof(full_old_path), base_dir, old_path);
                fs_path_join(full_new_path, sizeof(full_new_path), base_dir, cfg->input_dir);
            }
            break;
            
        case 2:
            strncpy(old_path, cfg->output_dir, sizeof(old_path) - 1);
            old_path[sizeof(old_path) - 1] = '\0';
            read_string_option(lang_settings("output_path"), cfg->output_dir, 
                              sizeof(cfg->output_dir), cfg->output_dir);
            path_changed = (strcmp(old_path, cfg->output_dir) != 0);
            if (path_changed) {
                fs_path_join(full_old_path, sizeof(full_old_path), base_dir, old_path);
                fs_path_join(full_new_path, sizeof(full_new_path), base_dir, cfg->output_dir);
            }
            break;
            
        case 3:
            strncpy(old_path, cfg->logs_dir, sizeof(old_path) - 1);
            old_path[sizeof(old_path) - 1] = '\0';
            read_string_option(lang_settings("logs_path"), cfg->logs_dir, 
                              sizeof(cfg->logs_dir), cfg->logs_dir);
            path_changed = (strcmp(old_path, cfg->logs_dir) != 0);
            if (path_changed) {
                fs_path_join(full_old_path, sizeof(full_old_path), base_dir, old_path);
                fs_path_join(full_new_path, sizeof(full_new_path), base_dir, cfg->logs_dir);
            }
            break;
            
        case 4:
            strncpy(old_path, cfg->lib_dir, sizeof(old_path) - 1);
            old_path[sizeof(old_path) - 1] = '\0';
            read_string_option(lang_settings("lib_path"), cfg->lib_dir, 
                              sizeof(cfg->lib_dir), cfg->lib_dir);
            path_changed = (strcmp(old_path, cfg->lib_dir) != 0);
            if (path_changed) {
                fs_path_join(full_old_path, sizeof(full_old_path), base_dir, old_path);
                fs_path_join(full_new_path, sizeof(full_new_path), base_dir, cfg->lib_dir);
            }
            break;
            
        case 0:
            return;
            
        default:
            return;
    }
    
    /* Мигрируем папку если путь изменился */
    if (path_changed) {
        printf("\n  %s...\n", lang_msg("processing"));
        
        if (fs_move_directory(full_old_path, full_new_path)) {
            color_println(COLOR_GREEN, "  ✓ %s", lang_msg("success"));
        } else {
            /* Если миграция не удалась, пробуем просто создать новую папку */
            if (fs_mkdir_recursive(full_new_path)) {
                color_println(COLOR_YELLOW, "  ⚠ %s", lang_msg("folder_created"));
            } else {
                color_println(COLOR_RED, "  ✗ %s", lang_err("cannot_create_dir"));
            }
        }
        
        menu_pause(NULL);
    }
    
    LOG_INFO("Settings changed: paths%s", "");
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Log Viewer
 * ══════════════════════════════════════════════════════════════════════════════ */

static void view_log_file(const char *filepath) {
    Config *cfg = config_get();
    
    if (cfg->ui_clear_screen) {
        platform_console_clear();
    }
    
    print_header(lang_logs("title"));
    
    printf("  %s: %s\n\n", lang_logs("file"), fs_path_filename(filepath));
    
    size_t size;
    char *content = fs_read_file(filepath, &size);
    
    if (!content) {
        color_println(COLOR_RED, "  %s", lang_err("cannot_read"));
        menu_pause(NULL);
        return;
    }
    
    const int max_lines = 30;
    int line_count = 0;
    for (size_t i = 0; i < size; i++) {
        if (content[i] == '\n') line_count++;
    }
    
    char *start = content;
    if (line_count > max_lines) {
        int skip = line_count - max_lines;
        for (char *p = content; *p && skip > 0; p++) {
            if (*p == '\n') {
                skip--;
                start = p + 1;
            }
        }
    }
    
    printf("  %s: %d\n", lang_logs("showing_lines"), max_lines);
    printf("  ─────────────────────────────────────────\n");
    
    char *line_start = start;
    for (char *p = start; ; p++) {
        if (*p == '\n' || *p == '\0') {
            char saved = *p;
            *p = '\0';
            
            if (strstr(line_start, "[DEBUG]")) {
                color_println(COLOR_CYAN, "  %s", line_start);
            } else if (strstr(line_start, "[INFO]")) {
                color_println(COLOR_GREEN, "  %s", line_start);
            } else if (strstr(line_start, "[WARN]")) {
                color_println(COLOR_YELLOW, "  %s", line_start);
            } else if (strstr(line_start, "[ERROR]")) {
                color_println(COLOR_RED, "  %s", line_start);
            } else if (strstr(line_start, "[FATAL]")) {
                color_println(COLOR_BRIGHT_RED, "  %s", line_start);
            } else {
                printf("  %s\n", line_start);
            }
            
            if (saved == '\0') break;
            *p = saved;
            line_start = p + 1;
        }
    }
    
    printf("  ─────────────────────────────────────────\n");
    printf("  %s: %d\n", lang_logs("total_lines"), line_count);
    
    free(content);
    menu_pause(NULL);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

int menu_main(void) {
    Config *cfg = config_get();
    const BoxChars *box = get_box_style();
    int width = (cfg->ui_menu_width > 0) ? cfg->ui_menu_width : 42;
    
    if (cfg->ui_clear_screen) {
        platform_console_clear();
    }
    
    printf("\n");
    print_box_line(box, width, true);
    
    char title[64];
    snprintf(title, sizeof(title), "%s v%s", lang_menu("title"), VERSION);
    print_centered_text(box, width, title);
    
    print_box_line(box, width, false);
    printf("\n");
    
    const char *e1 = cfg->show_emoji ? " 📄" : "";
    const char *e2 = cfg->show_emoji ? " 🎵" : "";
    const char *e3 = cfg->show_emoji ? " ⚙️" : "";
    const char *e4 = cfg->show_emoji ? " 📋" : "";
    const char *e0 = cfg->show_emoji ? " 🚪" : "";
    
    color_println(COLOR_WHITE, "  >> [1] %s%s", lang_menu("merge_text"), e1);
    color_println(COLOR_WHITE, "  >> [2] %s%s", lang_menu("merge_audio"), e2);
    color_println(COLOR_WHITE, "  >> [3] %s%s", lang_menu("settings"), e3);
    color_println(COLOR_WHITE, "  >> [4] %s%s", lang_menu("logs"), e4);
    color_println(COLOR_BRIGHT_RED, "  >> [0] %s%s", lang_menu("exit"), e0);
    printf("\n");
    
    char prompt[64];
    snprintf(prompt, sizeof(prompt), "  %s: >> ", lang_msg("select"));
    
    return input_read_choice(prompt);
}

void menu_settings(void) {
    Config *cfg = config_get();
    bool running = true;
    
    while (running) {
        if (cfg->ui_clear_screen) {
            platform_console_clear();
        }
        
        print_header(lang_settings("title"));
        
        printf("  1. %s\n", lang_settings("general"));
        printf("  2. %s\n", lang_settings("text_merger"));
        printf("  3. %s\n", lang_settings("audio_merger"));
        printf("  4. %s\n", lang_settings("ui"));
        printf("  5. %s\n", lang_settings("logging"));
        printf("  6. %s\n", lang_settings("paths"));
        printf("  ─────────────────\n");
        printf("  7. %s\n", lang_settings("save"));
        printf("  8. %s\n", lang_settings("reset"));
        printf("  0. %s\n\n", lang_msg("back"));
        
        char prompt[64];
        print_select_prompt(prompt, sizeof(prompt));
        int choice = input_read_choice(prompt);
        
        switch (choice) {
            case 1: settings_general(); break;
            case 2: settings_text_merger(); break;
            case 3: settings_audio_merger(); break;
            case 4: settings_ui(); break;
            case 5: settings_logging(); break;
            case 6: settings_paths(); break;
            case 7:
                if (config_save()) {
                    color_println(COLOR_GREEN, "\n  ✓ %s", lang_settings("saved"));
                    LOG_INFO("Settings saved to config file%s", "");
                } else {
                    color_println(COLOR_RED, "\n  ✗ %s", lang_settings("save_failed"));
                    LOG_ERROR("Failed to save settings%s", "");
                }
                menu_pause(NULL);
                break;
            case 8:
                if (menu_confirm(lang_settings("reset_confirm"))) {
                    config_create_default(cfg->config_path);
                    config_reload();
                    color_println(COLOR_GREEN, "\n  ✓ %s", lang_settings("reset_done"));
                    LOG_INFO("Settings reset to defaults%s", "");
                    menu_pause(NULL);
                }
                break;
            case 0:
            case -1:
                config_save();
                running = false;
                break;
        }
    }
}

void menu_logs(void) {
    Config *cfg = config_get();
    
    while (true) {
        if (cfg->ui_clear_screen) {
            platform_console_clear();
        }
        
        print_header(lang_logs("title"));
        
        char logs_dir[SPEECHER_PATH_MAX];
        fs_path_join(logs_dir, sizeof(logs_dir), app_get_base_dir(), cfg->logs_dir);
        
        printf("  %s: %s\n\n", lang_logs("directory"), logs_dir);
        
        FsFileList files;
        if (!fs_list_directory(logs_dir, &files, false, 1)) {
            color_println(COLOR_YELLOW, "  %s", lang_logs("no_files"));
            menu_pause(NULL);
            return;
        }
        
        size_t log_count = 0;
        FsFileInfo log_files[100];
        
        for (size_t i = 0; i < files.count && log_count < 100; i++) {
            if (str_ends_with(files.entries[i].name, ".log")) {
                log_files[log_count++] = files.entries[i];
            }
        }
        
        fs_list_free(&files);
        
        if (log_count == 0) {
            color_println(COLOR_YELLOW, "  %s", lang_logs("no_files"));
            menu_pause(NULL);
            return;
        }
        
        printf("  %s:\n", lang_logs("available_files"));
        for (size_t i = 0; i < log_count && i < 10; i++) {
            printf("    %zu. %s (%lu %s)\n", 
                   i + 1, log_files[i].name, (unsigned long)log_files[i].size, lang_msg("bytes"));
        }
        if (log_count > 10) {
            printf("    ... +%zu\n", log_count - 10);
        }
        printf("\n    0. %s\n\n", lang_msg("back"));
        
        char prompt[64];
        print_select_prompt(prompt, sizeof(prompt));
        int choice = input_read_choice(prompt);
        
        if (choice == 0 || choice == -1) {
            return;
        }
        
        if (choice >= 1 && (size_t)choice <= log_count) {
            view_log_file(log_files[choice - 1].path);
        } else {
            color_println(COLOR_RED, "  %s", lang_err("invalid_value"));
            platform_sleep_ms(1000);
        }
    }
}

bool menu_confirm(const char *message) {
    Config *cfg = config_get();
    char prompt[256];
    snprintf(prompt, sizeof(prompt), "  %s %s: ", 
             message, 
             input_get_yesno_hint(cfg->language));
    return input_read_bool_lang(prompt, cfg->language, false);
}

void menu_pause(const char *message) {
    char buffer[16];
    if (message) {
        printf("  %s", message);
    } else {
        printf("  %s", lang_msg("press_enter"));
    }
    input_read_line("", buffer, sizeof(buffer));
}