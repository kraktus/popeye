#if !defined(OUTPUT_PLAINTEXT_TREE_VARIATION_WRITER_H)
#define OUTPUT_PLAINTEXT_TREE_VARIATION_WRITER_H

#include "boolean.h"
#include "pystip.h"
#include "pyslice.h"

/* Used by STContinuationWriter and STBattlePlaySolutionWriter to
 * inform STVariationWriter about the maximum length of variations
 * after the attack just played. STVariationWriter uses this
 * information to suppress the output of variations that are deemed
 * too short to be interesting.
 */
extern stip_length_type max_variation_length[maxply+1];

/* Allocate a STVariationWriter slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of allocated slice
 */
slice_index alloc_variation_writer_slice(stip_length_type length,
                                         stip_length_type min_length);

/* Determine whether there is a solution in n half moves, by trying
 * n_min, n_min+2 ... n half-moves.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_min minimal number of half moves to try
 * @param n_max_unsolvable maximum number of half-moves that we
 *                         know have no solution
 * @return length of solution found, i.e.:
 *            slack_length_battle-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type
variation_writer_has_solution_in_n(slice_index si,
                                   stip_length_type n,
                                   stip_length_type n_min,
                                   stip_length_type n_max_unsolvable);

/* Solve a slice, by trying n_min, n_min+2 ... n half-moves.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_max_unsolvable maximum number of half-moves that we
 *                         know have no solution
 * @return length of solution found and written, i.e.:
 *            slack_length_battle-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type
variation_writer_solve_in_n(slice_index si,
                            stip_length_type n,
                            stip_length_type n_max_unsolvable);

#endif
