#include "stipulation/help_play/move.h"
#include "pydata.h"
#include "pyoutput.h"
#include "pyslice.h"
#include "pybrafrk.h"
#include "pypipe.h"
#include "pyleaf.h"
#include "stipulation/branch.h"
#include "stipulation/proxy.h"
#include "stipulation/help_play/play.h"
#include "stipulation/help_play/root.h"
#include "stipulation/help_play/shortcut.h"
#include "trace.h"

#include <assert.h>

/* Allocate a STHelpMove slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @return index of allocated slice
 */
slice_index alloc_help_move_slice(stip_length_type length,
                                  stip_length_type min_length)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParamListEnd();

  result = alloc_branch(STHelpMove,length,min_length);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Insert root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void help_move_insert_root(slice_index si, stip_structure_traversal *st)
{
  slice_index * const root = st->param;
  slice_index const prev = slices[si].prev;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  *root = alloc_help_root_slice(length,min_length);

  if (slices[next].prev==si)
  {
    /* si is part of a loop sping off a root branch */
    slice_index const shortcut = alloc_help_shortcut(length,min_length,prev);
    slice_index const root_branch = copy_slice(si);
    pipe_link(shortcut,root_branch);
    pipe_link(*root,shortcut);
  }
  else
  {
    /* si is not part of a loop - reuse it in the root branch */
    pipe_unlink(prev);
    pipe_link(*root,si);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Detect starter field with the starting side if possible.
 * @param si identifies slice being traversed
 * @param st status of traversal
 */
void help_move_detect_starter(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (slices[si].starter==no_side)
  {
    slice_index const next = slices[si].u.pipe.next;
    stip_traverse_structure_children(si,st);
    slices[si].starter = (slices[next].starter==no_side
                          ? no_side
                          : advers(slices[next].starter));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Try solving with all generated moves
 * @param si slice index
 * @param n exact number of moves to reach the end state
 * @return true iff solved
 */
static boolean foreach_move_solve(slice_index si, stip_length_type n)
{
  boolean result = false;
  slice_index const next = slices[si].u.pipe.next;

  while (encore())
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && help_solve_in_n(next,n-1))
      result = true;

    repcoup();
  }

  return result;
}

/* Determine and write the solution(s) in a help stipulation
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean help_move_solve_in_n(slice_index si, stip_length_type n)

{
  boolean result = false;
  Side const side_at_move = slices[si].starter;
  Goal const goal = slices[si].u.branch.imminent_goal;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  active_slice[nbply+1] = si;

  if (n==slack_length_help+1 && goal!=no_goal)
  {
    if (are_prerequisites_for_reaching_goal_met(goal,side_at_move))
    {
      empile_for_goal = goal;
      empile_for_target = slices[si].u.branch.imminent_target;
      generate_move_reaching_goal(side_at_move);
      empile_for_goal = no_goal;
      result = foreach_move_solve(si,n);
      finply();
    }
  }
  else
  {
    move_generation_mode= move_generation_not_optimized;
    TraceValue("->%u\n",move_generation_mode);
    genmove(side_at_move);
    result = foreach_move_solve(si,n);
    finply();
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine and write threats
 * @param threats table where to add first moves
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 */
void help_move_solve_threats_in_n(table threats,
                                  slice_index si,
                                  stip_length_type n)
{
  Side const side_at_move = slices[si].starter;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_help);

  move_generation_mode= move_generation_not_optimized;
  TraceValue("->%u\n",move_generation_mode);
  active_slice[nbply+1] = si;
  genmove(side_at_move);
  foreach_move_solve(si,n);
  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Determine whether the defense just played defends against the threats.
 * @param threats table containing the threats
 * @param si slice index
 * @return true iff the defense defends against at least one of the
 *         threats
 */
boolean help_move_are_threats_refuted(table threats, slice_index si)
{
  boolean result = true;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",table_length(threats));
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (table_length(threats)>0)
  {
    unsigned int nr_successful_threats = 0;
    boolean defense_found = false;
    Side const attacker = slices[si].starter;
    slice_index const next = slices[si].u.pipe.next;
    stip_length_type const length = slices[si].u.branch.length;

    move_generation_mode= move_generation_not_optimized;
    TraceValue("->%u\n",move_generation_mode);
    active_slice[nbply+1] = si;
    genmove(attacker);

    while (encore() && !defense_found)
    {
      if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
          && is_current_move_in_table(threats)
          && help_has_solution_in_n(next,length-1))
        ++nr_successful_threats;

      repcoup();
    }

    finply();

    /* this happens if >=1 threat no longer works or some threats can
     * no longer be played after the defense.
     */
    result = nr_successful_threats<table_length(threats);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Iterate moves until a solution has been found
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 * @return true iff a solution has been found
 */
static boolean find_solution(slice_index si, stip_length_type n)
{
  slice_index const next = slices[si].u.pipe.next;
  boolean result = false;
  
  while (!result && encore())
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && help_has_solution_in_n(next,n-1))
      result = true;

    repcoup();
  }

  return result;
}

/* Determine whether the slice has a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean help_move_has_solution_in_n(slice_index si, stip_length_type n)
{
  Side const side_at_move = slices[si].starter;
  boolean result = false;
  Goal const goal = slices[si].u.branch.imminent_goal;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  active_slice[nbply+1] = si;

  if (n==slack_length_help+1 && goal!=no_goal)
  {
    if (are_prerequisites_for_reaching_goal_met(goal,side_at_move))
    {
      empile_for_goal = goal;
      empile_for_target = slices[si].u.branch.imminent_target;
      generate_move_reaching_goal(side_at_move);
      empile_for_goal = no_goal;
      result = find_solution(si,n);
      finply();
    }
  }
  else
  {
    move_generation_mode= move_generation_not_optimized;
    TraceValue("->%u\n",move_generation_mode);
    genmove(side_at_move);
    result = find_solution(si,n);
    finply();
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Spin off a set play slice at root level
 * @param si slice index
 * @param st state of traversal
 */
void help_move_make_setplay_slice(slice_index si, stip_structure_traversal *st)
{
  setplay_slice_production * const prod = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  prod->setplay_slice = slices[si].u.pipe.next;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
