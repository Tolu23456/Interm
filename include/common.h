#ifndef INTERM_COMMON_H
#define INTERM_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Versioning
 */
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} im_version_t;

/**
 * Result codes for API calls
 */
typedef enum {
    IM_OK = 0,
    IM_ERR = 1,
    IM_ERR_INVALID_ARG = 2,
    IM_ERR_NOMEM = 3,
    IM_ERR_NOT_FOUND = 4,
    IM_ERR_ALREADY_EXISTS = 5,
    IM_ERR_TIMEOUT = 6,
    IM_ERR_CANCELLED = 7,
    IM_ERR_NOT_IMPLEMENTED = 8,
    IM_ERR_IO = 9,
} im_result_t;

/**
 * Opaque handle for any core object
 */
typedef void* im_handle_t;

/**
 * Status of a result
 */
typedef struct {
    im_result_t status;
    int32_t error_code;
    const char* message;
} im_status_t;

#define IM_STATUS_OK (im_status_t){IM_OK, 0, "OK"}

typedef enum {
    IM_PRIORITY_IDLE = 0,
    IM_PRIORITY_LOW,
    IM_PRIORITY_NORMAL,
    IM_PRIORITY_HIGH,
    IM_PRIORITY_CRITICAL
} im_priority_t;

#endif // INTERM_COMMON_H
