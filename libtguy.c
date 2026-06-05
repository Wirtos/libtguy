#include <libtguy.h>

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#ifdef TGUY_USE_UTF8PROC
#include <utf8proc.h>
#elif defined TGUY_USE_WGRAPHEME
#include <wgrapheme.h>
#else
#define TGUY_NO_GRAPHEME
#endif

#define ignored_ (void)

/**
 * @file libtguy.c
 */

/** \mainpage
 * For API documentation see libtguy.h. \n
 * For implementation details see libtguy.c. \n
 * For example python CFFI see
 *  <a href='https://gist.github.com/Wirtos/b9e0554e807109e6be14b136a5e737f2'>libtguy_python</a>.
 */

/**
 * Array of TGStrViews
 */
typedef struct {
    TGStrView *data; /**< array of size len */
    size_t len; /**< length */
} TGStrViewArr;

/**
 * @param[out] res  where to keep resulting TGStrView for str
 * @param str  string or NULL
 * @param len  str length or -1 if string is nul terminated
 * @return res or NULL if str is NULL
 */
static TGStrView *cstr2tgstrv(TGStrView *res, const char *str, size_t len) {
    if (str == NULL) {
        res->str = "";
        res->len = 0;
    } else {
        res->str = str;
        res->len = (len == (size_t)-1) ? strlen(str) : len;
    }
    return res;
}

static size_t strvarr_strlen(const TGStrView arr[], size_t len) {
    size_t plen = 0;
    if (arr == NULL) return 0;
    for (size_t i = 0; i < len; i++) { plen += arr[i].len; }
    return plen;
}

static size_t strvarr_write(char str_out[], const TGStrView arr[], size_t len) {
    size_t plen = 0;
    if (arr == NULL) return 0;
    for (size_t i = 0; i < len; i++) {
        memcpy(&str_out[plen], arr[i].str, arr[i].len);
        plen += arr[i].len;
    }
    return plen;
}

/* like strvarr_copy but uses str_mem as source for dst, it's assumed to be at least strvarr_strlen in size */
static size_t strvarr_copy_src(TGStrView *dst, const TGStrView *src, size_t len, const char str_mem[]) {
    size_t si = 0;
    if (src == NULL) return 0;
    for (size_t i = 0; i < len; i++) {
        dst[i].len = src[i].len;
        dst[i].str = &str_mem[si];
        si += dst[i].len;
    }
    return si;
}

/* copy one array of strviews into another */
static void strvarr_copy(TGStrView *dst, const TGStrView *src, size_t len) {
    for (size_t i = 0; i < len; i++) dst[i] = src[i];
}

/**
 * Struct to keep relevant TrashGuy data
 */
struct TrashGuyState {
    unsigned first_element_frames_count; /**< number of frames spent to process first element,
                                    * computed from spacing provided by the user: \n
                                    *   <code> (\ref tguy_from_arr_ex() "spacing" + 1) * 2 </code> */
    TGStrView sprite_right, /**< when facing right */
              sprite_left, /**< when facing left */
              sprite_can, /**< trash can sprite */
              sprite_space; /**< empty space sprite */
    TGStrViewArr text; /**< elements for TrashGuy to process, each one can contain one or more characters */
    TGStrViewArr arena; /**< array where we place current element */
#ifdef TGUY_FASTCLEAR
    TGStrViewArr empty_arena_; /**< arena, but filled with only trash can and space sprites */
#endif
    unsigned cur_frame; /**< current frame set, initially UINT_MAX */
    unsigned max_frames; /**< number of frames animation takes to complete -> 0 <= frame < max_frames */
    unsigned pos;
    unsigned facing_right;
    unsigned element_index;
    unsigned next_element_index;
    size_t buf_size; /**< computed size of the buffer to store one frame as string representation */
    char *output_str; /**< optional pointer to output string is stored here */
    TGStrView views_mem[]; /**< array of allocated views which are later distributed among fields */
};

/**
 *  Returns first frame of sequence of frames involving the work with element_index of TrashGuyState::text. \n
 *  You can perceive this function as a parabola, where each x is element_index and y is returned frame (starts from 0): \n
 *  <code> frame = element_index<sup>2</sup> + (TrashGuyState::first_element_frames_count - 1)element_index </code> \n
 *  See <a href='https://www.desmos.com/calculator/6z0ewdyxpq'>this visualization</a>. \n
 *  For example: \n
 *      <code> (get_first_frame_for_element(4, i + 1) - get_first_frame_for_element(4, i)) </code> \n
 *  will yield number of frames needed to process i-th element if
 *   TrashGuyState::first_element_frames_count was 4. \n
 * @param first_element_frames_count  TrashGuyState::first_element_frames_count
 * @param element_index         index of text element in TrashGuyState::text[element_index]
 * @return                      first frame index for element_index
 */
static inline unsigned get_first_frame_for_element(unsigned first_element_frames_count, unsigned element_index) {
    return element_index * (element_index + first_element_frames_count - 1); /* todo: math overflow checks? */
}

/**
 *  Replaces TrashGuyState::arena[1:n+1] with with TrashGuyState::sprite_space
 * @param st                Valid TrashGuyState
 * @param n_clear_elements  number of TrashGuyState::text elements to clear with TrashGuyState::sprite_space
 */
static inline void tguy_clear_field(const TrashGuyState *st, unsigned n_clear_elements) {
    TGStrViewArr arena = st->arena;
    TGStrViewArr text = st->text;
    size_t items_offset = arena.len - text.len + n_clear_elements;
#ifdef TGUY_FASTCLEAR
    memcpy(&arena.data[0], &st->empty_arena_.data[0],
           items_offset * sizeof(arena.data[0]));
#else
    TGStrView sprite_space = st->sprite_space;
    for (size_t i = 1; i < items_offset; i++) { arena.data[i] = sprite_space; }
#endif
    memcpy(&arena.data[items_offset], &text.data[n_clear_elements],
           sizeof(st->arena.data[0]) * (text.len - n_clear_elements));
}

TrashGuyState *tguy_from_arr_ex_2(const TGStrView arr[],
                                  size_t len,
                                  unsigned spacing,
                                  const TGStrView *sprite_space,
                                  const TGStrView *sprite_can,
                                  const TGStrView *sprite_right,
                                  const TGStrView *sprite_left,
                                  int preserve_strings) {
    if (arr == NULL) len = 0;
    struct TrashGuyState *st;
    size_t str_len = 0;
    char *str_mem = NULL;
    const size_t
        arena_size = 2 + spacing + len + 1, /* 3 additional places for: can, tguy sprite and nul */
        all_fields_len = (
            len
            + arena_size
#ifdef TGUY_FASTCLEAR
            /* additional memory for empty arena */
            + arena_size
#endif
        ),
        str_mem_off = offsetof(TrashGuyState, views_mem) + (sizeof(TGStrView) * all_fields_len);
    TGStrView
        sv_right = (sprite_right) ? *sprite_right : TGSTRV("(> ^_^)>"),
        sv_left = (sprite_left) ? *sprite_left : TGSTRV("<(^_^ <)"),
        sv_can = (sprite_can) ? *sprite_can : TGSTRV("\xf0\x9f\x97\x91"),
        sv_space = (sprite_space) ? *sprite_space : TGSTRV(" ");

    assert((ignored_"len is too big", len < (unsigned) -1));
    if (preserve_strings) {
        str_len = strvarr_strlen(arr, len)
            + sv_right.len
            + sv_left.len
            + sv_can.len
            + sv_space.len;
    }
    st = malloc(str_mem_off + str_len);
    if (st == NULL) return NULL;

    st->sprite_right = sv_right;
    st->sprite_left = sv_left;
    st->sprite_can = sv_can;
    st->sprite_space = sv_space;


    /* len here is the actual number of elements to process, not restricted to letters/glyphs */
    st->text = (TGStrViewArr){
        st->views_mem,
        len
    };

    st->arena = (TGStrViewArr){
        st->text.data + len,
        arena_size - 1 /* minus nul */
    };

#ifdef TGUY_FASTCLEAR
    /* fastclear option exploits the fact that linear memory copy is way faster to fill the arena with spaces */
    st->empty_arena_ = (TGStrViewArr){
        st->arena.data + arena_size,
        st->arena.len
    };
#endif

    if (preserve_strings) {
        /* copy strings from views to allocated linear memory block, then assign new addresses to views */
        char *str_base;
        str_mem = (char *)st + str_mem_off;
        str_base = str_mem;
        str_mem += strvarr_write(str_mem, arr, len);

        st->sprite_right.str = str_mem;
        str_mem += strvarr_write(str_mem, &sv_right, 1);

        st->sprite_left.str = str_mem;
        str_mem += strvarr_write(str_mem, &sv_left, 1);

        st->sprite_can.str = str_mem;
        str_mem += strvarr_write(str_mem, &sv_can, 1);

        st->sprite_space.str = str_mem;
        (void)strvarr_write(str_mem, &sv_space, 1);

        str_mem = str_base;
    }

    /* fields initialization */
    st->arena.data[0] = st->sprite_can;
    st->arena.data[st->arena.len] = (TGStrView){NULL, 0};

#ifdef TGUY_FASTCLEAR
    /* empty arena has the same size as arena, it's only filled with spaces and trash can sprite as first element */
    st->empty_arena_.data[0] = st->sprite_can;
    st->empty_arena_.data[st->empty_arena_.len] = (TGStrView){NULL, 0};
    for (size_t i = 1, flen = st->empty_arena_.len; i < flen; i++) {
        st->empty_arena_.data[i] = st->sprite_space;
    }
#endif

    if (preserve_strings) {
        strvarr_copy_src(st->text.data, arr, len, str_mem);
    } else {
        strvarr_copy(st->text.data, arr, len);
    }

    /* one frame for initial pos, spacing frames to walk over empty space to the first element, x2 to return back */
    st->first_element_frames_count = (spacing + 1) * 2;
    /* not computed yet and may not be computed at all */
    st->buf_size = 0;
    /* used to determine whether we should run set_frame and for unset assertions */
    st->cur_frame = (unsigned)-1;
    /* current element index we're working on, reduces computation for sequential set_frame */
    st->element_index = 0;
    st->next_element_index = 0;
    /* number of frames up to the last + 1 */
    st->max_frames = get_first_frame_for_element(st->first_element_frames_count, (unsigned)st->text.len) + 1;
    st->output_str = NULL;

    tguy_set_frame(st, 0);
    return st;
}

TrashGuyState *tguy_from_arr_ex(const TGStrView arr[],
                                size_t len,
                                unsigned spacing,
                                const TGStrView *sprite_space,
                                const TGStrView *sprite_can,
                                const TGStrView *sprite_right,
                                const TGStrView *sprite_left) {
    return tguy_from_arr_ex_2(arr, len, spacing, sprite_space, sprite_can, sprite_right, sprite_left, 1);
}

TrashGuyState *tguy_from_arr(const TGStrView arr[], size_t len, unsigned spacing) {
    return tguy_from_arr_ex(arr, len, spacing, NULL, NULL, NULL, NULL);
}

#ifdef TGUY_USE_UTF8PROC

static size_t tguy_iterate_graphemes(
    const char *str, size_t *read_bytes, size_t strlen,
    size_t *start, size_t *end
) {
    utf8proc_int32_t break_state = 0, codepoint, prev_codepoint = 0;
    if (*read_bytes == strlen)
        return 0;
    *start = *read_bytes;
    *end = *start;
    while (1) {
        utf8proc_ssize_t n = utf8proc_iterate((unsigned char *)str + *read_bytes, (utf8proc_ssize_t)(strlen - *read_bytes), &codepoint);
        if (*read_bytes == strlen) {
            codepoint = 0;  // Final dummy codepoint
        } else if (codepoint == -1) {
            *read_bytes = (size_t)-1;
            return 0;
        }
        *read_bytes = *read_bytes + (size_t)n;
        if (prev_codepoint != 0 && utf8proc_grapheme_break_stateful(prev_codepoint, codepoint, &break_state)) {
            *read_bytes = *read_bytes - (size_t)n;
            *end = *read_bytes;  // The last byte (not inclusive) of this grapheme
            return 1;
        }
        prev_codepoint = codepoint;
    }
    // Unreachable
}

static size_t tguy_graphemes_len(const char *str, size_t len) {
    size_t read_bytes = 0;
    size_t start, end;
    size_t rlen = 0;
    while (tguy_iterate_graphemes((char *) str, &read_bytes, len, &start, &end)) {
        rlen++;
    }
    return read_bytes != (size_t)-1 ? rlen : (size_t)-1;
}

#elif defined TGUY_USE_WGRAPHEME
static size_t tguy_iterate_graphemes(
    const char *str, size_t *read_bytes, size_t strlen,
    size_t *start, size_t *end
) {
    wgrapheme_status_t status;
    *start = *read_bytes;
    *end = *start;
    status = wgrapheme_next_boundary(str, strlen, *read_bytes, end);
    if (status != WGRAPHEME_OK) {
        if (status != WGRAPHEME_DONE) {
            *read_bytes = (size_t)-1;
        }
        return 0;
    }
    *read_bytes = *end;
    return *end - *start;
}

static size_t tguy_graphemes_len(const char *str, size_t len) {
    size_t count = 0;
    if (wgrapheme_count(str, len, &count) != WGRAPHEME_OK) {
        return -1;
    }
    return count;
}

#else

static char *tguy_utf8_next(const char *begin, const char *end) {
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

    return (char *) begin;
}

static size_t tguy_codepoints_len(const char *string, size_t len) {
    size_t rlen = 0;
    const char *start = string, *end = string + len;
    while ((start = tguy_utf8_next(start, end))) {
        rlen++;
    }
    return rlen;
}
#endif


TrashGuyState *tguy_from_utf8_ex(const char string[], size_t len, unsigned spacing,
                                 const char *sprite_space, size_t sprite_space_len,
                                 const char *sprite_can, size_t sprite_can_len,
                                 const char *sprite_right, size_t sprite_right_len,
                                 const char *sprite_left, size_t sprite_left_len) {
    TrashGuyState *st = NULL;
    TGStrView *strarr = NULL;
    TGStrView sv_sprite_space, sv_sprite_can, sv_sprite_right, sv_sprite_left;
    size_t flen = 0;

    if (string == NULL) len = 0;

    len = (len == (size_t)-1) ? strlen(string) : len;

    if (len > 0) {
#ifndef TGUY_NO_GRAPHEME
        flen = tguy_graphemes_len(string, len);
#else
        flen = tguy_codepoints_len(string, len);
#endif
        if (flen == (size_t)-1 || flen > INT_MAX) return NULL;

        if (flen) {
            strarr = malloc(sizeof(strarr[0]) * flen);
            if (strarr == NULL) return NULL;

            size_t i = 0;
#ifndef  TGUY_NO_GRAPHEME
            /* fill the array with ranges of the string representing whole utf-8 grapheme clusters */
            size_t read_bytes = 0;
            size_t start, end;
            while (tguy_iterate_graphemes(string, &read_bytes, len, &start, &end)) {
                cstr2tgstrv(&strarr[i++], &string[start], end - start);
            }
#else
            const char *start = string, *end = string + len;
            while ((start = tguy_utf8_next(start, end))) {
                cstr2tgstrv(&strarr[i++], string, start - string);
                string = start;
            }
#endif
        }
    }

    st = tguy_from_arr_ex(strarr, flen, spacing,
                          sprite_space ? cstr2tgstrv(&sv_sprite_space, sprite_space, sprite_space_len) : NULL,
                          sprite_can ? cstr2tgstrv(&sv_sprite_can, sprite_can, sprite_can_len) : NULL,
                          sprite_right ? cstr2tgstrv(&sv_sprite_right, sprite_right, sprite_right_len) : NULL,
                          sprite_left ? cstr2tgstrv(&sv_sprite_left, sprite_left, sprite_left_len) : NULL);
    free(strarr);
    return st;
}

TrashGuyState *tguy_from_utf8(const char string[], size_t len, unsigned spacing) {
    return tguy_from_utf8_ex(string, len, spacing,
                             NULL, 0,
                             NULL, 0,
                             NULL, 0,
                             NULL, 0);
}

TrashGuyState *tguy_from_cstr_arr_ex(const char *const arr[], size_t len, unsigned spacing,
                                     const char *sprite_space, size_t sprite_space_len,
                                     const char *sprite_can, size_t sprite_can_len,
                                     const char *sprite_right, size_t sprite_right_len,
                                     const char *sprite_left, size_t sprite_left_len) {
    TrashGuyState *st;
    TGStrView sv_sprite_space, sv_sprite_can, sv_sprite_right, sv_sprite_left;
    TGStrView *svarr = NULL;

    if (arr != NULL && len != 0) {
        svarr = malloc(sizeof(svarr[0]) * len);
        if (svarr == NULL) return NULL;
        /* create array of string views from C array */
        for (size_t i = 0; i < len; i++) {
            cstr2tgstrv(&svarr[i], arr[i], (size_t)-1);
        }
    } else {
        len = 0;
    }

    st = tguy_from_arr_ex(svarr, len, spacing,
                          sprite_space ? cstr2tgstrv(&sv_sprite_space, sprite_space, sprite_space_len) : NULL,
                          sprite_can ? cstr2tgstrv(&sv_sprite_can, sprite_can, sprite_can_len) : NULL,
                          sprite_right ? cstr2tgstrv(&sv_sprite_right, sprite_right, sprite_right_len) : NULL,
                          sprite_left ? cstr2tgstrv(&sv_sprite_left, sprite_left, sprite_left_len) : NULL);
    free(svarr);
    return st;
}

TrashGuyState *tguy_from_cstr_arr(const char *const arr[], size_t len, unsigned spacing) {
    return tguy_from_cstr_arr_ex(arr, len, spacing,
                                 NULL, 0,
                                 NULL, 0,
                                 NULL, 0,
                                 NULL, 0
    );
}

void tguy_free(TrashGuyState *st) {
    if (st == NULL) return;
    free(st->output_str);
    free(st);
}

/**
 * In order to properly set frame we need to know few things beforehand:
 *  -# element_index for TrashGuyState::text[element_index] we're currently working on
 *  -# number of frames per element
 *  -# first frame (boundary) involving the work on element with element_index
 *  -# sub frame (frame index)
 *  -# direction
 *  -# index within the arena
 *
 *  All we know is desired frame and number of frames per first element.
 *  -# Because of the get_first_frame_for_element() we know that initial frame of each element is computed as: \n
 *     <code> frame = element_index * (first_element_frames_count + element_index - 1) </code> \n
 *     which after moving frame to the right,
 *     is equivalent to this <a href='https://en.wikipedia.org/wiki/Quadratic_formula'>quadratic equation</a>: \n
 *     <code> 1*element_index<sup>2</sup> + (first_element_frames_count - 1)*element_index + (-frame) = 0 </code> \n
 *     <code> ax<sup>2</sup> + bx + c = 0 </code> \n
 *
 *     We solve this equation for element_index, which is x<sub>1</sub>. \n
 *     (x<sub>1</sub> means right wing of the parabola,
 *         x<sub>2</sub> (left side) is meaningless to us, valid indices/frames only reside on the right side) \n
 *     <code> x<sub>1</sub> = (-b + sqrt(b<sup>2</sup> - 4ac)) / 2a </code> \n
 *     but because c is always negative and a is always 1, we can rewrite it as: \n
 *     <code> x<sub>1</sub> = (sqrt(b<sup>2</sup> + 4c) - b) / 2 </code>, which is the final formula
 *
 *  -# Simple arithmetic progression, with each next element number of frames increases by 2.
 *
 *  -# We now have all the values needed to call get_first_frame_for_element().
 *
 *  -# <code> boundary <= (boundary + sub_frame) < boundary + frames_per_element </code>, \n
 *     thus <code> sub_frame = frame - boundary </code>
 *
 *  -# we spend the same number of frames moving left/right, if <code> sub_frame < (total / 2) </code> -> right, else -> left
 *
 *  -# index i within the arena is computed as <code> sub_frame % (total / 2) </code>
 */
unsigned tguy_set_frame(TrashGuyState *restrict st, unsigned frame) {
    /*         a                        b                              c       */
    /* (element_index)^2 + (first_element_frames_count - 1)element_index - frame = 0 */
    assert((ignored_"Frame is bigger than get_frames_count()", frame < st->max_frames));
    if (frame >= st->max_frames) return -1u;
    unsigned prev_frame = st->cur_frame;
    unsigned element_index;
    unsigned first_element_frames_count = st->first_element_frames_count;
    if (prev_frame == frame) return frame;

    if (prev_frame == frame - 1) {
        element_index = st->next_element_index;
    } else {
        /* unsigned a = 1; */
        unsigned b = (first_element_frames_count - 1),
                 c = frame,
                 t = (b * b) + (4 * c);
        element_index = ((unsigned)sqrt(t) - b) / 2;
    }

    /* number of frames needed to process element, see 2 */
    unsigned frames_per_element = first_element_frames_count + (2 * element_index);
    /* index of the frame in the frame series (up to frames_per_element) */
    unsigned sub_frame = (frame - get_first_frame_for_element(first_element_frames_count, element_index));
    /* if we're in the first half frames we're moving right, otherwise left */
    unsigned frames_per_direction = (frames_per_element / 2);
    unsigned right = (sub_frame < frames_per_direction);
    /* TrashGuy index yields 0 twice, the difference is whether we're moving right or left */
    unsigned i = (right) ? sub_frame : frames_per_element - sub_frame - 1;

    /* used to make set_frame faster by not setting same frame twice and to assert unset TrashGuyState */
    st->cur_frame = frame;
    st->next_element_index = element_index + (i == 0 && !right);
    st->element_index = element_index;
    st->pos = i + 1;
    st->facing_right = right;

    if (prev_frame != -1u && prev_frame == frame - 1) {
        if (i != 0 && right) {
            st->arena.data[i] = st->sprite_space;
        } else {
            st->arena.data[i + 2] = st->sprite_space;
        }
    } else {
        /* if we're not moving right, then we're not drawing the n-th element because TrashGuy "carries" it */
        tguy_clear_field(st, element_index + !right);
    }
    /* don't overwrite the trash can when placing the trash-guy */
    st->arena.data[i + 1] = (right) ? st->sprite_right : st->sprite_left;
    /* Draw the element TrashGuy carries if we're not right near the trash can */
    if (!right && i != 0) {
        st->arena.data[i] = st->text.data[element_index];
    }
    return frame;
}

unsigned tguy_set_pos(TrashGuyState *st, unsigned sprite_pos, unsigned facing_right, unsigned element_index) {
    /* We can't be in place of trash can sprite, and we can't be in place of last arena tile */
    /* last frame is the final one so pos can't be anything other than 1 facing right */
    if (sprite_pos == 0 || sprite_pos > st->arena.len - 1 || element_index > st->text.len) return -1u;

    if (element_index == st->text.len && (sprite_pos != 1 || !facing_right)) return -1u;

    unsigned frames_per_element = st->first_element_frames_count + (2 * element_index);
    if (sprite_pos > frames_per_element / 2) return -1u;
    unsigned frame = get_first_frame_for_element(st->first_element_frames_count, element_index);

    frame += facing_right ? sprite_pos - 1 : frames_per_element - sprite_pos;
    tguy_set_frame(st, frame);
    return frame;
}

void tguy_get_frame_state(const TrashGuyState *st, unsigned *frame, unsigned *sprite_pos,
                          unsigned *facing_right, unsigned *element_index) {
    if (st == NULL) return;
    if (frame) *frame = st->cur_frame;
    if (sprite_pos) *sprite_pos = st->pos;
    if (facing_right) *facing_right = st->facing_right;
    if (element_index) *element_index = st->element_index;
}

size_t tguy_fprint(const TrashGuyState *st, FILE *fp) {
    assert(st->cur_frame != -1u);
    size_t len = 0;
    for (size_t i = 0, flen = st->arena.len; i < flen; i++) {
        TGStrView sv = st->arena.data[i];
        len += fwrite(sv.str, 1, sv.len, fp);
    }
    return len;
}

size_t tguy_print(const TrashGuyState *st) { return tguy_fprint(st, stdout); }

size_t tguy_sprint(const TrashGuyState *st, char *buf) {
    assert(st->cur_frame != (unsigned) -1);
    char *start = buf;
    for (size_t i = 0, flen = st->arena.len; i < flen; i++) {
        TGStrView sv = st->arena.data[i];
        for (size_t j = 0, slen = sv.len; j < slen; j++) { *buf++ = sv.str[j]; }
    }
    *buf = '\0';
    return (size_t)(buf - start);
}

const TGStrView *tguy_get_arr(const TrashGuyState *st, size_t *len) {
    assert(st->cur_frame != (unsigned) -1);
    if (len != NULL) *len = st->arena.len;
    return st->arena.data;
}

unsigned tguy_get_frames_count(const TrashGuyState *st) { return st->max_frames; }

#define tg_max(a, b) ((a) > (b) ? (a) : (b))

/**
 * If bsize is not set, then iterate over possible arena layout and
 * compute buf size big enough to keep any frame plus nul terminator
 */
size_t tguy_get_bsize(TrashGuyState *st) {
    /* for nul terminator */
    size_t sz = 1;
    if (st->buf_size) return st->buf_size;
    /* overall text length */
    for (size_t i = 0, tlen = st->text.len; i < tlen; i++) {
        /* element will be replaced with space (filler sprite) eventually
         * by choosing the largest ensure the buffer is big enough */
        sz += tg_max(st->text.data[i].len, st->sprite_space.len);
    }
    /* overall free space length */
    sz += st->sprite_space.len * ((st->first_element_frames_count / 2) - 1);
    sz += st->sprite_can.len;
    sz += tg_max(st->sprite_right.len, st->sprite_left.len);

    st->buf_size = sz;
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

unsigned tguy_get_first_frame_for_element(const TrashGuyState *st, unsigned element_index) {
    return get_first_frame_for_element(st->first_element_frames_count, element_index);
}

unsigned tguy_get_version(void) {
    return 1000000 * TGUY_VER_MAJOR + 1000 * TGUY_VER_MINOR + TGUY_VER_PATCH;
}
