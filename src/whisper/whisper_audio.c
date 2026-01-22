/**
 * @file    whisper_audio.c
 * @brief   Audio conversion for Whisper
 */

/* POSIX - MUST be before includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
#endif

#include "whisper/whisper_internal.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

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

bool whisper_audio_is_wav(const char *path) {
    if (!path) return false;
    
    const char *ext = strrchr(path, '.');
    if (!ext) return false;
    
    char lower[8] = {0};
    for (int i = 0; ext[i] && i < 7; i++) {
        lower[i] = (char)tolower((unsigned char)ext[i]);
    }
    
    return strcmp(lower, ".wav") == 0;
}

bool whisper_audio_to_wav(const char *input, char *wav_out, size_t size) {
    if (!input || !wav_out || size == 0) return false;
    
    /* Already WAV - just copy and normalize the path */
    if (whisper_audio_is_wav(input)) {
        str_copy(wav_out, input, size);
        fix_slashes(wav_out);
        return true;
    }
    
    /* Need ffmpeg */
    if (!whisper_cmd_exists("ffmpeg")) {
        LOG_ERROR("ffmpeg required for non-WAV files: %s", input);
        printf("\n  Install ffmpeg:\n");
#ifdef WINDOWS
        printf("    winget install ffmpeg\n");
        printf("    or: choco install ffmpeg\n");
#elif defined(__APPLE__)
        printf("    brew install ffmpeg\n");
#else
        printf("    sudo apt install ffmpeg\n");
#endif
        return false;
    }
    
    /* Build temp WAV path - same directory as input */
    snprintf(wav_out, size, "%s.temp.wav", input);
    fix_slashes(wav_out);
    
    /* Normalize input path */
    char norm_input[SPEECHER_PATH_MAX];
    str_copy(norm_input, input, sizeof(norm_input));
    fix_slashes(norm_input);
    
    /* Convert: 16kHz mono 16-bit PCM */
    char cmd[4096];
    
#ifdef WINDOWS
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -y -i \"%s\" -ar 16000 -ac 1 -c:a pcm_s16le \"%s\" >nul 2>&1",
             norm_input, wav_out);
#else
    snprintf(cmd, sizeof(cmd),
             "ffmpeg -y -i \"%s\" -ar 16000 -ac 1 -c:a pcm_s16le \"%s\" >/dev/null 2>&1",
             norm_input, wav_out);
#endif
    
    LOG_DEBUG("FFmpeg command: %s", cmd);
    
    int result = system(cmd);
    
    if (result != 0 || !fs_exists(wav_out)) {
        LOG_ERROR("Audio conversion failed for: %s", input);
        return false;
    }
    
    LOG_DEBUG("Converted to WAV: %s", wav_out);
    return true;
}

void whisper_audio_cleanup(const char *original, const char *wav) {
    if (!original || !wav) return;
    
    /* Only delete if it's a temp file (different from original) */
    if (strcmp(original, wav) != 0 && fs_exists(wav)) {
        fs_delete_file(wav);
        LOG_DEBUG("Cleaned up temp: %s", wav);
    }
}