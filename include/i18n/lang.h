/**
 * @file    lang.h
 * @brief   Internationalization system
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef I18N_LANG_H
#define I18N_LANG_H

#include <stdbool.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Initialization
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize language system
 * @param lang_code Language code (e.g., "en", "ru", "tr", "ja")
 * @return true on success
 */
bool lang_init(const char *lang_code);

/**
 * @brief Free language resources
 */
void lang_free(void);

/**
 * @brief Get current language code
 * @return Current language code (e.g., "en")
 */
const char *lang_current(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              String Retrieval
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get translated string by section and key
 * @param section Section name (e.g., "menu", "msg")
 * @param key Key name (e.g., "title", "done")
 * @return Translated string or key if not found
 */
const char *lang_get(const char *section, const char *key);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Section Shortcuts
 * ══════════════════════════════════════════════════════════════════════════════ */

const char *lang_menu(const char *key);
const char *lang_msg(const char *key);
const char *lang_text(const char *key);
const char *lang_audio(const char *key);
const char *lang_whisper(const char *key);
const char *lang_settings(const char *key);
const char *lang_logs(const char *key);
const char *lang_err(const char *key);
const char *lang_install(const char *key);
const char *lang_progress(const char *key);
const char *lang_size(const char *key);
const char *lang_time(const char *key);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Language Info
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get list of supported languages
 * @param codes Output pointer to language codes array (can be NULL)
 * @param names Output pointer to language names array (can be NULL)
 * @return Number of supported languages
 */
int lang_get_supported(const char ***codes, const char ***names);

/**
 * @brief Check if language is supported
 * @param lang_code Language code to check
 * @return true if supported
 */
bool lang_is_supported(const char *lang_code);

/**
 * @brief Get language name by code
 * @param lang_code Language code (e.g., "ru")
 * @return Language name (e.g., "Русский") or NULL
 */
const char *lang_get_name(const char *lang_code);

/**
 * @brief Set emoji mode
 * @param enabled true to enable emoji, false for ASCII
 */
void lang_set_emoji(bool enabled);

/**
 * @brief Check if emoji mode is enabled
 */
bool lang_emoji_enabled(void);


/* ══════════════════════════════════════════════════════════════════════════════
 *                              Convenience Macros
 * ══════════════════════════════════════════════════════════════════════════════ */

#define L(section, key)     lang_get(section, key)
#define L_MENU(key)         lang_menu(key)
#define L_MSG(key)          lang_msg(key)
#define L_TEXT(key)         lang_text(key)
#define L_AUDIO(key)        lang_audio(key)
#define L_WHISPER(key)      lang_whisper(key)
#define L_SETTINGS(key)     lang_settings(key)
#define L_LOGS(key)         lang_logs(key)
#define L_ERR(key)          lang_err(key)
#define L_INSTALL(key)      lang_install(key)
#define L_PROGRESS(key)     lang_progress(key)
#define L_SIZE(key)         lang_size(key)
#define L_TIME(key)         lang_time(key)

#endif /* I18N_LANG_H */