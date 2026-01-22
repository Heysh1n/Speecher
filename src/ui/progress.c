/**
 * @file    progress.c
 * @brief   Progress bar - Full implementation
 * @author  SPEECHER Team
 * @date    2025
 */

#include "ui/progress.h"
#include "ui/colors.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Progress Bar Characters
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *filled;
    const char *empty;
    const char *left;
    const char *right;
} ProgressChars;

static const ProgressChars PROGRESS_ASCII = {
    .filled = "#",
    .empty = "-",
    .left = "[",
    .right = "]"
};

static const ProgressChars PROGRESS_UNICODE = {
    .filled = "█",
    .empty = "░",
    .left = "",
    .right = ""
};

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

ProgressBar *progress_create(size_t total, int width, ProgressStyle style) {
    ProgressBar *bar = calloc(1, sizeof(ProgressBar));
    if (!bar) return NULL;
    
    bar->total = total > 0 ? total : 1;
    bar->current = 0;
    bar->width = width > 10 ? width : 40;
    bar->style = style;
    bar->show_percent = true;
    bar->show_count = true;
    bar->prefix = NULL;
    
    return bar;
}

void progress_free(ProgressBar *bar) {
    if (bar) free(bar);
}

void progress_update(ProgressBar *bar, size_t current) {
    if (!bar) return;
    
    bar->current = current > bar->total ? bar->total : current;
    
    int percent = (int)((bar->current * 100) / bar->total);
    
    /* Select style */
    const ProgressChars *chars;
    switch (bar->style) {
        case PROGRESS_STYLE_ASCII:
            chars = &PROGRESS_ASCII;
            break;
        case PROGRESS_STYLE_UNICODE:
            chars = &PROGRESS_UNICODE;
            break;
        case PROGRESS_STYLE_MINIMAL:
            /* Just percentage */
            printf("\r  %3d%% (%zu/%zu)   ", percent, bar->current, bar->total);
            fflush(stdout);
            return;
        default:
            chars = &PROGRESS_UNICODE;
    }
    
    /* Calculate filled portion */
    int filled_width = (int)((bar->current * bar->width) / bar->total);
    int empty_width = bar->width - filled_width;
    
    /* Print progress bar */
    printf("\r  ");
    
    /* Prefix */
    if (bar->prefix) {
        printf("%s ", bar->prefix);
    }
    
    /* Bar */
    printf("%s", chars->left);
    
    /* Use color for filled part */
    if (colors_enabled()) {
        printf("%s", COLOR_GREEN);
    }
    
    for (int i = 0; i < filled_width; i++) {
        printf("%s", chars->filled);
    }
    
    if (colors_enabled()) {
        printf("%s", COLOR_RESET);
    }
    
    for (int i = 0; i < empty_width; i++) {
        printf("%s", chars->empty);
    }
    
    printf("%s", chars->right);
    
    /* Percentage */
    if (bar->show_percent) {
        printf(" %3d%%", percent);
    }
    
    /* Count */
    if (bar->show_count) {
        printf(" (%zu/%zu)", bar->current, bar->total);
    }
    
    /* Padding to clear previous longer text */
    printf("   ");
    
    fflush(stdout);
}

void progress_increment(ProgressBar *bar) {
    if (bar) {
        progress_update(bar, bar->current + 1);
    }
}

void progress_complete(ProgressBar *bar) {
    if (!bar) return;
    
    bar->current = bar->total;
    progress_update(bar, bar->total);
    printf("\n");
}

void progress_set_prefix(ProgressBar *bar, const char *prefix) {
    if (bar) {
        bar->prefix = prefix;
    }
}