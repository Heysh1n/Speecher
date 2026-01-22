/**
 * @file    input.c
 * @brief   Terminal input handling - Full implementation with multilingual Y/N
 * @author  SPEECHER Team
 * @date    2025
 */

#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
#endif

#include "utils/input.h"
#include "utils/platform.h"
#include "ui/colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WINDOWS
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Terminal State
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifndef WINDOWS
static struct termios g_orig_termios;
static bool g_termios_saved = false;
static bool g_termios_modified = false;
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                          Multilingual Yes/No Tables
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct YesNoEntry {
    const char *text;
    bool is_yes;
} YesNoEntry;

typedef struct LangYesNo {
    const char *lang_code;
    const char *hint;
    const YesNoEntry *entries;
} LangYesNo;

static const YesNoEntry g_yesno_universal[] = {
    { "y",   true  }, { "Y",   true  },
    { "yes", true  }, { "YES", true  }, { "Yes", true },
    { "1",   true  },
    { "n",   false }, { "N",   false },
    { "no",  false }, { "NO",  false }, { "No",  false },
    { "0",   false },
    { NULL,  false }
};

static const YesNoEntry g_yesno_ru[] = {
    /* UTF-8 */
    { "\xD0\xB4",                 true  },  /* д */
    { "\xD0\x94",                 true  },  /* Д */
    { "\xD0\xB4\xD0\xB0",         true  },  /* да */
    { "\xD0\x94\xD0\xB0",         true  },  /* Да */
    { "\xD0\x94\xD0\x90",         true  },  /* ДА */
    { "\xD0\xBD",                 false },  /* н */
    { "\xD0\x9D",                 false },  /* Н */
    { "\xD0\xBD\xD0\xB5\xD1\x82", false },  /* нет */
    { "\xD0\x9D\xD0\xB5\xD1\x82", false },  /* Нет */
    { "\xD0\x9D\xD0\x95\xD0\xA2", false },  /* НЕТ */
    /* CP866 */
    { "\xA4",         true  },
    { "\x84",         true  },
    { "\xA4\xA0",     true  },
    { "\x84\xA0",     true  },
    { "\x84\x80",     true  },
    { "\xAD",         false },
    { "\x8D",         false },
    { "\xAD\xA5\xE2", false },
    { "\x8D\xA5\xE2", false },
    { "\x8D\x85\x92", false },
    /* CP1251 */
    { "\xE4",         true  },
    { "\xC4",         true  },
    { "\xE4\xE0",     true  },
    { "\xC4\xE0",     true  },
    { "\xC4\xC0",     true  },
    { "\xED",         false },
    { "\xCD",         false },
    { "\xED\xE5\xF2", false },
    { "\xCD\xE5\xF2", false },
    { "\xCD\xC5\xD2", false },
    /* Plain text */
    { "д",    true  }, { "Д",    true  },
    { "да",   true  }, { "Да",   true  }, { "ДА",   true  },
    { "н",    false }, { "Н",    false },
    { "нет",  false }, { "Нет",  false }, { "НЕТ",  false },
    /* English layout */
    { "l",    true  }, { "L",    true  },
    { "lf",   true  }, { "LF",   true  },
    { "ytn",  false }, { "YTN",  false },
    { NULL,   false }
};

static const YesNoEntry g_yesno_tr[] = {
    { "e",     true  }, { "E",     true  },
    { "evet",  true  }, { "Evet",  true  }, { "EVET",  true  },
    { "h",     false }, { "H",     false },
    { "hayir", false }, { "Hayir", false }, { "HAYIR", false },
    { "hayır", false }, { "Hayır", false },
    { NULL,    false }
};

static const YesNoEntry g_yesno_ja[] = {
    { "\xE3\x81\xAF\xE3\x81\x84", true  },
    { "hai",    true  }, { "HAI",    true  }, { "Hai",    true  },
    { "\xE3\x81\x84\xE3\x81\x84\xE3\x81\x88", false },
    { "iie",    false }, { "IIE",    false }, { "Iie",    false },
    { NULL,     false }
};

static const LangYesNo g_lang_tables[] = {
    { "en", "[y/n]", NULL         },
    { "ru", "[д/н]", g_yesno_ru   },
    { "tr", "[e/h]", g_yesno_tr   },
    { "ja", "[y/n]", g_yesno_ja   },
    { NULL, NULL,    NULL         }
};

/* ══════════════════════════════════════════════════════════════════════════════
 *                          Yes/No Parsing
 * ══════════════════════════════════════════════════════════════════════════════ */

static int search_table(const YesNoEntry *table, const char *response) {
    if (!table || !response) return -1;
    for (int i = 0; table[i].text != NULL; i++) {
        if (strcmp(response, table[i].text) == 0) {
            return table[i].is_yes ? 1 : 0;
        }
    }
    return -1;
}

static const LangYesNo* find_lang_table(const char *lang) {
    if (!lang) return &g_lang_tables[0];
    for (int i = 0; g_lang_tables[i].lang_code != NULL; i++) {
        if (strcmp(g_lang_tables[i].lang_code, lang) == 0) {
            return &g_lang_tables[i];
        }
    }
    return &g_lang_tables[0];
}

int input_parse_yesno(const char *response, const char *lang) {
    if (!response || response[0] == '\0') return -1;
    
    /* Skip whitespace */
    while (*response == ' ' || *response == '\t') response++;
    
    /* Clean input */
    char clean[64];
    strncpy(clean, response, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    size_t len = strlen(clean);
    while (len > 0 && (clean[len-1] == ' ' || clean[len-1] == '\t' || 
                       clean[len-1] == '\r' || clean[len-1] == '\n')) {
        clean[--len] = '\0';
    }
    
    if (clean[0] == '\0') return -1;
    
    unsigned char *p = (unsigned char *)clean;
    
    /* ═══ UNIVERSAL ═══ */
    if (p[0] == 'y' || p[0] == 'Y' || p[0] == '1') return 1;
    if (p[0] == 'n' || p[0] == 'N' || p[0] == '0') return 0;
    
    /* ═══ RUSSIAN by bytes ═══ */
    /* UTF-8: д=D0B4, Д=D094 */
    if (p[0] == 0xD0) {
        if (p[1] == 0xB4 || p[1] == 0x94) return 1;  /* д, Д */
        if (p[1] == 0xBD || p[1] == 0x9D) return 0;  /* н, Н */
    }
    /* CP866 */
    if (p[0] == 0xA4 || p[0] == 0x84) return 1;
    if (p[0] == 0xAD || p[0] == 0x8D) return 0;
    /* CP1251 */
    if (p[0] == 0xE4 || p[0] == 0xC4) return 1;
    if (p[0] == 0xED || p[0] == 0xCD) return 0;
    
    /* ═══ TURKISH ═══ */
    if (p[0] == 'e' || p[0] == 'E') return 1;
    if (p[0] == 'h' || p[0] == 'H') return 0;
    
    /* ═══ English layout ═══ */
    if (p[0] == 'l' || p[0] == 'L') return 1;
    
    /* ═══ Tables ═══ */
    int result = search_table(g_yesno_universal, clean);
    if (result >= 0) return result;
    
    for (int i = 0; g_lang_tables[i].lang_code != NULL; i++) {
        if (g_lang_tables[i].entries) {
            result = search_table(g_lang_tables[i].entries, clean);
            if (result >= 0) return result;
        }
    }
    
    return -1;
}

const char* input_get_yesno_hint(const char *lang) {
    const LangYesNo *table = find_lang_table(lang);
    return table ? table->hint : "[y/n]";
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Terminal Setup
 * ══════════════════════════════════════════════════════════════════════════════ */

void input_init(void) {
#ifdef WINDOWS
    /* UTF-8 для консоли */
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#else
    if (!g_termios_saved && isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &g_orig_termios) == 0) {
            g_termios_saved = true;
            struct termios new_termios = g_orig_termios;
            new_termios.c_lflag |= (ICANON | ECHO | ECHOE | ECHOK);
            new_termios.c_cc[VERASE] = 0x7f;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == 0) {
                g_termios_modified = true;
            }
        }
    }
#endif
}

void input_cleanup(void) {
#ifndef WINDOWS
    if (g_termios_saved && g_termios_modified) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_termios_modified = false;
    }
#endif
}

void input_clear_buffer(void) {
#ifdef WINDOWS
    while (_kbhit()) _getch();
#else
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
#endif
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Line Reading
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifndef WINDOWS
static bool read_line_unix(char *buffer, size_t size) {
    if (!buffer || size == 0) return false;
    
    size_t pos = 0;
    int c;
    struct termios old_settings, new_settings;
    
    if (tcgetattr(STDIN_FILENO, &old_settings) != 0) {
        if (fgets(buffer, (int)size, stdin) == NULL) {
            buffer[0] = '\0';
            return false;
        }
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
        return true;
    }
    
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    
    buffer[0] = '\0';
    
    while (1) {
        c = getchar();
        
        if (c == EOF) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
            return false;
        }
        
        if (c == '\n' || c == '\r') {
            printf("\n");
            break;
        }
        
        if (c == 0x7f || c == 0x08 || c == '\b') {
            if (pos > 0) {
                do { pos--; } while (pos > 0 && (buffer[pos] & 0xC0) == 0x80);
                buffer[pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        
        if (c == 3) {
            tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
            buffer[0] = '\0';
            printf("\n");
            return false;
        }
        
        if (c == 21) {
            while (pos > 0) { pos--; printf("\b \b"); }
            buffer[0] = '\0';
            fflush(stdout);
            continue;
        }
        
        if (c == 27) {
            int next = getchar();
            if (next == '[') getchar();
            continue;
        }
        
        if (pos < size - 1 && (c >= 32 || (unsigned char)c >= 0x80)) {
            buffer[pos++] = (char)c;
            buffer[pos] = '\0';
            printf("%c", c);
            fflush(stdout);
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    return true;
}
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public Functions
 * ══════════════════════════════════════════════════════════════════════════════ */

bool input_read_line(const char *prompt, char *buffer, size_t size) {
    if (!buffer || size == 0) return false;
    
    if (prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    
#ifdef WINDOWS
    /* Получаем handle консоли */
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    
    /* Используем ReadConsoleW для корректного чтения Unicode */
    if (hInput != INVALID_HANDLE_VALUE && GetConsoleMode(hInput, &mode)) {
        wchar_t wbuffer[256];
        DWORD chars_read = 0;
        
        if (ReadConsoleW(hInput, wbuffer, 255, &chars_read, NULL)) {
            /* Удаляем \r\n в конце */
            while (chars_read > 0 && 
                   (wbuffer[chars_read-1] == L'\r' || wbuffer[chars_read-1] == L'\n')) {
                chars_read--;
            }
            wbuffer[chars_read] = L'\0';
            
            /* Конвертируем из UTF-16 в UTF-8 */
            int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, 
                                                buffer, (int)size, NULL, NULL);
            if (utf8_len > 0) {
                return true;
            }
        }
    }
    
    /* Fallback: fgets */
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return false;
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
        buffer[--len] = '\0';
    }
    return true;
    
#else
    return read_line_unix(buffer, size);
#endif
}

int input_read_int(const char *prompt, int default_value) {
    char buffer[64];
    if (!input_read_line(prompt, buffer, sizeof(buffer))) return default_value;
    if (buffer[0] == '\0') return default_value;
    
    char *endptr;
    long value = strtol(buffer, &endptr, 10);
    if (endptr == buffer || *endptr != '\0') return default_value;
    return (int)value;
}

bool input_read_bool(const char *prompt, bool default_value) {
    char buffer[64];
    if (!input_read_line(prompt, buffer, sizeof(buffer))) return default_value;
    if (buffer[0] == '\0') return default_value;
    
    int result = input_parse_yesno(buffer, NULL);
    return (result >= 0) ? (result == 1) : default_value;
}

bool input_read_bool_lang(const char *prompt, const char *lang, bool default_value) {
    char buffer[64];
    if (!input_read_line(prompt, buffer, sizeof(buffer))) return default_value;
    if (buffer[0] == '\0') return default_value;
    
    int result = input_parse_yesno(buffer, lang);
    return (result >= 0) ? (result == 1) : default_value;
}

int input_read_choice(const char *prompt) {
    char buffer[16];
    if (!input_read_line(prompt, buffer, sizeof(buffer))) return -1;
    if (buffer[0] == '\0') return -1;
    
    if (isdigit((unsigned char)buffer[0]) && buffer[1] == '\0') {
        return buffer[0] - '0';
    }
    
    char *endptr;
    long value = strtol(buffer, &endptr, 10);
    if (endptr != buffer && *endptr == '\0' && value >= 0 && value <= 999) {
        return (int)value;
    }
    
    return -1;
}