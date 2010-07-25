#if !defined(STIPULATION_BATTLE_PLAY_DEFENSE_PLAY_H)
#define STIPULATION_BATTLE_PLAY_DEFENSE_PLAY_H

/* Interface for dynamically dispatching defense operations to slices
 * depending on the slice type
 */

#include "pyslice.h"

/* Try to defend after an attacking move
 * When invoked with some n, the function assumes that the key doesn't
 * solve in less than n half moves.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_max_unsolvable maximum number of half-moves that we
 *                         know have no solution
 * @return <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - acceptable number of refutations found
 *         n+4 refuted - more refutations found than acceptable
 */
stip_length_type defense_defend_in_n(slice_index si,
                                     stip_length_type n,
                                     stip_length_type n_max_unsolvable);

/* Determine whether there are defenses after an attacking move
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_max_unsolvable maximum number of half-moves that we
 *                         know have no solution
 * @param max_nr_refutations how many refutations should we look for
 * @return <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - <=max_nr_refutations refutations found
 *         n+4 refuted - >max_nr_refutations refutations found
 */
stip_length_type defense_can_defend_in_n(slice_index si,
                                         stip_length_type n,
                                         stip_length_type n_max_unsolvable,
                                         unsigned int max_nr_refutations);

/* Try to defend after an attacking move
 * @param si slice index
 * @return <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 no solution found
 */
boolean defense_defend(slice_index si);

/* Determine whether there are defenses after an attacking move
 * @param si slice index
 * @return true iff there is >=1 refutation
 */
boolean defense_can_defend(slice_index si);

#endif
