#ifndef LIBTGUY_H
#define LIBTGUY_H

/**
 * @file libtguy.h
 */

#include <stddef.h>
#include <stdio.h>

/** @defgroup VERSION Version macros
 * Printable format: "%d.%d.%d", MAJOR, MINOR, PATCH
 *@{*/

/**MAJOR*/
#define TGUY_VER_MAJOR 0
/**MINOR*/
#define TGUY_VER_MINOR 18
/**PATCH*/
#define TGUY_VER_PATCH 1

/**@}*/

#ifndef LIBTGUY_SHARED_DEFINE
    #ifndef LIBTGUY_EXPORT
        #define LIBTGUY_EXPORT
    #endif
#else
    #ifndef LIBTGUY_EXPORT
        #if defined _WIN32 || defined __CYGWIN__
            #ifdef LIBTGUY_EXPORTS
                #define LIBTGUY_EXPORT __declspec(dllexport)
            #else
                #define LIBTGUY_EXPORT __declspec(dllimport)
            #endif
        #elif __GNUC__ >= 4
            #define LIBTGUY_EXPORT __attribute__((visibility("default")))
        #else
            #define LIBTGUY_EXPORT
        #endif
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @struct TGStrView
 *  String container
 */
typedef struct {
    const char *str; /**< Pointer to a non-nul-terminated string              */
    size_t len;      /**< Number of bytes string has excluding nul terminator */
} TGStrView;

/** @def TGSTRV(str)
 *  Creates TGStrView from compile-time string
 * @param  str constant string array or literal
 */
#define TGSTRV(str) ((TGStrView){str, sizeof(str) - 1})

/** @typedef TrashGuyState
 *  Anonymous struct typedef containing TrashGuy information
 */
typedef struct TrashGuyState TrashGuyState;

/**
 *  Creates new TrashGuysState from array of TGStrView. If pointer to sprite is NULL then function will use default one
 * @param arr          Array of string containers, each one is a separate element for TrashGuy to dump to the bin
 * @param len          Number of string containers
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @param sprite_space Sprite to be used as empty space
 * @param sprite_can   Sprite to be used as trash can
 * @param sprite_right Sprite to be used when TrashGuy moves right
 * @param sprite_left  Sprite to be used when TrashGuy moves left
 * @param preserve_strings If set to false function won't make a copy of all strings in passed TGStrView
 *  and will instead rely on caller to preserve those strings until tguy_free is called
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr_ex_2(const TGStrView *arr, size_t len, unsigned spacing,
    const TGStrView *sprite_space, const TGStrView *sprite_can, const TGStrView *sprite_right, const TGStrView *sprite_left,
    int preserve_strings);

/**
 *  Creates new TrashGuysState from array of TGStrView. If pointer to sprite is NULL then function will use default one
 * @param arr          Array of string containers, each one is a separate element for TrashGuy to dump to the bin
 * @param len          Number of string containers
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @param sprite_space Sprite to be used as empty space
 * @param sprite_can   Sprite to be used as trash can
 * @param sprite_right Sprite to be used when TrashGuy moves right
 * @param sprite_left  Sprite to be used when TrashGuy moves left
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr_ex(const TGStrView *arr, size_t len, unsigned spacing,
    const TGStrView *sprite_space, const TGStrView *sprite_can, const TGStrView *sprite_right, const TGStrView *sprite_left);

/**
 *  Creates new TrashGuysState from array of TGStrView using default sprites
 * @param arr          Array of string containers, each one is a separate element for TrashGuy to dump to the bin
 * @param len          Number of string containers
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr(const TGStrView *arr, size_t len, unsigned spacing);

/**
 *  Creates new TrashGuysState from a utf-8 string, where each grapheme cluster is made into an element to dump
 * @param string            Valid utf-8 string, in case of NULL, acts like empty string and len is set as 0
 * @param len               Number of bytes string has, if -1, then strlen will be used
 * @param spacing           Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @param sprite_space      Sprite to be used as empty space
 * @param sprite_space_len  Number of bytes for sprite_space
 * @param sprite_can        Sprite to be used as trash can
 * @param sprite_can_len    Number of bytes for sprite_can
 * @param sprite_right      Sprite to be used when TrashGuy moves right
 * @param sprite_right_len  Number of bytes for sprite_right
 * @param sprite_left       Sprite to be used when TrashGuy moves left
 * @param sprite_left_len   Number of bytes for sprite_left
 * @return TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8_ex(const char *string, size_t len, unsigned spacing,
    const char *sprite_space, size_t sprite_space_len,
    const char *sprite_can, size_t sprite_can_len,
    const char *sprite_right, size_t sprite_right_len,
    const char *sprite_left, size_t sprite_left_len);

/**
 *  Creates new TrashGuysState from valid utf-8 string, where each grapheme cluster is made into an element to dump
 * @param string       Valid utf-8 string, in case of NULL, acts like empty string and len is set as 0
 * @param len          Number of bytes string has, if -1, then strlen will be used
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8(const char string[], size_t len, unsigned spacing);

/**
 * @param arr               Array of nul-terminated C strings, in case of NULL, acts like empty array and len is set as 0
 * @param len               Number of elements in array
 * @param spacing           Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @param sprite_space      Sprite to be used as empty space
 * @param sprite_space_len  Number of bytes for sprite_space
 * @param sprite_can        Sprite to be used as trash can
 * @param sprite_can_len    Number of bytes for sprite_can
 * @param sprite_right      Sprite to be used when TrashGuy moves right
 * @param sprite_right_len  Number of bytes for sprite_right
 * @param sprite_left       Sprite to be used when TrashGuy moves left
 * @param sprite_left_len   Number of bytes for sprite_left
 * @return
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_cstr_arr_ex(const char *const arr[], size_t len, unsigned spacing,
    const char *sprite_space, size_t sprite_space_len,
    const char *sprite_can, size_t sprite_can_len,
    const char *sprite_right, size_t sprite_right_len,
    const char *sprite_left, size_t sprite_left_len);


/**
 * @param arr               Array of nul-terminated C strings, in case of NULL, acts like empty array and len is set as 0
 * @param len               Number of elements in array
 * @param spacing           Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_cstr_arr(const char *const arr[], size_t len, unsigned spacing);

/**
 *  Deallocates memory used by a TrashGuyState, does nothing if pointer is NULL
 * @param st           Valid TrashGuyState * or NULL
 */
LIBTGUY_EXPORT void tguy_free(TrashGuyState *st);

/**
 *  Sets the current frame for TrashGuyState
 * @param st           Valid TrashGuyState *
 * @param frame        0 <= frame < tguy_get_frames_count()
 * @return             frame on success, -1 (UINT_MAX) on failure
 */
LIBTGUY_EXPORT unsigned tguy_set_frame(TrashGuyState *st, unsigned frame);

/**
 *  Sets the current frame for TrashGuyState from state components
 *  Position is index into arena received via tguy_get_arr(). \n
 *  Example:
 *  \code{.unparsed}
 *  spacing = 1
 *  text = "test"
 *  tguy_get_frames_count() = 29
 *
 *  sprite_pos = 1, facing_right = 1, element_index = 0
 *  [ 0] "U(> ^_^)> test"
 *  sprite_pos = 2, facing_right = 1, element_index = 0
 *  [ 1] "U (> ^_^)>test"
 *  sprite_pos = 3, facing_right = 1, element_index = 0
 *  [-1] INVALID, can't overlap with element with index 0
 *  sprite_pos = 2, facing_right = 0, element_index = 0
 *  [ 2] "Ut<(^_^ <) est"
 *  sprite_pos = 1, facing_right = 0, element_index = 0
 *  [ 3] "U<(^_^ <)  est"
 *  sprite_pos = 1, facing_right = 1, element_index = 1
 *  [ 4] "U(> ^_^)>  est"
 *  There's no element with index 4, but this special case is allowed to face right with all elements cleared
 *  sprite_pos = 1, facing_right = 1, element_index = 4
 *  [28] "U(> ^_^)>     "
 *  \endcode
 * @param st            Valid TrashGuyState *
 * @param sprite_pos    Position within the arena, 0 < sprite_pos < tguy_get_arr().len
 * @param facing_right  Whether sprite should be facing right, otherwise left carrying the element
 * @param element_index Index of the element that's currently being processed, see tguy_get_first_frame_for_element()
 * @return              frame on success, -1 (UINT_MAX) on failure
 */
LIBTGUY_EXPORT unsigned tguy_set_pos(TrashGuyState *st, unsigned sprite_pos, unsigned facing_right,
    unsigned element_index);


/**
 *
 * @param               st            Valid TrashGuyState *
 * @param[out,optional] frame         Current frame, 0 <= frame < tguy_get_frames_count()
 * @param[out,optional] sprite_pos    Position within the arena, 0 < sprite_pos < tguy_get_arr().len
 * @param[out,optional] facing_right  Whether sprite is facing right
 * @param[out,optional] element_index Index of the element that's currently being processed, see tguy_get_first_frame_for_element()
 */
LIBTGUY_EXPORT void tguy_get_frame_state(const TrashGuyState *st, unsigned *frame, unsigned *sprite_pos,
    unsigned *facing_right, unsigned *element_index);

/**
 *  Returns number of frames particular TrashGuyState has
 * @param st           Valid TrashGuyState
 * @return             Number of frames, >= 1
 */
LIBTGUY_EXPORT unsigned tguy_get_frames_count(const TrashGuyState *st);

/**
 *  Writes currently set TrashGuy frame to fp without newline
 * @param st           Valid TrashGuyState with frame set
 * @param fp           Valid FILE
 * @return             Number of bytes written
 */
LIBTGUY_EXPORT size_t tguy_fprint(const TrashGuyState *st, FILE *fp);

/**
 *  Writes currently set TrashGuy frame to stdout without newline
 * @param st           Valid TrashGuyState with frame set
 * @return             Number of bytes written
 */
LIBTGUY_EXPORT size_t tguy_print(const TrashGuyState *st);

/**
 *  Writes currently set TrashGuy frame to buffer and appends nul terminator
 * @param st           Valid TrashGuyState with frame set
 * @param buf          Buffer at least tguy_get_bsize() bytes large
 * @return             Number of bytes written, excluding the nul terminator
 */
LIBTGUY_EXPORT size_t tguy_sprint(const TrashGuyState *st, char buf[]);

/**
 *  Get buffer size large enough to hold one frame including nul terminator
 * @param st           Valid TrashGuyState
 * @return             Needed buffer size in bytes, including nul terminator
 */
LIBTGUY_EXPORT size_t tguy_get_bsize(TrashGuyState *st);

/**
 *  Returns read-only array view of current TrashGuy frame: TGStrView[]{ {"t",1}, {"e",1}, {"ї",2}, {"s",1}, {"t",1}, {NULL,0} }
 * @param st           Valid TrashGuyState with frame set
 * @param[out,optional] len     Length of the returned array, excluding NULL terminator
 * @return             Array of const TGStrView,  terminated with TGStrView.str == NULL
 */
LIBTGUY_EXPORT const TGStrView *tguy_get_arr(const TrashGuyState *st, size_t *len);

/**
 *  Return read-only pointer to utf-8 encoded null terminated string containing current set frame.
 *  Does NOT need to be freed manually, is freed by tguy_free() later.
 *  Roughly equivalent to buf = malloc(tguy_get_bsize(st)) + tguy_sprint(st, buf)
 * @param st          Valid TrashGuyState with frame set
 * @param[out,optional] len    Length of the returned string in bytes
 * @return
 */
LIBTGUY_EXPORT const char *tguy_get_string(TrashGuyState * restrict st, size_t *len);

/**
 *  Returns first frame for when certain element is being processed.
 *  You can get a range of frames [first,last] for when certain element is processed by calling
 *  first = tguy_get_first_frame_for_element(element_index)
 *  last = tguy_get_first_frame_for_element(element_index + 1) - 1
 * @param st            Valid TrashGuyState
 * @param element_index Element index, in tguy_from_utf8("teїst",-1,4), 't' would be index 0, 'ї' would be index 2, etc.
 * @return
 */
LIBTGUY_EXPORT unsigned tguy_get_first_frame_for_element(const TrashGuyState *st, unsigned element_index);

/**
 *  Get version as integer in format MMMmmmppp. 020107002 -> 20.107.2
 * @return Version number
 */
LIBTGUY_EXPORT unsigned tguy_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBTGUY_H */
