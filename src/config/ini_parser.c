/**
 * @file    ini_parser.c
 * @brief   INI file parser - Full implementation with BOM support
 * @author  SPEECHER Team
 * @date    2025
 */

#include "config/ini_parser.h"
#include "utils/strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Internal Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Skip UTF-8 BOM if present
 */
static void skip_utf8_bom(FILE *f) {
    unsigned char bom[3];
    size_t read = fread(bom, 1, 3, f);
    
    if (read == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        /* BOM found, already skipped by reading */
        return;
    }
    
    /* Not a BOM or file too small, seek back to start */
    fseek(f, 0, SEEK_SET);
}

/**
 * @brief Find or create section
 */
static IniSection *ini_find_or_create_section(IniFile *ini, const char *name) {
    if (!ini || !name) return NULL;
    
    /* Search existing */
    IniSection *section = ini->sections;
    while (section) {
        if (str_compare_nocase(section->name, name) == 0) {
            return section;
        }
        section = section->next;
    }
    
    /* Create new */
    section = calloc(1, sizeof(IniSection));
    if (!section) return NULL;
    
    str_copy(section->name, name, sizeof(section->name));
    
    /* Add to list (at beginning for simplicity) */
    section->next = ini->sections;
    ini->sections = section;
    
    return section;
}

/**
 * @brief Find entry in section
 */
static IniEntry *ini_find_entry(IniSection *section, const char *key) {
    if (!section || !key) return NULL;
    
    IniEntry *entry = section->entries;
    while (entry) {
        if (str_compare_nocase(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * @brief Find or create entry in section
 */
static IniEntry *ini_find_or_create_entry(IniSection *section, const char *key) {
    if (!section || !key) return NULL;
    
    /* Search existing */
    IniEntry *entry = ini_find_entry(section, key);
    if (entry) return entry;
    
    /* Create new */
    entry = calloc(1, sizeof(IniEntry));
    if (!entry) return NULL;
    
    str_copy(entry->key, key, sizeof(entry->key));
    
    /* Add to list */
    entry->next = section->entries;
    section->entries = entry;
    
    return entry;
}

/**
 * @brief Parse a single line
 */
static void ini_parse_line(IniFile *ini, char *line, char *current_section) {
    /* Trim whitespace */
    char *trimmed = str_trim(line);
    
    /* Skip empty lines and comments */
    if (!trimmed || *trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
        return;
    }
    
    /* Check for section header [section] */
    if (*trimmed == '[') {
        char *end = strchr(trimmed, ']');
        if (end) {
            *end = '\0';
            str_copy(current_section, trimmed + 1, INI_MAX_SECTION);
            str_trim(current_section);
        }
        return;
    }
    
    /* Parse key = value */
    char *equals = strchr(trimmed, '=');
    if (!equals) return;
    
    *equals = '\0';
    char *key = str_trim(trimmed);
    char *value = str_trim(equals + 1);
    
    /* Remove quotes from value if present */
    size_t val_len = strlen(value);
    if (val_len >= 2) {
        if ((value[0] == '"' && value[val_len-1] == '"') ||
            (value[0] == '\'' && value[val_len-1] == '\'')) {
            value[val_len-1] = '\0';
            value++;
        }
    }
    
    /* Store value */
    if (current_section[0] != '\0' && key[0] != '\0') {
        ini_set_string(ini, current_section, key, value);
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

IniFile *ini_create(void) {
    IniFile *ini = calloc(1, sizeof(IniFile));
    return ini;
}

IniFile *ini_load(const char *filepath) {
    if (!filepath) return NULL;
    
    FILE *f = fopen(filepath, "rb");  /* Open in binary mode for BOM detection */
    if (!f) {
        /* File doesn't exist - return empty INI */
        IniFile *ini = ini_create();
        if (ini) {
            str_copy(ini->filepath, filepath, sizeof(ini->filepath));
        }
        return ini;
    }
    
    IniFile *ini = ini_create();
    if (!ini) {
        fclose(f);
        return NULL;
    }
    
    str_copy(ini->filepath, filepath, sizeof(ini->filepath));
    
    /* Skip UTF-8 BOM if present */
    skip_utf8_bom(f);
    
    char line[INI_MAX_VALUE + INI_MAX_KEY + 10];
    char current_section[INI_MAX_SECTION] = "";
    
    while (fgets(line, sizeof(line), f)) {
        /* Remove newline (handles both \n and \r\n) */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        ini_parse_line(ini, line, current_section);
    }
    
    fclose(f);
    return ini;
}

bool ini_save(IniFile *ini, const char *filepath) {
    if (!ini) return false;
    
    const char *path = filepath ? filepath : ini->filepath;
    if (!path || !*path) return false;
    
    FILE *f = fopen(path, "w");  /* Text mode, no BOM */
    if (!f) return false;
    
    /* Write header comment */
    fprintf(f, "# SPEECHER Configuration File\n");
    fprintf(f, "# Auto-generated - Edit with care!\n\n");
    
    /* Write sections */
    IniSection *section = ini->sections;
    while (section) {
        fprintf(f, "[%s]\n", section->name);
        
        IniEntry *entry = section->entries;
        while (entry) {
            /* Quote value if it contains special characters */
            bool needs_quotes = false;
            for (const char *p = entry->value; *p; p++) {
                if (*p == '#' || *p == ';' || *p == '=' || 
                    *p == '[' || *p == ']') {
                    needs_quotes = true;
                    break;
                }
            }
            
            if (needs_quotes) {
                fprintf(f, "%s = \"%s\"\n", entry->key, entry->value);
            } else {
                fprintf(f, "%s = %s\n", entry->key, entry->value);
            }
            
            entry = entry->next;
        }
        
        fprintf(f, "\n");
        section = section->next;
    }
    
    fclose(f);
    ini->modified = false;
    return true;
}

void ini_free(IniFile *ini) {
    if (!ini) return;
    
    IniSection *section = ini->sections;
    while (section) {
        IniSection *next_section = section->next;
        
        IniEntry *entry = section->entries;
        while (entry) {
            IniEntry *next_entry = entry->next;
            free(entry);
            entry = next_entry;
        }
        
        free(section);
        section = next_section;
    }
    
    free(ini);
}

const char *ini_get_string(IniFile *ini, const char *section, 
                           const char *key, const char *default_value) {
    if (!ini || !section || !key) return default_value;
    
    IniSection *sec = ini->sections;
    while (sec) {
        if (str_compare_nocase(sec->name, section) == 0) {
            IniEntry *entry = ini_find_entry(sec, key);
            if (entry) {
                return entry->value;
            }
            break;
        }
        sec = sec->next;
    }
    
    return default_value;
}

int ini_get_int(IniFile *ini, const char *section, 
                const char *key, int default_value) {
    const char *str = ini_get_string(ini, section, key, NULL);
    if (!str) return default_value;
    
    char *endptr;
    long val = strtol(str, &endptr, 10);
    
    if (endptr == str || *endptr != '\0') {
        return default_value;
    }
    
    return (int)val;
}

bool ini_get_bool(IniFile *ini, const char *section, 
                  const char *key, bool default_value) {
    const char *str = ini_get_string(ini, section, key, NULL);
    if (!str) return default_value;
    
    if (str_compare_nocase(str, "true") == 0 ||
        str_compare_nocase(str, "yes") == 0 ||
        str_compare_nocase(str, "on") == 0 ||
        str_compare_nocase(str, "1") == 0) {
        return true;
    }
    
    if (str_compare_nocase(str, "false") == 0 ||
        str_compare_nocase(str, "no") == 0 ||
        str_compare_nocase(str, "off") == 0 ||
        str_compare_nocase(str, "0") == 0) {
        return false;
    }
    
    return default_value;
}

double ini_get_float(IniFile *ini, const char *section,
                     const char *key, double default_value) {
    const char *str = ini_get_string(ini, section, key, NULL);
    if (!str) return default_value;
    
    char *endptr;
    double val = strtod(str, &endptr);
    
    if (endptr == str || *endptr != '\0') {
        return default_value;
    }
    
    return val;
}

bool ini_set_string(IniFile *ini, const char *section, 
                    const char *key, const char *value) {
    if (!ini || !section || !key) return false;
    
    IniSection *sec = ini_find_or_create_section(ini, section);
    if (!sec) return false;
    
    IniEntry *entry = ini_find_or_create_entry(sec, key);
    if (!entry) return false;
    
    str_copy(entry->value, value ? value : "", sizeof(entry->value));
    ini->modified = true;
    
    return true;
}

bool ini_set_int(IniFile *ini, const char *section, 
                 const char *key, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return ini_set_string(ini, section, key, buffer);
}

bool ini_set_bool(IniFile *ini, const char *section, 
                  const char *key, bool value) {
    return ini_set_string(ini, section, key, value ? "true" : "false");
}

bool ini_has_section(IniFile *ini, const char *section) {
    if (!ini || !section) return false;
    
    IniSection *sec = ini->sections;
    while (sec) {
        if (str_compare_nocase(sec->name, section) == 0) {
            return true;
        }
        sec = sec->next;
    }
    return false;
}

bool ini_has_key(IniFile *ini, const char *section, const char *key) {
    if (!ini || !section || !key) return false;
    
    IniSection *sec = ini->sections;
    while (sec) {
        if (str_compare_nocase(sec->name, section) == 0) {
            return ini_find_entry(sec, key) != NULL;
        }
        sec = sec->next;
    }
    return false;
}