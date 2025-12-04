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
#define TGUY_VER_MINOR 13
/**PATCH*/
#define TGUY_VER_PATCH 0

/**@}*/

#ifdef LIBTGUY_STATIC_DEFINE
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
    TGStrView *sprite_space, TGStrView *sprite_can, TGStrView *sprite_right, TGStrView *sprite_left,
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
    TGStrView *sprite_space, TGStrView *sprite_can, TGStrView *sprite_right, TGStrView *sprite_left);

/**
 *  Creates new TrashGuysState from array of TGStrView using default sprites
 * @param arr          Array of string containers, each one is a separate element for TrashGuy to dump to the bin
 * @param len          Number of string containers
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr(const TGStrView *arr, size_t len, unsigned spacing);

/**
 * todo: docs
 * @param string
 * @param len
 * @param spacing
 * @param sprite_space
 * @param sprite_can
 * @param sprite_right
 * @param sprite_left
 * @return
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8_ex(const char *string, size_t len, unsigned spacing,
    const char *sprite_space, size_t sprite_space_len,
    const char *sprite_can, size_t sprite_can_len,
    const char *sprite_right, size_t sprite_right_len,
    const char *sprite_left, size_t sprite_left_len);

/**
 *  Creates new TrashGuysState from valid utf-8 string
 * @param string       valid utf-8 string, each grapheme cluster is a separate element for TrashGuy to dump to the bin
 * @param len          Number of bytes string has, if -1, then strlen will be used
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8(const char string[], size_t len, unsigned spacing);

/**
 * todo: docs
 * @param arr
 * @param len
 * @param spacing
 * @param sprite_space
 * @param sprite_space_len
 * @param sprite_can
 * @param sprite_can_len
 * @param sprite_right
 * @param sprite_right_len
 * @param sprite_left
 * @param sprite_left_len
 * @return
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_cstr_arr_ex(const char *const arr[], size_t len, unsigned spacing,
    const char *sprite_space, size_t sprite_space_len,
    const char *sprite_can, size_t sprite_can_len,
    const char *sprite_right, size_t sprite_right_len,
    const char *sprite_left, size_t sprite_left_len);


/**
 * todo: docs
 * @param arr
 * @param len
 * @param spacing
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
 */
LIBTGUY_EXPORT void tguy_set_frame(TrashGuyState *st, unsigned frame);

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
 *  Returns read-only array of TrashGuy frame
 * @param st           Valid TrashGuyState with frame set
 * @param[out,optional] len     Length of the returned array, excluding nul terminator
 * @return             Array of const TGStrView, nul terminated with TGStrView.str == NULL
 */
LIBTGUY_EXPORT const TGStrView *tguy_get_arr(const TrashGuyState *st, size_t *len);

/**
 *  Return pointer to utf-8 encoded null terminated string containing current set frame
 * @param st          Valid TrashGuyState with frame set
 * @param[out,optional] len    Length of the returned string in bytes
 * @return
 */
LIBTGUY_EXPORT const char *tguy_get_string(TrashGuyState * restrict st, size_t *len);

#endif /* LIBTGUY_H */
