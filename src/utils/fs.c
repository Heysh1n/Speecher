/**
 * @file    fs.c
 * @brief   File system operations - Full implementation
 * @author  SPEECHER Team
 * @date    2025
 */

/* POSIX for directory operations - MUST be before includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
#endif

#include "utils/fs.h"
#include "utils/platform.h"
#include "utils/strings.h"
#include "speecher.h"  
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef WINDOWS
    #include <windows.h>
    #include <io.h>
#else
    #include <dirent.h>
    #include <unistd.h>
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Path Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

bool fs_path_join(char *dest, size_t size, const char *path1, const char *path2) {
    if (!dest || !size) return false;
    
    if (!path1 || !*path1) {
        if (path2) {
            str_copy(dest, path2, size);
        } else {
            dest[0] = '\0';
        }
        return true;
    }
    
    if (!path2 || !*path2) {
        str_copy(dest, path1, size);
        return true;
    }
    
    size_t len1 = strlen(path1);
    bool has_sep = (path1[len1 - 1] == PATH_SEPARATOR);
    bool path2_sep = (path2[0] == PATH_SEPARATOR);
    
    if (has_sep && path2_sep) {
        snprintf(dest, size, "%s%s", path1, path2 + 1);
    } else if (has_sep || path2_sep) {
        snprintf(dest, size, "%s%s", path1, path2);
    } else {
        snprintf(dest, size, "%s%c%s", path1, PATH_SEPARATOR, path2);
    }
    
    return true;
}

const char *fs_path_extension(const char *path) {
    if (!path) return "";
    const char *dot = strrchr(path, '.');
    const char *sep = strrchr(path, PATH_SEPARATOR);
    if (!dot || (sep && dot < sep)) return "";
    return dot;
}

const char *fs_path_filename(const char *path) {
    if (!path) return "";
    const char *sep = strrchr(path, PATH_SEPARATOR);
    return sep ? sep + 1 : path;
}

bool fs_path_dirname(char *dest, size_t size, const char *path) {
    if (!dest || !size || !path) return false;
    
    char temp[4096];
    str_copy(temp, path, sizeof(temp));
    
    size_t len = strlen(temp);
    while (len > 1 && temp[len - 1] == PATH_SEPARATOR) {
        temp[--len] = '\0';
    }
    
    char *sep = strrchr(temp, PATH_SEPARATOR);
    
    if (!sep) {
        dest[0] = '.';
        dest[1] = '\0';
        return true;
    }
    
    if (sep == temp) {
        dest[0] = PATH_SEPARATOR;
        dest[1] = '\0';
        return true;
    }
    
    *sep = '\0';
    str_copy(dest, temp, size);
    
    return true;
}

void fs_path_normalize(char *path) {
    if (!path) return;
#ifdef WINDOWS
    for (char *p = path; *p; p++) if (*p == '/') *p = '\\';
#else
    for (char *p = path; *p; p++) if (*p == '\\') *p = '/';
#endif
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              File/Directory Checks
 * ══════════════════════════════════════════════════════════════════════════════ */

bool fs_exists(const char *path) {
    if (!path) return false;
    
#ifdef WINDOWS
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

bool fs_is_file(const char *path) {
    if (!path) return false;
    
#ifdef WINDOWS
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return false;
    return !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
#endif
}

bool fs_is_directory(const char *path) {
    if (!path) return false;
    
#ifdef WINDOWS
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
#endif
}

int64_t fs_file_size(const char *path) {
    if (!path) return -1;
    
#ifdef WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) return -1;
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return (int64_t)size.QuadPart;
#else
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
#endif
}

bool fs_file_info(const char *path, FsFileInfo *info) {
    if (!path || !info) return false;
    
    memset(info, 0, sizeof(FsFileInfo));
    
    str_copy(info->path, path, sizeof(info->path));
    str_copy(info->name, fs_path_filename(path), sizeof(info->name));
    
#ifdef WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) return false;
    
    if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        info->type = FS_TYPE_DIRECTORY;
    } else {
        info->type = FS_TYPE_FILE;
    }
    
    LARGE_INTEGER size;
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    info->size = (uint64_t)size.QuadPart;
    
    ULARGE_INTEGER ull;
    ull.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fad.ftLastWriteTime.dwHighDateTime;
    info->modified = (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
    
    ull.LowPart = fad.ftCreationTime.dwLowDateTime;
    ull.HighPart = fad.ftCreationTime.dwHighDateTime;
    info->created = (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    
    if (S_ISDIR(st.st_mode)) {
        info->type = FS_TYPE_DIRECTORY;
    } else if (S_ISREG(st.st_mode)) {
        info->type = FS_TYPE_FILE;
    } else {
        info->type = FS_TYPE_UNKNOWN;
    }
    
    info->size = (uint64_t)st.st_size;
    info->modified = st.st_mtime;
    info->created = st.st_ctime;
#endif
    
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Directory Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

bool fs_mkdir(const char *path) {
    if (!path) return false;
    
    if (fs_is_directory(path)) return true;
    
    int result = platform_mkdir(path);
    return (result == 0);
}

bool fs_mkdir_recursive(const char *path) {
    if (!path || !*path) return false;
    
    if (fs_is_directory(path)) return true;
    
    char tmp[4096];
    str_copy(tmp, path, sizeof(tmp));
    
    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == PATH_SEPARATOR) {
        tmp[len - 1] = '\0';
    }
    
    for (char *p = tmp + 1; *p; p++) {
        if (*p == PATH_SEPARATOR) {
            *p = '\0';
            if (!fs_is_directory(tmp)) {
                fs_mkdir(tmp);
            }
            *p = PATH_SEPARATOR;
        }
    }
    
    return fs_mkdir(tmp);
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Directory Listing
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Add entry to file list, growing if needed
 */
static bool fs_list_add(FsFileList *list, const FsFileInfo *info) {
    if (!list || !info) return false;
    
    /* Grow array if needed */
    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        FsFileInfo *new_entries = realloc(list->entries, new_capacity * sizeof(FsFileInfo));
        if (!new_entries) return false;
        list->entries = new_entries;
        list->capacity = new_capacity;
    }
    
    list->entries[list->count++] = *info;
    return true;
}

bool fs_list_directory(const char *path, FsFileList *list, bool recursive, int max_depth) {
    if (!path || !list) return false;
    
    /* Initialize list */
    memset(list, 0, sizeof(FsFileList));
    
#ifdef WINDOWS
    char search_path[4096];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    do {
        /* Skip . and .. */
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
            continue;
        }
        
        FsFileInfo info;
        memset(&info, 0, sizeof(info));
        
        str_copy(info.name, fd.cFileName, sizeof(info.name));
        fs_path_join(info.path, sizeof(info.path), path, fd.cFileName);
        
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            info.type = FS_TYPE_DIRECTORY;
            
            /* Recurse into subdirectory */
            if (recursive && max_depth != 1) {
                FsFileList sublist;
                int new_depth = max_depth > 0 ? max_depth - 1 : 0;
                if (fs_list_directory(info.path, &sublist, true, new_depth)) {
                    /* Add all entries from sublist */
                    for (size_t i = 0; i < sublist.count; i++) {
                        fs_list_add(list, &sublist.entries[i]);
                    }
                    fs_list_free(&sublist);
                }
            }
        } else {
            info.type = FS_TYPE_FILE;
            
            LARGE_INTEGER size;
            size.HighPart = fd.nFileSizeHigh;
            size.LowPart = fd.nFileSizeLow;
            info.size = (uint64_t)size.QuadPart;
            
            /* Add file to list */
            fs_list_add(list, &info);
        }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
    return true;

#else
    /* Linux / macOS */
    DIR *dir = opendir(path);
    if (!dir) {
        return false;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        FsFileInfo info;
        memset(&info, 0, sizeof(info));
        
        /* Copy name */
        str_copy(info.name, entry->d_name, sizeof(info.name));
        
        /* Build full path */
        fs_path_join(info.path, sizeof(info.path), path, entry->d_name);
        
        /* Get file info using stat */
        struct stat st;
        if (stat(info.path, &st) != 0) {
            /* Can't stat, skip this file */
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            info.type = FS_TYPE_DIRECTORY;
            
            /* Recurse into subdirectory */
            if (recursive && max_depth != 1) {
                FsFileList sublist;
                int new_depth = max_depth > 0 ? max_depth - 1 : 0;
                if (fs_list_directory(info.path, &sublist, true, new_depth)) {
                    for (size_t i = 0; i < sublist.count; i++) {
                        fs_list_add(list, &sublist.entries[i]);
                    }
                    fs_list_free(&sublist);
                }
            }
        } else if (S_ISREG(st.st_mode)) {
            info.type = FS_TYPE_FILE;
            info.size = (uint64_t)st.st_size;
            info.modified = st.st_mtime;
            info.created = st.st_ctime;
            
            /* Add file to list */
            fs_list_add(list, &info);
        }
        /* Skip other file types (symlinks, devices, etc.) */
    }
    
    closedir(dir);
    return true;
#endif
}

void fs_list_free(FsFileList *list) {
    if (list) {
        if (list->entries) {
            free(list->entries);
        }
        list->entries = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              File Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

char *fs_read_file(const char *path, size_t *out_size) {
    if (!path) return NULL;
    
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    
    buffer[bytes_read] = '\0';
    
    if (out_size) *out_size = bytes_read;
    
    return buffer;
}

bool fs_write_file(const char *path, const void *data, size_t size) {
    if (!path || !data) return false;
    
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return written == size;
}

bool fs_append_file(const char *path, const void *data, size_t size) {
    if (!path || !data) return false;
    
    FILE *f = fopen(path, "ab");
    if (!f) return false;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return written == size;
}

bool fs_copy_file(const char *src, const char *dest) {
    if (!src || !dest) return false;
    
    size_t size;
    char *data = fs_read_file(src, &size);
    if (!data) return false;
    
    bool result = fs_write_file(dest, data, size);
    free(data);
    
    return result;
}

bool fs_move_file(const char *src, const char *dest) {
    if (!src || !dest) return false;
    return rename(src, dest) == 0;
}
bool fs_delete_file(const char *path) {
    if (!path) return false;
    return remove(path) == 0;
}
/* ══════════════════════════════════════════════════════════════════════════════
 *                              File Time
 * ══════════════════════════════════════════════════════════════════════════════ */

#include <sys/stat.h>

time_t fs_file_mtime(const char *path) {
    if (!path) return 0;
#ifdef WINDOWS
    struct _stat st;
    if (_stat(path, &st) != 0) return 0;
#else
    struct stat st;
    if (stat(path, &st) != 0) return 0;
#endif
    return st.st_mtime;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Directory Reading
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifdef WINDOWS

struct FsDir {
    HANDLE handle;
    WIN32_FIND_DATAA data;
    bool first;
};

FsDir *fs_dir_open(const char *path) {
    if (!path) return NULL;
    
    char pattern[SPEECHER_PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    
    FsDir *dir = malloc(sizeof(FsDir));
    if (!dir) return NULL;
    
    dir->handle = FindFirstFileA(pattern, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        free(dir);
        return NULL;
    }
    
    dir->first = true;
    return dir;
}

bool fs_dir_read(FsDir *dir, char *name, size_t size, bool *is_dir) {
    if (!dir || !name) return false;
    
    while (true) {
        if (dir->first) {
            dir->first = false;
        } else if (!FindNextFileA(dir->handle, &dir->data)) {
            return false;
        }
        
        if (strcmp(dir->data.cFileName, ".") == 0) continue;
        if (strcmp(dir->data.cFileName, "..") == 0) continue;
        
        str_copy(name, dir->data.cFileName, size);
        if (is_dir) *is_dir = (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        return true;
    }
}

void fs_dir_close(FsDir *dir) {
    if (dir) {
        if (dir->handle != INVALID_HANDLE_VALUE) FindClose(dir->handle);
        free(dir);
    }
}

#else /* Unix */

#include <dirent.h>

struct FsDir {
    DIR *handle;
    char path[SPEECHER_PATH_MAX];
};

FsDir *fs_dir_open(const char *path) {
    if (!path) return NULL;
    
    FsDir *dir = malloc(sizeof(FsDir));
    if (!dir) return NULL;
    
    dir->handle = opendir(path);
    if (!dir->handle) {
        free(dir);
        return NULL;
    }
    
    str_copy(dir->path, path, sizeof(dir->path));
    return dir;
}

bool fs_dir_read(FsDir *dir, char *name, size_t size, bool *is_dir) {
    if (!dir || !name) return false;
    
    struct dirent *entry;
    while ((entry = readdir(dir->handle))) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;
        
        str_copy(name, entry->d_name, size);
        
        if (is_dir) {
            char full[SPEECHER_PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dir->path, entry->d_name);
            struct stat st;
            *is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
        }
        return true;
    }
    return false;
}

void fs_dir_close(FsDir *dir) {
    if (dir) {
        if (dir->handle) closedir(dir->handle);
        free(dir);
    }
}

#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Directory Copy/Move Operations
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Remove directory (helper function)
 */
static bool fs_remove_directory_internal(const char *path, bool recursive) {
    if (!path) return false;
    
    if (!recursive) {
#ifdef WINDOWS
        return RemoveDirectoryA(path) != 0;
#else
        return rmdir(path) == 0;
#endif
    }
    
    /* Рекурсивное удаление */
    FsDir *dir = fs_dir_open(path);
    if (!dir) {
        /* Директория не существует или не может быть открыта */
#ifdef WINDOWS
        return RemoveDirectoryA(path) != 0;
#else
        return rmdir(path) == 0;
#endif
    }
    
    char name[512];
    bool is_dir;
    char full_path[SPEECHER_PATH_MAX];
    
    while (fs_dir_read(dir, name, sizeof(name), &is_dir)) {
        fs_path_join(full_path, sizeof(full_path), path, name);
        
        if (is_dir) {
            fs_remove_directory_internal(full_path, true);
        } else {
            remove(full_path);
        }
    }
    
    fs_dir_close(dir);
    
#ifdef WINDOWS
    return RemoveDirectoryA(path) != 0;
#else
    return rmdir(path) == 0;
#endif
}

bool fs_copy_directory(const char *src, const char *dst) {
    if (!src || !dst) return false;
    
    /* Проверяем что источник существует */
    if (!fs_is_directory(src)) {
        return false;
    }
    
    /* Создаём целевую директорию */
    if (!fs_is_directory(dst)) {
        if (!fs_mkdir_recursive(dst)) {
            return false;
        }
    }
    
    /* Открываем исходную директорию */
    FsDir *dir = fs_dir_open(src);
    if (!dir) {
        return true; /* Пустая директория - OK */
    }
    
    bool success = true;
    char name[512];
    bool is_dir;
    char src_path[SPEECHER_PATH_MAX];
    char dst_path[SPEECHER_PATH_MAX];
    
    while (fs_dir_read(dir, name, sizeof(name), &is_dir)) {
        fs_path_join(src_path, sizeof(src_path), src, name);
        fs_path_join(dst_path, sizeof(dst_path), dst, name);
        
        if (is_dir) {
            /* Рекурсивно копируем поддиректорию */
            if (!fs_copy_directory(src_path, dst_path)) {
                success = false;
            }
        } else {
            /* Копируем файл */
            if (!fs_copy_file(src_path, dst_path)) {
                success = false;
            }
        }
    }
    
    fs_dir_close(dir);
    return success;
}

bool fs_move_directory(const char *old_path, const char *new_path) {
    if (!old_path || !new_path) return false;
    
    /* Если старая папка не существует - создаём новую и выходим */
    if (!fs_is_directory(old_path)) {
        return fs_mkdir_recursive(new_path);
    }
    
    /* Если пути одинаковые - ничего делать не надо */
    if (strcmp(old_path, new_path) == 0) {
        return true;
    }
    
    /* Если новая папка уже существует */
    if (fs_is_directory(new_path)) {
        /* Папка уже есть, копируем содержимое старой в новую */
        if (fs_copy_directory(old_path, new_path)) {
            fs_remove_directory_internal(old_path, true);
            return true;
        }
        return false;
    }
    
    /* Создаём родительскую директорию для нового пути */
    char parent[SPEECHER_PATH_MAX];
    if (fs_path_dirname(parent, sizeof(parent), new_path)) {
        fs_mkdir_recursive(parent);
    }
    
    /* Пробуем переименовать (быстрый способ, работает на одном диске) */
#ifdef WINDOWS
    if (MoveFileExA(old_path, new_path, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
        return true;
    }
#else
    if (rename(old_path, new_path) == 0) {
        return true;
    }
#endif
    
    /* Если переименование не сработало - копируем и удаляем */
    if (fs_copy_directory(old_path, new_path)) {
        fs_remove_directory_internal(old_path, true);
        return true;
    }
    
    return false;
}

bool fs_is_dir_empty(const char *path) {
    FsDir *dir = fs_dir_open(path);
    if (!dir) return true;
    
    char name[256];
    bool has_entry = fs_dir_read(dir, name, sizeof(name), NULL);
    fs_dir_close(dir);
    
    return !has_entry;
}