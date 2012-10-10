#if !defined(CONDITIONS_TIBET_H)
#define CONDITIONS_TIBET_H

/* This module implements royal squares */

#include "solving/solve.h"

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type double_tibet_solve(slice_index si, stip_length_type n);

/* Instrument a stipulation
 * @param si identifies root slice of stipulation
 */
void stip_insert_double_tibet(slice_index si);

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type tibet_solve(slice_index si, stip_length_type n);

/* Instrument a stipulation
 * @param si identifies root slice of stipulation
 */
void stip_insert_tibet(slice_index si);

#endif