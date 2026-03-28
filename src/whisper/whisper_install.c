/**
 * @file    whisper_install.c
 * @brief   Whisper installation
 */

/* POSIX - MUST be before includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
#endif

#include "whisper/whisper_internal.h"
#include "whisper/whisper_manager.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "ui/colors.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Constants
 * ══════════════════════════════════════════════════════════════════════════════ */

#define MODEL_URL_BASE "https://huggingface.co/ggerganov/whisper.cpp/resolve/main"

#ifdef WINDOWS
static const char *RELEASE_URLS[] = {
    "https://github.com/ggerganov/whisper.cpp/releases/latest/download/whisper-bin-x64.zip",
    "https://github.com/ggerganov/whisper.cpp/releases/download/v1.7.3/whisper-bin-x64.zip",
    "https://github.com/ggerganov/whisper.cpp/releases/download/v1.7.2/whisper-bin-x64.zip",
    "https://github.com/ggerganov/whisper.cpp/releases/download/v1.6.2/whisper-bin-x64.zip",
    NULL
};

/* Required DLLs for whisper to work */
static const char *REQUIRED_DLLS[] = {
    "whisper.dll",
    "ggml.dll",
    "ggml-base.dll",
    "ggml-cpu.dll",
    NULL
};
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Utilities
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_cmd_exists(const char *cmd) {
#ifdef WINDOWS
    char check[256];
    snprintf(check, sizeof(check), "where %s >nul 2>&1", cmd);
#else
    char check[256];
    snprintf(check, sizeof(check), "command -v %s >/dev/null 2>&1", cmd);
#endif
    return system(check) == 0;
}

bool whisper_has_curl(void) {
    return whisper_cmd_exists("curl");
}

bool whisper_download_file(const char *url, const char *dest) {
    char cmd[2048];
    
    printf("    URL: %s\n", url);
    printf("    Downloading...\n");
    LOG_DEBUG("Download: %s -> %s", url, dest);
    
    if (whisper_has_curl()) {
        snprintf(cmd, sizeof(cmd),
                 "curl -L --progress-bar --fail -o \"%s\" \"%s\"", dest, url);
        if (system(cmd) == 0 && fs_exists(dest)) {
            int64_t size = fs_file_size(dest);
            printf("    Downloaded: %.1f MB\n", size / 1048576.0);
            return true;
        }
        printf("    curl failed, trying alternative...\n");
    }
    
#ifdef WINDOWS
    if (whisper_cmd_exists("powershell")) {
        printf("    Using PowerShell...\n");
        snprintf(cmd, sizeof(cmd),
                 "powershell -NoProfile -Command \""
                 "$ProgressPreference='SilentlyContinue';"
                 "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;"
                 "Invoke-WebRequest -Uri '%s' -OutFile '%s' -UseBasicParsing\"",
                 url, dest);
        if (system(cmd) == 0 && fs_exists(dest)) {
            int64_t size = fs_file_size(dest);
            printf("    Downloaded: %.1f MB\n", size / 1048576.0);
            return true;
        }
    }
#else
    if (whisper_cmd_exists("wget")) {
        printf("    Using wget...\n");
        snprintf(cmd, sizeof(cmd),
                 "wget --show-progress -O \"%s\" \"%s\"", dest, url);
        if (system(cmd) == 0 && fs_exists(dest)) {
            int64_t size = fs_file_size(dest);
            printf("    Downloaded: %.1f MB\n", size / 1048576.0);
            return true;
        }
    }
#endif
    
    LOG_ERROR("Download failed: %s", url);
    return false;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Windows Installation
 * ══════════════════════════════════════════════════════════════════════════════ */

#ifdef WINDOWS

static bool extract_zip(const char *zip, const char *dest) {
    char cmd[1024];
    
    printf("    Extracting archive...\n");
    
    /* Try tar first (Windows 10+) */
    if (whisper_cmd_exists("tar")) {
        snprintf(cmd, sizeof(cmd), "tar -xf \"%s\" -C \"%s\" 2>nul", zip, dest);
        if (system(cmd) == 0) {
            printf("    Extracted with tar\n");
            return true;
        }
    }
    
    /* Fallback to PowerShell */
    if (whisper_cmd_exists("powershell")) {
        snprintf(cmd, sizeof(cmd),
                 "powershell -NoProfile -Command \""
                 "Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"",
                 zip, dest);
        if (system(cmd) == 0) {
            printf("    Extracted with PowerShell\n");
            return true;
        }
    }
    
    printf("    Extract failed!\n");
    return false;
}

/**
 * @brief Find whisper-cli.exe and copy it along with required DLLs
 */
static bool find_and_install_whisper(const char *search_dir, const char *install_dir) {
    const char *exe_names[] = {
        "whisper-cli.exe",
        "main.exe",
        "whisper.exe",
        NULL
    };
    
    const char *search_subdirs[] = {
        "Release",
        "bin",
        "whisper-bin-x64",
        "",
        NULL
    };
    
    printf("    Searching for Whisper binary...\n");
    
    char found_dir[SPEECHER_PATH_MAX] = {0};
    char found_exe[SPEECHER_PATH_MAX] = {0};
    
    /* Find the executable */
    for (int s = 0; search_subdirs[s] && !found_exe[0]; s++) {
        char subdir[SPEECHER_PATH_MAX];
        
        if (search_subdirs[s][0]) {
            paths_join(subdir, sizeof(subdir), search_dir, search_subdirs[s]);
        } else {
            str_copy(subdir, search_dir, sizeof(subdir));
        }
        
        if (!fs_exists(subdir)) continue;
        
        for (int e = 0; exe_names[e]; e++) {
            char exe_path[SPEECHER_PATH_MAX];
            paths_join(exe_path, sizeof(exe_path), subdir, exe_names[e]);
            
            if (fs_exists(exe_path)) {
                str_copy(found_dir, subdir, sizeof(found_dir));
                str_copy(found_exe, exe_path, sizeof(found_exe));
                printf("    Found: %s\n", exe_path);
                LOG_INFO("Found whisper: %s", exe_path);
                break;
            }
        }
    }
    
    if (!found_exe[0]) {
        printf("    Whisper executable not found!\n");
        return false;
    }
    
    /* Copy executable */
    char dest_exe[SPEECHER_PATH_MAX];
    paths_join(dest_exe, sizeof(dest_exe), install_dir, "whisper-cli.exe");
    
    printf("    Copying executable...\n");
    if (!fs_copy_file(found_exe, dest_exe)) {
        printf("    Failed to copy executable!\n");
        return false;
    }
    
    /* Copy required DLLs from the same directory */
    printf("    Copying required DLLs...\n");
    int dll_count = 0;
    
    for (int i = 0; REQUIRED_DLLS[i]; i++) {
        char src_dll[SPEECHER_PATH_MAX];
        char dest_dll[SPEECHER_PATH_MAX];
        
        paths_join(src_dll, sizeof(src_dll), found_dir, REQUIRED_DLLS[i]);
        paths_join(dest_dll, sizeof(dest_dll), install_dir, REQUIRED_DLLS[i]);
        
        if (fs_exists(src_dll)) {
            if (fs_copy_file(src_dll, dest_dll)) {
                printf("      [OK] %s\n", REQUIRED_DLLS[i]);
                dll_count++;
            } else {
                printf("      [FAIL] %s\n", REQUIRED_DLLS[i]);
            }
        } else {
            printf("      [SKIP] %s (not found)\n", REQUIRED_DLLS[i]);
        }
    }
    
    /* Also copy any other DLLs that might be needed */
    char search_pattern[SPEECHER_PATH_MAX + 16];
    snprintf(search_pattern, sizeof(search_pattern), "dir /b \"%s\\*.dll\" 2>nul", found_dir);
    
    FILE *pipe = _popen(search_pattern, "r");
    if (pipe) {
        char dll_name[256];
        while (fgets(dll_name, sizeof(dll_name), pipe)) {
            /* Remove newline */
            size_t len = strlen(dll_name);
            if (len > 0 && (dll_name[len-1] == '\n' || dll_name[len-1] == '\r')) {
                dll_name[len-1] = '\0';
            }
            if (len > 1 && (dll_name[len-2] == '\n' || dll_name[len-2] == '\r')) {
                dll_name[len-2] = '\0';
            }
            
            /* Check if we already copied this one */
            bool already_copied = false;
            for (int i = 0; REQUIRED_DLLS[i]; i++) {
                if (strcmp(dll_name, REQUIRED_DLLS[i]) == 0) {
                    already_copied = true;
                    break;
                }
            }
            
            if (!already_copied && strlen(dll_name) > 0) {
                char src_dll[SPEECHER_PATH_MAX];
                char dest_dll[SPEECHER_PATH_MAX];
                
                paths_join(src_dll, sizeof(src_dll), found_dir, dll_name);
                paths_join(dest_dll, sizeof(dest_dll), install_dir, dll_name);
                
                if (fs_exists(src_dll) && !fs_exists(dest_dll)) {
                    if (fs_copy_file(src_dll, dest_dll)) {
                        printf("      [OK] %s (extra)\n", dll_name);
                        dll_count++;
                    }
                }
            }
        }
        _pclose(pipe);
    }
    
    printf("    Copied %d DLL(s)\n", dll_count);
    
    /* Refresh paths */
    paths_rebuild();
    
    return true;
}

bool whisper_install_binary(void) {
    printf("\n");
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    color_println(COLOR_CYAN, "       Installing Whisper for Windows");
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    printf("\n");
    
    /* Create directory */
    if (!fs_mkdir_recursive(PATH_WHISPER_DIR)) {
        color_println(COLOR_RED, "  Failed to create directory!");
        return false;
    }
    
    char zip_path[SPEECHER_PATH_MAX];
    paths_build(zip_path, sizeof(zip_path), PATH_WHISPER, "whisper-download.zip");
    
    /* Try each URL */
    bool downloaded = false;
    
    for (int i = 0; RELEASE_URLS[i] && !downloaded; i++) {
        printf("  Attempt %d:\n", i + 1);
        
        /* Delete old zip */
        if (fs_exists(zip_path)) {
            fs_delete_file(zip_path);
        }
        
        if (whisper_download_file(RELEASE_URLS[i], zip_path)) {
            int64_t sz = fs_file_size(zip_path);
            if (sz > 1024 * 1024) { /* At least 1MB */
                downloaded = true;
            } else {
                printf("    File too small, trying next...\n");
                fs_delete_file(zip_path);
            }
        }
        printf("\n");
    }
    
    if (!downloaded) {
        color_println(COLOR_RED, "  Download failed!");
        printf("\n");
        printf("  Please check:\n");
        printf("    - Internet connection\n");
        printf("    - Firewall settings\n");
        printf("    - Try manual download from:\n");
        printf("      https://github.com/ggerganov/whisper.cpp/releases\n");
        return false;
    }
    
    /* Extract */
    if (!extract_zip(zip_path, PATH_WHISPER_DIR)) {
        color_println(COLOR_RED, "  Extract failed!");
        fs_delete_file(zip_path);
        return false;
    }
    
    /* Delete zip */
    fs_delete_file(zip_path);
    printf("\n");
    
    /* Find and install whisper with DLLs */
    if (!find_and_install_whisper(PATH_WHISPER_DIR, PATH_WHISPER_DIR)) {
        color_println(COLOR_RED, "  Whisper installation failed!");
        printf("\n");
        printf("  Files found in directory:\n");
        char cmd[SPEECHER_PATH_MAX + 32];
        snprintf(cmd, sizeof(cmd), "dir /s /b \"%s\" 2>nul", PATH_WHISPER_DIR);
        system(cmd);
        return false;
    }
    
    /* Verify installation */
    printf("\n");
    printf("  Verifying installation...\n");
    
    paths_rebuild();
    
    if (!fs_exists(PATH_WHISPER_EXE)) {
        color_println(COLOR_RED, "  Verification failed - executable not found!");
        return false;
    }
    
    /* Test if whisper runs */
    char test_cmd[SPEECHER_PATH_MAX + 64];
    snprintf(test_cmd, sizeof(test_cmd), "\"\"%s\" --help\" >nul 2>&1", PATH_WHISPER_EXE);
    
    int test_result = system(test_cmd);
    
    if (test_result == -1073741515) {
        color_println(COLOR_RED, "  Missing DLL libraries!");
        printf("\n");
        printf("  Install Visual C++ Redistributable:\n");
        printf("    winget install Microsoft.VCRedist.2015+.x64\n");
        return false;
    }
    
    printf("\n");
    color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
    color_println(COLOR_GREEN, "       Whisper installed successfully!");
    color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
    printf("\n");
    printf("    Location: %s\n", PATH_WHISPER_EXE);
    
    return true;
}

#else /* Linux/macOS */

bool whisper_install_binary(void) {
    printf("\n");
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    color_println(COLOR_CYAN, "       Compiling Whisper from source");
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    printf("\n");
    
    /* Check required tools */
    printf("  Checking build tools:\n");
    
    bool has_git = whisper_cmd_exists("git");
    bool has_make = whisper_cmd_exists("make");
    bool has_gcc = whisper_cmd_exists("gcc") || whisper_cmd_exists("cc") || whisper_cmd_exists("clang");
    
    printf("    git:     %s\n", has_git ? "OK" : "MISSING");
    printf("    make:    %s\n", has_make ? "OK" : "MISSING");
    printf("    gcc/cc:  %s\n", has_gcc ? "OK" : "MISSING");
    printf("\n");
    
    if (!has_git || !has_make || !has_gcc) {
        color_println(COLOR_RED, "  Missing build tools!");
        printf("\n  Install with:\n");
#ifdef __APPLE__
        printf("    xcode-select --install\n");
#else
        printf("    Ubuntu/Debian: sudo apt install git build-essential\n");
        printf("    Arch Linux:    sudo pacman -S git base-devel\n");
        printf("    Fedora:        sudo dnf install git gcc make\n");
#endif
        return false;
    }
    
    /* Create directory */
    fs_mkdir_recursive(PATH_WHISPER_DIR);
    
    char src_dir[SPEECHER_PATH_MAX];
    paths_build(src_dir, sizeof(src_dir), PATH_WHISPER, "src");
    
    /* Clone repository */
    if (!fs_exists(src_dir)) {
        printf("  Cloning whisper.cpp repository...\n\n");
        
        char cmd[2048];
        snprintf(cmd, sizeof(cmd),
                 "git clone --depth 1 https://github.com/ggerganov/whisper.cpp.git \"%s\" 2>&1",
                 src_dir);
        
        int result = system(cmd);
        
        if (result != 0) {
            color_println(COLOR_RED, "  Clone failed!");
            printf("\n  Check your internet connection and try again.\n");
            return false;
        }
        
        printf("\n");
        color_println(COLOR_GREEN, "  [OK] Repository cloned");
    } else {
        printf("  Using existing source: %s\n", src_dir);
    }
    
/* Compile */
printf("\n  Compiling (this may take a few minutes)...\n\n");

char cmd[2048];

/* Try old Makefile first (if exists and works) */
bool compiled = false;

#ifdef __APPLE__
snprintf(cmd, sizeof(cmd), 
         "cd \"%s\" && make -j$(sysctl -n hw.ncpu) main 2>&1", src_dir);
#else
snprintf(cmd, sizeof(cmd), 
         "cd \"%s\" && make -j$(nproc) main 2>&1", src_dir);
#endif

printf("  Attempting build with Makefile...\n");
int result = system(cmd);

char test_bin[SPEECHER_PATH_MAX];
paths_join(test_bin, sizeof(test_bin), src_dir, "main");

if (result == 0 && fs_exists(test_bin)) {
    printf("  Built successfully with Makefile\n");
    compiled = true;
} else {
    printf("  Makefile build failed, trying CMake...\n\n");
    
    /* Check if CMake is available */
    if (!whisper_cmd_exists("cmake")) {
        color_println(COLOR_RED, "  CMake is required!");
        printf("\n  Install CMake:\n");
#ifdef __APPLE__
        printf("    brew install cmake\n");
#else
        printf("    Ubuntu/Debian: sudo apt install cmake\n");
        printf("    Arch Linux:    sudo pacman -S cmake\n");
        printf("    Fedora:        sudo dnf install cmake\n");
#endif
        return false;
    }
    
    /* Build with CMake */
    snprintf(cmd, sizeof(cmd),
             "cd \"%s\" && cmake -B build -DCMAKE_BUILD_TYPE=Release 2>&1", src_dir);
    
    result = system(cmd);
    if (result != 0) {
        color_println(COLOR_RED, "  CMake configuration failed!");
        return false;
    }
    
    printf("  CMake configured, building...\n");
    
#ifdef __APPLE__
    snprintf(cmd, sizeof(cmd),
             "cd \"%s\" && cmake --build build --config Release -j$(sysctl -n hw.ncpu) 2>&1", src_dir);
#else
    snprintf(cmd, sizeof(cmd),
             "cd \"%s\" && cmake --build build --config Release -j$(nproc) 2>&1", src_dir);
#endif
    
    result = system(cmd);
    if (result != 0) {
        color_println(COLOR_RED, "  CMake build failed!");
        return false;
    }
    
    compiled = true;
}

if (!compiled) {
    color_println(COLOR_RED, "  Compilation failed!");
    printf("\n  Common fixes:\n");
    printf("    - Install missing dependencies\n");
    printf("    - Check disk space\n");
    printf("    - Try: cd %s && make clean && make main\n", src_dir);
    return false;
}
    
    /* Copy binary */
/* Find and copy binary */
char src_bin[SPEECHER_PATH_MAX];
char dest_bin[SPEECHER_PATH_MAX];

/* Try different locations where binary might be */
const char *possible_paths[] = {
    "main",                          // old Makefile
    "build/bin/whisper-cli",         // new CMake
    "build/bin/main",                // CMake alternative
    NULL
};

bool found = false;
for (int i = 0; possible_paths[i] && !found; i++) {
    paths_join(src_bin, sizeof(src_bin), src_dir, possible_paths[i]);
    if (fs_exists(src_bin)) {
        printf("  Found binary: %s\n", possible_paths[i]);
        found = true;
        break;
    }
}

if (!found) {
    color_println(COLOR_RED, "  Binary not found!");
    printf("  Searched in:\n");
    for (int i = 0; possible_paths[i]; i++) {
        char path[SPEECHER_PATH_MAX];
        paths_join(path, sizeof(path), src_dir, possible_paths[i]);
        printf("    %s\n", path);
    }
    return false;
}

paths_build(dest_bin, sizeof(dest_bin), PATH_WHISPER, "whisper-cli");

if (!fs_copy_file(src_bin, dest_bin)) {
    color_println(COLOR_RED, "  Failed to copy binary!");
    return false;
}
    
    /* Make executable */
    char chmod_cmd[SPEECHER_PATH_MAX + 32];
    snprintf(chmod_cmd, sizeof(chmod_cmd), "chmod +x \"%s\"", dest_bin);
    system(chmod_cmd);
    
    /* Refresh paths */
    paths_rebuild();
    
    /* Verify */
    if (fs_exists(PATH_WHISPER_EXE)) {
        printf("\n");
        color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
        color_println(COLOR_GREEN, "       Whisper compiled successfully!");
        color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
        printf("\n");
        printf("    Location: %s\n", PATH_WHISPER_EXE);
        return true;
    }
    
    return false;
}

#endif /* WINDOWS / Unix */

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Model Download
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_install_model(const WhisperModel *model) {
    if (!model) return false;
    
    printf("\n");
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    color_println(COLOR_CYAN, "       Downloading Model: %s", model->name);
    color_println(COLOR_CYAN, "  ══════════════════════════════════════════");
    printf("\n");
    
    /* Create models directory */
    if (!fs_mkdir_recursive(PATH_WHISPER_MODELS_DIR)) {
        color_println(COLOR_RED, "  Failed to create models directory!");
        return false;
    }
    
    /* Build paths */
    char model_path[SPEECHER_PATH_MAX];
    paths_build(model_path, sizeof(model_path), PATH_WHISPER_MODELS, model->filename);
    
    char url[512];
    snprintf(url, sizeof(url), "%s/%s", MODEL_URL_BASE, model->filename);
    
    printf("  Model: %s\n", model->name);
    printf("  Size:  ~%d MB\n", model->size_mb);
    printf("  RAM:   ~%d GB required\n\n", model->ram_gb);
    
    /* Download */
    if (!whisper_download_file(url, model_path)) {
        color_println(COLOR_RED, "  Download failed!");
        printf("\n  Manual download:\n");
        printf("    URL:  %s\n", url);
        printf("    Save: %s\n", model_path);
        return false;
    }
    
    /* Verify size */
    int64_t actual_size = fs_file_size(model_path);
    int64_t expected_min = (int64_t)(model->size_mb - 50) * 1024 * 1024;
    
    if (actual_size < expected_min) {
        color_println(COLOR_RED, "  Download incomplete!");
        printf("    Expected: ~%d MB\n", model->size_mb);
        printf("    Got: %.1f MB\n", actual_size / 1048576.0);
        fs_delete_file(model_path);
        return false;
    }
    
    printf("\n");
    color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
    color_println(COLOR_GREEN, "       Model downloaded successfully!");
    color_println(COLOR_GREEN, "  ══════════════════════════════════════════");
    printf("\n");
    printf("    Size: %.1f MB\n", actual_size / 1048576.0);
    printf("    Path: %s\n", model_path);
    
    return true;
}
