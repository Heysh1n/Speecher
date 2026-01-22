/**
 * @file    lang.c
 * @brief   Internationalization
 * @author  SPEECHER Team
 * @date    2025
 */

#include "i18n/lang.h"
#include "i18n/embedded_data.h"
#include "config/ini_parser.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "utils/platform.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Constants
 * ══════════════════════════════════════════════════════════════════════════════ */

static const char *LANG_CODES[] = {"en", "ru", "tr", "ja", NULL};
static const char *LANG_NAMES[] = {"English", "Russian", "Turkish", "Japanese", NULL};

/* ══════════════════════════════════════════════════════════════════════════════
 *                              State
 * ══════════════════════════════════════════════════════════════════════════════ */

static IniFile *g_lang = NULL;
static char g_lang_code[16] = "en";
static char g_lang_dir[SPEECHER_PATH_MAX] = {0};
static bool g_emoji_enabled = true;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Internal Functions
 * ══════════════════════════════════════════════════════════════════════════════ */

static const char *get_lang_content(const char *code) {
    if (strcmp(code, "en") == 0) return LANG_EN;
    if (strcmp(code, "ru") == 0) return LANG_RU;
    if (strcmp(code, "tr") == 0) return LANG_TR;
    if (strcmp(code, "ja") == 0) return LANG_JA;
    return LANG_EN;
}

static bool create_lang_file(const char *code) {
    const char *content = get_lang_content(code);
    
    char filepath[SPEECHER_PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%c%s.ini", 
             g_lang_dir, PATH_SEPARATOR, code);
    
    FILE *f = fopen(filepath, "w");
    if (!f) return false;
    
    fputs(content, f);
    fclose(f);
    
    LOG_DEBUG("Created language file: %s", filepath);
    return true;
}

static void ensure_lang_files(void) {
    extern const char *app_get_base_dir(void);
    const char *base = app_get_base_dir();
    
    if (base) {
        snprintf(g_lang_dir, sizeof(g_lang_dir), "%s%cdata%clang", 
                 base, PATH_SEPARATOR, PATH_SEPARATOR);
    } else {
        snprintf(g_lang_dir, sizeof(g_lang_dir), "data%clang", PATH_SEPARATOR);
    }
    
    fs_mkdir_recursive(g_lang_dir);
    
    for (int i = 0; LANG_CODES[i]; i++) {
        char filepath[SPEECHER_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s%c%s.ini", 
                 g_lang_dir, PATH_SEPARATOR, LANG_CODES[i]);
        
        if (!fs_exists(filepath)) {
            create_lang_file(LANG_CODES[i]);
        }
    }
}

static IniFile *load_language(const char *code) {
    char filepath[SPEECHER_PATH_MAX];
    snprintf(filepath, sizeof(filepath), "%s%c%s.ini", 
             g_lang_dir, PATH_SEPARATOR, code);
    
    if (fs_exists(filepath)) {
        IniFile *ini = ini_load(filepath);
        if (ini) return ini;
    }
    
    if (create_lang_file(code)) {
        return ini_load(filepath);
    }
    
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

bool lang_init(const char *lang_code) {
    if (!lang_code || !*lang_code) lang_code = "en";
    
    if (g_lang) {
        ini_free(g_lang);
        g_lang = NULL;
    }
    
    str_copy(g_lang_code, lang_code, sizeof(g_lang_code));
    
    ensure_lang_files();
    
    g_lang = load_language(lang_code);
    
    if (!g_lang && strcmp(lang_code, "en") != 0) {
        LOG_WARN("Language '%s' not available, using English", lang_code);
        str_copy(g_lang_code, "en", sizeof(g_lang_code));
        g_lang = load_language("en");
    }
    
    if (g_lang) {
        LOG_INFO("Language loaded: %s", g_lang_code);
        return true;
    }
    
    return false;
}

void lang_free(void) {
    if (g_lang) {
        ini_free(g_lang);
        g_lang = NULL;
    }
}

const char *lang_current(void) {
    return g_lang_code;
}

const char *lang_get(const char *section, const char *key) {
    if (!section || !key) return "";
    if (g_lang) {
        const char *value = ini_get_string(g_lang, section, key, NULL);
        if (value) return value;
    }
    return key;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Section Shortcuts
 * ══════════════════════════════════════════════════════════════════════════════ */

const char *lang_menu(const char *key)     { return lang_get("menu", key); }
const char *lang_msg(const char *key)      { return lang_get("msg", key); }
const char *lang_text(const char *key)     { return lang_get("text", key); }
const char *lang_audio(const char *key)    { return lang_get("audio", key); }
const char *lang_whisper(const char *key)  { return lang_get("whisper", key); }
const char *lang_settings(const char *key) { return lang_get("settings", key); }
const char *lang_logs(const char *key)     { return lang_get("logs", key); }
const char *lang_err(const char *key)      { return lang_get("err", key); }
const char *lang_install(const char *key)  { return lang_get("install", key); }
const char *lang_progress(const char *key) { return lang_get("progress", key); }
const char *lang_size(const char *key)     { return lang_get("size", key); }
const char *lang_time(const char *key)     { return lang_get("time", key); }

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Language Info
 * ══════════════════════════════════════════════════════════════════════════════ */

int lang_get_supported(const char ***codes, const char ***names) {
    if (codes) *codes = LANG_CODES;
    if (names) *names = LANG_NAMES;
    
    int count = 0;
    while (LANG_CODES[count]) count++;
    return count;
}

bool lang_is_supported(const char *code) {
    if (!code) return false;
    for (int i = 0; LANG_CODES[i]; i++) {
        if (strcmp(LANG_CODES[i], code) == 0) return true;
    }
    return false;
}

const char *lang_get_name(const char *code) {
    if (!code) return NULL;
    for (int i = 0; LANG_CODES[i]; i++) {
        if (strcmp(LANG_CODES[i], code) == 0) {
            return LANG_NAMES[i];
        }
    }
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Emoji Support
 * ══════════════════════════════════════════════════════════════════════════════ */

void lang_set_emoji(bool enabled) {
    g_emoji_enabled = enabled;
}

bool lang_emoji_enabled(void) {
    return g_emoji_enabled;
}