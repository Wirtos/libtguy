#ifndef LIBTGUY_H
#define LIBTGUY_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char *str;
    size_t len;
} CString;

typedef struct TrashGuyState TrashGuyState;

TrashGuyState *tguy_init_arr(const CString arr[], size_t len, int starting_distance);
TrashGuyState *tguy_init_str(const char *string, size_t len, int starting_distance);
void tguy_free(TrashGuyState *st);

void tguy_from_frame(TrashGuyState *st, int frame);
int tguy_get_frames_count(const TrashGuyState *st);
size_t tguy_get_bsize(const TrashGuyState *st);

void tguy_fprint(const TrashGuyState *st, FILE *fp);
void tguy_print(const TrashGuyState *st);
void tguy_bprint(const TrashGuyState * st, char *buf);
#endif //LIBTGUY_H
