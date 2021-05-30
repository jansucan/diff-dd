#include "backup.h"

#include "file.h"
#include "print.h"

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

int
backup(const options_t *const opts, resources_t *const res)
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

    /* Allocate the buffers */
    /* The output buffer contains also the offsets */
    const size_t out_buffer_size =
        ((opts->buffer_size / opts->sector_size) * sizeof(uint64_t)) +
        opts->buffer_size;

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

    size_t out_buffer_index = 0;
    uint64_t offset = 0;

    for (;;) {
        /* Read the sectors from the input file to the buffer */
        const size_t in_read =
            fread(res->in_buffer, 1U, opts->buffer_size, res->in_file);

        if ((in_read % opts->sector_size) != 0) {
            print_error(
                "data read from input file is not multiple of sector size");
            return 1;
        } else if (ferror(res->in_file)) {
            print_error("cannot read from input file: %s", strerror(errno));
            return 1;
        }

        /* Read sectors from the reference file to the buffer */
        const size_t ref_read =
            fread(res->ref_buffer, 1U, opts->buffer_size, res->ref_file);

        if ((ref_read % opts->sector_size) != 0) {
            print_error(
                "data read from reference file is not multiple of sector size");
            return 1;
        } else if (ferror(res->ref_file)) {
            print_error("%s", strerror(errno));
            return 1;
        }

        if (in_read != ref_read) {
            print_error(
                "data read from input file and reference file differ in size");
            return 1;
        }

        /* Process sectors in the buffer */
        for (size_t i = 0; i < in_read; i += opts->sector_size) {
            /* Compare the sectors  */
            int changed = 0;
            const size_t j_end = i + opts->sector_size;
            for (size_t j = i; j < j_end; ++j) {
                if (res->in_buffer[j] != res->ref_buffer[j]) {
                    changed = 1;
                    break;
                }
            }

            if (changed) {
                /* Write changed sector */
                if (out_buffer_index >= out_buffer_size) {
                    /* The output buffer is full. Write it to the
                     * output file.
                     */
                    const size_t x = fwrite(res->out_buffer, 1U,
                                            out_buffer_index, res->out_file);

                    if (x != out_buffer_index) {
                        print_error("cannot write to output file: %s",
                                    strerror(errno));
                        return 1;
                    }

                    out_buffer_index = 0;
                }

                const uint64_t o = htole64(offset);

                memcpy(res->out_buffer + out_buffer_index, (void *)&o,
                       sizeof(o));
                out_buffer_index += sizeof(o);
                memcpy(res->out_buffer + out_buffer_index, &(res->in_buffer[i]),
                       opts->sector_size);
                out_buffer_index += opts->sector_size;
            }

            offset += opts->sector_size;
        }

        if (feof(res->in_file)) {
            break;
        }
    }

    /* Write out the output buffer */
    if (out_buffer_index >= 0) {
        const size_t x =
            fwrite(res->out_buffer, 1U, out_buffer_index, res->out_file);

        if (x != out_buffer_index) {
            print_error("cannot write to output file: %s", strerror(errno));
            return 1;
        }
    }

    return 0;
}
