/**
 * @file errors.h
 * @brief Error handling utilities for the project.
 */

#ifndef cslo_errors_h
#define cslo_errors_h

/**
 * @enum ErrorType
 */
enum ErrorType {
    ERROR_RUNTIME,
    ERROR_SYNTAX,
    ERROR_TYPE,
    ERROR_ATTRIBUTE,
    ERROR_IO,
    ERROR_MEMORY,
    ERROR_INDEX,
    ERROR_IMPORT,
    ERROR_NAME,
};

/**
 * @struct Exception
 * @brief Represents an error or exception in the system.
 */
typedef struct {
    enum ErrorType type;
    const char* message;
    int line;
    int column;
    const char* file;
    char* stacktrace;
} Exception;

void reportError(Exception* exc);

#endif // cslo_errors_h
