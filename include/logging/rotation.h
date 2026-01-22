/**
 * @file    rotation.h
 * @brief   Log rotation functionality
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef LOGGING_ROTATION_H
#define LOGGING_ROTATION_H

#include <stdbool.h>
#include <stddef.h>

/** Rotation mode */
typedef enum {
    ROTATION_NONE = 0,
    ROTATION_SIZE,
    ROTATION_DATE,
    ROTATION_BOTH,
} RotationMode;

/**
 * @brief Check if rotation is needed
 * @param filepath Current log file path
 * @param mode Rotation mode
 * @param max_size_mb Max size in MB (for size-based rotation)
 * @return true if rotation should occur
 */
bool rotation_needed(const char *filepath, RotationMode mode, int max_size_mb);

/**
 * @brief Perform log rotation
 * @param filepath Current log file path
 * @param max_files Max number of files to keep (0 = unlimited)
 * @return true on success
 */
bool rotation_rotate(const char *filepath, int max_files);

/**
 * @brief Generate new log filename with date
 * @param dest Destination buffer
 * @param size Buffer size
 * @param template Filename template (e.g., "speecher_{date}.log")
 * @return true on success
 */
bool rotation_generate_filename(char *dest, size_t size, const char *name_template);

#endif /* LOGGING_ROTATION_H */