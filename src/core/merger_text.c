/**
 * @file    merger_text.c
 * @brief   Text file merger - Full implementation with i18n
 * @author  SPEECHER Team
 * @date    2025
 */

#include "core/merger_text.h"
#include "i18n/lang.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Sorting
 * ══════════════════════════════════════════════════════════════════════════════ */

typedef int (*CompareFunc)(const void *, const void *);

static int compare_name_asc(const void *a, const void *b) {
    const FsFileInfo *fa = (const FsFileInfo *)a;
    const FsFileInfo *fb = (const FsFileInfo *)b;
    return str_compare_nocase(fa->name, fb->name);
}

static int compare_name_desc(const void *a, const void *b) {
    return -compare_name_asc(a, b);
}

static int compare_date_asc(const void *a, const void *b) {
    const FsFileInfo *fa = (const FsFileInfo *)a;
    const FsFileInfo *fb = (const FsFileInfo *)b;
    if (fa->modified < fb->modified) return -1;
    if (fa->modified > fb->modified) return 1;
    return 0;
}

static int compare_date_desc(const void *a, const void *b) {
    return -compare_date_asc(a, b);
}

static int compare_size_asc(const void *a, const void *b) {
    const FsFileInfo *fa = (const FsFileInfo *)a;
    const FsFileInfo *fb = (const FsFileInfo *)b;
    if (fa->size < fb->size) return -1;
    if (fa->size > fb->size) return 1;
    return 0;
}

static int compare_size_desc(const void *a, const void *b) {
    return -compare_size_asc(a, b);
}

static CompareFunc get_compare_func(const char *sort_order) {
    if (strcmp(sort_order, "name_desc") == 0) return compare_name_desc;
    if (strcmp(sort_order, "date_asc") == 0) return compare_date_asc;
    if (strcmp(sort_order, "date_desc") == 0) return compare_date_desc;
    if (strcmp(sort_order, "size_asc") == 0) return compare_size_asc;
    if (strcmp(sort_order, "size_desc") == 0) return compare_size_desc;
    return compare_name_asc;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Filtering
 * ══════════════════════════════════════════════════════════════════════════════ */

static bool is_extension_allowed(const char *filename, const char *extensions) {
    const char *ext = fs_path_extension(filename);
    if (!ext || !*ext) return false;
    
    char ext_copy[512];
    str_copy(ext_copy, extensions, sizeof(ext_copy));
    
    char *start = ext_copy;
    char *end;
    
    while (*start) {
        while (*start == ' ' || *start == ',') start++;
        if (!*start) break;
        
        end = start;
        while (*end && *end != ',' && *end != ' ') end++;
        
        char saved = *end;
        *end = '\0';
        
        if (str_compare_nocase(ext, start) == 0) {
            return true;
        }
        
        if (saved) {
            *end = saved;
            start = end + 1;
        } else {
            break;
        }
    }
    
    return false;
}

static void filter_by_extension(FsFileList *list, const char *extensions) {
    if (!list || !extensions) return;
    
    size_t write_idx = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (is_extension_allowed(list->entries[i].name, extensions)) {
            if (write_idx != i) {
                list->entries[write_idx] = list->entries[i];
            }
            write_idx++;
        }
    }
    list->count = write_idx;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Helpers
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Format file size with localized units
 */
static void format_size(size_t bytes, char *buffer, size_t buf_size) {
    if (bytes >= 1073741824) {
        snprintf(buffer, buf_size, "%.2f %s", 
                 (double)bytes / 1073741824.0, lang_size("gb"));
    } else if (bytes >= 1048576) {
        snprintf(buffer, buf_size, "%.2f %s", 
                 (double)bytes / 1048576.0, lang_size("mb"));
    } else if (bytes >= 1024) {
        snprintf(buffer, buf_size, "%.2f %s", 
                 (double)bytes / 1024.0, lang_size("kb"));
    } else {
        snprintf(buffer, buf_size, "%zu %s", bytes, lang_size("bytes"));
    }
}

/**
 * @brief Generate output filename from template
 */
static void generate_output_filename(char *dest, size_t size, 
                                      const char *tmpl, const char *format) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char date_str[32];
    char time_str[32];
    
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
    strftime(time_str, sizeof(time_str), "%H-%M-%S", tm_info);
    
    char result[512];
    char *out = result;
    size_t remaining = sizeof(result);
    
    while (*tmpl && remaining > 1) {
        if (strncmp(tmpl, "{date}", 6) == 0) {
            size_t len = strlen(date_str);
            if (len < remaining) {
                memcpy(out, date_str, len);
                out += len;
                remaining -= len;
            }
            tmpl += 6;
        } else if (strncmp(tmpl, "{time}", 6) == 0) {
            size_t len = strlen(time_str);
            if (len < remaining) {
                memcpy(out, time_str, len);
                out += len;
                remaining -= len;
            }
            tmpl += 6;
        } else {
            *out++ = *tmpl++;
            remaining--;
        }
    }
    *out = '\0';
    
    snprintf(dest, size, "%s%s", result, format);
}

/**
 * @brief Write separator to file
 */
static void write_separator(FILE *f, const Config *cfg, const FsFileInfo *file_info,
                            size_t index, size_t total) {
    if (!cfg->text_separator_enabled) return;
    
    char sep_line[256];
    int sep_len = cfg->text_separator_length;
    if (sep_len > (int)sizeof(sep_line) - 1) sep_len = sizeof(sep_line) - 1;
    
    memset(sep_line, cfg->text_separator_style[0], sep_len);
    sep_line[sep_len] = '\0';
    
    char size_str[32];
    format_size(file_info->size, size_str, sizeof(size_str));
    
    fprintf(f, "%s\n", sep_line);
    fprintf(f, "  File: %s\n", file_info->name);
    fprintf(f, "  Path: %s\n", file_info->path);
    fprintf(f, "  %s: %s\n", lang_text("total_size"), size_str);
    fprintf(f, "  [%zu / %zu]\n", index + 1, total);
    fprintf(f, "%s\n\n", sep_line);
}

/**
 * @brief Print section header
 */
static void print_header(void) {
    printf("\n");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════");
    color_println(COLOR_CYAN, "           %s", lang_text("title"));
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════");
    printf("\n");
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Public API
 * ══════════════════════════════════════════════════════════════════════════════ */

bool merger_text_run(void) {
    Config *cfg = config_get();
    
    print_header();
    
    /* Get input directory */
    char input_dir[SPEECHER_PATH_MAX];
    fs_path_join(input_dir, sizeof(input_dir), app_get_base_dir(), cfg->input_dir);
    
    LOG_INFO("Scanning directory: %s", input_dir);
    printf("  %s: %s\n", lang_settings("input_dir"), input_dir);
    printf("  %s: %s\n", lang_settings("extensions"), cfg->text_extensions);
    printf("  %s: %s (%s: %d)\n\n", 
           lang_settings("recursive"),
           cfg->text_recursive ? lang_msg("found") : lang_msg("not_found"), 
           lang_settings("max_depth"), cfg->text_max_depth);
    
    /* Check if input directory exists */
    if (!fs_is_directory(input_dir)) {
        color_println(COLOR_RED, "  %s: %s", lang_err("dir_not_exist"), input_dir);
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    /* Scan directory */
    printf("  %s\n", lang_text("scanning"));
    
    FsFileList files;
    memset(&files, 0, sizeof(files));
    
    bool list_result = fs_list_directory(input_dir, &files, cfg->text_recursive, cfg->text_max_depth);
    
    if (!list_result) {
        color_println(COLOR_RED, "  %s", lang_err("cannot_read"));
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    /* Filter by extension */
    size_t before_filter = files.count;
    filter_by_extension(&files, cfg->text_extensions);
    
    LOG_DEBUG("Files before filter: %zu, after: %zu", before_filter, files.count);
    
    if (files.count == 0) {
        color_println(COLOR_YELLOW, "  %s", lang_text("no_files"));
        printf("  %s: %s\n", lang_settings("extensions"), cfg->text_extensions);
        
        /* Show what files ARE there */
        FsFileList all_files;
        memset(&all_files, 0, sizeof(all_files));
        if (fs_list_directory(input_dir, &all_files, false, 1)) {
            if (all_files.count > 0) {
                printf("\n  Files in directory:\n");
                for (size_t i = 0; i < all_files.count && i < 10; i++) {
                    printf("    - %s\n", all_files.entries[i].name);
                }
            }
            fs_list_free(&all_files);
        }
        
        fs_list_free(&files);
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    /* Sort files */
    CompareFunc cmp = get_compare_func(cfg->text_sort_order);
    qsort(files.entries, files.count, sizeof(FsFileInfo), cmp);
    
    /* Calculate total size */
    size_t total_size = 0;
    for (size_t i = 0; i < files.count; i++) {
        total_size += files.entries[i].size;
    }
    
    /* Show file list */
    char size_str[32];
    format_size(total_size, size_str, sizeof(size_str));
    
    printf("\n  %s: ", lang_text("found_files"));
    color_print(COLOR_GREEN, "%zu", files.count);
    printf(" (%s)\n\n", size_str);
    
    for (size_t i = 0; i < files.count && i < 10; i++) {
        char fsize[32];
        format_size(files.entries[i].size, fsize, sizeof(fsize));
        printf("    %zu. %s (%s)\n", i + 1, files.entries[i].name, fsize);
    }
    if (files.count > 10) {
        printf("    ... +%zu %s\n", files.count - 10, lang_msg("files"));
    }
    printf("\n");
    
    /* Confirm */
    if (!menu_confirm(lang_text("proceed"))) {
        color_println(COLOR_YELLOW, "  %s", lang_msg("cancelled"));
        fs_list_free(&files);
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    /* Generate output filename */
    char output_filename[512];
    generate_output_filename(output_filename, sizeof(output_filename),
                             cfg->text_output_template, cfg->text_output_format);
    
    char output_dir[SPEECHER_PATH_MAX];
    fs_path_join(output_dir, sizeof(output_dir), app_get_base_dir(), cfg->output_dir);
    fs_mkdir_recursive(output_dir);
    
    char output_path[SPEECHER_PATH_MAX];
    fs_path_join(output_path, sizeof(output_path), output_dir, output_filename);
    
    LOG_INFO("Output file: %s", output_path);
    printf("  %s: %s\n", lang_text("output"), output_path);
    
    /* Open output file */
    FILE *out = fopen(output_path, "wb");
    if (!out) {
        color_println(COLOR_RED, "  %s: %s", lang_err("cannot_create"), output_path);
        LOG_ERROR("Could not create: %s", output_path);
        fs_list_free(&files);
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    /* Write header */
    fprintf(out, "# Merged by SPEECHER v%s\n", VERSION);
    fprintf(out, "# Files: %zu\n\n", files.count);
    
    /* Create progress bar */
    ProgressBar *progress = progress_create(files.count, cfg->ui_progress_width,
                                            PROGRESS_STYLE_UNICODE);
    
    printf("\n  %s\n", lang_text("merging"));
    
    /* Process files */
    size_t total_bytes = 0;
    size_t success_count = 0;
    size_t total_lines = 0;
    
    for (size_t i = 0; i < files.count; i++) {
        progress_update(progress, i);
        
        /* Write separator */
        write_separator(out, cfg, &files.entries[i], i, files.count);
        
        /* Read and write file content */
        size_t file_size;
        char *content = fs_read_file(files.entries[i].path, &file_size);
        
        if (content) {
            fwrite(content, 1, file_size, out);
            fwrite("\n\n", 1, 2, out);
            
            /* Count lines */
            for (size_t j = 0; j < file_size; j++) {
                if (content[j] == '\n') total_lines++;
            }
            
            total_bytes += file_size;
            success_count++;
            free(content);
        } else {
            LOG_WARN("Could not read: %s", files.entries[i].path);
            fprintf(out, "[ERROR: %s]\n\n", lang_err("cannot_read"));
        }
    }
    
    progress_complete(progress);
    progress_free(progress);
    
    fclose(out);
    
    /* Summary */
    printf("\n");
    color_println(COLOR_GREEN, "  ════════════════════════════════════════");
    color_println(COLOR_GREEN, "         ✓ %s", lang_text("complete"));
    color_println(COLOR_GREEN, "  ════════════════════════════════════════");
    printf("\n");
    
    format_size(total_bytes, size_str, sizeof(size_str));
    
    printf("  %s: %zu / %zu\n", lang_text("files_merged"), success_count, files.count);
    printf("  %s: %s\n", lang_text("total_size"), size_str);
    printf("  %s: %zu\n", lang_text("total_lines"), total_lines);
    printf("  %s: %s\n", lang_text("output"), output_filename);
    
    LOG_INFO("Merge complete: %zu files, %s -> %s", 
             success_count, size_str, output_filename);
    
    fs_list_free(&files);
    menu_pause(lang_msg("press_enter"));
    
    return true;
}