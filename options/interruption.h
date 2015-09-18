#if !defined(OPTIONS_INTERRUPTION_H)
#define OPTIONS_INTERRUPTION_H

/* This module allows various options to report whether they have caused solving
 * to be interrupted. And to the output modules to report that the written
 * solution may not be complete for that reason.
 * Solving is interrupted per phase - the information that information was
 * interrupted is automatically propagated to the problem.
 */

#include "stipulation/stipulation.h"

/* Reset our state before delegating, then be ready to report our state
 * @param si identifies the STProblemSolvingInterrupted slice
 */
void problem_solving_interrupted_solve(slice_index si);

/* Remember that solving has been interrupted
 * @param si identifies the STProblemSolvingInterrupted slice
 */
void problem_solving_remember_interruption(slice_index si);

/* Report whether solving has been interrupted
 * @param si identifies the STProblemSolvingInterrupted slice
 * @return true iff solving has been interrupted
 */
boolean problem_solving_is_interrupted(slice_index si);

/* Allocate a STPhaseSolvingInterrupted slice
 * @param base base for searching for the STProblemSolvingInterrupted slice
 *             that the result will propagate the information about
 *             interruptions to.
 * @return identiifer of the allocates slice
 */
slice_index alloc_phase_solving_interrupted(slice_index base);

/* Reset our state before delegating, then be ready to report our state
 * @param si identifies the STProblemSolvingInterrupted slice
 */
void phase_solving_interrupted_solve(slice_index si);

/* Remember that solving has been interrupted
 * @param si identifies the STProblemSolvingInterrupted slice
 */
void phase_solving_remember_interruption(slice_index si);

/* Report whether solving has been interrupted
 * @param si identifies the STProblemSolvingInterrupted slice
 * @return true iff solving has been interrupted
 */
boolean phase_solving_is_interrupted(slice_index si);

#endif