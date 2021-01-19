#include <libtguy.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/*
 * Create a constant containerized string
 */
#define CStringConst(str) ((CString){str, sizeof(str) - 1})

typedef struct {
    CString *data;
    size_t len;
} TrashField;

struct TrashGuyState {
    void *data_;
    int a1;
    CString sprite_right, sprite_left, sprite_can, sprite_space;
    TrashField text;
    TrashField field;
    #ifdef TGUY_FASTCLEAR
    TrashField empty_field_;
    #endif
    int cur_frame;
    int max_frames;

};

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
    while ((begin = utf8_next(begin, end)) != 0) dist++;
    return dist;
}

/*
 * Clear the field(including the guy) and remove the first n chars of the string
 */
static inline void tguy_clear_field(TrashGuyState *st, int n) {
    int items_offset = st->a1 / 2 + n + 1;
    size_t flen = st->field.len - st->text.len + n;
    #ifdef TGUY_FASTCLEAR
    memcpy(&st->field.data[0], &st->empty_field_.data[0], flen * sizeof(*st->field.data));
    #else
    for (size_t i = 1; i < flen; i++) {
        st->field.data[i] = st->sprite_space;
    }
    #endif
    memcpy(&st->field.data[items_offset], &st->text.data[n], sizeof(*st->field.data) * (st->text.len - n));
}

/*
 * initialize state from array of strings
 */
TrashGuyState *tguy_init_arr(const CString arr[], size_t len, int starting_distance) {
    struct TrashGuyState *st;
    st = malloc(sizeof(*st));
    if (st == NULL) goto fail;
    {
        st->a1 = (starting_distance + 1) * 2;
        st->sprite_right = CStringConst("(> ^_^)>");
        st->sprite_left = CStringConst("<(^_^ <)");
        st->sprite_can = CStringConst("\xf0\x9f\x97\x91");
        st->sprite_space = CStringConst(" ");
        st->text.len = len;
        /* additional 2 elements to hold the guy and can sprites */
        st->field.len = starting_distance + st->text.len + 2;
        #ifdef TGUY_FASTCLEAR
        st->empty_field_.len = st->field.len;
        #endif
        /* currently unused */
        st->cur_frame = 0;
        /* number of frames up to the last + 1 */
        st->max_frames = get_frame_lower_boundary(st->a1, (int) st->text.len) + 1;
        st->data_ = NULL;
    }
    {
        size_t fields_data_sz = (st->text.len + st->field.len
            #ifdef TGUY_FASTCLEAR
            + st->empty_field_.len
            #endif
        );

        st->text.data = malloc(sizeof(*st->field.data) * fields_data_sz);
        if (st->text.data == NULL) goto fail;
        st->field.data = st->text.data + len;
        st->field.data[0] = st->sprite_can;

        #ifdef TGUY_FASTCLEAR
        st->empty_field_.len = st->field.len;
        st->empty_field_.data = st->field.data + st->field.len;
        st->empty_field_.data[0] = st->sprite_can;
        for (size_t i = 1, flen = st->empty_field_.len; i < flen; i++) {
            st->empty_field_.data[i] = st->sprite_space;
        }
        #endif

        for (size_t i = 0, slen = st->text.len; i < slen; i++) {
            st->text.data[i] = arr[i];
        }
        tguy_clear_field(st, 0);
    }
    return st;
    fail:
    free(st);
    return NULL;
}

/*
 * initialize state from utf-8 string (each codepoint will be used as an element)
 */
TrashGuyState *tguy_init_str(const char *string, size_t len, int starting_distance) {
    TrashGuyState *st;
    CString *strarr;
    size_t flen = utf8_distance(&string[0], &string[len]);
    strarr = malloc(sizeof(*strarr) * flen);
    if (strarr == NULL) return NULL;
    { /* fill the array with ranges of the string representing whole utf-8 codepoints (up to 4 per element) */
        const char *it = string;
        for (size_t i = 0; i < flen; i++) {
            const char *next;
            next = utf8_next(it, &string[len]);
            strarr[i] = (CString) {it, next - it};
            it = next;
        }
    }
    st = tguy_init_arr(strarr, flen, starting_distance);
    if (st == NULL) {
        free(strarr);
    } else {
        st->data_ = strarr;
    }
    return st;
}

void tguy_free(TrashGuyState *st) {
    if (st == NULL) return;
    free(st->data_);
    free(st->text.data);
    free(st);
}

/*
 * Fill the state with desirable frame
 */
void tguy_from_frame(TrashGuyState *st, int frame) {
    /* (n ^ 2) + (a1 - 1)n - frame = 0 */
    assert(frame >= 0 && frame < st->max_frames);
    {
        /* int a = 1,*/
        int b = (st->a1 - 1),
            c = -frame;
        /* school math */
        int n = ((int) sqrt((b * b) - 4 * c /* a */ ) - b) / 2 /* a */;
        assert(n >= 0);
        /* total number of frames drawn for moving the letter with index n (moving from the start to the end and backwards) */
        int frames_per_n = st->a1 + (2 * n);
        /* order of the frame in the frame series (up to frames_per_n) */
        int sub_frame = (frame - get_frame_lower_boundary(st->a1, n));
        /* if we're in the first half frames we're moving right, otherwise left */
        int right = (sub_frame < frames_per_n / 2);
        /* index yields 0 twice, the difference is whether we're moving right */
        int i = right ? sub_frame : frames_per_n - sub_frame - 1;

        st->cur_frame = frame; /* unused */
        /* if we're not moving right, then we're not drawing the element n because trashguy holds it */
        tguy_clear_field(st, n + !right);

        /* don't overwrite the trash can */
        st->field.data[i + 1] = right ? st->sprite_right : st->sprite_left;
        /* Draw the element trashguy holds */
        if (!right && i != 0) {
            st->field.data[i] = st->text.data[n];
        }
    }
}

void tguy_fprint(const TrashGuyState *st, FILE *fp) {
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        fprintf(fp, "%.*s", (int) st->field.data[i].len, st->field.data[i].str);
    }
}

void tguy_print(const TrashGuyState *st) { tguy_fprint(st, stdout); }

void tguy_bprint(const TrashGuyState *st, char *buf) {
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        const char *s = st->field.data[i].str;
        for (size_t j = 0, slen = st->field.data[i].len; j < slen; j++) {
            *buf++ = s[j];
        }
    }
    *buf = '\0';
}

int tguy_get_frames_count(const TrashGuyState *st) {
    return st->max_frames;
}

size_t tguy_get_bsize(const TrashGuyState *st) {
    size_t sz = 0;
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        sz += st->field.data[i].len;
    }
    return sz;
}
