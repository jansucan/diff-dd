#include <stdio.h>
#include <stdarg.h>

#include "print.h"

#define PRINT_ERROR_PREFIX  "ERROR: "
#define PRINT_ERROR_SUFFIX  "\n"

void
print_error(const char * const format, ...)
{
    fprintf(stderr, PRINT_ERROR_PREFIX);

    va_list args;
	
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, PRINT_ERROR_SUFFIX);
}
