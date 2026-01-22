/**
 * @file    platform.c
 * @brief   Platform-specific implementations
 * @author  SPEECHER Team
 * @date    2025
 */

/* Enable POSIX features on Linux/Unix */
#if !defined(_WIN32) && !defined(_WIN64)
    #define _POSIX_C_SOURCE 200809L
    #define _DEFAULT_SOURCE
    #define _BSD_SOURCE
#endif

#include "utils/platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef WINDOWS
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/time.h>
    #include <limits.h>
    #include <stdio.h>
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Console Handling
 * ══════════════════════════════════════════════════════════════════════════════ */

bool platform_console_init(void) {
#ifdef WINDOWS
    /* Enable UTF-8 output */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* Enable ANSI escape sequences (Windows 10+) */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        /* Windows 10 pre-1511 doesn't support this, but we can still work */
        return true;
    }

    return true;
#else
    /* Unix-like systems generally support this out of the box */
    return true;
#endif
}

void platform_console_clear(void) {
#ifdef WINDOWS
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {0, 0};
    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hOut, ' ', cellCount, coord, &count);
        FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, coord, &count);
        SetConsoleCursorPosition(hOut, coord);
    }
#else
    /* ANSI escape sequence */
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

bool platform_console_colors_supported(void) {
#ifdef WINDOWS
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;

    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    return (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    /* Check if stdout is a terminal */
    if (!isatty(STDOUT_FILENO)) {
        return false;
    }

    /* Check TERM environment variable */
    const char *term = getenv("TERM");
    if (term == NULL) {
        return false;
    }

    /* Most terminals support colors */
    return (strcmp(term, "dumb") != 0);
#endif
}

int platform_console_width(void) {
#ifdef WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_col;
    }
    return 80;
#endif
}

int platform_console_height(void) {
#ifdef WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
    return 24;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        return w.ws_row;
    }
    return 24;
#endif
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                              Sleep / Time
 * ══════════════════════════════════════════════════════════════════════════════ */

void platform_sleep_ms(unsigned int ms) {
#ifdef WINDOWS
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

uint64_t platform_time_ms(void) {
#ifdef WINDOWS
    return (uint64_t)GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Executable Path
 * ══════════════════════════════════════════════════════════════════════════════ */

bool platform_get_exe_path(char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return false;
    }

#ifdef WINDOWS
    DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)size);
    return (len > 0 && len < size);

#elif defined(__APPLE__)
    uint32_t bufsize = (uint32_t)size;
    if (_NSGetExecutablePath(buffer, &bufsize) == 0) {
        return true;
    }
    return false;

#else /* Linux */
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return true;
    }
    return false;
#endif
}

bool platform_get_exe_dir(char *buffer, size_t size) {
    if (!platform_get_exe_path(buffer, size)) {
        return false;
    }

    /* Find last path separator and truncate */
    char *last_sep = strrchr(buffer, PATH_SEPARATOR);
    if (last_sep != NULL) {
        *last_sep = '\0';
        return true;
    }

    return false;
}

/* ══════════════════════════════════════════════════════════════════════════════
 *                           Environment
 * ══════════════════════════════════════════════════════════════════════════════ */

bool platform_getenv(const char *name, char *buffer, size_t size) {
    if (name == NULL || buffer == NULL || size == 0) {
        return false;
    }

#ifdef WINDOWS
    DWORD len = GetEnvironmentVariableA(name, buffer, (DWORD)size);
    return (len > 0 && len < size);
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return false;
    }

    size_t len = strlen(value);
    if (len >= size) {
        return false;
    }

    strcpy(buffer, value);
    return true;
#endif
}

bool platform_setenv(const char *name, const char *value) {
    if (name == NULL) {
        return false;
    }

#ifdef WINDOWS
    return SetEnvironmentVariableA(name, value) != 0;
#else
    if (value == NULL) {
        return unsetenv(name) == 0;
    }
    return setenv(name, value, 1) == 0;
#endif
}