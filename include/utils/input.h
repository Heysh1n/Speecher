/**
 * @file    input.h
 * @brief   Terminal input handling
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UTILS_INPUT_H
#define UTILS_INPUT_H

#include <stdbool.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Initialization
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize terminal for proper input handling
 */
void input_init(void);

/**
 * @brief Restore terminal to original state
 */
void input_cleanup(void);

/**
 * @brief Clear input buffer (discard pending input)
 */
void input_clear_buffer(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Basic Input Functions
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Read a line of text from user
 * @param prompt Prompt to display (can be NULL)
 * @param buffer Buffer to store input
 * @param size Buffer size
 * @return true if input was received
 */
bool input_read_line(const char *prompt, char *buffer, size_t size);

/**
 * @brief Read an integer from user
 * @param prompt Prompt to display
 * @param default_value Default if input is empty or invalid
 * @return Entered integer or default
 */
int input_read_int(const char *prompt, int default_value);

/**
 * @brief Read yes/no from user (auto-detects language)
 * @param prompt Prompt to display
 * @param default_value Default if input is empty
 * @return true for yes, false for no
 */
bool input_read_bool(const char *prompt, bool default_value);

/**
 * @brief Read yes/no with explicit language
 * @param prompt Prompt to display
 * @param lang Language code ("en", "ru", "tr", "ja") or NULL for auto
 * @param default_value Default if input is empty
 * @return true for yes, false for no
 */
bool input_read_bool_lang(const char *prompt, const char *lang, bool default_value);

/**
 * @brief Read a menu choice (single digit or number)
 * @param prompt Prompt to display
 * @return Choice number or -1 on error
 */
int input_read_choice(const char *prompt);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Yes/No Utilities
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Parse yes/no response with multilingual support
 * 
 * Supports:
 * - English: y/n, yes/no
 * - Russian: д/н, да/нет
 * - Turkish: e/h, evet/hayır
 * - Japanese: はい/いいえ, hai/iie
 * 
 * @param response User input (trimmed)
 * @param lang Language code or NULL for auto-detection
 * @return 1 = yes, 0 = no, -1 = not recognized
 */
int input_parse_yesno(const char *response, const char *lang);

/**
 * @brief Get localized yes/no hint for display
 * @param lang Language code ("en", "ru", "tr", "ja")
 * @return Hint string like "[y/n]", "[д/н]", "[e/h]"
 */
const char* input_get_yesno_hint(const char *lang);

#endif /* UTILS_INPUT_H */