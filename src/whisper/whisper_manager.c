/**
 * @file    whisper_manager.c
 * @brief   Whisper integration - Main API
 */

#include "whisper/whisper_manager.h"
#include "whisper/whisper_internal.h"
#include "config/config.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "ui/colors.h"
#include "ui/menu.h"
#include "i18n/lang.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Model Data
 * ══════════════════════════════════════════════════════════════════════════════ */

const WhisperModel WHISPER_MODELS[WHISPER_MODEL_COUNT + 1] = {
    {"tiny",   "ggml-tiny.bin",       75,  1},
    {"base",   "ggml-base.bin",      142,  1},
    {"small",  "ggml-small.bin",     466,  2},
    {"medium", "ggml-medium.bin",   1500,  5},
    {"large",  "ggml-large-v3.bin", 2900, 10},
    {NULL, NULL, 0, 0}
};

const WhisperModel *whisper_model_get(const char *name) {
    if (!name) return &WHISPER_MODELS[2];
    
    for (int i = 0; i < WHISPER_MODEL_COUNT; i++) {
        if (strcmp(WHISPER_MODELS[i].name, name) == 0) {
            return &WHISPER_MODELS[i];
        }
    }
    return &WHISPER_MODELS[2]; /* small */
}

const WhisperModelInfo *whisper_get_models(int *count) {
    if (count) *count = WHISPER_MODEL_COUNT;
    return (const WhisperModelInfo *)WHISPER_MODELS;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Model Selection UI
 * ══════════════════════════════════════════════════════════════════════════════ */

static void print_models_table(const char *current_model) {
    printf("\n");
    printf("  +---+--------+----------+--------+\n");
    printf("  | # | Model  |   Size   |  RAM   |\n");
    printf("  +---+--------+----------+--------+\n");
    
    for (int i = 0; i < WHISPER_MODEL_COUNT; i++) {
        const WhisperModel *m = &WHISPER_MODELS[i];
        bool is_current = current_model && (strcmp(m->name, current_model) == 0);
        
        char size_str[16];
        if (m->size_mb >= 1000) {
            snprintf(size_str, sizeof(size_str), "%.1f GB", m->size_mb / 1000.0);
        } else {
            snprintf(size_str, sizeof(size_str), "%d MB", m->size_mb);
        }
        
        if (is_current) {
            color_print(COLOR_GREEN, "  | %d | %-6s | %8s | ~%2dGB  | <- current\n",
                       i + 1, m->name, size_str, m->ram_gb);
        } else {
            printf("  | %d | %-6s | %8s | ~%2dGB  |\n",
                   i + 1, m->name, size_str, m->ram_gb);
        }
    }
    
    printf("  +---+--------+----------+--------+\n");
    printf("\n");
    printf("  Recommended: small (good quality, moderate size)\n");
    printf("\n");
}

static const WhisperModel *select_model_interactive(void) {
    Config *cfg = config_get();
    
    print_models_table(cfg->whisper_model);
    
    printf("  Select model (1-5) [Enter = %s]: ", cfg->whisper_model);
    fflush(stdout);
    
    char input[16] = {0};
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return whisper_model_get(cfg->whisper_model);
    }
    
    /* Remove newline */
    size_t len = strlen(input);
    if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';
    
    /* Empty = keep current */
    if (input[0] == '\0') {
        return whisper_model_get(cfg->whisper_model);
    }
    
    int choice = atoi(input);
    if (choice >= 1 && choice <= WHISPER_MODEL_COUNT) {
        const WhisperModel *selected = &WHISPER_MODELS[choice - 1];
        
        /* Save selection */
        str_copy(cfg->whisper_model, selected->name, sizeof(cfg->whisper_model));
        config_save();
        
        printf("  Selected: %s\n", selected->name);
        return selected;
    }
    
    return whisper_model_get(cfg->whisper_model);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Status
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_is_available(void) {
    return paths_whisper_installed();
}

bool whisper_check(void) {
    if (!paths_is_init()) return false;
    
    if (!fs_exists(PATH_WHISPER_EXE)) {
        LOG_DEBUG("Whisper binary not found: %s", PATH_WHISPER_EXE);
        return false;
    }
    
    Config *cfg = config_get();
    if (!paths_find_model(cfg->whisper_model)) {
        LOG_DEBUG("Model not found: %s", cfg->whisper_model);
        return false;
    }
    
    return true;
}

bool whisper_has_ffmpeg(void) {
    return whisper_cmd_exists("ffmpeg");
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Setup
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_install(void) {
    return whisper_install_binary();
}

bool whisper_download_model(const char *model_name) {
    const WhisperModel *model = whisper_model_get(model_name);
    return whisper_install_model(model);
}

bool whisper_setup(void) {
    Config *cfg = config_get();
    
    printf("\n");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    color_println(COLOR_CYAN, "              WHISPER SETUP WIZARD");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 1: SELECT MODEL
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  STEP 1: Select Model\n");
    printf("  ─────────────────────\n");
    
    const WhisperModel *model = select_model_interactive();
    
    printf("\n  Using model: ");
    color_print(COLOR_GREEN, "%s", model->name);
    printf(" (%d MB, ~%d GB RAM)\n\n", model->size_mb, model->ram_gb);
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 2: INSTALL ENGINE
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  STEP 2: Install Whisper Engine\n");
    printf("  ───────────────────────────────\n\n");
    
    /* Refresh paths after model selection */
    paths_rebuild();
    
    if (!fs_exists(PATH_WHISPER_EXE)) {
        printf("  Whisper engine not found.\n");
        printf("  Path: %s\n\n", PATH_WHISPER_DIR);
        
        if (!menu_confirm("  Install Whisper engine now?")) {
            printf("\n  Setup cancelled.\n");
            menu_pause(lang_msg("press_enter"));
            return false;
        }
        
        if (!whisper_install_binary()) {
            printf("\n");
            color_println(COLOR_RED, "  Installation failed!");
            printf("\n");
            whisper_diagnose();
            menu_pause(lang_msg("press_enter"));
            return false;
        }
        
        /* Refresh paths after installation */
        paths_rebuild();
    } else {
        color_println(COLOR_GREEN, "  [OK] Whisper engine found");
        printf("    %s\n\n", PATH_WHISPER_EXE);
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 3: DOWNLOAD MODEL
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  STEP 3: Download Model\n");
    printf("  ───────────────────────\n\n");
    
    const char *model_path = paths_find_model(cfg->whisper_model);
    
    if (!model_path) {
        printf("  Model '%s' not found.\n\n", cfg->whisper_model);
        
        if (!menu_confirm("  Download model now?")) {
            printf("\n  Setup cancelled. You can download later.\n");
            menu_pause(lang_msg("press_enter"));
            return false;
        }
        
        if (!whisper_install_model(model)) {
            printf("\n");
            color_println(COLOR_RED, "  Model download failed!");
            menu_pause(lang_msg("press_enter"));
            return false;
        }
    } else {
        color_println(COLOR_GREEN, "  [OK] Model found: %s", cfg->whisper_model);
        
        int64_t size = fs_file_size(model_path);
        printf("    Size: %.1f MB\n\n", size / 1048576.0);
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 4: CHECK FFMPEG
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  STEP 4: Check Dependencies\n");
    printf("  ───────────────────────────\n\n");
    
    if (whisper_has_ffmpeg()) {
        color_println(COLOR_GREEN, "  [OK] ffmpeg found - all audio formats supported");
    } else {
        color_println(COLOR_YELLOW, "  [!] ffmpeg not found");
        printf("    Only WAV files will work.\n");
        printf("    Install ffmpeg for MP3, OGG, FLAC support:\n");
#ifdef WINDOWS
        printf("      winget install ffmpeg\n");
#elif defined(__APPLE__)
        printf("      brew install ffmpeg\n");
#else
        printf("      sudo apt install ffmpeg\n");
#endif
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         SUCCESS
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("\n");
    color_println(COLOR_GREEN, "  ═══════════════════════════════════════════════");
    color_println(COLOR_GREEN, "              WHISPER SETUP COMPLETE!");
    color_println(COLOR_GREEN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    printf("  Summary:\n");
    printf("    Model:   %s\n", cfg->whisper_model);
    printf("    Engine:  %s\n", PATH_WHISPER_EXE);
    printf("    ffmpeg:  %s\n", whisper_has_ffmpeg() ? "Yes" : "No (only WAV)");
    printf("\n");
    
    menu_pause(lang_msg("press_enter"));
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Diagnostics
 * ══════════════════════════════════════════════════════════════════════════════ */

void whisper_diagnose(void) {
    printf("\n");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    color_println(COLOR_CYAN, "              WHISPER DIAGNOSTICS");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    /* Paths */
    printf("  Paths:\n");
    printf("  ──────\n");
    printf("    Whisper dir:   %s\n", PATH_WHISPER_DIR);
    printf("      Exists: %s\n", fs_exists(PATH_WHISPER_DIR) ? "YES" : "NO");
    printf("    Expected exe:  %s\n", PATH_WHISPER_EXE);
    printf("      Exists: %s\n", fs_exists(PATH_WHISPER_EXE) ? "YES" : "NO");
    printf("    Models dir:    %s\n", PATH_WHISPER_MODELS_DIR);
    printf("      Exists: %s\n", fs_exists(PATH_WHISPER_MODELS_DIR) ? "YES" : "NO");
    printf("\n");
    
    /* List files in whisper directory */
    if (fs_exists(PATH_WHISPER_DIR)) {
        printf("  Files in Whisper directory:\n");
        printf("  ────────────────────────────\n");
        
#ifdef WINDOWS
        char cmd[SPEECHER_PATH_MAX + 64];
        snprintf(cmd, sizeof(cmd), "dir /s /b \"%s\" 2>nul", PATH_WHISPER_DIR);
        system(cmd);
#else
        char cmd[SPEECHER_PATH_MAX + 64];
        snprintf(cmd, sizeof(cmd), "find \"%s\" -type f 2>/dev/null | head -20", PATH_WHISPER_DIR);
        system(cmd);
#endif
        printf("\n");
    }
    
    /* Check models */
    printf("  Available Models:\n");
    printf("  ─────────────────\n");
    
    for (int i = 0; i < WHISPER_MODEL_COUNT; i++) {
        char model_path[SPEECHER_PATH_MAX];
        paths_build_model(model_path, sizeof(model_path), WHISPER_MODELS[i].name);
        
        if (fs_exists(model_path)) {
            int64_t size = fs_file_size(model_path);
            color_print(COLOR_GREEN, "    [OK] %s (%.1f MB)\n", 
                       WHISPER_MODELS[i].name, size / 1048576.0);
        } else {
            printf("    [ ] %s (not downloaded)\n", WHISPER_MODELS[i].name);
        }
    }
    printf("\n");
    
    /* System tools */
    printf("  System Tools:\n");
    printf("  ─────────────\n");
    printf("    ffmpeg:   %s\n", whisper_cmd_exists("ffmpeg") ? "Found" : "Not found");
    printf("    curl:     %s\n", whisper_cmd_exists("curl") ? "Found" : "Not found");
#ifdef WINDOWS
    printf("    tar:      %s\n", whisper_cmd_exists("tar") ? "Found" : "Not found");
    printf("    powershell: %s\n", whisper_cmd_exists("powershell") ? "Found" : "Not found");
#else
    printf("    wget:     %s\n", whisper_cmd_exists("wget") ? "Found" : "Not found");
    printf("    git:      %s\n", whisper_cmd_exists("git") ? "Found" : "Not found");
    printf("    make:     %s\n", whisper_cmd_exists("make") ? "Found" : "Not found");
    printf("    gcc:      %s\n", whisper_cmd_exists("gcc") ? "Found" : "Not found");
#endif
    printf("\n");
}