#ifndef RESOURCES_H
#define RESOURCES_H

typedef struct {
    FILE * in_file;
    FILE * ref_file;
    FILE * out_file;

    char * in_buffer;
    char * ref_buffer;
    char * out_buffer;
} resources_t;

void resources_init(resources_t * const res);
void resources_free(resources_t * const res);

#endif	/* RESOURCES_H */
