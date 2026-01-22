/**
 * @file    merger_audio.c
 * @brief   Audio transcription
 */

#include "core/merger_audio.h"
#include "whisper/whisper_manager.h"
#include "config/config.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "ui/colors.h"
#include "ui/progress.h"
#include "ui/menu.h"
#include "logging/logger.h"
#include "i18n/lang.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 1000

static const char *AUDIO_EXT[] = {
    ".wav", ".mp3", ".ogg", ".flac", ".m4a", ".wma", ".aac", NULL
};

typedef struct {
    char path[SPEECHER_PATH_MAX];
    char name[256];
    int64_t size;
} AudioFile;

static AudioFile g_files[MAX_FILES];
static int g_file_count = 0;

/**
 * @brief Fix path slashes for current platform
 */
static void fix_slashes(char *path) {
    if (!path) return;
#ifdef WINDOWS
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
#else
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
#endif
}

static bool is_audio(const char *name) {
    if (!name) return false;
    const char *ext = strrchr(name, '.');
    if (!ext) return false;
    
    char lower[16] = {0};
    for (int i = 0; ext[i] && i < 15; i++) {
        char c = ext[i];
        lower[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
    }
    
    for (int i = 0; AUDIO_EXT[i]; i++) {
        if (strcmp(lower, AUDIO_EXT[i]) == 0) return true;
    }
    return false;
}

static void scan_dir(const char *dir) {
    FsDir *d = fs_dir_open(dir);
    if (!d) return;
    
    char name[256];
    bool isdir;
    
    while (fs_dir_read(d, name, sizeof(name), &isdir) && g_file_count < MAX_FILES) {
        if (name[0] == '.') continue;
        
        char full[SPEECHER_PATH_MAX];
        paths_join(full, sizeof(full), dir, name);
        fix_slashes(full);
        
        if (!isdir && is_audio(name)) {
            str_copy(g_files[g_file_count].path, full, SPEECHER_PATH_MAX);
            str_copy(g_files[g_file_count].name, name, 256);
            g_files[g_file_count].size = fs_file_size(full);
            g_file_count++;
        }
    }
    
    fs_dir_close(d);
}

static void gen_output(char *dest, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char name[128];
    snprintf(name, sizeof(name), "transcription_%04d-%02d-%02d_%02d-%02d-%02d.txt",
             t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    paths_build(dest, size, PATH_OUTPUT, name);
    fix_slashes(dest);
}

bool merger_audio_run(void) {
    Config *cfg = config_get();
    
    printf("\n");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════");
    color_println(COLOR_CYAN, "        AUDIO TO TEXT (Whisper)");
    color_println(COLOR_CYAN, "  ═══════════════════════════════════════");
    printf("\n");
    
    /* Check whisper */
    if (!whisper_check()) {
        color_println(COLOR_YELLOW, "  Whisper not ready.");
        if (menu_confirm("  Run setup?")) {
            if (!whisper_setup()) return false;
        } else {
            return false;
        }
    } else {
        color_println(COLOR_GREEN, "  [OK] Whisper ready (model: %s)", cfg->whisper_model);
    }
    
    if (!whisper_has_ffmpeg()) {
        color_println(COLOR_YELLOW, "  [!] ffmpeg missing - only WAV supported");
    }
    
    /* Get normalized input directory */
    char input_dir[SPEECHER_PATH_MAX];
    str_copy(input_dir, PATH_INPUT_DIR, sizeof(input_dir));
    fix_slashes(input_dir);
    
    /* Scan */
    printf("\n  Scanning: %s\n", input_dir);
    g_file_count = 0;
    scan_dir(input_dir);
    
    if (g_file_count == 0) {
        color_println(COLOR_YELLOW, "\n  No audio files found!");
        printf("  Place audio in: %s\n\n", input_dir);
        menu_pause(lang_msg("press_enter"));
        return false;
    }
    
    printf("  Found: %d file(s)\n\n", g_file_count);
    
    for (int i = 0; i < g_file_count && i < 10; i++) {
        printf("    %d. %s (%.1f MB)\n", i+1, g_files[i].name, g_files[i].size/1048576.0);
    }
    if (g_file_count > 10) printf("    ... +%d more\n", g_file_count - 10);
    printf("\n");
    
    if (!menu_confirm("  Start transcription?")) return false;
    
    /* Transcribe */
    printf("\n");
    
    char **results = calloc(g_file_count, sizeof(char*));
    int ok_count = 0;
    
    /* Create progress bar */
    ProgressBar *progress = progress_create(g_file_count, 40, PROGRESS_STYLE_UNICODE);
    if (progress) {
        progress_set_prefix(progress, "    Transcribing");
    }
    
    /* Get normalized output directory */
    char output_dir[SPEECHER_PATH_MAX];
    str_copy(output_dir, PATH_OUTPUT_DIR, sizeof(output_dir));
    fix_slashes(output_dir);
    
    for (int i = 0; i < g_file_count; i++) {
        printf("\n  [%d/%d] %s\n", i+1, g_file_count, g_files[i].name);
        
        /* Update progress */
        if (progress) {
            progress_update(progress, i);
        }
        
        /* Build temp output path - USE paths_join for correct separators! */
        char temp_name[64];
        snprintf(temp_name, sizeof(temp_name), "temp_%d", i);
        
        char temp[SPEECHER_PATH_MAX];
        paths_join(temp, sizeof(temp), output_dir, temp_name);
        fix_slashes(temp);
        
        LOG_DEBUG("Temp output path: %s", temp);
        
        if (whisper_transcribe(g_files[i].path, temp, cfg->whisper_language)) {
            results[ok_count] = malloc(SPEECHER_PATH_MAX);
            if (results[ok_count]) {
                snprintf(results[ok_count], SPEECHER_PATH_MAX, "%s.txt", temp);
                fix_slashes(results[ok_count]);
                ok_count++;
            }
            color_println(COLOR_GREEN, "    [OK]");
        } else {
            color_println(COLOR_RED, "    [FAILED]");
        }
    }
    
    /* Complete progress bar */
    if (progress) {
        progress_complete(progress);
        progress_free(progress);
    }
    
    printf("\n");
    
    /* Merge */
    if (ok_count > 0) {
        char output[SPEECHER_PATH_MAX];
        gen_output(output, sizeof(output));
        
        FILE *out = fopen(output, "w");
        if (out) {
            fprintf(out, "SPEECHER Transcription\n");
            fprintf(out, "Files: %d\n", ok_count);
            fprintf(out, "═══════════════════════════════════════\n\n");
            
            for (int i = 0; i < ok_count; i++) {
                fprintf(out, "--- File %d ---\n", i+1);
                FILE *in = fopen(results[i], "r");
                if (in) {
                    char line[4096];
                    while (fgets(line, sizeof(line), in)) fputs(line, out);
                    fclose(in);
                }
                fprintf(out, "\n");
                fs_delete_file(results[i]);
                free(results[i]);
            }
            
            fclose(out);
            
            color_println(COLOR_GREEN, "  ═══════════════════════════════════════");
            color_println(COLOR_GREEN, "        TRANSCRIPTION COMPLETE!");
            color_println(COLOR_GREEN, "  ═══════════════════════════════════════");
            printf("\n  Success: %d/%d\n", ok_count, g_file_count);
            printf("  Output: %s\n", output);
        }
    } else {
        color_println(COLOR_RED, "  All transcriptions failed!");
        printf("\n  Troubleshooting:\n");
        printf("    1. Run Whisper diagnostics (Settings > Whisper)\n");
        printf("    2. Check if audio files are valid\n");
        printf("    3. Try WAV files (16kHz mono)\n");
    }
    
    free(results);
    printf("\n");
    menu_pause(lang_msg("press_enter"));
    return ok_count > 0;
}

bool merger_audio_transcribe_file(const char *audio, const char *output) {
    if (!whisper_check()) return false;
    
    char out[SPEECHER_PATH_MAX];
    if (!output) {
        const char *name = fs_path_filename(audio);
        paths_build(out, sizeof(out), PATH_OUTPUT, name);
        char *dot = strrchr(out, '.');
        if (dot) *dot = '\0';
        output = out;
    }
    
    Config *cfg = config_get();
    return whisper_transcribe(audio, output, cfg->whisper_language);
}