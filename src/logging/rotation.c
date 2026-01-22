/**
 * @file    rotation.c
 * @brief   Log rotation - Full implementation
 * @author  SPEECHER Team
 * @date    2025
 */

#include "logging/rotation.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "utils/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool rotation_needed(const char *filepath, RotationMode mode, int max_size_mb) {
    if (!filepath || mode == ROTATION_NONE) {
        return false;
    }
    
    if (!fs_exists(filepath)) {
        return false;
    }
    
    if (mode == ROTATION_SIZE || mode == ROTATION_BOTH) {
        int64_t size = fs_file_size(filepath);
        int64_t max_bytes = (int64_t)max_size_mb * 1024 * 1024;
        
        if (size >= max_bytes) {
            return true;
        }
    }
    
    /* Date-based rotation is handled by filename generation */
    return false;
}

bool rotation_rotate(const char *filepath, int max_files) {
    if (!filepath) return false;
    
    /* Generate rotated filename with timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    /* Build new filename: original.log -> original_20250117_221500.log */
    char new_path[4096];
    char dir[4096];
    char name[256];
    
    fs_path_dirname(dir, sizeof(dir), filepath);
    str_copy(name, fs_path_filename(filepath), sizeof(name));
    
    /* Find extension */
    char *dot = strrchr(name, '.');
    if (dot) {
        char ext[32];
        str_copy(ext, dot, sizeof(ext));
        *dot = '\0';
        snprintf(new_path, sizeof(new_path), "%s%c%s_%s%s",
                 dir, PATH_SEPARATOR, name, timestamp, ext);
    } else {
        snprintf(new_path, sizeof(new_path), "%s%c%s_%s",
                 dir, PATH_SEPARATOR, name, timestamp);
    }
    
    /* Rename current log file */
    if (!fs_move_file(filepath, new_path)) {
        return false;
    }
    
    /* Clean up old log files if max_files is set */
    if (max_files > 0) {
        FsFileList files;
        if (fs_list_directory(dir, &files, false, 1)) {
            /* Count log files with same base name */
            /* TODO: Delete oldest files if count > max_files */
            fs_list_free(&files);
        }
    }
    
    return true;
}

bool rotation_generate_filename(char *dest, size_t size, const char *name_template) {
    if (!dest || !size || !name_template) return false;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
    
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H-%M-%S", tm_info);
    
    /* Replace placeholders */
    char result[1024];
    char *out = result;
    size_t remaining = sizeof(result) - 1;
    
    const char *p = name_template;
    while (*p && remaining > 0) {
        if (strncmp(p, "{date}", 6) == 0) {
            size_t len = strlen(date_str);
            if (len > remaining) len = remaining;
            memcpy(out, date_str, len);
            out += len;
            remaining -= len;
            p += 6;
        } else if (strncmp(p, "{time}", 6) == 0) {
            size_t len = strlen(time_str);
            if (len > remaining) len = remaining;
            memcpy(out, time_str, len);
            out += len;
            remaining -= len;
            p += 6;
        } else {
            *out++ = *p++;
            remaining--;
        }
    }
    *out = '\0';
    
    str_copy(dest, result, size);
    return true;
}