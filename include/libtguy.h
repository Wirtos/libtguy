#ifndef LIBTGUY_H
#define LIBTGUY_H

#include <stddef.h>
#include <stdio.h>
#include <libtguy_export.h>

#define TGUY_VER_MAJOR 0
#define TGUY_VER_MINOR 4
#define TGUY_VER_PATCH 0

typedef struct {
    const char *str;
    size_t len;
} TGString;

/*
 * Create a constant containerized string
 */
#define TGStringConst(str) ((TGString){str, sizeof(str) - 1})

typedef struct TrashGuyState TrashGuyState;

LIBTGUY_EXPORT TrashGuyState *tguy_from_arr_ex(const TGString *arr, size_t len, unsigned spacing,
        TGString sprite_space, TGString sprite_can, TGString sprite_left, TGString sprite_right);
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr(const TGString *arr, size_t len, unsigned spacing);
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8(const char *string, size_t len, unsigned spacing);
LIBTGUY_EXPORT void tguy_free(TrashGuyState *st);

LIBTGUY_EXPORT void tguy_set_frame(TrashGuyState *st, unsigned frame);
LIBTGUY_EXPORT unsigned tguy_get_frames_count(const TrashGuyState *st);
LIBTGUY_EXPORT size_t tguy_get_bsize(TrashGuyState *st);

LIBTGUY_EXPORT void tguy_fprint(const TrashGuyState *st, FILE *fp);
LIBTGUY_EXPORT void tguy_print(const TrashGuyState *st);
LIBTGUY_EXPORT void tguy_bprint(const TrashGuyState * st, char *buf);
LIBTGUY_EXPORT const TGString *tguy_get_arr(const TrashGuyState *st, size_t *len);

#endif /* LIBTGUY_H */
