/**
 * @file cli.h
 * @brief Command Line Interface Header
 */

#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stdarg.h>

/* CLI API */
int32_t CLI_init(void);
int32_t CLI_processLine(char* line);
void CLI_printf(const char* format, ...);

#endif /* CLI_H */
