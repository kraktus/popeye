#include "stipulation/help_play/branch.h"
#include "pypipe.h"
#include "stipulation/branch.h"
#include "stipulation/help_play/fork.h"
#include "stipulation/help_play/move.h"
#include "stipulation/help_play/move_to_goal.h"
#include "trace.h"

#include <assert.h>

/* Shorten a help slice by 2 half moves
 * @param si identifies slice to be shortened
 */
void help_branch_shorten_slice(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  slices[si].u.branch.length -= 2;
  if (slices[si].u.branch.min_length-slack_length_help>=2)
    slices[si].u.branch.min_length -= 2;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Shorten a help branch from an odd to an even number of half-moves
 * @param si identifies entry slice
 * @return entry slice of shortened branch
 *         no_slice if shortening isn't applicable
 */
static slice_index shorten_odd_to_even(slice_index si)
{
  slice_index result;
  
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    stip_length_type const length = slices[si].u.branch.length;
    slice_index const move = slices[si].u.pipe.next;
    slice_index const played = slices[move].u.pipe.next;
    slice_index const checked = slices[played].u.pipe.next;
    slice_index const dealt = slices[checked].u.pipe.next;
    slice_index const ready = slices[dealt].u.pipe.next;
    slice_index const fork = slices[ready].u.pipe.next;

    assert(slices[si].type==STReadyForHelpMove);
    assert(slices[move].type==STHelpMove);
    assert(slices[played].type==STHelpMovePlayed);
    assert(slices[checked].type==STHelpMoveLegalityChecked);
    assert(slices[dealt].type==STHelpMoveDealtWith);
    assert(slices[ready].type==STReadyForHelpMove);
    assert(slices[fork].type==STHelpFork);
    
    if (length==slack_length_help+1)
    {
      result = slices[fork].u.branch_fork.towards_goal;
      slices[fork].u.branch_fork.towards_goal = no_slice;
      dealloc_slices(si);
    }
    else
    {
      result = fork;

      help_branch_shorten_slice(si);
      help_branch_shorten_slice(move);
      help_branch_shorten_slice(played);
      help_branch_shorten_slice(checked);
      help_branch_shorten_slice(dealt);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Shorten a help branch from an even to an odd number of half-moves
 * @param si identifies entry slice
 * @return entry slice of shortened branch
 *         no_slice if shortening isn't applicable
 */
static slice_index shorten_even_to_odd(slice_index si)
{
  slice_index result;
  
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    slice_index const fork = slices[si].u.pipe.next;
    slice_index const move = slices[fork].u.pipe.next;
    slice_index const played = slices[move].u.pipe.next;
    slice_index const checked = slices[played].u.pipe.next;
    slice_index const dealt = slices[checked].u.pipe.next;
    slice_index const ready = slices[dealt].u.pipe.next;

    assert(slices[si].type==STReadyForHelpMove);
    assert(slices[fork].type==STHelpFork);
    assert(slices[move].type==STHelpMove);
    assert(slices[played].type==STHelpMovePlayed);
    assert(slices[checked].type==STHelpMoveLegalityChecked);
    assert(slices[dealt].type==STHelpMoveDealtWith);
    assert(slices[ready].type==STReadyForHelpMove);
    
    result = ready;

    help_branch_shorten_slice(si);
    help_branch_shorten_slice(fork);
    help_branch_shorten_slice(move);
    help_branch_shorten_slice(played);
    help_branch_shorten_slice(checked);
    help_branch_shorten_slice(dealt);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a help branch with an even number of half moves
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice leading towards goal
 * @return index of initial slice of allocated help branch
 */
static slice_index alloc_help_branch_even(stip_length_type length,
                                          stip_length_type min_length,
                                          slice_index proxy_to_goal)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_goal);
  TraceFunctionParamListEnd();

  {
    slice_index const ready1 = alloc_branch(STReadyForHelpMove,
                                            length,min_length);
    slice_index const fork = alloc_help_fork_slice(length,min_length,
                                                   proxy_to_goal);
    slice_index const move1 = alloc_help_move_slice(length,min_length);
    slice_index const played1 = alloc_branch(STHelpMovePlayed,
                                             length-1,min_length-1);
    slice_index const checked2 = alloc_branch(STHelpMoveLegalityChecked,
                                              length-1,min_length-1);
    slice_index const dealt2 = alloc_branch(STHelpMoveDealtWith,
                                            length-1,min_length-1);
    slice_index const ready2 = alloc_branch(STReadyForHelpMove,
                                            length-1,min_length-1);
    slice_index const move2 = alloc_help_move_slice(length-1,min_length-1);
    slice_index const played2 = alloc_branch(STHelpMovePlayed,
                                             length-2,min_length-2);
    slice_index const checked1 = alloc_branch(STHelpMoveLegalityChecked,
                                              length-2,min_length-2);
    slice_index const dealt1 = alloc_branch(STHelpMoveDealtWith,
                                            length-2,min_length-2);

    pipe_link(ready1,fork);
    pipe_link(fork,move1);
    pipe_link(move1,played1);
    pipe_link(played1,checked2);
    pipe_link(checked2,dealt2);
    pipe_link(dealt2,ready2);
    pipe_link(ready2,move2);
    pipe_link(move2,played2);
    pipe_link(played2,checked1);
    pipe_link(checked1,dealt1);
    pipe_link(dealt1,ready1);

    result = ready1;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a help branch.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_next identifies slice leading towards goal
 * @return index of entry slice into allocated series branch
 */
slice_index alloc_help_branch(stip_length_type length,
                              stip_length_type min_length,
                              slice_index proxy_to_next)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_next);
  TraceFunctionParamListEnd();

  if ((length-slack_length_help)%2==0)
    result = alloc_help_branch_even(length,min_length,proxy_to_next);
  else
  {
    /* this indirect approach avoids some code duplication */
    slice_index const branch = alloc_help_branch_even(length+1,min_length+1,
                                                      proxy_to_next);
    result = shorten_even_to_odd(branch);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Shorten a help branch by 1 half move
 * @param identifies entry slice of branch to be shortened
 * @return entry slice of shortened branch
 *         no_slice if shortening isn't applicable
 */
slice_index help_branch_shorten(slice_index si)
{
  slice_index result;
  
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if ((slices[si].u.branch.length-slack_length_help)%2==0)
    result = shorten_even_to_odd(si);
  else
    result = shorten_odd_to_even(si);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Insert a the appropriate proxy slices before each
 * STGoalReachedTester slice
 * @param si identifies STGoalReachedTester slice
 * @param st address of structure representing the traversal
 */
static void instrument_tester(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    Goal const goal = slices[si].u.goal_reached_tester.goal;
    slice_index const move_to_goal = alloc_help_move_to_goal_slice(goal);
    slice_index const played = alloc_branch(STHelpMovePlayed,
                                            slack_length_help,
                                            slack_length_help);
    slice_index const checked = alloc_branch(STHelpMoveLegalityChecked,
                                             slack_length_help,
                                             slack_length_help);
    slice_index const dealt = alloc_branch(STHelpMoveDealtWith,
                                           slack_length_help,
                                           slack_length_help);
    assert(slices[si].type==STGoalReachedTester);
    pipe_append(slices[si].prev,move_to_goal);
    pipe_append(move_to_goal,played);
    pipe_append(si,checked);
    pipe_append(checked,dealt);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitors help_goal_instrumenters[] =
{
  { STGoalReachedTester, &instrument_tester }
};

enum
{
  nr_help_goal_instrumenters = (sizeof help_goal_instrumenters
                                / sizeof help_goal_instrumenters[0])
};

/* Instrument a branch leading to a goal to be a help goal branch
 * @param si identifies entry slice of branch
 */
void stip_make_help_goal_branch(slice_index si)
{
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,0);
  stip_structure_traversal_override(&st,
                                    help_goal_instrumenters,
                                    nr_help_goal_instrumenters);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
