/**
 * @file    whisper_transcribe.c
 * @brief   Whisper transcription
 */

/* POSIX for popen/pclose - MUST be before includes */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
#endif

#include "whisper/whisper_manager.h"
#include "whisper/whisper_internal.h"
#include "config/config.h"
#include "utils/paths.h"
#include "utils/fs.h"
#include "utils/strings.h"
#include "logging/logger.h"
#include "speecher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
    #define popen _popen
    #define pclose _pclose
#else
    #include <sys/wait.h>
#endif

/**
 * @brief Normalize path for current platform (fix slashes)
 */
static void fix_path_slashes(char *path) {
    if (!path) return;
    
#ifdef WINDOWS
    /* Windows: convert / to \ */
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
#else
    /* Unix: convert \ to / */
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
#endif
}

bool whisper_transcribe(const char *audio_path, const char *output_path, const char *language) {
    if (!audio_path || !output_path) {
        LOG_ERROR("Invalid arguments: audio=%p, output=%p", (void*)audio_path, (void*)output_path);
        return false;
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         NORMALIZE ALL PATHS
     * ═══════════════════════════════════════════════════════════════════════ */
    
    char norm_audio[SPEECHER_PATH_MAX];
    char norm_output[SPEECHER_PATH_MAX];
    char norm_whisper[SPEECHER_PATH_MAX];
    char norm_model[SPEECHER_PATH_MAX];
    
    /* Copy and normalize audio path */
    str_copy(norm_audio, audio_path, sizeof(norm_audio));
    fix_path_slashes(norm_audio);
    
    /* Copy and normalize output path */
    str_copy(norm_output, output_path, sizeof(norm_output));
    fix_path_slashes(norm_output);
    
    /* Get and normalize whisper binary path */
    str_copy(norm_whisper, PATH_WHISPER_EXE, sizeof(norm_whisper));
    fix_path_slashes(norm_whisper);
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         VALIDATION
     * ═══════════════════════════════════════════════════════════════════════ */
    
    /* Check whisper binary */
    if (!fs_exists(norm_whisper)) {
        LOG_ERROR("Whisper not found: %s", norm_whisper);
        return false;
    }
    
    /* Get model path */
    Config *cfg = config_get();
    const char *model_ptr = paths_find_model(cfg->whisper_model);
    if (!model_ptr) {
        LOG_ERROR("Model not found: %s", cfg->whisper_model);
        return false;
    }
    
    str_copy(norm_model, model_ptr, sizeof(norm_model));
    fix_path_slashes(norm_model);
    
    /* Check audio file */
    if (!fs_exists(norm_audio)) {
        LOG_ERROR("Audio not found: %s", norm_audio);
        return false;
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         AUDIO CONVERSION
     * ═══════════════════════════════════════════════════════════════════════ */
    
    char wav_path[SPEECHER_PATH_MAX];
    if (!whisper_audio_to_wav(norm_audio, wav_path, sizeof(wav_path))) {
        LOG_ERROR("Audio conversion failed: %s", norm_audio);
        return false;
    }
    
    /* Normalize WAV path too */
    fix_path_slashes(wav_path);
    
    LOG_DEBUG("Paths:");
    LOG_DEBUG("  Whisper: %s", norm_whisper);
    LOG_DEBUG("  Model:   %s", norm_model);
    LOG_DEBUG("  Audio:   %s", wav_path);
    LOG_DEBUG("  Output:  %s", norm_output);
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         BUILD COMMAND
     * ═══════════════════════════════════════════════════════════════════════ */
    
    char cmd[8192];
    int len = 0;
    
#ifdef WINDOWS
    /* Windows: need to be careful with quotes */
    len = snprintf(cmd, sizeof(cmd),
                   "\"\"%s\" -m \"%s\" -f \"%s\" -otxt -of \"%s\"",
                   norm_whisper, norm_model, wav_path, norm_output);
#else
    len = snprintf(cmd, sizeof(cmd),
                   "\"%s\" -m \"%s\" -f \"%s\" -otxt -of \"%s\"",
                   norm_whisper, norm_model, wav_path, norm_output);
#endif
    
    /* Add language if specified */
    if (language && strcmp(language, "auto") != 0 && language[0]) {
        len += snprintf(cmd + len, sizeof(cmd) - len, " -l %s", language);
    }
    
    /* Add threads if specified */
    if (cfg->whisper_threads > 0) {
        len += snprintf(cmd + len, sizeof(cmd) - len, " -t %d", cfg->whisper_threads);
    }
    
#ifdef WINDOWS
    /* Close outer quotes and redirect stderr */
    strcat(cmd, "\" 2>&1");
#else
    strcat(cmd, " 2>&1");
#endif
    
    LOG_INFO("Transcribing: %s", fs_path_filename(audio_path));
    LOG_DEBUG("Command: %s", cmd);
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         EXECUTE
     * ═══════════════════════════════════════════════════════════════════════ */
    
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        LOG_ERROR("Failed to execute whisper command");
        whisper_audio_cleanup(norm_audio, wav_path);
        return false;
    }
    
    /* Read output */
    char output[8192] = {0};
    char line[512];
    size_t total = 0;
    
    while (fgets(line, sizeof(line), pipe)) {
        size_t line_len = strlen(line);
        if (total + line_len < sizeof(output) - 1) {
            strcat(output, line);
            total += line_len;
        }
    }
    
    int status = pclose(pipe);
    
#ifndef WINDOWS
    if (WIFEXITED(status)) {
        status = WEXITSTATUS(status);
    } else {
        status = -1;
    }
#endif
    
    /* Cleanup temp WAV */
    whisper_audio_cleanup(norm_audio, wav_path);
    
    /* ═══════════════════════════════════════════════════════════════════════
     *                         CHECK RESULT
     * ═══════════════════════════════════════════════════════════════════════ */
    
    if (status != 0) {
        LOG_ERROR("Whisper failed with code %d", status);
        if (output[0]) {
            LOG_ERROR("Output: %s", output);
        }
        return false;
    }
    
    /* Check output file (whisper adds .txt) */
    char result_path[SPEECHER_PATH_MAX];
    snprintf(result_path, sizeof(result_path), "%s.txt", norm_output);
    fix_path_slashes(result_path);
    
    if (!fs_exists(result_path)) {
        LOG_ERROR("Output not created: %s", result_path);
        return false;
    }
    
    int64_t size = fs_file_size(result_path);
    if (size == 0) {
        LOG_WARN("Transcription is empty (no speech detected?)");
    }
    
    LOG_INFO("Saved: %s (%lld bytes)", fs_path_filename(result_path), (long long)size);
    return true;
}