#include "backup.h"

#include "file.h"
#include "print.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static int check_files(const options_t *const opts,
                       const resources_t *const res);
static int allocate_buffers(const options_t *const opts, size_t out_buffer_size,
                            resources_t *const res);
static int read_sector(FILE *const file, char *const buffer,
                       uint32_t buffer_size, uint32_t sector_size);
static int write_out_buffer(const char *const buffer, size_t size,
                            FILE *const file);

static int
check_files(const options_t *const opts, const resources_t *const res)
{
    const long in_size = file_size(res->in_file);

    if (in_size < 0) {
        print_error("cannot get size of input file: %s", strerror(errno));
        return 1;
    }

    const long ref_size = file_size(res->ref_file);

    if (ref_size < 0) {
        print_error("cannot get size of reference file: %s", strerror(errno));
        return 1;
    }

    /* Check sizes of the input file and the reference file */
    if (in_size != ref_size) {
        print_error("input file and reference file differ in size");
        return 1;
    } else if ((in_size % opts->sector_size) != 0) {
        print_error(
            "size of input file and reference file is not multiple of" PRIu32,
            opts->sector_size);
        return 1;
    }

    return 0;
}

static int
allocate_buffers(const options_t *const opts, size_t out_buffer_size,
                 resources_t *const res)
{

    if ((res->in_buffer = (char *)malloc(opts->buffer_size)) == NULL) {
        print_error("cannot allocate buffer for input file data");
        return 1;
    } else if ((res->ref_buffer = (char *)malloc(opts->buffer_size)) == NULL) {
        print_error("cannot allocate buffer for reference file data");
        return 1;
    } else if ((res->out_buffer = (char *)malloc(out_buffer_size)) == NULL) {
        print_error("cannot allocate buffer for output file data");
        return 1;
    }

    return 0;
}

static int
read_sector(FILE *const file, char *const buffer, uint32_t buffer_size,
            uint32_t sector_size)
{
    const size_t bytes_read = fread(buffer, 1U, buffer_size, file);

    if (ferror(file)) {
        print_error("cannot read from input file: %s", strerror(errno));
        return 1;
    } else if ((bytes_read % sector_size) != 0) {
        print_error("data read from input file is not multiple of sector size");
        return 1;
    } else if (bytes_read != buffer_size) {
        print_error("cannot read enough data from input file");
        return 1;
    }

    return 0;
}

static int
write_out_buffer(const char *const buffer, size_t size, FILE *const file)
{
    const size_t bytes_written = fwrite(buffer, 1U, size, file);

    if (bytes_written != size) {
        print_error("cannot write to output file: %s", strerror(errno));
        return 1;
    }

    return 0;
}

int
backup(const options_t *const opts, resources_t *const res)
{
    if (check_files(opts, res) != 0) {
        return 1;
    }

    /* The output buffer contains also the offsets */
    const size_t out_buffer_size =
        ((opts->buffer_size / opts->sector_size) * sizeof(uint64_t)) +
        opts->buffer_size;

    if (allocate_buffers(opts, out_buffer_size, res) != 0) {
        return 1;
    }

    size_t out_buffer_index = 0;
    uint64_t input_file_offset = 0;

    while (!feof(res->in_file)) {
        /* Read the sectors from the input file to the buffer */
        if (read_sector(res->in_file, res->in_buffer, opts->buffer_size,
                        opts->sector_size) != 0) {
            return 1;
        }

        /* Read sectors from the reference file to the buffer */
        if (read_sector(res->ref_file, res->ref_buffer, opts->buffer_size,
                        opts->sector_size) != 0) {
            return 1;
        }

        /* Process sectors in the buffer */
        for (size_t buffer_offset = 0; buffer_offset < opts->buffer_size;
             buffer_offset += opts->sector_size) {
            if (memcmp(res->in_buffer + buffer_offset,
                       res->ref_buffer + buffer_offset,
                       opts->sector_size) != 0) {
                /* Backup the changed sector */
                if (out_buffer_index >= out_buffer_size) {
                    /* The output buffer is full. Write it to the output file.
                     */
                    if (write_out_buffer(res->out_buffer, out_buffer_index,
                                         res->out_file) != 1) {
                        return 1;
                    }
                    out_buffer_index = 0;
                }
                /* Write the next backup record */
                const uint64_t o = htole64(input_file_offset);
                memcpy(res->out_buffer + out_buffer_index, (void *)&o,
                       sizeof(o));
                out_buffer_index += sizeof(o);

                memcpy(res->out_buffer + out_buffer_index,
                       res->in_buffer + buffer_offset, opts->sector_size);
                out_buffer_index += opts->sector_size;
            }

            input_file_offset += opts->sector_size;
        }
    }

    /* Write out the output buffer */
    if (out_buffer_index >= 0) {
        if (write_out_buffer(res->out_buffer, out_buffer_index,
                             res->out_file) != 1) {
            return 1;
        }
    }

    return 0;
}
