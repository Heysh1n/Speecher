/**
 * @file    paths.c
 * @brief   Centralized path management
 */

#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Constants
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifdef WINDOWS
#define PATH_SEP '\\'
#define PATH_SEP_OTHER '/'
#else
#define PATH_SEP '/'
#define PATH_SEP_OTHER '\\'
#endif

#define DIR_DATA    "data"
#define DIR_INPUT   "input"
#define DIR_OUTPUT  "output"
#define DIR_LOGS    "logs"
#define DIR_LANG    "lang"
#define DIR_LIB     "lib"
#define DIR_WHISPER "whisper"
#define DIR_MODELS  "models"

#define FILE_CONFIG     "config.ini"
#define FILE_LANG_PACK  "SPEECHER_lang-pack.zip"

#ifdef WINDOWS
#define WHISPER_BIN     "whisper-cli.exe"
#define WHISPER_BIN_ALT "main.exe"
#else
#define WHISPER_BIN     "whisper-cli"
#define WHISPER_BIN_ALT "main"
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              State
 * ══════════════════════════════════════════════════════════════════════════════ */

static char g_paths[PATH_COUNT][SPEECHER_PATH_MAX] = {{0}};
static bool g_initialized = false;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Normalization
 * ══════════════════════════════════════════════════════════════════════════════ */

void paths_normalize(char *path) {
    if (!path) return;
    
    for (char *p = path; *p; p++) {
        if (*p == PATH_SEP_OTHER) {
            *p = PATH_SEP;
        }
    }
    
    /* Remove trailing separator */
    size_t len = strlen(path);
    if (len > 1 && path[len-1] == PATH_SEP) {
#ifdef WINDOWS
        /* Keep C:\ */
        if (!(len == 3 && path[1] == ':')) {
            path[len-1] = '\0';
        }
#else
        path[len-1] = '\0';
#endif
    }
}

void paths_normalize_copy(char *dest, size_t size, const char *src) {
    if (!dest || !src || size == 0) return;
    str_copy(dest, src, size);
    paths_normalize(dest);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Path Joining
 * ══════════════════════════════════════════════════════════════════════════════ */

bool paths_join(char *dest, size_t size, const char *base, const char *name) {
    if (!dest || size == 0) return false;
    
    if (!base || base[0] == '\0') {
        if (name) {
            str_copy(dest, name, size);
            paths_normalize(dest);
        } else {
            dest[0] = '\0';
        }
        return true;
    }
    
    if (!name || name[0] == '\0') {
        str_copy(dest, base, size);
        paths_normalize(dest);
        return true;
    }
    
    str_copy(dest, base, size);
    paths_normalize(dest);
    
    size_t base_len = strlen(dest);
    
    /* Add separator */
    if (base_len > 0 && dest[base_len-1] != PATH_SEP) {
        if (base_len + 1 >= size) return false;
        dest[base_len] = PATH_SEP;
        dest[base_len + 1] = '\0';
        base_len++;
    }
    
    /* Skip leading separators in name */
    while (*name == '/' || *name == '\\') name++;
    
    if (base_len + strlen(name) >= size) return false;
    strcat(dest, name);
    paths_normalize(dest);
    
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Internal
 * ══════════════════════════════════════════════════════════════════════════════ */

static bool get_exe_dir(char *dest, size_t size, const char *exe_path) {
    if (!dest || !exe_path || size == 0) return false;
    
    str_copy(dest, exe_path, size);
    paths_normalize(dest);
    
    /* Find last separator */
    char *last_sep = strrchr(dest, PATH_SEP);
    
    if (last_sep) {
        *last_sep = '\0';
    } else {
        dest[0] = '.';
        dest[1] = '\0';
    }
    
    /* Handle build/bin directories */
    const char *dir_name = strrchr(dest, PATH_SEP);
    if (dir_name) dir_name++; else dir_name = dest;
    
    if (strcmp(dir_name, "build") == 0 || 
        strcmp(dir_name, "bin") == 0 ||
        strcmp(dir_name, "out") == 0) {
        char *sep = strrchr(dest, PATH_SEP);
        if (sep) *sep = '\0';
    }
    
    return true;
}

static void build_all_paths(void) {
    char *base = g_paths[PATH_BASE];
    
    /* data/ */
    paths_join(g_paths[PATH_DATA], SPEECHER_PATH_MAX, base, DIR_DATA);
    
    /* data/input/ */
    paths_join(g_paths[PATH_INPUT], SPEECHER_PATH_MAX, g_paths[PATH_DATA], DIR_INPUT);
    
    /* data/output/ */
    paths_join(g_paths[PATH_OUTPUT], SPEECHER_PATH_MAX, g_paths[PATH_DATA], DIR_OUTPUT);
    
    /* data/logs/ */
    paths_join(g_paths[PATH_LOGS], SPEECHER_PATH_MAX, g_paths[PATH_DATA], DIR_LOGS);
    
    /* data/config.ini */
    paths_join(g_paths[PATH_CONFIG], SPEECHER_PATH_MAX, g_paths[PATH_DATA], FILE_CONFIG);
    
    /* data/lang/ */
    paths_join(g_paths[PATH_LANG], SPEECHER_PATH_MAX, g_paths[PATH_DATA], DIR_LANG);
    
    /* data/lib/ - ИСПРАВЛЕНО: теперь внутри data/ */
    paths_join(g_paths[PATH_LIB], SPEECHER_PATH_MAX, g_paths[PATH_DATA], DIR_LIB);
    
    /* data/lib/whisper/ */
    paths_join(g_paths[PATH_WHISPER], SPEECHER_PATH_MAX, g_paths[PATH_LIB], DIR_WHISPER);
    
    /* data/lib/whisper/whisper-cli[.exe] - try both names */
    char bin_path[SPEECHER_PATH_MAX];
    paths_join(bin_path, sizeof(bin_path), g_paths[PATH_WHISPER], WHISPER_BIN);
    
    if (fs_exists(bin_path)) {
        str_copy(g_paths[PATH_WHISPER_BIN], bin_path, SPEECHER_PATH_MAX);
    } else {
        paths_join(bin_path, sizeof(bin_path), g_paths[PATH_WHISPER], WHISPER_BIN_ALT);
        if (fs_exists(bin_path)) {
            str_copy(g_paths[PATH_WHISPER_BIN], bin_path, SPEECHER_PATH_MAX);
        } else {
            /* Default to new name */
            paths_join(g_paths[PATH_WHISPER_BIN], SPEECHER_PATH_MAX, 
                      g_paths[PATH_WHISPER], WHISPER_BIN);
        }
    }
    
    /* data/lib/whisper/models/ */
    paths_join(g_paths[PATH_WHISPER_MODELS], SPEECHER_PATH_MAX, 
               g_paths[PATH_WHISPER], DIR_MODELS);
    
    /* SPEECHER_lang-pack.zip (в корне, рядом с exe) */
    paths_join(g_paths[PATH_LANG_PACK], SPEECHER_PATH_MAX, base, FILE_LANG_PACK);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

bool paths_init(const char *exe_path) {
    if (!exe_path) return false;
    
    if (!get_exe_dir(g_paths[PATH_BASE], SPEECHER_PATH_MAX, exe_path)) {
        return false;
    }
    
    build_all_paths();
    g_initialized = true;
    
    return true;
}

void paths_rebuild(void) {
    if (g_initialized) {
        build_all_paths();
    }
}

bool paths_is_init(void) {
    return g_initialized;
}

const char *paths_get(PathId id) {
    if (id < 0 || id >= PATH_COUNT || !g_initialized) {
        return "";
    }
    return g_paths[id];
}

bool paths_build(char *dest, size_t size, PathId base_id, const char *name) {
    if (base_id < 0 || base_id >= PATH_COUNT) return false;
    return paths_join(dest, size, g_paths[base_id], name);
}

bool paths_build_model(char *dest, size_t size, const char *model_name) {
    if (!dest || !model_name || size == 0) return false;
    
    char filename[64];
    snprintf(filename, sizeof(filename), "ggml-%s.bin", model_name);
    
    return paths_build(dest, size, PATH_WHISPER_MODELS, filename);
}

bool paths_ensure_dirs(void) {
    if (!g_initialized) return false;
    
    bool ok = true;
    ok = fs_mkdir_recursive(g_paths[PATH_DATA]) && ok;
    ok = fs_mkdir_recursive(g_paths[PATH_INPUT]) && ok;
    ok = fs_mkdir_recursive(g_paths[PATH_OUTPUT]) && ok;
    ok = fs_mkdir_recursive(g_paths[PATH_LOGS]) && ok;
    ok = fs_mkdir_recursive(g_paths[PATH_LANG]) && ok;
    fs_mkdir_recursive(g_paths[PATH_LIB]); /* Optional */
    
    return ok;
}

bool paths_whisper_installed(void) {
    if (!g_initialized) return false;
    if (!fs_exists(g_paths[PATH_WHISPER_BIN])) return false;
    
    /* Check at least one model */
    const char *models[] = {"tiny", "base", "small", "medium", "large", NULL};
    char path[SPEECHER_PATH_MAX];
    
    for (int i = 0; models[i]; i++) {
        if (paths_build_model(path, sizeof(path), models[i])) {
            if (fs_exists(path)) return true;
        }
    }
    
    return false;
}

const char *paths_find_model(const char *model_name) {
    static char path[SPEECHER_PATH_MAX];
    
    if (!model_name) return NULL;
    
    if (paths_build_model(path, sizeof(path), model_name)) {
        if (fs_exists(path)) return path;
    }
    
    /* Try large-v3 */
    if (strcmp(model_name, "large") == 0) {
        paths_build(path, sizeof(path), PATH_WHISPER_MODELS, "ggml-large-v3.bin");
        if (fs_exists(path)) return path;
    }
    
    return NULL;
}