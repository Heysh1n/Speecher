/**
 * @file    app.h
 * @brief   Application lifecycle
 */

#ifndef SPEECHER_APP_H
#define SPEECHER_APP_H

#include <stdbool.h>

/**
 * @brief Initialize application
 */
bool app_init(int argc, char *argv[]);

/**
 * @brief Run main loop
 */
int app_run(void);

/**
 * @brief Shutdown
 */
void app_shutdown(void);

/**
 * @brief Check if initialized
 */
bool app_is_initialized(void);

/* Path getters (compatibility - prefer paths.h) */
const char *app_get_base_dir(void);
const char *app_get_data_dir(void);
const char *app_get_input_dir(void);
const char *app_get_output_dir(void);
const char *app_get_logs_dir(void);
const char *app_get_config_path(void);

#endif /* SPEECHER_APP_H */