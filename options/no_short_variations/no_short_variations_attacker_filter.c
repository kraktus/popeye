#include "options/no_short_variations/no_short_variations_attacker_filter.h"
#include "pydata.h"
#include "pyproc.h"
#include "pyoutput.h"
#include "pypipe.h"
#include "stipulation/branch.h"
#include "stipulation/battle_play/branch.h"
#include "stipulation/battle_play/attack_play.h"
#include "trace.h"

#include <assert.h>

/* Allocate a STNoShortVariations slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of allocated slice
 */
slice_index alloc_no_short_variations_slice(stip_length_type length,
                                            stip_length_type min_length)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  result = alloc_branch(STNoShortVariations,length,min_length);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a short solution after the defense played
 * in a slice
 * @param si identifies slice that just played the defense
 * @param n maximum number of half moves until end of branch
 */
static boolean has_short_solution(slice_index si, stip_length_type n)
{
  boolean result;
  stip_length_type const n_min = battle_branch_calc_n_min(si,n);

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  result = attack_has_solution_in_n(slices[si].u.pipe.next,n,n_min)<=n;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the defense just played defends against the threats.
 * @param threats table containing the threats
 * @param len_threat length of threat(s) in table threats
 * @param si slice index
 * @param n maximum number of moves until goal (after the defense)
 * @return true iff the defense defends against at least one of the
 *         threats
 */
boolean no_short_variations_are_threats_refuted_in_n(table threats,
                                                     stip_length_type len_threat,
                                                     slice_index si,
                                                     stip_length_type n)
{
  slice_index const next = slices[si].u.pipe.next;
  boolean result = true;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",threats);
  TraceFunctionParam("%u",table_length(threats));
  TraceFunctionParam("%u",len_threat);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  if (n>slack_length_battle+1 && has_short_solution(si,n-2))
    result = false;
  else
    result = attack_are_threats_refuted_in_n(threats,len_threat,next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether attacker can end in n half moves.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_min minimal number of half moves to try
 * @return length of solution found, i.e.:
 *            n_min-4 defense put defender into self-check,
 *                    or some similar dead end
 *            n_min-2 defense has solved
 *            n_min..n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type no_short_variations_has_solution_in_n(slice_index si,
                                                       stip_length_type n,
                                                       stip_length_type n_min)
{
  stip_length_type result;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  result = attack_has_solution_in_n(next,n,n_min);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine and write the threats after the move that has just been
 * played.
 * @param threats table where to add threats
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_min minimal number of half moves to try
 * @return length of threats
 *         (n-slack_length_battle)%2 if the attacker has something
 *           stronger than threats (i.e. has delivered check)
 *         n+2 if there is no threat
 */
stip_length_type no_short_variations_solve_threats_in_n(table threats,
                                                        slice_index si,
                                                        stip_length_type n,
                                                        stip_length_type n_min)
{
  stip_length_type result;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",threats);
  TraceFunctionParam("%u",table_length(threats));
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  result = attack_solve_threats_in_n(threats,next,n,n_min);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve a slice
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_min minimal number of half moves to try
 * @return number of half moves effectively used
 *         n+2 if no solution was found
 *         (n-slack_length_battle)%2 if the previous move led to a
 *            dead end (e.g. self-check)
 */
stip_length_type no_short_variations_solve_in_n(slice_index si,
                                                stip_length_type n,
                                                stip_length_type n_min)
{
  stip_length_type result;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  if (n>slack_length_battle+1 && has_short_solution(si,n-2))
    result = false;
  else
    result = attack_solve_in_n(next,n,n_min);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}
