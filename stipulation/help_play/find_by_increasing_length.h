#if !defined(STIPULATION_HELP_PLAY_ROOT_H)
#define STIPULATION_HELP_PLAY_ROOT_H

#include "boolean.h"
#include "pystip.h"
#include "pyslice.h"

/* This module provides functionality dealing with STHelpMove
 * stipulation slices.
 */

/* Allocate a STHelpRoot slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of allocated slice
 */
slice_index alloc_help_root_slice(stip_length_type length,
                                  stip_length_type min_length);

/* Shorten a help branch by a half-move. If the branch represents a
 * half-move only, deallocates the branch.
 * @param si identifies the branch
 * @return if the branch slice represents a half-move only, the slice
 *         representing the subsequent play; otherwise si
 */
slice_index help_root_shorten_help_play(slice_index si);

/* Solve a branch slice at root level.
 * @param si slice index
 * @return no_slice if set play not applicable
 *         new root slice index (may be equal to old one) otherwise
 * @return true iff >=1 solution was found
 */
boolean help_root_root_solve(slice_index si);

/* Determine whether a slice has a solution
 * @param si slice index
 * @return whether there is a solution and (to some extent) why not
 */
has_solution_type help_root_has_solution(slice_index si);

#endif
