#include <libtguy.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <utf8proc.h>

typedef struct {
    TGString *data;
    size_t len;
} TrashField;

struct TrashGuyState {
    void *data_;
    unsigned initial_frames_count;
    TGString sprite_right, sprite_left, sprite_can, sprite_space;
    TrashField text;
    TrashField field;
    #ifdef TGUY_FASTCLEAR
    TrashField empty_field_;
    #endif
    unsigned cur_frame;
    unsigned max_frames;
    size_t bufsize;
};

/*
 * Get the first frame index involving working with the element n
 */
static inline unsigned get_frame_lower_boundary(unsigned initial_frames_count, unsigned n_elem) {
    return n_elem * (initial_frames_count + n_elem - 1);
}

/*
 * Clear the field(including the guy) and remove the first n chars of the string
 */
static inline void tguy_clear_field(TrashGuyState *st, unsigned n_erase_elements) {
    unsigned items_offset = st->initial_frames_count / 2 + n_erase_elements + 1;
    size_t flen = st->field.len - st->text.len + n_erase_elements;
    #ifdef TGUY_FASTCLEAR
    memcpy(&st->field.data[0], &st->empty_field_.data[0],
        flen * sizeof(*st->field.data));
    #else
    for (size_t i = 1; i < flen; i++) {
        st->field.data[i] = st->sprite_space;
    }
    #endif
    memcpy(&st->field.data[items_offset], &st->text.data[n_erase_elements],
        sizeof(*st->field.data) * (st->text.len - n_erase_elements));
}

/*
 * initialize state from array of strings
 */
TrashGuyState *tguy_from_arr_ex(const TGString *arr, size_t len, unsigned spacing,
    TGString sprite_space, TGString sprite_can, TGString sprite_right, TGString sprite_left) {
    struct TrashGuyState *st;
    assert(len < (unsigned) -1);
    st = malloc(sizeof(*st));
    if (st == NULL) goto fail;
    {
        st->initial_frames_count = (spacing + 1) * 2;
        st->sprite_right = sprite_right;
        st->sprite_left = sprite_left;
        st->sprite_can = sprite_can;
        st->sprite_space = sprite_space;
        st->text.len = len;
        /* additional 2 elements to hold the guy and can sprites */
        st->field.len = spacing + st->text.len + 2;
        #ifdef TGUY_FASTCLEAR
        st->empty_field_.len = st->field.len;
        #endif
        /* not computed yet and may not be computed at all */
        st->bufsize = 0;
        /* currently unused */
        st->cur_frame = (unsigned) -1;
        /* number of frames up to the last + 1 */
        st->max_frames = get_frame_lower_boundary(st->initial_frames_count, (unsigned) st->text.len) + 1;
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

TrashGuyState *tguy_from_arr(const TGString *arr, size_t len, unsigned spacing) {
    return tguy_from_arr_ex(arr, len, spacing,
        TGStringConst(" "),
        TGStringConst("\xf0\x9f\x97\x91"),
        TGStringConst("(> ^_^)>"),
        TGStringConst("<(^_^ <)")
    );
}

/*
 * initialize state from utf-8 string (each codepoint will be used as an element)
 */
TrashGuyState *tguy_from_utf8(const char *string, size_t len, unsigned spacing) {
    TrashGuyState *st;
    TGString *strarr;
    size_t flen = 0;
    len = (len == (size_t) -1) ? strlen(string) : len;
    {
        int read_bytes = 0;
        unsigned start, end;
        while (utf8proc_iterate_graphemes((unsigned char *)string, &read_bytes, len, &start, &end)) {
            flen++;
        }
    }
    strarr = malloc(sizeof(*strarr) * flen);
    if (strarr == NULL) return NULL;
    { /* fill the array with ranges of the string representing whole utf-8 grapheme clusters */
        size_t i = 0;
        int read_bytes = 0;
        unsigned start, end;
        while (utf8proc_iterate_graphemes((unsigned char *)string, &read_bytes, len, &start, &end)) {
            strarr[i] = (TGString){&string[start], end - start};
            i++;
        }
    }
    st = tguy_from_arr(strarr, flen, spacing);
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
void tguy_set_frame(TrashGuyState *st, unsigned frame) {
    /*         a                        b                              c       */
    /* (element_index)^2 + (initial_frames_count - 1)element_index - frame = 0 */
    assert(frame < st->max_frames);
    if (st->cur_frame == frame) return;
    {
        /* unsigned a = 1,*/
        unsigned b = (st->initial_frames_count - 1),
            c = frame;
        /* school math */
        unsigned element_index = ((unsigned) sqrt((b * b) + (c << 2)/* a */ ) - b) / 2 /* a */;
        /* total number of frames drawn for moving the letter with index n (moving from the start to the end and backwards) */
        unsigned frames_per_n = st->initial_frames_count + (2 * element_index);
        /* order of the frame in the frame series (up to frames_per_n) */
        unsigned sub_frame = (frame - get_frame_lower_boundary(st->initial_frames_count, element_index));
        /* if we're in the first half frames we're moving right, otherwise left */
        unsigned right = (sub_frame < frames_per_n / 2);
        /* index yields 0 twice, the difference is whether we're moving right */
        unsigned i = right ? sub_frame : frames_per_n - sub_frame - 1;

        st->cur_frame = frame; /* unused */
        /* if we're not moving right, then we're not drawing the element n because trashguy holds it */
        tguy_clear_field(st, element_index + !right);

        /* don't overwrite the trash can */
        st->field.data[i + 1] = right ? st->sprite_right : st->sprite_left;
        /* Draw the element trashguy holds */
        if (!right && i != 0) {
            st->field.data[i] = st->text.data[element_index];
        }
    }
}

void tguy_fprint(const TrashGuyState *st, FILE *fp) {
    assert(st->cur_frame != (unsigned) -1);
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        fprintf(fp, "%.*s", (int) st->field.data[i].len, st->field.data[i].str);
    }
}

void tguy_print(const TrashGuyState *st) { tguy_fprint(st, stdout); }

void tguy_bprint(const TrashGuyState *st, char *buf) {
    assert(st->cur_frame != (unsigned) -1);
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        const char *s = st->field.data[i].str;
        for (size_t j = 0, slen = st->field.data[i].len; j < slen; j++) {
            *buf++ = s[j];
        }
    }
    *buf = '\0';
}

const TGString *tguy_get_arr(const TrashGuyState *st, size_t *len) {
    assert(st->cur_frame != (unsigned) -1);
    *len = st->field.len;
    return st->field.data;
}

unsigned tguy_get_frames_count(const TrashGuyState *st) {
    return st->max_frames;
}

#define tg_max(a, b) ((a) > (b) ? (a) : (b))

size_t tguy_get_bsize(TrashGuyState *st) {
    /* nul terminator */
    size_t sz = 1;
    if (st->bufsize) {
        return st->bufsize;
    }
    /* overall text length */
    for (size_t i = 0, tlen = st->text.len; i < tlen; i++) {
        /* text letter will be replaced with space (filler sprite) eventually
         * by choosing the largest ensure the buffer is large enough */
        sz += tg_max(st->text.data[i].len, st->sprite_space.len);
    }
    /* overall free space length */
    for (size_t i = 0, slen = (st->initial_frames_count / 2) - 1; i < slen; i++) {
        sz += st->sprite_space.len;
    }
    /* singleton sprites lengths */
    sz += st->sprite_can.len;
    sz += tg_max(st->sprite_right.len, st->sprite_left.len);

    st->bufsize = sz;
    return sz;
}

#undef tg_max
