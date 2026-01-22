/**
 * @file    app.c
 * @brief   Application lifecycle
 */

#include "core/app.h"
#include "core/merger_text.h"
#include "core/merger_audio.h"
#include "config/config.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "utils/platform.h"
#include "utils/input.h"
#include "ui/colors.h"
#include "ui/menu.h"
#include "logging/logger.h"
#include "i18n/lang.h"
#include "speecher.h"
#include <stdio.h>
#include <string.h>

static bool g_initialized = false;

static void print_banner(void) {
    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║         SPEECHER v%-18s║\n", SPEECHER_VERSION);
    printf("  ╚══════════════════════════════════════╝\n");
    printf("\n");
}

bool app_init(int argc, char *argv[]) {
    (void)argc;
    
    /* Platform init */
    platform_console_init();
    input_init();
    
    /* Initialize paths */
    const char *exe = (argv && argv[0]) ? argv[0] : ".";
    if (!paths_init(exe)) {
        fprintf(stderr, "Error: Failed to init paths\n");
        return false;
    }
    
    /* Banner */
    print_banner();
    
#ifdef DEBUG
    printf("  DEBUG MODE\n");
    printf("  Base:   %s\n", PATH_BASE_DIR);
    printf("  Data:   %s\n", PATH_DATA_DIR);
    printf("  Input:  %s\n", PATH_INPUT_DIR);
    printf("  Output: %s\n", PATH_OUTPUT_DIR);
    printf("\n");
#endif
    
    /* Create directories */
    if (!paths_ensure_dirs()) {
        fprintf(stderr, "Error: Failed to create directories\n");
        return false;
    }
    
    /* Colors */
    colors_init(platform_console_colors_supported());
    
    /* Logger */
    if (!logger_init(PATH_LOGS_DIR, true)) {
        fprintf(stderr, "Warning: Logger init failed\n");
    }
    
    LOG_INFO("SPEECHER v%s starting", SPEECHER_VERSION);
    
    /* Config */
    if (!fs_exists(PATH_CONFIG_FILE)) {
        LOG_INFO("Creating default config: %s", PATH_CONFIG_FILE);
        config_create_default(PATH_CONFIG_FILE);
    }
    
    if (!config_load(PATH_CONFIG_FILE)) {
        LOG_WARN("Config load failed, using defaults");
    }
    
    Config *cfg = config_get();
    
    /* Language */
    if (!lang_init(cfg->language)) {
        LOG_WARN("Language '%s' failed, using English", cfg->language);
        lang_init("en");
    }
    

    lang_set_emoji(cfg->show_emoji);

    
    /* Re-init colors with config */
    colors_init(cfg->log_colored && platform_console_colors_supported());
    
    g_initialized = true;
    LOG_INFO("Initialization complete");
    
    /* Show paths info */
    printf("  Input folder:  %s\n", PATH_INPUT_DIR);
    printf("  Output folder: %s\n", PATH_OUTPUT_DIR);
    printf("\n");
    
    return true;
}

int app_run(void) {
    if (!g_initialized) {
        fprintf(stderr, "Error: Not initialized\n");
        return 1;
    }
    
    Config *cfg = config_get();
    bool running = true;
    
    while (running) {
        if (cfg->ui_clear_screen) {
            platform_console_clear();
        }
        
        int choice = menu_main();
        
        switch (choice) {
            case 1:
                LOG_INFO("User selected: Merge text files");
                merger_text_run();
                break;
            case 2:
                LOG_INFO("User selected: Merge audio files");
                merger_audio_run();
                break;
            case 3:
                LOG_INFO("User selected: Settings");
                menu_settings();
                lang_init(cfg->language);
#ifdef WINDOWS
                lang_set_emoji(false);
#else
                lang_set_emoji(cfg->show_emoji);
#endif
                break;
            case 4:
                LOG_INFO("User selected: Logs");
                menu_logs();
                break;
            case 0:
                LOG_INFO("User selected: Exit");
                running = false;
                break;
            default:
                color_println(COLOR_RED, "  Invalid option");
                platform_sleep_ms(1000);
                break;
        }
    }
    
    return 0;
}

void app_shutdown(void) {
    if (!g_initialized) return;
    
    LOG_INFO("Application shutting down");
    input_cleanup();
    config_save();
    config_free();
    lang_free();
    logger_shutdown();
    g_initialized = false;
}

bool app_is_initialized(void) { return g_initialized; }
const char *app_get_base_dir(void) { return PATH_BASE_DIR; }
const char *app_get_data_dir(void) { return PATH_DATA_DIR; }
const char *app_get_input_dir(void) { return PATH_INPUT_DIR; }
const char *app_get_output_dir(void) { return PATH_OUTPUT_DIR; }
const char *app_get_logs_dir(void) { return PATH_LOGS_DIR; }
const char *app_get_config_path(void) { return PATH_CONFIG_FILE; }