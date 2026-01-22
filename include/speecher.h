/**
 * @file    speecher.h
 * @brief   Main header file - includes all modules
 * @author  SPEECHER Team
 * @date    2025
 * 
 * This is the main header that should be included in most source files.
 * It provides access to all modules and common definitions.
 */

#ifndef SPEECHER_H
#define SPEECHER_H

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Version Info
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifndef VERSION
    #define VERSION "1.0.0"
#endif

#define SPEECHER_VERSION VERSION 

#define SPEECHER_VERSION_MAJOR  1
#define SPEECHER_VERSION_MINOR  0
#define SPEECHER_VERSION_PATCH  0

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Platform Detection
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Platform macros are set by Makefile, but we add fallback detection */
#if !defined(WINDOWS) && !defined(LINUX) && !defined(MACOS)
    #if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        #define WINDOWS
    #elif defined(__APPLE__) && defined(__MACH__)
        #define MACOS
    #elif defined(__linux__)
        #define LINUX
    #else
        #error "Unsupported platform"
    #endif
#endif

/* Unified UNIX check */
#if defined(LINUX) || defined(MACOS)
    #define UNIX_LIKE
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Standard Includes
 * ══════════════════════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Common Constants
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Buffer sizes */
#define SPEECHER_PATH_MAX       4096
#define SPEECHER_LINE_MAX       8192
#define SPEECHER_NAME_MAX       256
#define SPEECHER_BUFFER_SIZE    16384

/* Default paths */
#define DEFAULT_DATA_DIR        "data"
#define DEFAULT_INPUT_DIR       "data/input"
#define DEFAULT_OUTPUT_DIR      "data/output"
#define DEFAULT_LOGS_DIR        "data/logs"
#define DEFAULT_LIB_DIR         "lib"
#define DEFAULT_CONFIG_FILE     "data/config.ini"
#define DEFAULT_LANG_DIR        "lang"

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Return Codes
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef enum {
    SPEECHER_OK             =  0,   /* Success */
    SPEECHER_ERROR          = -1,   /* Generic error */
    SPEECHER_ERROR_MEMORY   = -2,   /* Memory allocation failed */
    SPEECHER_ERROR_IO       = -3,   /* File I/O error */
    SPEECHER_ERROR_CONFIG   = -4,   /* Configuration error */
    SPEECHER_ERROR_NOTFOUND = -5,   /* File/resource not found */
    SPEECHER_ERROR_INVALID  = -6,   /* Invalid argument/data */
    SPEECHER_ERROR_PLATFORM = -7,   /* Platform-specific error */
} SpeecherResult;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Module Includes
 * ══════════════════════════════════════════════════════════════════════════════ */

/* Utilities (no dependencies) */
#include "utils/platform.h"
#include "utils/strings.h"
#include "utils/fs.h"
#include "utils/input.h"

/* Logging (depends on: utils) */
#include "logging/logger.h"

/* Configuration (depends on: utils, logging) */
#include "config/ini_parser.h"
#include "config/config.h"
#include "config/validator.h"

/* UI (depends on: utils, logging, config) */
#include "ui/colors.h"
#include "ui/progress.h"
#include "ui/menu.h"

/* Core functionality (depends on: all above) */
#include "core/app.h"
#include "core/merger_text.h"
#include "core/merger_audio.h"

/* Whisper (depends on: all above) */
#include "whisper/whisper_manager.h"

/* Internationalization (depends on: utils, config) */
#include "i18n/lang.h"

#endif /* SPEECHER_H */