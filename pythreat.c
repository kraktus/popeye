#include "pythreat.h"
#include "pydata.h"
#include "pypipe.h"
#include "stipulation/testing_pipe.h"
#include "stipulation/branch.h"
#include "stipulation/dummy_move.h"
#include "stipulation/battle_play/branch.h"
#include "stipulation/battle_play/attack_play.h"
#include "solving/solving.h"
#include "solving/battle_play/check_detector.h"
#include "trace.h"

#include <assert.h>
#include <stdlib.h>

static stip_length_type max_len_threat;

/* Reset the max threats setting to off
 */
void reset_max_threat_length(void)
{
  max_len_threat = no_stip_length;
}

/* Read the requested max threat length setting from a text token
 * entered by the user
 * @param textToken text token from which to read
 * @return true iff max threat setting was successfully read
 */
boolean read_max_threat_length(const char *textToken)
{
  boolean result;
  char *end;
  unsigned long const requested_max_threat_length = strtoul(textToken,&end,10);

  if (textToken!=end && requested_max_threat_length<=UINT_MAX)
  {
    max_len_threat = (stip_length_type)requested_max_threat_length;
    result = true;
  }
  else
    result = false;

  return result;
}

/* Retrieve the current max threat length setting
 * @return current max threat length setting
 *         no_stip_length if max threats option is not active
 */
stip_length_type get_max_threat_length(void)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",max_len_threat);
  TraceFunctionResultEnd();
  return max_len_threat;
}

/* **************** Private helpers ***************
 */

/* Determine whether the threat after the attacker's move just played
 * is too long respective to user input.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @return true iff threat is too long
 */
static boolean is_threat_too_long(slice_index si,
                                  stip_length_type n,
                                  stip_length_type n_max)
{
  boolean result;
  slice_index const tester = slices[si].u.fork.fork;
  stip_length_type const save_max_unsolvable = max_unsolvable;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  max_unsolvable = slack_length_battle-1;
  result = n_max<can_defend(tester,n_max);
  max_unsolvable = save_max_unsolvable;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* **************** Initialisation ***************
 */

/* Allocate a STMaxThreatLength slice
 */
static slice_index alloc_maxthreatlength_guard(slice_index threat_start)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",threat_start);
  TraceFunctionParamListEnd();

  result = alloc_testing_pipe(STMaxThreatLength);
  slices[result].u.fork.fork = threat_start;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there are defenses after an attacking move
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @return <slack_length_battle - no legal defense found
 *         <=n solved  - <=acceptable number of refutations found
 *                       return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - >acceptable number of refutations found
 */
stip_length_type maxthreatlength_guard_can_defend(slice_index si,
                                                  stip_length_type n)
{
  slice_index const next = slices[si].u.fork.next;
  unsigned int result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  /* determining whether the attack move has delivered check is
     expensive, so let's try to avoid it */
  if (max_len_threat==0)
  {
    if (echecc(nbply,slices[si].starter))
      result = can_defend(next,n);
    else
      result = n+2;
  }
  else
  {
    stip_length_type const n_max = 2*(max_len_threat-1)+slack_length_battle+2;
    if (n>=n_max)
    {
      if (echecc(nbply,slices[si].starter))
        result = can_defend(next,n);
      else if (is_threat_too_long(si,n,n_max))
        result = n+2;
      else
        result = can_defend(next,n);
    }
    else
      /* remainder of play is too short for max_len_threat to apply */
      result = can_defend(next,n);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}


/* **************** Stipulation instrumentation ***************
 */

typedef struct
{
    boolean inserted;
    boolean testing;
} insertion_state;

static void maxthreatlength_guard_inserter(slice_index si,
                                           stip_structure_traversal *st)
{
  insertion_state * const state = st->param;
  stip_length_type const length = slices[si].u.branch.length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (state->testing
      && (max_len_threat==0
          || length>=2*(max_len_threat-1)+slack_length_battle+2))
  {
    slice_index const checked_prototype = alloc_pipe(STMaxThreatLengthStart);
    battle_branch_insert_slices(si,&checked_prototype,1);

    {
      slice_index const threat_start = branch_find_slice(STMaxThreatLengthStart,si);
      slice_index const dummy = alloc_dummy_move_slice();
      slice_index const prototype = alloc_maxthreatlength_guard(dummy);
      assert(threat_start!=no_slice);
      link_to_branch(dummy,threat_start);
      battle_branch_insert_slices(si,&prototype,1);
    }

    state->inserted = true;
  }

  stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void testing_only(slice_index si, stip_structure_traversal *st)
{
  insertion_state * const state = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (state->testing)
    stip_traverse_structure_children(si,st);
  else
  {
    state->testing = true;
    stip_traverse_structure(slices[si].u.fork.fork,st);
    state->testing = false;

    stip_traverse_structure_pipe(si,st);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitors maxthreatlength_guards_inserters[] =
{
  { STDefenseAdapter,  &maxthreatlength_guard_inserter },
  { STReadyForDefense, &maxthreatlength_guard_inserter }
};

enum
{
  nr_maxthreatlength_guards_inserters =
  (sizeof maxthreatlength_guards_inserters
   / sizeof maxthreatlength_guards_inserters[0])
};

/* Instrument stipulation with STMaxThreatLength slices
 * @param si identifies slice where to start
 */
boolean stip_insert_maxthreatlength_guards(slice_index si)
{
  insertion_state state = { false, false };
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceStipulation(si);

  stip_structure_traversal_init(&st,&state);
  stip_structure_traversal_override_by_function(&st,
                                                slice_function_conditional_pipe,
                                                &testing_only);
  stip_structure_traversal_override_by_function(&st,
                                                slice_function_testing_pipe,
                                                &testing_only);
  stip_structure_traversal_override(&st,
                                    maxthreatlength_guards_inserters,
                                    nr_maxthreatlength_guards_inserters);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",state.inserted);
  TraceFunctionResultEnd();
  return state.inserted;
}
