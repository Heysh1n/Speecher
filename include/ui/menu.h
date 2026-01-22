/**
 * @file    menu.h
 * @brief   Interactive menu
 * @author  SPEECHER Team
 * @date    2025
 */

#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdbool.h>

/** Menu style */
typedef enum {
    MENU_STYLE_SINGLE,  /* ┌─────┐ */
    MENU_STYLE_DOUBLE,  /* ╔═════╗ */
    MENU_STYLE_ROUNDED, /* ╭─────╮ */
    MENU_STYLE_ASCII,   /* +-----+ */
} MenuStyle;

/**
 * @brief Display main menu and get user choice
 * @return Selected option (0 = exit)
 */
int menu_main(void);

/**
 * @brief Display settings menu
 */
void menu_settings(void);

/**
 * @brief Display logs viewer
 */
void menu_logs(void);

/**
 * @brief Show confirmation dialog
 * @param message Message to display
 * @return true if user confirmed
 */
bool menu_confirm(const char *message);

/**
 * @brief Wait for user to press Enter
 */
void menu_pause(const char *message);

#endif /* UI_MENU_H */