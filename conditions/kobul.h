#if !defined(CONDITION_KOBUL_KINGS_H)
#define CONDITION_KOBUL_KINGS_H

#include "solving/solve.h"

/* This module implements Kobul Kings.
 */

extern boolean kobulking[nr_sides];

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type kobul_king_substitutor_solve(slice_index si,
                                               stip_length_type n);

/* Instrument a stipulation
 * @param si identifies root slice of stipulation
 */
void stip_insert_kobul_king_substitutors(slice_index si);

#endif