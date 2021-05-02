#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool help;
    uint32_t sector_size;
    uint32_t buffer_size;

    const char *in_file_path;
    const char *ref_file_path;
    const char *out_file_path;
} options_t;

int options_parse(int argc, char **argv, options_t *const opts);

#endif /* OPTIONS_H */
