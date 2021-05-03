#include <libtguy.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <utf8proc.h>

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
 * Array of TGStrings
 */
typedef struct {
    TGString *data; /**< array of size len */
    size_t len;     /**< length */
} TrashField;

/**
 * Struct to keep relevant TrashGuy data
 */
struct TrashGuyState {
    void *udata; /**< pointer to user-allocated memory */
    unsigned initial_frames_count; /**< number of frames spent to process first element */
    TGString sprite_right, /**< when facing right */
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
};

/**
 *  Returns first frame of sequence of frames involving the work with element element_index of
 *  (\ref TrashGuyState::text). \n
 *  You can perceive this function as a parabola, where each x is element_index and y is returned frame: \n
 *  <code> frame = element_index<sup>2</sup> + (\ref TrashGuyState::initial_frames_count "initial_frames_count" - 1)element_index </code> \n
 *  See <a href='https://www.desmos.com/calculator/6z0ewdyxpq'>this visualization</a>. \n
 *  For example: \n
 *      <code> (get_frame_lower_boundary(const, i + 1) - get_frame_lower_boundary(const, i)) </code> \n
 *  will yield number of frames needed to process element with index i. \n
 *  Initial number of frames is computed as: \n
 *      <code> (\ref tguy_from_arr_ex() "spacing" + 1) * 2 </code>
 * @param initial_frames_count  \ref TrashGuyState::initial_frames_count
 * @param element_index         index of text element in \ref TrashGuyState::text[element_index]
 * @return                      first frame of processed element_index
 */
static inline unsigned get_frame_lower_boundary(unsigned initial_frames_count, unsigned element_index) {
    return element_index * (element_index + initial_frames_count - 1);
}

/**
 *  Clears TrashGuyState::field excluding first element (\ref TrashGuyState::sprite_can)
 *  removes n \ref TrashGuyState::text elements and replaces them with \ref TrashGuyState::sprite_space
 * @param st                Valid \ref TrashGuyState
 * @param n_erase_elements  number of \ref TrashGuyState::text elements to clear with \ref TrashGuyState::sprite_space
 */
static inline void tguy_clear_field(TrashGuyState *st, unsigned n_erase_elements) {
    unsigned items_offset = st->field.len - st->text.len + n_erase_elements;
    #ifdef TGUY_FASTCLEAR
    memcpy(&st->field.data[0], &st->empty_field_.data[0],
        items_offset * sizeof(*st->field.data));
    #else
    for (size_t i = 1; i < items_offset; i++) {
        st->field.data[i] = st->sprite_space;
    }
    #endif
    memcpy(&st->field.data[items_offset], &st->text.data[n_erase_elements],
        sizeof(*st->field.data) * (st->text.len - n_erase_elements));
}

TrashGuyState *tguy_from_arr_ex(const TGString *arr, size_t len, unsigned spacing,
    TGString *sprite_space, TGString *sprite_can, TGString *sprite_right, TGString *sprite_left) {
    struct TrashGuyState *st;
    assert(((void) "len is too big", len < (unsigned) -1));
    st = malloc(sizeof(*st));
    if (st == NULL) goto fail;
    {
        /* one frame for initial pos, spacing frames to walk over empty space to the first element, x2 to return back */
        st->initial_frames_count = (spacing + 1) * 2;
        st->sprite_right  = (sprite_right) ? *sprite_right : TGSTR("(> ^_^)>");
        st->sprite_left   = (sprite_left ) ? *sprite_left  : TGSTR("<(^_^ <)");
        st->sprite_can    = (sprite_can  ) ? *sprite_can   : TGSTR("\xf0\x9f\x97\x91");
        st->sprite_space  = (sprite_space) ? *sprite_space : TGSTR(" ");
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
        /* tguy_clear_field(st, 0); */
    }
    return st;

fail:
    free(st);
    return NULL;
}

TrashGuyState *tguy_from_arr(const TGString *arr, size_t len, unsigned spacing) {
    return tguy_from_arr_ex(arr, len, spacing, NULL, NULL, NULL, NULL);
}

TrashGuyState *tguy_from_utf8(const char *string, size_t len, unsigned spacing) {
    TrashGuyState *st;
    TGString *strarr;
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
            strarr[i] = (TGString) {&string[start], end - start};
            i++;
        }
    }
    st = tguy_from_arr(strarr, flen, spacing);
    if (st == NULL) {
        free(strarr);
    } else {
        st->udata = strarr;
    }
    return st;
}

/**
 * Also deallocates udata
 */
void tguy_free(TrashGuyState *st) {
    if (st == NULL) return;
    free(st->udata);
    free(st->text.data);
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
    assert(((void)"Frame is bigger than get_frames_count()", frame < st->max_frames));
    if (st->cur_frame == frame) return;
    {
        /* unsigned a = 1,*/
        unsigned b = (st->initial_frames_count - 1), c = frame;
        /* school math, see 1 */
        unsigned element_index = ((unsigned) sqrt((b * b) + (c << 2)/* a */ ) - b) / 2 /* a */;
        /* number of frames needed to process element, see 2 */
        unsigned frames_per_element = st->initial_frames_count + (2 * element_index);
        /* index of the frame in the frame series (up to frames_per_element) */
        unsigned sub_frame = (frame - get_frame_lower_boundary(st->initial_frames_count, element_index));
        /* if we're in the first half frames we're moving right, otherwise left */
        unsigned right = (sub_frame < (frames_per_element / 2));
        /* TrashGuy index yields 0 twice, the difference is whether we're moving right or left */
        unsigned i = right ? sub_frame : frames_per_element - sub_frame - 1;

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
         * by choosing the largest ensure the buffer is large enough */
        sz += tg_max(st->text.data[i].len, st->sprite_space.len);
    }
    /* overall free space length */
    for (size_t i = 0, slen = (st->initial_frames_count / 2) - 1; i < slen; i++) {
        sz += st->sprite_space.len;
    }

    sz += st->sprite_can.len;
    sz += tg_max(st->sprite_right.len, st->sprite_left.len);

    st->bufsize = sz;
    return sz;
}

#undef tg_max
