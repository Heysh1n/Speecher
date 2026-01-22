/**
 * @file    logger.h
 * @brief   Logging system
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef LOGGING_LOGGER_H
#define LOGGING_LOGGER_H

#include <stdbool.h>
#include <stdarg.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Log Levels
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
} LogLevel;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Logger API
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize logger
 */
bool logger_init(const char *log_dir, bool console_output);

/**
 * @brief Shutdown logger
 */
void logger_shutdown(void);

/**
 * @brief Set minimum log level for file output
 */
void logger_set_level(LogLevel level);

/**
 * @brief Set minimum log level for console output
 */
void logger_set_console_level(LogLevel level);

/**
 * @brief Enable/disable colored console output
 */
void logger_set_colored(bool enabled);

/**
 * @brief Log a message
 */
void logger_log(LogLevel level, const char *file, int line, const char *fmt, ...);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Convenience Macros
 * ══════════════════════════════════════════════════════════════════════════════ */

/* C99 compatible variadic macros - always pass at least one argument */
#define LOG_DEBUG(fmt, ...) \
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* LOGGING_LOGGER_H */