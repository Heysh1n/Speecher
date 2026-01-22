/**
 * @file    paths.h
 * @brief   Centralized path management for SPEECHER
 */

#ifndef SPEECHER_PATHS_H
#define SPEECHER_PATHS_H

#include <stdbool.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Path IDs
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef enum {
    PATH_BASE,              /* Base directory (where exe is) */
    PATH_DATA,              /* data/ */
    PATH_INPUT,             /* data/input/ */
    PATH_OUTPUT,            /* data/output/ */
    PATH_LOGS,              /* data/logs/ */
    PATH_CONFIG,            /* data/config.ini */
    PATH_LANG,              /* data/lang/ */
    PATH_LIB,               /* lib/ */
    PATH_WHISPER,           /* lib/whisper/ */
    PATH_WHISPER_BIN,       /* lib/whisper/whisper-cli[.exe] */
    PATH_WHISPER_MODELS,    /* lib/whisper/models/ */
    PATH_LANG_PACK,         /* SPEECHER_lang-pack.zip */
    PATH_COUNT
} PathId;

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Initialization
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Initialize path system with executable path
 * @param exe_path Path to executable (argv[0])
 * @return true on success
 */
bool paths_init(const char *exe_path);

/**
 * @brief Rebuild all paths (call after config change)
 */
void paths_rebuild(void);

/**
 * @brief Check if path system is initialized
 */
bool paths_is_init(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Getters
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Get path by ID
 * @param id Path identifier
 * @return Pointer to path string (static, do not free)
 */
const char *paths_get(PathId id);

/* Convenience macros */
#define PATH_BASE_DIR           paths_get(PATH_BASE)
#define PATH_DATA_DIR           paths_get(PATH_DATA)
#define PATH_INPUT_DIR          paths_get(PATH_INPUT)
#define PATH_OUTPUT_DIR         paths_get(PATH_OUTPUT)
#define PATH_LOGS_DIR           paths_get(PATH_LOGS)
#define PATH_CONFIG_FILE        paths_get(PATH_CONFIG)
#define PATH_LANG_DIR           paths_get(PATH_LANG)
#define PATH_LIB_DIR            paths_get(PATH_LIB)
#define PATH_WHISPER_DIR        paths_get(PATH_WHISPER)
#define PATH_WHISPER_EXE        paths_get(PATH_WHISPER_BIN)
#define PATH_WHISPER_MODELS_DIR paths_get(PATH_WHISPER_MODELS)

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Path Building
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Join two path components
 */
bool paths_join(char *dest, size_t size, const char *base, const char *name);

/**
 * @brief Build path from base ID and additional component
 */
bool paths_build(char *dest, size_t size, PathId base_id, const char *name);

/**
 * @brief Build path to whisper model file
 */
bool paths_build_model(char *dest, size_t size, const char *model_name);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Normalization
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Normalize path separators for current platform
 */
void paths_normalize(char *path);

/**
 * @brief Copy and normalize path
 */
void paths_normalize_copy(char *dest, size_t size, const char *src);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Validation
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Create all required directories
 */
bool paths_ensure_dirs(void);

/**
 * @brief Check if Whisper is installed
 */
bool paths_whisper_installed(void);

/**
 * @brief Find whisper model file
 * @return Path to model or NULL if not found
 */
const char *paths_find_model(const char *model_name);

#endif /* SPEECHER_PATHS_H */