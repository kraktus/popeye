#if !defined(PYSELFCG_H)
#define PYSELFCG_H

/* STSelfCheckGuard slice - stops solutions with moves that expose the
 * own king
 */

#include "pyhelp.h"
#include "pyslice.h"

/* Instrument stipulation with STKeepMatingGuard slices
 */
void stip_insert_selfcheck_guards(void);

/* Solve in a number of half-moves
 * @param si identifies slice
 * @param n number of half moves until end state has to be reached
 * @return true iff >=1 solution was found
 */
boolean selfcheck_guard_solve_in_n(slice_index si, stip_length_type n);

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean selfcheck_guard_has_solution_in_n(slice_index si, stip_length_type n);

/* Determine and write solution(s): add first moves to table (as
 * threats for the parent slice. First consult hash table.
 * @param continuations table where to add first moves
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 */
void selfcheck_guard_solve_continuations_in_n(table continuations,
                                              slice_index si,
                                              stip_length_type n);

/* Is there no chance left for reaching the solution?
 * E.g. did the help side just allow a mate in 1 in a hr#N?
 * Tests may rely on the current position being hash-encoded.
 * @param si slice index
 * @param just_moved side that has just moved
 * @return true iff no chance is left
 */
boolean selfcheck_guard_must_starter_resign_hashed(slice_index si);

/* Impose the starting side on a stipulation
 * @param si identifies branch
 * @param st address of structure that holds the state of the traversal
 * @return true iff the operation is successful in the subtree of
 *         which si is the root
 */
boolean selfcheck_guard_impose_starter(slice_index si, slice_traversal *st);

#endif
