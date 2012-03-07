#if !defined(SOLVING_MAXIMUMMER_CANDIDATE_MOVE_GENERATOR_H)
#define SOLVING_MAXIMUMMER_CANDIDATE_MOVE_GENERATOR_H

#include "stipulation/help_play/play.h"

/* This module provides functionality dealing with the attacking side
 * in STSingleMoveGenerator stipulation slices.
 */

/* Allocate a STSingleMoveGenerator slice.
 * @return index of allocated slice
 */
slice_index alloc_single_move_generator_slice(void);

/* Initialise the next move generation
 * @param sq_departure departure square of move to be generated
 * @param sq_arrival arrival square of move to be generated
 * @param sq_capture capture square of move to be generated
 * @param sq_mren Mars Circe rebirth square
 */
void init_single_move_generator(square sq_departure,
                                square sq_arrival,
                                square sq_capture,
                                square sq_mren);

/* Solve in a number of half-moves
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move leading to the current position has
 *                           turned out to be illegal
 *            n   solution found
 *            n+2 no solution found
 */
stip_length_type single_move_generator_help(slice_index si, stip_length_type n);

#endif
