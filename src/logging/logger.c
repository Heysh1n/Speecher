/**
 * @file    logger.c
 * @brief   Logging system - Full implementation
 * @author  SPEECHER Team
 * @date    2025
 */

/* POSIX features */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
#endif

#include "logging/logger.h"
#include "utils/platform.h"
#include "utils/strings.h"
#include "utils/fs.h"
#include "ui/colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Logger State
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    bool initialized;
    bool console_output;
    bool colored;
    bool file_output;
    LogLevel level;
    LogLevel console_level;
    FILE *log_file;
    char log_dir[4096];
    char current_log_path[4096];
    
    /* Color settings */
    const char *level_colors[LOG_LEVEL_COUNT];
} LoggerState;

static LoggerState g_logger = {
    .initialized = false,
    .console_output = true,
    .colored = true,
    .file_output = false,
    .level = LOG_LEVEL_INFO,
    .console_level = LOG_LEVEL_INFO,
    .log_file = NULL,
    .log_dir = "",
    .current_log_path = "",
    .level_colors = {
        COLOR_CYAN,         /* DEBUG */
        COLOR_GREEN,        /* INFO */
        COLOR_YELLOW,       /* WARN */
        COLOR_RED,          /* ERROR */
        COLOR_BRIGHT_RED,   /* FATAL */
    }
};

/* Level names */
static const char *g_level_names[LOG_LEVEL_COUNT] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Internal Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get current timestamp string
 */
static void logger_get_timestamp(char *buffer, size_t size, bool include_date) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    if (include_date) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strftime(buffer, size, "%H:%M:%S", tm_info);
    }
}

/**
 * @brief Generate log filename for today
 */
static void logger_generate_filename(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
    
    snprintf(buffer, size, "%s%cspeechr_%s.log", 
             g_logger.log_dir, PATH_SEPARATOR, date_str);
}

/**
 * @brief Open or rotate log file
 */
static bool logger_open_file(void) {
    if (!g_logger.file_output || g_logger.log_dir[0] == '\0') {
        return true;
    }
    
    char new_path[4096];
    logger_generate_filename(new_path, sizeof(new_path));
    
    /* Check if we need to open a new file */
    if (strcmp(new_path, g_logger.current_log_path) == 0 && g_logger.log_file) {
        return true;  /* Same file, already open */
    }
    
    /* Close old file */
    if (g_logger.log_file) {
        fclose(g_logger.log_file);
        g_logger.log_file = NULL;
    }
    
    /* Open new file */
    g_logger.log_file = fopen(new_path, "a");
    if (!g_logger.log_file) {
        fprintf(stderr, "Warning: Could not open log file: %s\n", new_path);
        return false;
    }
    
    str_copy(g_logger.current_log_path, new_path, sizeof(g_logger.current_log_path));
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

bool logger_init(const char *log_dir, bool console_output) {
    g_logger.console_output = console_output;
    g_logger.colored = platform_console_colors_supported();
    g_logger.initialized = true;
    
    if (log_dir && log_dir[0] != '\0') {
        str_copy(g_logger.log_dir, log_dir, sizeof(g_logger.log_dir));
        g_logger.file_output = true;
        
        /* Create log directory if needed */
        fs_mkdir_recursive(log_dir);
        
        /* Open initial log file */
        logger_open_file();
    }
    
    return true;
}

void logger_shutdown(void) {
    if (g_logger.log_file) {
        fclose(g_logger.log_file);
        g_logger.log_file = NULL;
    }
    g_logger.initialized = false;
}

void logger_set_level(LogLevel level) {
    if (level >= 0 && level < LOG_LEVEL_COUNT) {
        g_logger.level = level;
    }
}

void logger_set_console_level(LogLevel level) {
    if (level >= 0 && level < LOG_LEVEL_COUNT) {
        g_logger.console_level = level;
    }
}

void logger_set_colored(bool enabled) {
    g_logger.colored = enabled && platform_console_colors_supported();
}

void logger_log(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (!g_logger.initialized) {
        /* Fallback: just print to stderr */
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        return;
    }
    
    /* Get timestamp */
    char timestamp[32];
    logger_get_timestamp(timestamp, sizeof(timestamp), true);
    
    /* Extract filename from path */
    const char *filename = file;
    if (file) {
        const char *sep = strrchr(file, PATH_SEPARATOR);
        if (sep) filename = sep + 1;
    }
    
    /* Format message */
    char message[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);
    
    /* Console output */
    if (g_logger.console_output && level >= g_logger.console_level) {
        if (g_logger.colored) {
            printf("%s[%s]%s %s\n", 
                   g_logger.level_colors[level],
                   g_level_names[level],
                   COLOR_RESET,
                   message);
        } else {
            printf("[%s] %s\n", g_level_names[level], message);
        }
        fflush(stdout);
    }
    
    /* File output */
    if (g_logger.file_output && level >= g_logger.level) {
        /* Check for date rotation */
        logger_open_file();
        
        if (g_logger.log_file) {
            fprintf(g_logger.log_file, "[%s] [%s] [%s:%d] %s\n",
                    timestamp,
                    g_level_names[level],
                    filename ? filename : "?",
                    line,
                    message);
            fflush(g_logger.log_file);
        }
    }
}