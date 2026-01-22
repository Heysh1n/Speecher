/**
 * @file    validator.h
 * @brief   Configuration validation
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef CONFIG_VALIDATOR_H
#define CONFIG_VALIDATOR_H

#include <stdbool.h>
#include "config/config.h"

/**
 * @brief Validate and fix configuration values
 * @param config Configuration to validate
 * @return Number of values that were corrected
 */
int config_validate(Config *config);

/**
 * @brief Check if sort order value is valid
 */
bool validate_sort_order(const char *value);

/**
 * @brief Check if log level is valid
 */
bool validate_log_level(const char *value);

/**
 * @brief Check if color name is valid
 */
bool validate_color(const char *value);

/**
 * @brief Check if path is safe (no directory traversal)
 */
bool validate_path_safe(const char *path);

#endif /* CONFIG_VALIDATOR_H */