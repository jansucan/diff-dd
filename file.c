#include <errno.h>
#include <string.h>

#include "file.h"
#include "print.h"

long
file_size(FILE * const file)
{
    fpos_t p;

    if ((fgetpos(file, &p) != 0)
	|| (fseek(file, 0L, SEEK_END) != 0)) {
	return -1;
    }

    const long size = ftell(file);

    if (fsetpos(file, &p) != 0) {
	return -1;
    }

    return size;
}
