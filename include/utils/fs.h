/**
 * @file    fs.h
 * @brief   File system operations
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UTILS_FS_H
#define UTILS_FS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Types
 * ══════════════════════════════════════════════════════════════════════════════ */

/** File type enumeration */
typedef enum {
    FS_TYPE_UNKNOWN = 0,
    FS_TYPE_FILE,
    FS_TYPE_DIRECTORY,
    FS_TYPE_SYMLINK,
} FsFileType;

/** File information structure */
typedef struct {
    char name[256];         /* File name only */
    char path[4096];        /* Full path */
    FsFileType type;        /* File type */
    uint64_t size;          /* Size in bytes */
    time_t modified;        /* Last modification time */
    time_t created;         /* Creation time */
} FsFileInfo;

/** Directory entry list */
typedef struct {
    FsFileInfo *entries;    /* Array of entries */
    size_t count;           /* Number of entries */
    size_t capacity;        /* Allocated capacity */
} FsFileList;

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Path Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Join two path components
 * @param dest Destination buffer
 * @param size Buffer size
 * @param path1 First path component
 * @param path2 Second path component
 * @return true on success
 */
bool fs_path_join(char *dest, size_t size, const char *path1, const char *path2);

/**
 * @brief Get file extension (including dot)
 * @param path File path
 * @return Pointer to extension or empty string
 */
const char *fs_path_extension(const char *path);

/**
 * @brief Get file name from path
 * @param path File path
 * @return Pointer to filename
 */
const char *fs_path_filename(const char *path);

/**
 * @brief Get directory part of path
 * @param dest Destination buffer
 * @param size Buffer size
 * @param path File path
 * @return true on success
 */
bool fs_path_dirname(char *dest, size_t size, const char *path);

/**
 * @brief Normalize path separators for current platform
 */
void fs_path_normalize(char *path);

/* ══════════════════════════════════════════════════════════════════════════════
 *                           File/Directory Checks
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Check if path exists
 */
bool fs_exists(const char *path);

/**
 * @brief Check if path is a file
 */
bool fs_is_file(const char *path);

/**
 * @brief Check if path is a directory
 */
bool fs_is_directory(const char *path);

/**
 * @brief Get file size in bytes
 * @return File size or -1 on error
 */
int64_t fs_file_size(const char *path);

/**
 * @brief Get file info
 * @param path File path
 * @param info Output file info structure
 * @return true on success
 */
bool fs_file_info(const char *path, FsFileInfo *info);

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Directory Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Create directory (single level)
 * @return true on success or if already exists
 */
bool fs_mkdir(const char *path);

/**
 * @brief Create directory and all parent directories
 * @return true on success
 */
bool fs_mkdir_recursive(const char *path);

/**
 * @brief List directory contents
 * @param path Directory path
 * @param list Output file list (must be freed with fs_list_free)
 * @param recursive Include subdirectories
 * @param max_depth Maximum recursion depth (0 = unlimited)
 * @return true on success
 */
bool fs_list_directory(const char *path, FsFileList *list, 
                       bool recursive, int max_depth);

/**
 * @brief Free file list
 */
void fs_list_free(FsFileList *list);

/* ══════════════════════════════════════════════════════════════════════════════
 *                           File Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Read entire file into memory
 * @param path File path
 * @param size Output size (can be NULL)
 * @return File contents (caller must free) or NULL on error
 */
char *fs_read_file(const char *path, size_t *size);

/**
 * @brief Write data to file
 * @param path File path
 * @param data Data to write
 * @param size Data size
 * @return true on success
 */
bool fs_write_file(const char *path, const void *data, size_t size);

/**
 * @brief Append data to file
 */
bool fs_append_file(const char *path, const void *data, size_t size);

/**
 * @brief Copy file
 * @return true on success
 */
bool fs_copy_file(const char *src, const char *dest);

/**
 * @brief Move/rename file
 * @return true on success
 */
bool fs_move_file(const char *src, const char *dest);

/**
 * @brief Delete file
 * @return true on success
 */
bool fs_delete_file(const char *path);

/**
 * @brief Get file modification time
 */
time_t fs_file_mtime(const char *path);

/**
 * @brief Directory handle
 */
typedef struct FsDir FsDir;

/**
 * @brief Open directory
 */
FsDir *fs_dir_open(const char *path);

/**
 * @brief Read next entry
 */
bool fs_dir_read(FsDir *dir, char *name, size_t size, bool *is_dir);

/**
 * @brief Close directory
 */
void fs_dir_close(FsDir *dir);

/**
 * @brief Check if directory is empty
 */
bool fs_is_dir_empty(const char *path);

/**
 * @brief Move/rename directory
 * @param old_path Source directory path
 * @param new_path Destination directory path
 * @return true on success
 */
bool fs_move_directory(const char *old_path, const char *new_path);

/**
 * @brief Copy directory recursively
 * @param src Source directory
 * @param dst Destination directory
 * @return true on success
 */
bool fs_copy_directory(const char *src, const char *dst);

#endif /* UTILS_FS_H */