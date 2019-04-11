#ifndef FILE_H
#define FILE_H

#include <stdio.h>

long   file_size(FILE * const file);
FILE * file_open(const char * const path, const char * const mode);

#endif	/* FILE_H */
