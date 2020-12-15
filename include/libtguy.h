#ifndef _LIBTGUY_H_
#define _LIBTGUY_H_

#include <stddef.h>

typedef struct {
    const char *str;
    size_t len;
} CString;

typedef struct {
    CString arr[100];
    size_t len;
} TrashField;

typedef struct {
    int a1;
    CString sprite_right, sprite_left, sprite_can;
    TrashField text;
    TrashField field;
    int cur_frame;
    int max_frames;
} TrashGuyState;

TrashGuyState tguy_init_arr(const CString arr[], size_t len, int starting_distance);
TrashGuyState tguy_init_str(const char *string, size_t len, int starting_distance);

void tguy_from_frame(TrashGuyState *st, int frame);

void tguy_fprint(const TrashGuyState *st, FILE *fp);
void tguy_print(const TrashGuyState *st);
void tguy_bprint(const TrashGuyState *st, char *buf);
#endif //_LIBTGUY_H_
