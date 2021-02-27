#ifndef LIBTGUY_H
#define LIBTGUY_H

#include <stddef.h>
#include <stdio.h>
#include <libtguy_export.h>

#define TGUY_VER_MAJOR 0
#define TGUY_VER_MINOR 3
#define TGUY_VER_PATCH 0

typedef struct {
    const char *str;
    size_t len;
} CString;

typedef struct TrashGuyState TrashGuyState;

LIBTGUY_EXPORT TrashGuyState *tguy_from_arr(const CString *arr, size_t len, int starting_distance);
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8(const char *string, size_t len, int starting_distance);
LIBTGUY_EXPORT void tguy_free(TrashGuyState *st);

LIBTGUY_EXPORT void tguy_from_frame(TrashGuyState *st, int frame);
LIBTGUY_EXPORT int tguy_get_frames_count(const TrashGuyState *st);
LIBTGUY_EXPORT size_t tguy_get_bsize(TrashGuyState *st);

LIBTGUY_EXPORT void tguy_fprint(const TrashGuyState *st, FILE *fp);
LIBTGUY_EXPORT void tguy_print(const TrashGuyState *st);
LIBTGUY_EXPORT void tguy_bprint(const TrashGuyState * st, char *buf);
#endif //LIBTGUY_H
