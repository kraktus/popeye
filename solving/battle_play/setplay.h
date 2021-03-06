#if !defined(SOLVING_BATTLE_PLAY_SETPLAY_H)
#define SOLVING_BATTLE_PLAY_SETPLAY_H

#include "stipulation/stipulation.h"

/* This module provides functionality dealing with writing set play.
 */

/* Instrument the stipulation structure with slices solving set play
 * @param root_slice root slice of the stipulation
 */
void solving_insert_setplay_solvers(slice_index si);

#endif
