/**
 * @file    ini_parser.h
 * @brief   INI file parser
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef CONFIG_INI_PARSER_H
#define CONFIG_INI_PARSER_H

#include <stdbool.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Types
 * ══════════════════════════════════════════════════════════════════════════════ */

/** Maximum sizes */
#define INI_MAX_SECTION     64
#define INI_MAX_KEY         64
#define INI_MAX_VALUE       1024

/** INI key-value pair */
typedef struct IniEntry {
    char key[INI_MAX_KEY];
    char value[INI_MAX_VALUE];
    struct IniEntry *next;
} IniEntry;

/** INI section */
typedef struct IniSection {
    char name[INI_MAX_SECTION];
    IniEntry *entries;
    struct IniSection *next;
} IniSection;

/** INI file structure */
typedef struct {
    IniSection *sections;
    char filepath[4096];
    bool modified;
} IniFile;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              API
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Create new empty INI file structure
 * @return New INI file or NULL on error
 */
IniFile *ini_create(void);

/**
 * @brief Load INI file from disk
 * @param filepath Path to INI file
 * @return Loaded INI file or NULL on error
 */
IniFile *ini_load(const char *filepath);

/**
 * @brief Save INI file to disk
 * @param ini INI file structure
 * @param filepath Path to save (NULL = use original path)
 * @return true on success
 */
bool ini_save(IniFile *ini, const char *filepath);

/**
 * @brief Free INI file structure
 */
void ini_free(IniFile *ini);

/**
 * @brief Get string value
 * @param ini INI file
 * @param section Section name
 * @param key Key name
 * @param default_value Default if not found
 * @return Value string (do not modify) or default_value
 */
const char *ini_get_string(IniFile *ini, const char *section, 
                           const char *key, const char *default_value);

/**
 * @brief Get integer value
 */
int ini_get_int(IniFile *ini, const char *section, 
                const char *key, int default_value);

/**
 * @brief Get boolean value (true/false, yes/no, 1/0)
 */
bool ini_get_bool(IniFile *ini, const char *section, 
                  const char *key, bool default_value);

/**
 * @brief Get float value
 */
double ini_get_float(IniFile *ini, const char *section,
                     const char *key, double default_value);

/**
 * @brief Set string value
 * @return true on success
 */
bool ini_set_string(IniFile *ini, const char *section, 
                    const char *key, const char *value);

/**
 * @brief Set integer value
 */
bool ini_set_int(IniFile *ini, const char *section, 
                 const char *key, int value);

/**
 * @brief Set boolean value
 */
bool ini_set_bool(IniFile *ini, const char *section, 
                  const char *key, bool value);

/**
 * @brief Check if section exists
 */
bool ini_has_section(IniFile *ini, const char *section);

/**
 * @brief Check if key exists in section
 */
bool ini_has_key(IniFile *ini, const char *section, const char *key);

#endif /* CONFIG_INI_PARSER_H */