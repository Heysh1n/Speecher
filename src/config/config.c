/**
 * @file    config.c
 * @brief   Configuration system - Full implementation with i18n
 * @author  SPEECHER Team
 * @date    2025
 */

#include "config/config.h"
#include "config/validator.h"
#include "utils/strings.h"
#include "logging/logger.h"
#include "i18n/lang.h"
#include <string.h>
#include <stdio.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Global Config
 * ══════════════════════════════════════════════════════════════════════════════ */

static Config g_config = {0};
static bool g_config_loaded = false;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Defaults
 * ══════════════════════════════════════════════════════════════════════════════ */

static void config_set_defaults(void) {
    /* [general] */
    str_copy(g_config.language, "en", sizeof(g_config.language));
    g_config.show_emoji = false;
    
    /* [paths] */
    str_copy(g_config.input_dir, "data/input", sizeof(g_config.input_dir));
    str_copy(g_config.output_dir, "data/output", sizeof(g_config.output_dir));
    str_copy(g_config.logs_dir, "data/logs", sizeof(g_config.logs_dir));
    str_copy(g_config.lib_dir, "data/lib", sizeof(g_config.lib_dir));
    
    /* [text_merger] */
    str_copy(g_config.text_extensions, ".txt,.log,.csv,.md,.json,.xml,.ini",
             sizeof(g_config.text_extensions));
    str_copy(g_config.text_encoding, "UTF-8", sizeof(g_config.text_encoding));
    str_copy(g_config.text_sort_order, "name_asc", sizeof(g_config.text_sort_order));
    g_config.text_recursive = false;
    g_config.text_max_depth = 3;
    g_config.text_separator_enabled = true;
    str_copy(g_config.text_separator_style, "=", sizeof(g_config.text_separator_style));
    g_config.text_separator_length = 60;
    str_copy(g_config.text_separator_template, 
             "{separator}\n  File: {filename}\n{separator}\n",
             sizeof(g_config.text_separator_template));
    str_copy(g_config.text_post_action, "keep", sizeof(g_config.text_post_action));
    str_copy(g_config.text_output_format, ".txt", sizeof(g_config.text_output_format));
    str_copy(g_config.text_output_template, "merged_{date}_{time}",
             sizeof(g_config.text_output_template));
    
    /* [audio_merger] */
    str_copy(g_config.audio_extensions, ".wav",
             sizeof(g_config.audio_extensions));
    str_copy(g_config.audio_sort_order, "name_asc", sizeof(g_config.audio_sort_order));
    g_config.audio_recursive = false;
    g_config.audio_max_depth = 3;
    str_copy(g_config.audio_post_action, "keep", sizeof(g_config.audio_post_action));
    str_copy(g_config.audio_output_format, ".txt", sizeof(g_config.audio_output_format));
    str_copy(g_config.audio_output_template, "transcription_{date}_{time}",
             sizeof(g_config.audio_output_template));
    g_config.audio_timestamps = false;
    str_copy(g_config.audio_timestamp_format, "[MM:SS]",
             sizeof(g_config.audio_timestamp_format));
    
    /* [whisper] */
    str_copy(g_config.whisper_backend, "local", sizeof(g_config.whisper_backend));
    str_copy(g_config.whisper_model, "small", sizeof(g_config.whisper_model));
    str_copy(g_config.whisper_language, "auto", sizeof(g_config.whisper_language));
    g_config.whisper_threads = 0;
    str_copy(g_config.whisper_api_provider, "openai", sizeof(g_config.whisper_api_provider));
    g_config.whisper_api_key[0] = '\0';
    g_config.whisper_api_endpoint[0] = '\0';
    
    /* [logging] */
    g_config.log_enabled = true;
    str_copy(g_config.log_level, "INFO", sizeof(g_config.log_level));
    g_config.log_console_output = true;
    str_copy(g_config.log_console_level, "INFO", sizeof(g_config.log_console_level));
    g_config.log_colored = true;
    str_copy(g_config.log_color_debug, "cyan", sizeof(g_config.log_color_debug));
    str_copy(g_config.log_color_info, "green", sizeof(g_config.log_color_info));
    str_copy(g_config.log_color_warn, "yellow", sizeof(g_config.log_color_warn));
    str_copy(g_config.log_color_error, "red", sizeof(g_config.log_color_error));
    str_copy(g_config.log_color_fatal, "bright_red", sizeof(g_config.log_color_fatal));
    str_copy(g_config.log_rotation_mode, "date", sizeof(g_config.log_rotation_mode));
    g_config.log_max_size_mb = 10;
    str_copy(g_config.log_date_rotation, "daily", sizeof(g_config.log_date_rotation));
    g_config.log_max_files = 30;
    str_copy(g_config.log_template, "speecher_{date}.log", sizeof(g_config.log_template));
    
    /* [ui] */
    g_config.ui_show_progress = true;
    str_copy(g_config.ui_progress_style, "unicode", sizeof(g_config.ui_progress_style));
    g_config.ui_progress_width = 40;
    g_config.ui_clear_screen = true;
    str_copy(g_config.ui_menu_style, "double", sizeof(g_config.ui_menu_style));
    g_config.ui_menu_width = 48;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Load/Save Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

#define LOAD_STRING(section, key, field) \
    str_copy(g_config.field, \
             ini_get_string(g_config.ini, section, key, g_config.field), \
             sizeof(g_config.field))

#define LOAD_INT(section, key, field) \
    g_config.field = ini_get_int(g_config.ini, section, key, g_config.field)

#define LOAD_BOOL(section, key, field) \
    g_config.field = ini_get_bool(g_config.ini, section, key, g_config.field)

static void config_load_from_ini(void) {
    if (!g_config.ini) return;
    
    /* [general] */
    LOAD_STRING("general", "language", language);
    LOAD_BOOL("general", "show_emoji", show_emoji);
    
    /* [paths] */
    LOAD_STRING("paths", "input_dir", input_dir);
    LOAD_STRING("paths", "output_dir", output_dir);
    LOAD_STRING("paths", "logs_dir", logs_dir);
    LOAD_STRING("paths", "lib_dir", lib_dir);
    
    /* [text_merger] */
    LOAD_STRING("text_merger", "extensions", text_extensions);
    LOAD_STRING("text_merger", "encoding", text_encoding);
    LOAD_STRING("text_merger", "sort_order", text_sort_order);
    LOAD_BOOL("text_merger", "recursive", text_recursive);
    LOAD_INT("text_merger", "max_depth", text_max_depth);
    LOAD_BOOL("text_merger", "separator_enabled", text_separator_enabled);
    LOAD_STRING("text_merger", "separator_style", text_separator_style);
    LOAD_INT("text_merger", "separator_length", text_separator_length);
    LOAD_STRING("text_merger", "separator_template", text_separator_template);
    LOAD_STRING("text_merger", "post_action", text_post_action);
    LOAD_STRING("text_merger", "output_format", text_output_format);
    LOAD_STRING("text_merger", "output_template", text_output_template);
    
    /* [audio_merger] */
    LOAD_STRING("audio_merger", "extensions", audio_extensions);
    LOAD_STRING("audio_merger", "sort_order", audio_sort_order);
    LOAD_BOOL("audio_merger", "recursive", audio_recursive);
    LOAD_INT("audio_merger", "max_depth", audio_max_depth);
    LOAD_STRING("audio_merger", "post_action", audio_post_action);
    LOAD_STRING("audio_merger", "output_format", audio_output_format);
    LOAD_STRING("audio_merger", "output_template", audio_output_template);
    LOAD_BOOL("audio_merger", "include_timestamps", audio_timestamps);
    LOAD_STRING("audio_merger", "timestamp_format", audio_timestamp_format);
    
    /* [whisper] */
    LOAD_STRING("whisper", "backend", whisper_backend);
    LOAD_STRING("whisper", "model", whisper_model);
    LOAD_STRING("whisper", "language", whisper_language);
    LOAD_INT("whisper", "threads", whisper_threads);
    LOAD_STRING("whisper", "api_provider", whisper_api_provider);
    LOAD_STRING("whisper", "api_key", whisper_api_key);
    LOAD_STRING("whisper", "api_endpoint", whisper_api_endpoint);
    
    /* [logging] */
    LOAD_BOOL("logging", "enabled", log_enabled);
    LOAD_STRING("logging", "level", log_level);
    LOAD_BOOL("logging", "console_output", log_console_output);
    LOAD_STRING("logging", "console_level", log_console_level);
    LOAD_BOOL("logging", "colored_output", log_colored);
    LOAD_STRING("logging", "color_debug", log_color_debug);
    LOAD_STRING("logging", "color_info", log_color_info);
    LOAD_STRING("logging", "color_warn", log_color_warn);
    LOAD_STRING("logging", "color_error", log_color_error);
    LOAD_STRING("logging", "color_fatal", log_color_fatal);
    LOAD_STRING("logging", "rotation_mode", log_rotation_mode);
    LOAD_INT("logging", "max_size_mb", log_max_size_mb);
    LOAD_STRING("logging", "date_rotation", log_date_rotation);
    LOAD_INT("logging", "max_files", log_max_files);
    LOAD_STRING("logging", "log_template", log_template);
    
    /* [ui] */
    LOAD_BOOL("ui", "show_progress", ui_show_progress);
    LOAD_STRING("ui", "progress_style", ui_progress_style);
    LOAD_INT("ui", "progress_width", ui_progress_width);
    LOAD_BOOL("ui", "clear_screen", ui_clear_screen);
    LOAD_STRING("ui", "menu_style", ui_menu_style);
    LOAD_INT("ui", "menu_width", ui_menu_width);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

bool config_load(const char *filepath) {
    /* Set defaults first */
    config_set_defaults();
    
    /* Store config path */
    if (filepath) {
        str_copy(g_config.config_path, filepath, sizeof(g_config.config_path));
    }
    
    /* Load INI file */
    g_config.ini = ini_load(filepath);
    if (!g_config.ini) {
        /* Note: Can't use lang_* here because lang may not be initialized yet */
        LOG_WARN("Could not load config file: %s", filepath);
        return false;
    }
    
    /* Load values from INI */
    config_load_from_ini();
    
    /* Validate */
    int corrections = config_validate(&g_config);
    if (corrections > 0) {
        LOG_WARN("Config: %d values were corrected to defaults", corrections);
    }
    
    g_config_loaded = true;
    LOG_INFO("Config loaded: %s", filepath);
    return true;
}

bool config_save(void) {
    if (!g_config.ini || g_config.config_path[0] == '\0') {
        LOG_ERROR("Cannot save config: no INI or path");
        return false;
    }
    
    /* Update INI with current values */
    /* [general] */
    ini_set_string(g_config.ini, "general", "language", g_config.language);
    ini_set_bool(g_config.ini, "general", "show_emoji", g_config.show_emoji);
    
    /* [paths] */
    ini_set_string(g_config.ini, "paths", "input_dir", g_config.input_dir);
    ini_set_string(g_config.ini, "paths", "output_dir", g_config.output_dir);
    ini_set_string(g_config.ini, "paths", "logs_dir", g_config.logs_dir);
    ini_set_string(g_config.ini, "paths", "lib_dir", g_config.lib_dir);
    
    /* [text_merger] */
    ini_set_string(g_config.ini, "text_merger", "extensions", g_config.text_extensions);
    ini_set_string(g_config.ini, "text_merger", "encoding", g_config.text_encoding);
    ini_set_string(g_config.ini, "text_merger", "sort_order", g_config.text_sort_order);
    ini_set_bool(g_config.ini, "text_merger", "recursive", g_config.text_recursive);
    ini_set_int(g_config.ini, "text_merger", "max_depth", g_config.text_max_depth);
    ini_set_bool(g_config.ini, "text_merger", "separator_enabled", g_config.text_separator_enabled);
    ini_set_string(g_config.ini, "text_merger", "separator_style", g_config.text_separator_style);
    ini_set_int(g_config.ini, "text_merger", "separator_length", g_config.text_separator_length);
    ini_set_string(g_config.ini, "text_merger", "output_format", g_config.text_output_format);
    ini_set_string(g_config.ini, "text_merger", "output_template", g_config.text_output_template);
    
    /* [audio_merger] */
    ini_set_string(g_config.ini, "audio_merger", "extensions", g_config.audio_extensions);
    ini_set_string(g_config.ini, "audio_merger", "sort_order", g_config.audio_sort_order);
    ini_set_bool(g_config.ini, "audio_merger", "recursive", g_config.audio_recursive);
    ini_set_int(g_config.ini, "audio_merger", "max_depth", g_config.audio_max_depth);
    ini_set_string(g_config.ini, "audio_merger", "output_format", g_config.audio_output_format);
    ini_set_string(g_config.ini, "audio_merger", "output_template", g_config.audio_output_template);
    ini_set_bool(g_config.ini, "audio_merger", "include_timestamps", g_config.audio_timestamps);
    
    /* [whisper] */
    ini_set_string(g_config.ini, "whisper", "backend", g_config.whisper_backend);
    ini_set_string(g_config.ini, "whisper", "model", g_config.whisper_model);
    ini_set_string(g_config.ini, "whisper", "language", g_config.whisper_language);
    ini_set_int(g_config.ini, "whisper", "threads", g_config.whisper_threads);
    
    /* [logging] */
    ini_set_bool(g_config.ini, "logging", "enabled", g_config.log_enabled);
    ini_set_string(g_config.ini, "logging", "level", g_config.log_level);
    ini_set_bool(g_config.ini, "logging", "console_output", g_config.log_console_output);
    ini_set_string(g_config.ini, "logging", "console_level", g_config.log_console_level);
    ini_set_bool(g_config.ini, "logging", "colored_output", g_config.log_colored);
    ini_set_int(g_config.ini, "logging", "max_size_mb", g_config.log_max_size_mb);
    ini_set_int(g_config.ini, "logging", "max_files", g_config.log_max_files);
    
    /* [ui] */
    ini_set_bool(g_config.ini, "ui", "show_progress", g_config.ui_show_progress);
    ini_set_string(g_config.ini, "ui", "progress_style", g_config.ui_progress_style);
    ini_set_int(g_config.ini, "ui", "progress_width", g_config.ui_progress_width);
    ini_set_bool(g_config.ini, "ui", "clear_screen", g_config.ui_clear_screen);
    ini_set_string(g_config.ini, "ui", "menu_style", g_config.ui_menu_style);
    ini_set_int(g_config.ini, "ui", "menu_width", g_config.ui_menu_width);
    
    /* Save to file */
    bool result = ini_save(g_config.ini, g_config.config_path);
    if (result) {
        LOG_INFO("Config saved: %s", g_config.config_path);
    } else {
        LOG_ERROR("Failed to save config: %s", g_config.config_path);
    }
    
    return result;
}

Config *config_get(void) {
    if (!g_config_loaded) {
        config_set_defaults();
    }
    return &g_config;
}

void config_free(void) {
    if (g_config.ini) {
        ini_free(g_config.ini);
        g_config.ini = NULL;
    }
    g_config_loaded = false;
}

bool config_create_default(const char *filepath) {
    /* Set defaults */
    config_set_defaults();
    
    /* Create new INI */
    IniFile *ini = ini_create();
    if (!ini) return false;
    
    /* Add all default values */
    /* [general] */
    ini_set_string(ini, "general", "language", g_config.language);
    ini_set_bool(ini, "general", "show_emoji", g_config.show_emoji);
    
    /* [paths] */
    ini_set_string(ini, "paths", "input_dir", g_config.input_dir);
    ini_set_string(ini, "paths", "output_dir", g_config.output_dir);
    ini_set_string(ini, "paths", "logs_dir", g_config.logs_dir);
    ini_set_string(ini, "paths", "lib_dir", g_config.lib_dir);
    
    /* [text_merger] */
    ini_set_string(ini, "text_merger", "extensions", g_config.text_extensions);
    ini_set_string(ini, "text_merger", "encoding", g_config.text_encoding);
    ini_set_string(ini, "text_merger", "sort_order", g_config.text_sort_order);
    ini_set_bool(ini, "text_merger", "recursive", g_config.text_recursive);
    ini_set_int(ini, "text_merger", "max_depth", g_config.text_max_depth);
    ini_set_bool(ini, "text_merger", "separator_enabled", g_config.text_separator_enabled);
    ini_set_string(ini, "text_merger", "separator_style", g_config.text_separator_style);
    ini_set_int(ini, "text_merger", "separator_length", g_config.text_separator_length);
    ini_set_string(ini, "text_merger", "separator_template", g_config.text_separator_template);
    ini_set_string(ini, "text_merger", "post_action", g_config.text_post_action);
    ini_set_string(ini, "text_merger", "output_format", g_config.text_output_format);
    ini_set_string(ini, "text_merger", "output_template", g_config.text_output_template);
    
    /* [audio_merger] */
    ini_set_string(ini, "audio_merger", "extensions", g_config.audio_extensions);
    ini_set_string(ini, "audio_merger", "sort_order", g_config.audio_sort_order);
    ini_set_bool(ini, "audio_merger", "recursive", g_config.audio_recursive);
    ini_set_int(ini, "audio_merger", "max_depth", g_config.audio_max_depth);
    ini_set_string(ini, "audio_merger", "post_action", g_config.audio_post_action);
    ini_set_string(ini, "audio_merger", "output_format", g_config.audio_output_format);
    ini_set_string(ini, "audio_merger", "output_template", g_config.audio_output_template);
    ini_set_bool(ini, "audio_merger", "include_timestamps", g_config.audio_timestamps);
    ini_set_string(ini, "audio_merger", "timestamp_format", g_config.audio_timestamp_format);
    
    /* [whisper] */
    ini_set_string(ini, "whisper", "backend", g_config.whisper_backend);
    ini_set_string(ini, "whisper", "model", g_config.whisper_model);
    ini_set_string(ini, "whisper", "language", g_config.whisper_language);
    ini_set_int(ini, "whisper", "threads", g_config.whisper_threads);
    ini_set_string(ini, "whisper", "api_provider", g_config.whisper_api_provider);
    ini_set_string(ini, "whisper", "api_key", g_config.whisper_api_key);
    ini_set_string(ini, "whisper", "api_endpoint", g_config.whisper_api_endpoint);
    
    /* [logging] */
    ini_set_bool(ini, "logging", "enabled", g_config.log_enabled);
    ini_set_string(ini, "logging", "level", g_config.log_level);
    ini_set_bool(ini, "logging", "console_output", g_config.log_console_output);
    ini_set_string(ini, "logging", "console_level", g_config.log_console_level);
    ini_set_bool(ini, "logging", "colored_output", g_config.log_colored);
    ini_set_string(ini, "logging", "rotation_mode", g_config.log_rotation_mode);
    ini_set_int(ini, "logging", "max_size_mb", g_config.log_max_size_mb);
    ini_set_int(ini, "logging", "max_files", g_config.log_max_files);
    
    /* [ui] */
    ini_set_bool(ini, "ui", "show_progress", g_config.ui_show_progress);
    ini_set_string(ini, "ui", "progress_style", g_config.ui_progress_style);
    ini_set_int(ini, "ui", "progress_width", g_config.ui_progress_width);
    ini_set_bool(ini, "ui", "clear_screen", g_config.ui_clear_screen);
    ini_set_string(ini, "ui", "menu_style", g_config.ui_menu_style);
    ini_set_int(ini, "ui", "menu_width", g_config.ui_menu_width);

    /* Save */
    bool result = ini_save(ini, filepath);
    ini_free(ini);
    
    if (result) {
        LOG_INFO("Default config created: %s", filepath);
    }
    
    return result;
}

bool config_reload(void) {
    LOG_INFO("Reloading config...");
    return config_load(g_config.config_path);
}