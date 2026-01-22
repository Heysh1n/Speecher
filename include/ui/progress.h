/**
 * @file    progress.h
 * @brief   Progress bar
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UI_PROGRESS_H
#define UI_PROGRESS_H

#include <stdbool.h>
#include <stddef.h>

/** Progress bar style */
typedef enum {
    PROGRESS_STYLE_ASCII,
    PROGRESS_STYLE_UNICODE,
    PROGRESS_STYLE_MINIMAL,
} ProgressStyle;

/** Progress bar structure */
typedef struct {
    size_t total;
    size_t current;
    int width;
    ProgressStyle style;
    bool show_percent;
    bool show_count;
    const char *prefix;
} ProgressBar;

/**
 * @brief Create new progress bar
 */
ProgressBar *progress_create(size_t total, int width, ProgressStyle style);

/**
 * @brief Free progress bar
 */
void progress_free(ProgressBar *bar);

/**
 * @brief Update progress bar
 */
void progress_update(ProgressBar *bar, size_t current);

/**
 * @brief Increment progress by 1
 */
void progress_increment(ProgressBar *bar);

/**
 * @brief Complete progress bar (fills to 100% and prints newline)
 */
void progress_complete(ProgressBar *bar);

/**
 * @brief Set progress bar prefix text
 */
void progress_set_prefix(ProgressBar *bar, const char *prefix);

#endif /* UI_PROGRESS_H */