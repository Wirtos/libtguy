#ifndef LIBTGUY_H
#define LIBTGUY_H

/**
 * @file libtguy.h
 */

#include <stddef.h>
#include <stdio.h>
#include "libtguy_export.h"

/** @defgroup VERSION Version macros
 * Printable format: "%d.%d.%d", MAJOR, MINOR, PATCH
 *@{*/

/**MAJOR*/
#define TGUY_VER_MAJOR 0
/**MINOR*/
#define TGUY_VER_MINOR 7
/**PATCH*/
#define TGUY_VER_PATCH 0

/**@}*/

/** @struct TGStrView
 *  String container
 */
typedef struct {
    const char *str; /**< Pointer to a non-nul-terminated string              */
    size_t len;      /**< Number of bytes string has excluding nul terminator */
} TGStrView;

/** @def TGSTR(str)
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
 * @return             \ref TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr_ex(const TGStrView *arr, size_t len, unsigned spacing,
    TGStrView *sprite_space, TGStrView *sprite_can, TGStrView *sprite_right, TGStrView *sprite_left);

/**
 *  Creates new TrashGuysState from array of TGStrView using default sprites
 * @param arr          Array of string containers, each one is a separate element for TrashGuy to dump to the bin
 * @param len          Number of string containers
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             \ref TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_arr(const TGStrView *arr, size_t len, unsigned spacing);

/**
 *  Creates new TrashGuysState from valid utf-8 string
 * @param string       valid utf-8 string, each grapheme cluster is a separate element for TrashGuy to dump to the bin
 * @param len          Number of bytes string has, if -1, then strlen will be used
 * @param spacing      Number of space sprites to be placed between the TrashGuy sprite and fist element initially
 * @return             \ref TrashGuyState * or NULL on allocation failure, must be freed with tguy_free() after use
 */
LIBTGUY_EXPORT TrashGuyState *tguy_from_utf8(const char *string, size_t len, unsigned spacing);

/**
 *  Deallocates memory used by a \ref TrashGuyState, does nothing if pointer is NULL
 * @param st           Valid \ref TrashGuyState * or NULL
 */
LIBTGUY_EXPORT void tguy_free(TrashGuyState *st);

/**
 *  Sets the current frame for \ref TrashGuyState
 * @param st           Valid \ref TrashGuyState *
 * @param frame        0 <= frame < tguy_get_frames_count()
 */
LIBTGUY_EXPORT void tguy_set_frame(TrashGuyState *st, unsigned frame);

/**
 *  Returns number of frames particular \ref TrashGuyState has
 * @param st           Valid \ref TrashGuyState
 * @return             Number of frames, >= 1
 */
LIBTGUY_EXPORT unsigned tguy_get_frames_count(const TrashGuyState *st);

/**
 *  Get buffer size large enough to hold one frame including nul terminator
 * @param st           Valid \ref TrashGuyState
 * @return             Needed buffer size in bytes
 */
LIBTGUY_EXPORT size_t tguy_get_bsize(TrashGuyState *st);

/**
 *  Writes currently set TrashGuy frame to fp without newline
 * @param st           Valid \ref TrashGuyState with frame set
 * @param fp           Valid FILE
 * @return             Number of characters written
 */
LIBTGUY_EXPORT size_t tguy_fprint(const TrashGuyState *st, FILE *fp);

/**
 *  Writes currently set TrashGuy frame to stdout without newline
 * @param st           Valid \ref TrashGuyState with frame set
 * @return             Number of characters written
 */
LIBTGUY_EXPORT size_t tguy_print(const TrashGuyState *st);

/**
 *  Writes currently set TrashGuy frame to buffer and appends nul terminator
 * @param st           Valid \ref TrashGuyState with frame set
 * @param buf          Buffer at least tguy_get_bsize() bytes large
 * @return             Number of characters written, excluding the nul terminator
 */
LIBTGUY_EXPORT size_t tguy_sprint(const TrashGuyState *st, char *buf);

/**
 *  Returns read-only array of TrashGuy frame
 * @param st           Valid \ref TrashGuyState with frame set
 * @param[out] len     Length of the returned array
 * @return             Array of const TGStrView
 */
LIBTGUY_EXPORT const TGStrView *tguy_get_arr(const TrashGuyState *st, size_t *len);

#endif /* LIBTGUY_H */
