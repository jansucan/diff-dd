#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "options.h"
#include "print.h"
#include "resources.h"

static void
clean_exit(resources_t *const res, int exit_code)
{
    resources_free(res);
    exit(exit_code);
}

static int
open_files(const options_t *const opts, resources_t *const res,
           bool is_action_backup)
{
    /* Open the input file */
    if ((opts->in_file_path != NULL) &&
        ((res->in_file = fopen(opts->in_file_path, "r")) == NULL)) {
        print_error("cannot open input file: %s", strerror(errno));
        return 1;
    }

    /* Open the reference file */
    if ((res->ref_file = fopen(opts->ref_file_path, "r")) == NULL) {
        print_error("cannot open reference file: %s", strerror(errno));
        return 1;
    }

    /* Open the output file
     *
     * When restoring, the file must be opened for writing and not
     * truncated
     */
    char out_mode[] = "r+";

    if (is_action_backup) {
        /* When backing up, the output file is truncated to hold the
         * new data
         */
        out_mode[0] = 'w';
        out_mode[1] = '\0';
    }

    if ((res->out_file = fopen(opts->out_file_path, out_mode)) == NULL) {
        print_error("cannot open output file: %s", strerror(errno));
        return 1;
    }

    return 0;
}

static bool
is_reference_file_valid(resources_t *const res, uint32_t sector_size)
{
    const long ref_size = file_size(res->ref_file);

    if (ref_size < 0) {
        print_error("cannot get size of reference file: %s", strerror(errno));
        return false;
    } else if (ref_size == 0) {
        print_error("reference file is empty");
        return false;
    } else if ((ref_size % (sizeof(uint64_t) + sector_size)) != 0) {
        /* The reference file must hold equally sized sectors and the
         * offset of each of them
         */
        print_error(
            "reference file has size that cannot contain valid diff data");
        return false;
    }

    const long out_size = file_size(res->out_file);

    if (out_size < 0) {
        print_error("cannot get size of output file: %s", strerror(errno));
        return 1;
    }

    uint64_t ref_offset = 0;
    uint64_t prev_out_offset;

    /* Scan the reference file and check  */
    for (;;) {
        uint64_t out_offset;
        /* Read the next offset */
        const size_t ref_read =
            fread(&out_offset, sizeof(out_offset), 1U, res->ref_file);
        out_offset = le64toh(out_offset);

        if (feof(res->ref_file)) {
            break;
        } else if ((ref_read != 1U) || ferror(res->ref_file)) {
            print_error("cannot read from reference file: %s", strerror(errno));
            return false;
        } else if (((ref_offset != 0) && (out_offset <= prev_out_offset)) ||
                   ((out_offset + sector_size) > out_size)) {
            /* The offset must be higher than the previous one and it
             * must point into the file
             */
            print_error("reference file is not valid");
            return false;
        } else if (fseek(res->ref_file, sector_size, SEEK_CUR) != 0) {
            print_error("cannot seek in reference file: %s", strerror(errno));
            return false;
        }

        prev_out_offset = out_offset;
    }

    if (ftell(res->ref_file) != ref_size) {
        /* The reference file must be read completely */
        print_error("reference file is not valid");
        return false;
    } else if (fseek(res->ref_file, 0L, SEEK_SET) != 0) {
        /* The file must be prepared for the restoring */
        print_error("cannot seek in reference file: %s", strerror(errno));
        return false;
    }

    return true;
}

static int
diff_backup(const options_t *const opts, resources_t *const res)
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
                memcpy(res->out_buffer + out_buffer_index, &(res->in_buffer[i]),
                       opts->sector_size);
                out_buffer_index += sizeof(o) + opts->sector_size;
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

static int
diff_restore(const options_t *const opts, resources_t *const res)
{
    /* Check validity of the reference file */
    if (!is_reference_file_valid(res, opts->sector_size)) {
        return 1;
    }

    const long ref_size = file_size(res->ref_file);

    if (ref_size < 0) {
        print_error("cannot get size of reference file: %s", strerror(errno));
        return 1;
    }

    /* Allocate the buffer for data from the reference file */
    /* The reference buffer contains also the offsets */
    const size_t ref_buffer_size =
        ((opts->buffer_size / opts->sector_size) * sizeof(uint64_t)) +
        opts->buffer_size;

    if ((res->ref_buffer = (char *)malloc(ref_buffer_size)) == NULL) {
        print_error("cannot allocate buffer for reference file data");
        return 1;
    }

    /* Restore data from the differential image */
    uint64_t out_offset;

    for (;;) {
        /* Read data of the offset and the next sector */
        const size_t ref_read =
            fread(res->ref_buffer, ref_buffer_size, 1U, res->ref_file);

        if (feof(res->ref_file)) {
            break;
        } else if ((ref_read != 1U) || ferror(res->ref_file)) {
            print_error("cannot read from reference file: %s", strerror(errno));
            return 1;
        }

        /* Get offset */
        out_offset = le64toh(*((uint64_t *)res->ref_buffer));

        if (fseek(res->out_file, out_offset, SEEK_SET) != 0) {
            print_error("cannot seek in output file: %s", strerror(errno));
            return 1;
        }

        /* Write the sector data to the output file */
        const size_t out_written = fwrite(res->ref_buffer + sizeof(out_offset),
                                          opts->sector_size, 1U, res->out_file);

        if (out_written != 1U) {
            print_error("cannot write to output file: %s", strerror(errno));
            return 1;
        }
    }

    /* The reference file must be read completely */
    if (ftell(res->ref_file) != ref_size) {
        print_error("reference file is not valid");
        return 1;
    }

    return 0;
}

int
main(int argc, char **argv)
{
    options_t opts;

    if (options_parse(argc, argv, &opts) || opts.help) {
        options_usage(1);
    }

    const bool is_action_backup = (opts.in_file_path != NULL);
    resources_t res;

    resources_init(&res);

    if (open_files(&opts, &res, is_action_backup) != 0) {
        clean_exit(&res, 1);
    } else if (is_action_backup) {
        clean_exit(&res, diff_backup(&opts, &res));
    } else {
        clean_exit(&res, diff_restore(&opts, &res));
    }
}
