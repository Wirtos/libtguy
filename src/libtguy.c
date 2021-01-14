#include "../include/libtguy.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

/*
 * Create a constant containerized string
 */
#define CStringConst(str) ((CString){str, sizeof(str) - 1})

/*
 * Get the first frame index involving working with the element n
 */
static inline int get_frame_lower_boundary(int a1, int n) {
    return n * (a1 + n - 1);
}

static inline const char *utf8_next(const char *begin, const char *end) {
    if (begin == end) {
        return NULL;
    }

    if ((*begin & 0x80) == 0x0) {
        begin += 1;
    } else if ((*begin & 0xE0) == 0xC0) {
        begin += 2;
    } else if ((*begin & 0xF0) == 0xE0) {
        begin += 3;
    } else if ((*begin & 0xF8) == 0xF0) {
        begin += 4;
    }

    return begin;
}

static inline size_t utf8_distance(const char *begin, const char *end) {
    size_t dist = 0;
    while ((begin = utf8_next(begin, end))) dist++;
    return dist;
}

/*
 * Clear the field(including the guy) and remove the first n chars of the string
 */
static inline void tguy_clear_field(TrashGuyState *st, size_t n) {
    size_t i = 1;
    for (size_t j = 0; j < (st->a1 / 2) + n; i++, j++) {
        st->field.arr[i] = CStringConst(" ");
    }
    for (size_t j = n; j < st->text.len; j++, i++) {
        st->field.arr[i] = st->text.arr[j];
    }
}

/*
 * initialize state from array of strings
 */
TrashGuyState tguy_init_arr(const CString arr[], size_t len, int starting_distance) {
    TrashGuyState st = {
        .a1 = (starting_distance + 1) * 2,
        .sprite_right = CStringConst("(> ^_^)>"),
        .sprite_left = CStringConst("<(^_^ <)"),
        .sprite_can = CStringConst("\xf0\x9f\x97\x91"),
        .text = {
            .len = len,
        },
        .field = {
            .len = starting_distance + st.text.len + 2 /* additional 2 elements to hold the guy and can sprites */
        },
        .cur_frame = 0, /* currently unused */
        .max_frames = get_frame_lower_boundary(st.a1, st.text.len) + 1, /* number of frames up to the last + 1 */
    };

    st.field.arr[0] = st.sprite_can;
    tguy_clear_field(&st, 0);
    st.field.arr[1] = st.sprite_right;
    for (size_t i = 0; i < st.text.len; i++) {
        st.text.arr[i] = arr[i];
    }
    return st;
}

/*
 * initialize state from utf-8 string (each codepoint will be used as an element)
 */
TrashGuyState tguy_init_str(const char *string, size_t len, int starting_distance) {
    TrashField tf = {.len = utf8_distance(&string[0], &string[len])};
    { /* fill the array with ranges of the string representing whole utf-8 codepoints (up to 4 per element) */
        const char *it = string;
        for (size_t i = 0; i < tf.len; i++) {
            const char *next;
            next = utf8_next(it, &string[len]);
            tf.arr[i] = (CString) {it, next - it};
            it = next;
        }
    }
    return tguy_init_arr(tf.arr, tf.len, starting_distance);
}

/*
 * Fill the state with desirable frame
 */
void tguy_from_frame(TrashGuyState *st, int frame) {
    /* (n ^ 2) + (a1 - 1)n - frame = 0 */
    assert(frame < st->max_frames);
    {
        /* int a = 1,*/
        int b = (st->a1 - 1),
            c = -frame;
        /* school math */
        size_t n = ((size_t)sqrt((b * b) - 4 * c /* a */ ) - b) / 2 /* a */;
        /* total number of frame drawn for moving the letter with index n (moving from the start to the end)*/
        size_t frames_per_n = st->a1 + (2 * n);
        /* order of the frame in the frame series (up to frames_per_n) */
        size_t sub_frame = (frame - get_frame_lower_boundary(st->a1, n));
        /* if we're in the first half frames we're moving right, otherwise left */
        int right = (sub_frame < frames_per_n / 2);
        /* index yields 0 twice, the difference is whether we're moving right */
        size_t i = right ? sub_frame : frames_per_n - sub_frame - 1;

        st->cur_frame = frame; /* unused */
        /* if we're not moving right, then we're not drawing the element n because trashguy holds it */
        tguy_clear_field(st, n + !right);

        /* don't overwrite the trash can */
        st->field.arr[i + 1] = right ? st->sprite_right : st->sprite_left;
        /* Draw the element trashguy holds */
        if (!right && i != 0) {
            st->field.arr[i] = st->text.arr[n];
        }
    }
}

void tguy_fprint(const TrashGuyState *st, FILE *fp) {
    for (size_t i = 0; i < st->field.len; i++) {
        printf("%.*s", (int) st->field.arr[i].len, st->field.arr[i].str);
    }
}

void tguy_print(const TrashGuyState *st) { tguy_fprint(st, stdout); }

void tguy_bprint(const TrashGuyState *st, char *buf) {
    size_t c = 0;
    for (size_t i = 0; i < st->field.len; i++) {
        for (size_t j = 0; j < st->field.arr[i].len; ++j) {
            buf[c++] = st->field.arr[i].str[j];
        }
    }
    buf[c] = '\0';
}