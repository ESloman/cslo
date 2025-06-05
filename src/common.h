#ifndef cslo_common_h
#define cslo_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef DEBUG
#define DEBUG_PRINT_CODE 1
#define DEBUG_TRACE_EXECUTION 1
#define DEBUG_LOGGING 1
#endif

#ifdef DEBUG1
#define DEBUG_STRESS_GC 1
#define DEBUG_LOG_GC 0
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
