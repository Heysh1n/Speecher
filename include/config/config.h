/**
 * @file    config.h
 * @brief   Application configuration
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include <stdbool.h>
#include "config/ini_parser.h"

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Configuration Structure
 * ══════════════════════════════════════════════════════════════════════════════ */

/** Configuration data structure */
typedef struct {
    /* [general] */
    char language[16];
    bool show_emoji;
    
    /* [paths] */
    char input_dir[4096];
    char output_dir[4096];
    char logs_dir[4096];
    char lib_dir[4096];
    
    /* [text_merger] */
    char text_extensions[512];
    char text_encoding[32];
    char text_sort_order[32];
    bool text_recursive;
    int text_max_depth;
    bool text_separator_enabled;
    char text_separator_style[8];
    int text_separator_length;
    char text_separator_template[512];
    char text_post_action[16];
    char text_output_format[16];
    char text_output_template[256];
    
    /* [audio_merger] */
    char audio_extensions[256];
    char audio_sort_order[32];
    bool audio_recursive;
    int audio_max_depth;
    char audio_post_action[16];
    char audio_output_format[16];
    char audio_output_template[256];
    bool audio_timestamps;
    char audio_timestamp_format[32];
    
    /* [whisper] */
    char whisper_backend[16];
    char whisper_model[16];
    char whisper_language[16];
    int whisper_threads;
    char whisper_api_provider[32];
    char whisper_api_key[256];
    char whisper_api_endpoint[512];
    
    /* [logging] */
    bool log_enabled;
    char log_level[16];
    bool log_console_output;
    char log_console_level[16];
    bool log_colored;
    char log_color_debug[16];
    char log_color_info[16];
    char log_color_warn[16];
    char log_color_error[16];
    char log_color_fatal[16];
    char log_rotation_mode[16];
    int log_max_size_mb;
    char log_date_rotation[16];
    int log_max_files;
    char log_template[256];
    
    /* [ui] */
    bool ui_show_progress;
    char ui_progress_style[16];
    int ui_progress_width;
    bool ui_clear_screen;
    char ui_menu_style[16];
    int ui_menu_width;
    
    /* Internal */
    IniFile *ini;
    char config_path[4096];

} Config;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              API
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Load configuration from file
 * @param filepath Path to config file (NULL = default path)
 * @return true on success
 */
bool config_load(const char *filepath);

/**
 * @brief Save current configuration to file
 * @return true on success
 */
bool config_save(void);

/**
 * @brief Get global config instance
 * @return Pointer to config (never NULL after config_load)
 */
Config *config_get(void);

/**
 * @brief Free configuration
 */
void config_free(void);

/**
 * @brief Create default configuration file
 * @param filepath Path to create file
 * @return true on success
 */
bool config_create_default(const char *filepath);

/**
 * @brief Reload configuration from disk
 * @return true on success
 */
bool config_reload(void);

#endif /* CONFIG_CONFIG_H */