#if !defined(STIPULATION_BATTLE_PLAY_BRANCH_H)
#define STIPULATION_BATTLE_PLAY_BRANCH_H

#include "pystip.h"

/* This module provides functionality dealing with battle play
 * branches
 */

/* Allocate a branch consisting mainly of an attack move
 * @param  length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of entry slice to allocated branch
 */
slice_index alloc_attack_branch(stip_length_type length,
                                stip_length_type min_length);

/* Allocate a branch consisting mainly of an defense move
 * @param  length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of entry slice to allocated branch
 */
slice_index alloc_defense_branch(stip_length_type length,
                                 stip_length_type min_length);

/* Allocate a branch that represents battle play
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of entry slice of allocated branch
 */
slice_index alloc_battle_branch(stip_length_type length,
                                stip_length_type min_length);

/* Shorten a battle slice by 2 half moves
 * @param si identifies slice to be shortened
 */
void battle_branch_shorten_slice(slice_index si);

#endif
