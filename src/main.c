/**
 * @file    main.c
 * @brief   Entry point
 */

#include "core/app.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (!app_init(argc, argv)) {
        return EXIT_FAILURE;
    }
    
    int result = app_run();
    
    app_shutdown();
    
    return result;
}