/**
 * @file    platform.h
 * @brief   Platform-specific definitions and abstractions
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UTILS_PLATFORM_H
#define UTILS_PLATFORM_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Platform-Specific Includes
 * ══════════════════════════════════════════════════════════════════════════════ */

#if defined(_WIN32) || defined(_WIN64) || defined(WINDOWS)
    #ifndef WINDOWS
        #define WINDOWS
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <direct.h>
    #include <io.h>

    /* Path separator */
    #define PATH_SEPARATOR      '\\'
    #define PATH_SEPARATOR_STR  "\\"

    /* Directory functions */
    #define platform_mkdir(path)    _mkdir(path)
    #define platform_getcwd(buf, size)  _getcwd(buf, size)
    #define platform_chdir(path)    _chdir(path)

#else /* UNIX-like (Linux, macOS) */
    #ifndef UNIX_LIKE
        #define UNIX_LIKE
    #endif
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <dirent.h>
    #include <dlfcn.h>

    /* Path separator */
    #define PATH_SEPARATOR      '/'
    #define PATH_SEPARATOR_STR  "/"

    /* Directory functions */
    #define platform_mkdir(path)    mkdir(path, 0755)
    #define platform_getcwd(buf, size)  getcwd(buf, size)
    #define platform_chdir(path)    chdir(path)

#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Console Handling
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize console for proper Unicode and color support
 * @return true on success
 */
bool platform_console_init(void);

/**
 * @brief Clear console screen
 */
void platform_console_clear(void);

/**
 * @brief Check if console supports colors
 * @return true if colors are supported
 */
bool platform_console_colors_supported(void);

/**
 * @brief Get console width in characters
 * @return Console width, or 80 as default
 */
int platform_console_width(void);

/**
 * @brief Get console height in characters
 * @return Console height, or 24 as default
 */
int platform_console_height(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Sleep / Time
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void platform_sleep_ms(unsigned int ms);

/**
 * @brief Get current time in milliseconds (for measuring durations)
 * @return Milliseconds since some fixed point
 */
uint64_t platform_time_ms(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Executable Path
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get path to the current executable
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return true on success
 */
bool platform_get_exe_path(char *buffer, size_t size);

/**
 * @brief Get directory containing the executable
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return true on success
 */
bool platform_get_exe_dir(char *buffer, size_t size);

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Environment
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get environment variable
 * @param name Variable name
 * @param buffer Buffer to store value
 * @param size Buffer size
 * @return true if variable exists
 */
bool platform_getenv(const char *name, char *buffer, size_t size);

/**
 * @brief Set environment variable
 * @param name Variable name
 * @param value Variable value
 * @return true on success
 */
bool platform_setenv(const char *name, const char *value);

#endif /* UTILS_PLATFORM_H */