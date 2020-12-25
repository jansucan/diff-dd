#include <stdio.h>
#include <stdlib.h>

#include "resources.h"

void
resources_init(resources_t *const res)
{
    res->in_file = NULL;
    res->ref_file = NULL;
    res->out_file = NULL;
    res->in_buffer = NULL;
    res->ref_buffer = NULL;
    res->out_buffer = NULL;
}

void
resources_free(resources_t *const res)
{
    if (res->in_file != NULL) {
        fclose(res->in_file);
        res->in_file = NULL;
    }
    if (res->ref_file != NULL) {
        fclose(res->ref_file);
        res->ref_file = NULL;
    }
    if (res->out_file != NULL) {
        fclose(res->out_file);
        res->out_file = NULL;
    }
    if (res->in_buffer != NULL) {
        free(res->in_buffer);
        res->in_buffer = NULL;
    }
    if (res->ref_buffer != NULL) {
        free(res->ref_buffer);
        res->ref_buffer = NULL;
    }
    if (res->out_buffer != NULL) {
        free(res->out_buffer);
        res->out_buffer = NULL;
    }
}
