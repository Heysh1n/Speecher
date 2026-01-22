/**
 * @file    merger_audio.h
 * @brief   Audio transcription
 */

#ifndef SPEECHER_MERGER_AUDIO_H
#define SPEECHER_MERGER_AUDIO_H

#include <stdbool.h>

/**
 * @brief Run audio transcription (interactive)
 */
bool merger_audio_run(void);

/**
 * @brief Transcribe single file
 */
bool merger_audio_transcribe_file(const char *audio_path, const char *output_path);

#endif /* SPEECHER_MERGER_AUDIO_H */