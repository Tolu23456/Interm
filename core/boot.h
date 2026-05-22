#ifndef INTERM_BOOT_H
#define INTERM_BOOT_H

#include "common.h"

/**
 * Initializes all core subsystems in the correct order.
 */
im_result_t im_boot_init();

/**
 * Shuts down all systems gracefully.
 */
im_result_t im_boot_shutdown();

#endif // INTERM_BOOT_H
