/**
 * @file    whisper_manager.h
 * @brief   Whisper integration - Public API
 */

#ifndef SPEECHER_WHISPER_MANAGER_H
#define SPEECHER_WHISPER_MANAGER_H

#include <stdbool.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Status
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Check if Whisper is ready (binary + model)
 */
bool whisper_is_available(void);

/**
 * @brief Silent check (no prompts)
 */
bool whisper_check(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Setup
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Interactive setup wizard
 */
bool whisper_setup(void);

/**
 * @brief Install whisper binary
 */
bool whisper_install(void);

/**
 * @brief Download model
 */
bool whisper_download_model(const char *model_name);

/**
 * @brief Show diagnostics
 */
void whisper_diagnose(void);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Transcription
 * ══════════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Transcribe audio file
 */
bool whisper_transcribe(const char *audio_path, const char *output_path, const char *language);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Info
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_has_ffmpeg(void);

typedef struct {
    const char *name;
    const char *filename;
    int size_mb;
    int ram_gb;
} WhisperModelInfo;

const WhisperModelInfo *whisper_get_models(int *count);

#endif /* SPEECHER_WHISPER_MANAGER_H */