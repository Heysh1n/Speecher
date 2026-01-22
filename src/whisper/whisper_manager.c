/**
 * @file    whisper_manager.c
 * @brief   Whisper integration - Main API (Localized)
 */

#include "whisper/whisper_manager.h"
#include "whisper/whisper_internal.h"
#include "config/config.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "utils/input.h"
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
    printf("  | # | %-6s | %8s | %-6s |\n", 
           lang_whisper("table_model"), 
           lang_whisper("table_size"), 
           lang_whisper("table_ram"));
    printf("  +---+--------+----------+--------+\n");
    
    for (int i = 0; i < WHISPER_MODEL_COUNT; i++) {
        const WhisperModel *m = &WHISPER_MODELS[i];
        bool is_current = current_model && (strcmp(m->name, current_model) == 0);
        
        char size_str[16];
        if (m->size_mb >= 1000) {
            snprintf(size_str, sizeof(size_str), "%.1f %s", 
                     m->size_mb / 1000.0, lang_size("gb"));
        } else {
            snprintf(size_str, sizeof(size_str), "%d %s", 
                     m->size_mb, lang_size("mb"));
        }
        
        if (is_current) {
            color_print(COLOR_GREEN, "  | %d | %-6s | %8s | ~%2d%s  | <- %s\n",
                       i + 1, m->name, size_str, m->ram_gb, 
                       lang_size("gb"), lang_msg("current"));
        } else {
            printf("  | %d | %-6s | %8s | ~%2d%s  |\n",
                   i + 1, m->name, size_str, m->ram_gb, lang_size("gb"));
        }
    }
    
    printf("  +---+--------+----------+--------+\n");
    printf("\n");
    printf("  %s: small (%s)\n", lang_msg("recommended"), lang_whisper("quality_good"));
    printf("\n");
}

static const WhisperModel *select_model_interactive(void) {
    Config *cfg = config_get();
    
    print_models_table(cfg->whisper_model);
    
    char prompt[128];
    snprintf(prompt, sizeof(prompt), "  %s (1-5) [Enter = %s]: ", 
             lang_whisper("select_model"), cfg->whisper_model);
    
    int choice = input_read_int(prompt, 0);
    
    /* 0 or invalid = keep current */
    if (choice < 1 || choice > WHISPER_MODEL_COUNT) {
        return whisper_model_get(cfg->whisper_model);
    }
    
    const WhisperModel *selected = &WHISPER_MODELS[choice - 1];
    
    /* Save selection */
    str_copy(cfg->whisper_model, selected->name, sizeof(cfg->whisper_model));
    config_save();
    
    printf("\n  %s: %s\n", lang_msg("select"), selected->name);
    return selected;
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
    color_println(COLOR_CYAN, "              %s", lang_whisper("title"));
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 1: SELECT MODEL
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  %s\n", lang_whisper("step_model"));
    printf("  ─────────────────────\n");
    
    const WhisperModel *model = select_model_interactive();
    
    printf("\n  %s: ", lang_whisper("using_model"));
    color_print(COLOR_GREEN, "%s", model->name);
    printf(" (%d %s, ~%d %s %s)\n\n", 
           model->size_mb, lang_size("mb"),
           model->ram_gb, lang_size("gb"), lang_whisper("table_ram"));
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 2: INSTALL ENGINE
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  %s\n", lang_whisper("step_engine"));
    printf("  ───────────────────────────────\n\n");
    
    /* Refresh paths after model selection */
    paths_rebuild();
    
    if (!fs_exists(PATH_WHISPER_EXE)) {
        printf("  %s\n", lang_whisper("engine_not_found"));
        printf("  %s: %s\n\n", lang_whisper("path"), PATH_WHISPER_DIR);
        
        if (!menu_confirm(lang_whisper("install_engine"))) {
            printf("\n  %s\n", lang_msg("cancelled"));
            menu_pause(NULL);
            return false;
        }
        
        if (!whisper_install_binary()) {
            printf("\n");
            color_println(COLOR_RED, "  %s", lang_whisper("install_failed"));
            printf("\n");
            whisper_diagnose();
            menu_pause(NULL);
            return false;
        }
        
        /* Refresh paths after installation */
        paths_rebuild();
    } else {
        color_println(COLOR_GREEN, "  [OK] %s", lang_whisper("engine_found"));
        printf("    %s\n\n", PATH_WHISPER_EXE);
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 3: DOWNLOAD MODEL
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  %s\n", lang_whisper("step_download"));
    printf("  ───────────────────────\n\n");
    
    const char *model_path = paths_find_model(cfg->whisper_model);
    
    if (!model_path) {
        printf("  %s '%s'.\n\n", lang_whisper("model_not_found"), cfg->whisper_model);
        
        if (!menu_confirm(lang_whisper("download_model"))) {
            printf("\n  %s\n", lang_msg("cancelled"));
            menu_pause(NULL);
            return false;
        }
        
        if (!whisper_install_model(model)) {
            printf("\n");
            color_println(COLOR_RED, "  %s", lang_whisper("download_failed"));
            menu_pause(NULL);
            return false;
        }
    } else {
        color_println(COLOR_GREEN, "  [OK] %s: %s", lang_whisper("model_found"), cfg->whisper_model);
        
        int64_t size = fs_file_size(model_path);
        printf("    %s: %.1f %s\n\n", lang_whisper("table_size"), 
               size / 1048576.0, lang_size("mb"));
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         STEP 4: CHECK FFMPEG
     * ═══════════════════════════════════════════════════════════════════════ */
    
    printf("  %s\n", lang_whisper("step_dependencies"));
    printf("  ───────────────────────────\n\n");
    
    if (whisper_has_ffmpeg()) {
        color_println(COLOR_GREEN, "  [OK] %s", lang_whisper("ffmpeg_found"));
    } else {
        color_println(COLOR_YELLOW, "  [!] %s", lang_whisper("ffmpeg_not_found"));
        printf("    %s\n", lang_whisper("ffmpeg_hint"));
        printf("    %s:\n", lang_whisper("install_ffmpeg"));
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
    color_println(COLOR_GREEN, "              %s", lang_whisper("ready"));
    color_println(COLOR_GREEN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    printf("  %s:\n", lang_whisper("summary"));
    printf("    %s:   %s\n", lang_whisper("table_model"), cfg->whisper_model);
    printf("    %s:  %s\n", lang_whisper("engine"), PATH_WHISPER_EXE);
    printf("    ffmpeg:  %s\n", whisper_has_ffmpeg() ? lang_msg("yes") : lang_whisper("wav_only"));
    printf("\n");
    
    menu_pause(NULL);
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Diagnostics
 * ══════════════════════════════════════════════════════════════════════════════ */

void whisper_diagnose(void) {
    printf("\n");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    color_println(COLOR_CYAN, "              %s", lang_whisper("diagnostics"));
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════════════");
    printf("\n");
    
    /* Paths */
    printf("  %s:\n", lang_whisper("paths"));
    printf("  ──────\n");
    printf("    Whisper dir:   %s\n", PATH_WHISPER_DIR);
    printf("      %s: %s\n", lang_msg("found"), fs_exists(PATH_WHISPER_DIR) ? lang_msg("yes") : lang_msg("no"));
    printf("    Expected exe:  %s\n", PATH_WHISPER_EXE);
    printf("      %s: %s\n", lang_msg("found"), fs_exists(PATH_WHISPER_EXE) ? lang_msg("yes") : lang_msg("no"));
    printf("    Models dir:    %s\n", PATH_WHISPER_MODELS_DIR);
    printf("      %s: %s\n", lang_msg("found"), fs_exists(PATH_WHISPER_MODELS_DIR) ? lang_msg("yes") : lang_msg("no"));
    printf("\n");
    
    /* List files in whisper directory */
    if (fs_exists(PATH_WHISPER_DIR)) {
        printf("  %s:\n", lang_whisper("files_in_dir"));
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
    printf("  %s:\n", lang_whisper("available_models"));
    printf("  ─────────────────\n");
    
    for (int i = 0; i < WHISPER_MODEL_COUNT; i++) {
        char model_path[SPEECHER_PATH_MAX];
        paths_build_model(model_path, sizeof(model_path), WHISPER_MODELS[i].name);
        
        if (fs_exists(model_path)) {
            int64_t size = fs_file_size(model_path);
            color_print(COLOR_GREEN, "    [OK] %s (%.1f %s)\n", 
                       WHISPER_MODELS[i].name, size / 1048576.0, lang_size("mb"));
        } else {
            printf("    [ ] %s (%s)\n", WHISPER_MODELS[i].name, lang_whisper("not_downloaded"));
        }
    }
    printf("\n");
    
    /* System tools */
    printf("  %s:\n", lang_whisper("system_tools"));
    printf("  ─────────────\n");
    printf("    ffmpeg:   %s\n", whisper_cmd_exists("ffmpeg") ? lang_msg("found") : lang_msg("not_found"));
    printf("    curl:     %s\n", whisper_cmd_exists("curl") ? lang_msg("found") : lang_msg("not_found"));
#ifdef WINDOWS
    printf("    tar:      %s\n", whisper_cmd_exists("tar") ? lang_msg("found") : lang_msg("not_found"));
    printf("    powershell: %s\n", whisper_cmd_exists("powershell") ? lang_msg("found") : lang_msg("not_found"));
#else
    printf("    wget:     %s\n", whisper_cmd_exists("wget") ? lang_msg("found") : lang_msg("not_found"));
    printf("    git:      %s\n", whisper_cmd_exists("git") ? lang_msg("found") : lang_msg("not_found"));
    printf("    make:     %s\n", whisper_cmd_exists("make") ? lang_msg("found") : lang_msg("not_found"));
    printf("    gcc:      %s\n", whisper_cmd_exists("gcc") ? lang_msg("found") : lang_msg("not_found"));
#endif
}