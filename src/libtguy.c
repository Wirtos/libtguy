#include <libtguy.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <utf8proc.h>

#define ignored_ (void)

/**
 * @file libtguy.c
 */

/** \mainpage
 * For API documentation see \ref libtguy.h. \n
 * For implementation details see \ref libtguy.c. \n
 * For example python CFFI see
 *  <a href='https://gist.github.com/Wirtos/b9e0554e807109e6be14b136a5e737f2'>libtguy_python</a>.
 */

/**
 * Array of TGStrViews
 */
typedef struct {
    TGStrView *data; /**< array of size len */
    size_t len;     /**< length */
} TrashField;

/**
 *
 * @param str  string or NULL
 * @param len  str length or -1 if string is nul terminated
 * @return out or NULL if str is NULL
 */
static TGStrView *cstrtstrv(TGStrView out[1], const char *str, size_t len) {
    if (str == NULL) {
        return NULL;
    } else {
        out->str = str;
        out->len = (len == (size_t) -1) ? strlen(str) : len;
        return out;
    }
}

/**
 * Struct to keep relevant TrashGuy data
 */
struct TrashGuyState {
    void *udata; /**< pointer to user-allocated memory */
    unsigned initial_frames_count; /**< number of frames spent to process first element,
                                    * computed from spacing provided by the user: \n
                                    *   <code> (\ref tguy_from_arr_ex() "spacing" + 1) * 2 </code> */
    TGStrView sprite_right, /**< when facing right */
             sprite_left,  /**< when facing left */
             sprite_can,   /**< trash can sprite */
             sprite_space; /**< empty space sprite */
    TrashField text;       /**< elements for TrashGuy to process, each one can contain one or more characters */
    TrashField field;      /**< array where we place current element */
    #ifdef TGUY_FASTCLEAR
    TrashField empty_field_;/**< field, but filled with only trash can and space sprites */
    #endif
    unsigned cur_frame;     /**< current frame set, initially UINT_MAX */
    unsigned max_frames;    /**< number of frames animation takes to complete -> 0 <= frame < max_frames */
    size_t bufsize;         /**< computed size of the buffer to store one frame as string representation, initially 0 */
    char *output_str;
};

/**
 *  Returns first frame of sequence of frames involving the work with element_index of \ref TrashGuyState::text. \n
 *  You can perceive this function as a parabola, where each x is element_index and y is returned frame (starts from 0): \n
 *  <code> frame = element_index<sup>2</sup> + (\ref TrashGuyState::initial_frames_count "initial_frames_count" - 1)element_index </code> \n
 *  See <a href='https://www.desmos.com/calculator/6z0ewdyxpq'>this visualization</a>. \n
 *  For example: \n
 *      <code> (get_frame_lower_boundary(4, i + 1) - get_frame_lower_boundary(4, i)) </code> \n
 *  will yield number of frames needed to process i-th element if
 *   \ref TrashGuyState::initial_frames_count "initial_frames_count" was 4. \n
 * @param initial_frames_count  \ref TrashGuyState::initial_frames_count
 * @param element_index         index of text element in \ref TrashGuyState::text[element_index]
 * @return                      first frame of for element_index
 */
static inline unsigned get_frame_lower_boundary(unsigned initial_frames_count, unsigned element_index) {
    return element_index * (element_index + initial_frames_count - 1);
}

/**
 *  Replaces \ref TrashGuyState::field[1:n+1] with with \ref TrashGuyState::sprite_space
 * @param st                Valid \ref TrashGuyState
 * @param n_clear_elements  number of \ref TrashGuyState::text elements to clear with \ref TrashGuyState::sprite_space
 */
static inline void tguy_clear_field(TrashGuyState *st, unsigned n_clear_elements) {
    unsigned items_offset = st->field.len - st->text.len + n_clear_elements;
    #ifdef TGUY_FASTCLEAR
    memcpy(&st->field.data[0], &st->empty_field_.data[0],
        items_offset * sizeof(*st->field.data));
    #else
    for (size_t i = 1; i < items_offset; i++) {
        st->field.data[i] = st->sprite_space;
    }
    #endif
    memcpy(&st->field.data[items_offset], &st->text.data[n_clear_elements],
        sizeof(*st->field.data) * (st->text.len - n_clear_elements));
}

TrashGuyState *tguy_from_arr_ex(const TGStrView *arr, size_t len, unsigned spacing,
    TGStrView *sprite_space, TGStrView *sprite_can, TGStrView *sprite_right, TGStrView *sprite_left) {
    struct TrashGuyState *st;
    assert((ignored_"len is too big", len < (unsigned) -1));
    st = malloc(sizeof(*st));
    if (st == NULL) goto fail;
    {
        /* one frame for initial pos, spacing frames to walk over empty space to the first element, x2 to return back */
        st->initial_frames_count = (spacing + 1) * 2;
        st->sprite_right  = (sprite_right) ? *sprite_right : TGSTRV("(> ^_^)>");
        st->sprite_left   = (sprite_left ) ? *sprite_left  : TGSTRV("<(^_^ <)");
        st->sprite_can    = (sprite_can  ) ? *sprite_can   : TGSTRV("\xf0\x9f\x97\x91");
        st->sprite_space  = (sprite_space) ? *sprite_space : TGSTRV(" ");
        /* len here is the actual number of elements to process, not restricted to letters/glyphs */
        st->text.len = len;
        /* additional 2 elements to hold the guy and can sprites */
        st->field.len = 2 + spacing + st->text.len;
        #ifdef TGUY_FASTCLEAR
        /* fastclear option exploits the fact that linear memory copy is way faster to fill the field with spaces */
        st->empty_field_.len = st->field.len;
        #endif
        /* not computed yet and may not be computed at all */
        st->bufsize = 0;
        /* used to determine whether we should run set_frame and for unset assertions */
        st->cur_frame = (unsigned) -1;
        /* number of frames up to the last + 1 */
        st->max_frames = get_frame_lower_boundary(st->initial_frames_count, (unsigned) st->text.len) + 1;
        st->udata = NULL;
        st->output_str = NULL;
    }
    {
        size_t fields_data_sz = (st->text.len + st->field.len
            #ifdef TGUY_FASTCLEAR
                                 + st->empty_field_.len
            #endif
        );

        /* allocates memory for text, field and optionally empty_field in one malloc call,
         * memory layout: TTTTTffffffffffffEEEEEEEEEEEE, where T - text, f - field, E - empty field
         * therefore address of field is address of text + text.len and so on. */
        st->text.data = malloc(sizeof(*st->field.data) * fields_data_sz);
        if (st->text.data == NULL) goto fail;
        st->field.data = st->text.data + len;
        st->field.data[0] = st->sprite_can;

        #ifdef TGUY_FASTCLEAR
        /* empty field has the same size as field, it's only filled with spaces and trash can sprite as first element */
        st->empty_field_.data = st->field.data + st->field.len;
        st->empty_field_.data[0] = st->sprite_can;
        for (size_t i = 1, flen = st->empty_field_.len; i < flen; i++) {
            st->empty_field_.data[i] = st->sprite_space;
        }
        #endif

        for (size_t i = 0, slen = st->text.len; i < slen; i++) {
            st->text.data[i] = arr[i];
        }
        /* no need to clear the field because set_frame will do it anyway */
        /* calling print functions or get_arr at this point is undefined behavior */
    }
    return st;

fail:
    free(st);
    return NULL;
}

TrashGuyState *tguy_from_arr(const TGStrView *arr, size_t len, unsigned spacing) {
    return tguy_from_arr_ex(arr, len, spacing, NULL, NULL, NULL, NULL);
}

TrashGuyState *tguy_from_utf8_ex(const char *string, size_t len, unsigned spacing,
    const char *sprite_space, const char *sprite_can, const char *sprite_right, const char *sprite_left) {
    TrashGuyState *st;
    TGStrView *strarr;
    TGStrView sv_sprite_space, sv_sprite_can, sv_sprite_right, sv_sprite_left;
    size_t flen = 0;
    len = (len == (size_t) -1) ? strlen(string) : len;
    {
        int read_bytes = 0;
        unsigned start, end;
        while (utf8proc_iterate_graphemes((unsigned char *) string, &read_bytes, len, &start, &end)) {
            flen++;
        }
    }
    strarr = malloc(sizeof(*strarr) * flen);
    if (strarr == NULL) return NULL;
    { /* fill the array with ranges of the string representing whole utf-8 grapheme clusters */
        size_t i = 0;
        int read_bytes = 0;
        unsigned start, end;
        while (utf8proc_iterate_graphemes((unsigned char *) string, &read_bytes, len, &start, &end)) {
            strarr[i] = (TGStrView) {&string[start], end - start};
            i++;
        }
    }

    st = tguy_from_arr_ex(strarr, flen, spacing,
        cstrtstrv(&sv_sprite_space, sprite_space, -1),
        cstrtstrv(&sv_sprite_can, sprite_can, -1),
        cstrtstrv(&sv_sprite_right, sprite_right, -1),
        cstrtstrv(&sv_sprite_left, sprite_left, -1));
    if (st == NULL) {
        free(strarr);
    } else {
        st->udata = strarr;
    }
    return st;
}

TrashGuyState *tguy_from_utf8(const char *string, size_t len, unsigned spacing) {
    return tguy_from_utf8_ex(string, len, spacing, NULL, NULL, NULL, NULL);
}

/**
 * Also deallocates udata
 */
void tguy_free(TrashGuyState *st) {
    if (st == NULL) return;
    free(st->udata);
    free(st->text.data);
    free(st->output_str);
    free(st);
}

/**
 * In order to properly set frame we need to know few things beforehand:
 *  -# element_index for \ref TrashGuyState::text[element_index] we're currently working on
 *  -# number of frames per element
 *  -# first frame (boundary) involving the work on element with element_index
 *  -# sub frame (frame index)
 *  -# direction
 *  -# index within the field
 *
 *  All we know is desired frame and number of frames per first element.
 *  -# Because of the get_frame_lower_boundary() we know that initial frame of each element is computed as: \n
 *     <code> frame = element_index * (initial_frames_count + element_index - 1) </code> \n
 *     which is equivalent to this <a href='https://en.wikipedia.org/wiki/Quadratic_formula'>quadratic equation</a>: \n
 *     <code> (element_index)<sup>2</sup> + (initial_frames_count - 1)element_index - frame = 0 </code> \n
 *     We solve this equation for element_index, which is x<sub>1</sub>. \n
 *     (x<sub>1</sub> means right wing of the parabola,
 *         x<sub>2</sub> (left side) is meaningless to us, valid indices/frames only reside on the right side) \n
 *     <code> x<sub>1</sub> = (-b + sqrt(b<sup>2</sup> - 4ac)) / 2a </code> \n
 *     but because c is always negative and a is always 1, we can rewrite it as: \n
 *     <code> x<sub>1</sub> = (sqrt(b<sup>2</sup> + 4c) - b) / 2 </code>, which is the final formula
 *
 *  -# Simple arithmetic progression, with each next element number of frames increases by 2.
 *
 *  -# We now have all the values needed to call get_frame_lower_boundary().
 *
 *  -# <code> boundary <= (boundary + sub_frame) < boundary + frames_per_element </code>, \n
 *     thus <code> sub_frame = frame - boundary </code>
 *
 *  -# we spend the same number of frames moving left/right, if <code> sub_frame < (total / 2) </code> -> right, else -> left
 *
 *  -# index i within the field is computed as <code> sub_frame % (total / 2) </code>
 */
void tguy_set_frame(TrashGuyState *st, unsigned frame) {
    /*         a                        b                              c       */
    /* (element_index)^2 + (initial_frames_count - 1)element_index - frame = 0 */
    assert((ignored_"Frame is bigger than get_frames_count()", frame < st->max_frames));
    if (st->cur_frame == frame) return;
    {
        /* unsigned a = 1; */
        unsigned b = (st->initial_frames_count - 1), c = frame;
        /* school math, see 1 */
        unsigned element_index = ((unsigned) sqrt((b * b) + (4 * c)) - b) / 2;
        /* number of frames needed to process element, see 2 */
        unsigned frames_per_element = st->initial_frames_count + (2 * element_index);
        /* index of the frame in the frame series (up to frames_per_element) */
        unsigned sub_frame = (frame - get_frame_lower_boundary(st->initial_frames_count, element_index));
        /* if we're in the first half frames we're moving right, otherwise left */
        unsigned right = (sub_frame < (frames_per_element / 2));
        /* TrashGuy index yields 0 twice, the difference is whether we're moving right or left */
        unsigned i = (right) ? sub_frame : frames_per_element - sub_frame - 1;

        /* used to make set_frame faster by not setting same frame twice and to assert unset TrashGuyState */
        st->cur_frame = frame;

        /* if we're not moving right, then we're not drawing the n-th element because TrashGuy "carries" it */
        tguy_clear_field(st, element_index + !right);

        /* don't overwrite the trash can when placing the trash-guy */
        st->field.data[i + 1] = (right) ? st->sprite_right : st->sprite_left;
        /* Draw the element TrashGuy carries if we're not right near the trash can */
        if (!right && i != 0) {
            st->field.data[i] = st->text.data[element_index];
        }
    }
}

size_t tguy_fprint(const TrashGuyState *st, FILE *fp) {
    assert(st->cur_frame != (unsigned) -1);
    size_t len = 0;
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        TGStrView sv = st->field.data[i];
        len += fprintf(fp, "%.*s", (int) sv.len, sv.str);
    }
    return len;
}

size_t tguy_print(const TrashGuyState *st) { return tguy_fprint(st, stdout); }

size_t tguy_sprint(const TrashGuyState *st, char *buf) {
    assert(st->cur_frame != (unsigned) -1);
    char *start = buf;
    for (size_t i = 0, flen = st->field.len; i < flen; i++) {
        TGStrView sv = st->field.data[i];
        for (size_t j = 0, slen = sv.len; j < slen; j++) {
            *buf++ = sv.str[j];
        }
    }
    *buf = '\0';
    return buf - start;
}

const TGStrView *tguy_get_arr(const TrashGuyState *st, size_t *len) {
    assert(st->cur_frame != (unsigned) -1);
    *len = st->field.len;
    return st->field.data;
}

unsigned tguy_get_frames_count(const TrashGuyState *st) {
    return st->max_frames;
}

/** returns a or b */
#define tg_max(a, b) ((a) > (b) ? (a) : (b))

/**
 * If bsize is not set, then iterate over possible field layout and
 * compute bufsize enough to keep any frame plus nul terminator
 */
size_t tguy_get_bsize(TrashGuyState *st) {
    /* for nul terminator */
    size_t sz = 1;
    if (st->bufsize) {
        return st->bufsize;
    }
    /* overall text length */
    for (size_t i = 0, tlen = st->text.len; i < tlen; i++) {
        /* element will be replaced with space (filler sprite) eventually
         * by choosing the largest ensure the buffer is big enough */
        sz += tg_max(st->text.data[i].len, st->sprite_space.len);
    }
    /* overall free space length */
    sz += st->sprite_space.len * ((st->initial_frames_count / 2) - 1);
    sz += st->sprite_can.len;
    sz += tg_max(st->sprite_right.len, st->sprite_left.len);

    st->bufsize = sz;
    return sz;
}

#undef tg_max

const char *tguy_get_string(TrashGuyState *st, size_t *len) {
    size_t plen;
    if (st->output_str == NULL) {
        st->output_str = malloc(tguy_get_bsize(st));
    }
    if (st->output_str == NULL) {
        plen = 0;
    } else {
        plen = tguy_sprint(st, st->output_str);
    }
    if (len != NULL) *len = plen;
    return st->output_str;
}
