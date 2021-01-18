#ifndef LIBTGUY_H
#define LIBTGUY_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char *str;
    size_t len;
} CString;

typedef struct {
    CString data[200];
    size_t len;
} TrashField;

typedef struct {
    void *padding1_;
    int a1;
    CString sprite_right, sprite_left, sprite_can, sprite_space;
    TrashField text;
    TrashField field;
    #ifdef TGUY_FASTCLEAR
    TrashField empty_field_;
    #endif
    int cur_frame;
    int max_frames;
} TrashGuyState;

TrashGuyState tguy_init_arr(const CString arr[], size_t len, int starting_distance);
TrashGuyState tguy_init_str(const char *string, size_t len, int starting_distance);

void tguy_from_frame(TrashGuyState *st, int frame);

void tguy_fprint(const TrashGuyState *st, FILE *fp);
void tguy_print(const TrashGuyState *st);
void tguy_bprint(const TrashGuyState * st, char *buf);
#endif //LIBTGUY_H
