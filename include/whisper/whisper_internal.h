/**
 * @file   whisper_internal.h
 * @brief   Whisper internal functions
 */

#ifndef SPEECHER_WHISPER_INTERNAL_H
#define SPEECHER_WHISPER_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Model Info
 * ══════════════════════════════════════════════════════════════════════════════ */

#define WHISPER_MODEL_COUNT 5

typedef struct {
    const char *name;
    const char *filename;
    int size_mb;
    int ram_gb;
} WhisperModel;

extern const WhisperModel WHISPER_MODELS[WHISPER_MODEL_COUNT + 1];

const WhisperModel *whisper_model_get(const char *name);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Audio (whisper_audio.c)
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_audio_to_wav(const char *input, char *wav_out, size_t size);
void whisper_audio_cleanup(const char *original, const char *wav);
bool whisper_audio_is_wav(const char *path);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Install (whisper_install.c)
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_install_binary(void);
bool whisper_install_model(const WhisperModel *model);

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Utilities
 * ══════════════════════════════════════════════════════════════════════════════ */

bool whisper_cmd_exists(const char *cmd);
bool whisper_has_curl(void);
bool whisper_download_file(const char *url, const char *dest);

#endif /* SPEECHER_WHISPER_INTERNAL_H */