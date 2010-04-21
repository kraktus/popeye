#include "pykeepmt.h"
#include "pypipe.h"
#include "stipulation/branch.h"
#include "stipulation/battle_play/attack_play.h"
#include "stipulation/help_play/play.h"
#include "stipulation/series_play/play.h"
#include "pyleaf.h"
#include "trace.h"

#include <assert.h>


/* **************** Initialisation ***************
 */

/* Allocate a STKeepMatingGuardRootDefenderFilter slice
 * @param side mating side
 * @return identifier of allocated slice
 */
static slice_index alloc_keepmating_guard_root_defender_filter(Side mating)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,mating,"");
  TraceFunctionParamListEnd();

  result = alloc_pipe(STKeepMatingGuardRootDefenderFilter);
  slices[result].u.keepmating_guard.mating = mating;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a STKeepMatingGuardAttackerFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param mating mating side
 * @return identifier of allocated slice
 */
static
slice_index alloc_keepmating_guard_attacker_filter(stip_length_type length,
                                                   stip_length_type min_length,
                                                   Side mating)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceEnumerator(Side,mating,"");
  TraceFunctionParamListEnd();

  result = alloc_branch(STKeepMatingGuardAttackerFilter,length,min_length);
  slices[result].u.keepmating_guard.mating = mating;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a STKeepMatingGuardDefenderFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param mating mating side
 * @return identifier of allocated slice
 */
static
slice_index alloc_keepmating_guard_defender_filter(stip_length_type length,
                                                   stip_length_type min_length,
                                                   Side mating)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceEnumerator(Side,mating,"");
  TraceFunctionParamListEnd();

  result = alloc_branch(STKeepMatingGuardDefenderFilter,length,min_length);
  slices[result].u.keepmating_guard.mating = mating;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a STKeepMatingGuardHelpFilter slice
 * @param side mating side
 * @return identifier of allocated slice
 */
static
slice_index alloc_keepmating_guard_help_filter(stip_length_type length,
                                               stip_length_type min_length,
                                               Side mating)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,mating,"");
  TraceFunctionParamListEnd();

  result = alloc_branch(STKeepMatingGuardHelpFilter,length,min_length);
  slices[result].u.keepmating_guard.mating = mating;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a STKeepMatingGuardSeriesFilter slice
 * @param side mating side
 * @return identifier of allocated slice
 */
static slice_index alloc_keepmating_guard_series_filter(Side mating)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,mating,"");
  TraceFunctionParamListEnd();

  result = alloc_pipe(STKeepMatingGuardSeriesFilter);
  slices[result].u.keepmating_guard.mating = mating;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* **************** Implementation of interface Direct ***************
 */

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimal number of half moves to try
 * @return length of solution found, i.e.:
 *            n_min-4 defense put defender into self-check,
 *                    or some similar dead end
 *            n_min-2 defense has solved
 *            n_min..n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type
keepmating_guard_direct_has_solution_in_n(slice_index si,
                                          stip_length_type n,
                                          stip_length_type n_min)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = attack_has_solution_in_n(slices[si].u.pipe.next,n,n_min);
  else
    result = n+2;

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
stip_length_type keepmating_guard_direct_solve_in_n(slice_index si,
                                                    stip_length_type n,
                                                    stip_length_type n_min)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = attack_solve_in_n(slices[si].u.pipe.next,n,n_min);
  else
    result = n+2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the defense just played defends against the threats.
 * @param threats table containing the threats
 * @param len_threat length of threat(s) in table threats
 * @param si slice index
 * @param n maximum number of moves until goal
 * @return true iff the defense defends against at least one of the
 *         threats
 */
boolean keepmating_guard_are_threats_refuted_in_n(table threats,
                                                  stip_length_type len_threat,
                                                  slice_index si,
                                                  stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",len_threat);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = attack_are_threats_refuted_in_n(threats,len_threat,
                                             slices[si].u.pipe.next,
                                             n);
  else
    result = true;

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
stip_length_type
keepmating_guard_direct_solve_threats_in_n(table threats,
                                           slice_index si,
                                           stip_length_type n,
                                           stip_length_type n_min)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  if (is_a_mating_piece_left(mating))
  {
    slice_index const next = slices[si].u.pipe.next;
    result = attack_solve_threats_in_n(threats,next,n,n_min);
  }
  else
    result = n+2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}


/* **************** Implementation of interface DirectDefender **********
 */

/* Try to defend after an attempted key move at root level
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @param max_nr_refutations how many refutations should we look for
 * @return <slack_length_battle - stalemate
 *         <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - <=max_nr_refutations refutations found
 *         n+4 refuted - >max_nr_refutations refutations found
 */
stip_length_type keepmating_guard_root_defend(slice_index si,
                                              stip_length_type n,
                                              stip_length_type n_min,
                                              unsigned int max_nr_refutations)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParam("%u",max_nr_refutations);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = defense_root_defend(next,n,n_min,max_nr_refutations);
  else
    result = n+4;

  TraceFunctionExit(__func__);
  TraceValue("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to defend after an attempted key move at non-root level.
 * When invoked with some n, the function assumes that the key doesn't
 * solve in less than n half moves.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @return true iff the defender can defend
 */
boolean keepmating_guard_defend_in_n(slice_index si,
                                     stip_length_type n,
                                     stip_length_type n_min)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  slice_index const next = slices[si].u.pipe.next;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = defense_defend_in_n(next,n,n_min);
  else
    result = true;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there are refutations after an attempted key move
 * at non-root level
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @param max_nr_refutations how many refutations should we look for
 * @return <slack_length_battle - stalemate
           <=n solved  - return value is maximum number of moves
                         (incl. defense) needed
           n+2 refuted - <=max_nr_refutations refutations found
           n+4 refuted - >max_nr_refutations refutations found
 */
stip_length_type
keepmating_guard_can_defend_in_n(slice_index si,
                                 stip_length_type n,
                                 stip_length_type n_min,
                                 unsigned int max_nr_refutations)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  slice_index const next = slices[si].u.pipe.next;
  unsigned int result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParam("%u",max_nr_refutations);
  TraceFunctionParamListEnd();

  TraceEnumerator(Side,mating,"\n");

  if (is_a_mating_piece_left(mating))
    result = defense_can_defend_in_n(next,n,n_min,max_nr_refutations);
  else
    result = n+4;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}


/* **************** Implementation of interface Help ***************
 */

/* Solve in a number of half-moves
 * @param si identifies slice
 * @param n exact number of half moves until end state has to be reached
 * @return true iff >=1 solution was found
 */
boolean keepmating_guard_help_solve_in_n(slice_index si, stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  result = (is_a_mating_piece_left(mating)
            && help_solve_in_n(slices[si].u.pipe.next,n));

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean keepmating_guard_help_has_solution_in_n(slice_index si,
                                                stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  result = (is_a_mating_piece_left(mating)
            && help_has_solution_in_n(slices[si].u.pipe.next,n));

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
void keepmating_guard_help_solve_threats_in_n(table threats,
                                              slice_index si,
                                              stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  if (is_a_mating_piece_left(mating))
    help_solve_threats_in_n(threats,slices[si].u.pipe.next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


/* **************** Implementation of interface Series ***************
 */

/* Solve in a number of half-moves
 * @param si identifies slice
 * @param n exact number of half moves until end state has to be reached
 * @return true iff >=1 solution was found
 */
boolean keepmating_guard_series_solve_in_n(slice_index si, stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_series);

  result = (is_a_mating_piece_left(mating)
            && series_solve_in_n(slices[si].u.pipe.next,n));

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean keepmating_guard_series_has_solution_in_n(slice_index si,
                                                  stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_series);

  result = (is_a_mating_piece_left(mating)
            && series_has_solution_in_n(slices[si].u.pipe.next,n));

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
void keepmating_guard_series_solve_threats_in_n(table threats,
                                                slice_index si,
                                                stip_length_type n)
{
  Side const mating = slices[si].u.keepmating_guard.mating;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_series);

  if (is_a_mating_piece_left(mating))
    series_solve_threats_in_n(threats,slices[si].u.pipe.next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


/* **************** Implementation of interface Slice ***************
 */


/* **************** Stipulation instrumentation ***************
 */

/* Data structure for remembering the side(s) that needs to keep >= 1
 * piece that could deliver mate
 */
typedef boolean keepmating_type[nr_sides];

static void keepmating_guards_inserter_leaf(slice_index si,
                                            stip_structure_traversal *st)
{
  keepmating_type * const km = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  (*km)[slices[si].starter] = true;
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_leaf_forced(slice_index si,
                                                   stip_structure_traversal *st)
{
  keepmating_type * const km = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  (*km)[advers(slices[si].starter)] = true;
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_quodlibet(slice_index si,
                                                 stip_structure_traversal *st)
{
  keepmating_type * const km = st->param;
  keepmating_type km1 = { false, false };
  keepmating_type km2 = { false, false };

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  st->param = &km1;
  stip_traverse_structure(slices[si].u.binary.op1,st);

  st->param = &km2;
  stip_traverse_structure(slices[si].u.binary.op2,st);

  (*km)[White] = km1[White] && km2[White];
  (*km)[Black] = km1[Black] && km2[Black];

  st->param = km;
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_reciprocal(slice_index si,
                                                  stip_structure_traversal *st)
{
  keepmating_type * const km = st->param;
  keepmating_type km1 = { false, false };
  keepmating_type km2 = { false, false };

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  st->param = &km1;
  stip_traverse_structure(slices[si].u.binary.op1,st);

  st->param = &km2;
  stip_traverse_structure(slices[si].u.binary.op2,st);

  (*km)[White] = km1[White] || km2[White];
  (*km)[Black] = km1[Black] || km2[Black];

  st->param = km;
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_branch_fork(slice_index si,
                                                   stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  /* we can't rely on the (arbitrary) order stip_traverse_structure_children()
   * would use; instead make sure that we first traverse towards the
   * goal(s).
   */
  stip_traverse_structure(slices[si].u.branch_fork.towards_goal,st);
  stip_traverse_structure(slices[si].u.pipe.next,st);
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_attack_root(slice_index si,
                                                   stip_structure_traversal *st)
{
  keepmating_type const * const km = st->param;
  slice_index guard = no_slice;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if ((*km)[White])
    guard = alloc_keepmating_guard_root_defender_filter(White);

  if ((*km)[Black])
    guard = alloc_keepmating_guard_root_defender_filter(Black);

  if (guard!=no_slice)
  {
    slices[guard].starter = advers(slices[si].starter);
    pipe_append(si,guard);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_defender(slice_index si,
                                                stip_structure_traversal *st)
{
  keepmating_type const * const km = st->param;
  slice_index guard = no_slice;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type const length = slices[next].u.branch.length;
  stip_length_type const min_length = slices[next].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if ((*km)[White])
    guard = alloc_keepmating_guard_attacker_filter(length,min_length,White);

  if ((*km)[Black])
    guard = alloc_keepmating_guard_attacker_filter(length,min_length,Black);
  
  if (guard!=no_slice)
  {
    slice_index const next = slices[si].u.pipe.next;
    slice_index const next_prev = slices[next].prev;
    if (next_prev==si)
    {
      slices[guard].starter = advers(slices[si].starter);
      pipe_append(si,guard);
    }
    else
    {
      assert(slices[next_prev].type==STKeepMatingGuardAttackerFilter);
      pipe_set_successor(si,next_prev);
      dealloc_slice(guard);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_battle_fork(slice_index si,
                                                   stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  /* towards goal first, to detect the mating side */
  stip_traverse_structure(slices[si].u.branch_fork.towards_goal,st);
  stip_traverse_structure(slices[si].u.branch_fork.next,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_attack_move(slice_index si,
                                                   stip_structure_traversal *st)
{
  keepmating_type const * const km = st->param;
  slice_index guard = no_slice;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type const length = slices[next].u.branch.length;
  stip_length_type const min_length = slices[next].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if ((*km)[White])
    guard = alloc_keepmating_guard_defender_filter(length,min_length,White);

  if ((*km)[Black])
    guard = alloc_keepmating_guard_defender_filter(length,min_length,Black);

  if (guard!=no_slice)
  {
    slices[guard].starter = advers(slices[si].starter);
    pipe_append(si,guard);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_help_move(slice_index si,
                                                 stip_structure_traversal *st)
{
  keepmating_type const * const km = st->param;
  slice_index guard = no_slice;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type const length = slices[next].u.branch.length;
  stip_length_type const min_length = slices[next].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if ((*km)[White])
    guard = alloc_keepmating_guard_help_filter(length,min_length,White);

  if ((*km)[Black])
    guard = alloc_keepmating_guard_help_filter(length,min_length,Black);

  if (guard!=no_slice)
  {
    slices[guard].starter = advers(slices[si].starter);
    pipe_append(si,guard);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void keepmating_guards_inserter_series_move(slice_index si,
                                                   stip_structure_traversal *st)
{
  keepmating_type const * const km = st->param;
  slice_index guard = no_slice;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if ((*km)[White])
    guard = alloc_keepmating_guard_series_filter(White);

  if ((*km)[Black])
    guard = alloc_keepmating_guard_series_filter(Black);

  if (guard!=no_slice)
  {
    slices[guard].starter = advers(slices[si].starter);
    pipe_append(si,guard);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static stip_structure_visitor const keepmating_guards_inserters[] =
{
  &stip_traverse_structure_children,       /* STProxy */
  &keepmating_guards_inserter_attack_move, /* STAttackMove */
  &keepmating_guards_inserter_defender,    /* STDefenseMove */
  &keepmating_guards_inserter_help_move,   /* STHelpMove */
  &keepmating_guards_inserter_branch_fork, /* STHelpFork */
  &keepmating_guards_inserter_series_move, /* STSeriesMove */
  &keepmating_guards_inserter_branch_fork, /* STSeriesFork */
  &keepmating_guards_inserter_leaf,        /* STLeafDirect */
  &keepmating_guards_inserter_leaf,        /* STLeafHelp */
  &keepmating_guards_inserter_leaf_forced, /* STLeafForced */
  &keepmating_guards_inserter_reciprocal,  /* STReciprocal */
  &keepmating_guards_inserter_quodlibet,   /* STQuodlibet */
  &stip_traverse_structure_children,       /* STNot */
  &stip_traverse_structure_children,       /* STMoveInverterRootSolvableFilter */
  &stip_traverse_structure_children,       /* STMoveInverterSolvableFilter */
  &stip_traverse_structure_children,       /* STMoveInverterSeriesFilter */
  &keepmating_guards_inserter_attack_root, /* STAttackRoot */
  &stip_traverse_structure_children,       /* STBattlePlaySolutionWriter */
  &stip_traverse_structure_children,       /* STPostKeyPlaySolutionWriter */
  &stip_traverse_structure_children,       /* STPostKeyPlaySuppressor */
  &stip_traverse_structure_children,       /* STContinuationWriter */
  &stip_traverse_structure_children,       /* STRefutationsWriter */
  &stip_traverse_structure_children,       /* STThreatWriter */
  &stip_traverse_structure_children,       /* STThreatEnforcer */
  &stip_traverse_structure_children,       /* STRefutationsCollector */
  &stip_traverse_structure_children,       /* STVariationWriter */
  &stip_traverse_structure_children,       /* STRefutingVariationWriter */
  &stip_traverse_structure_children,       /* STNoShortVariations */
  &stip_traverse_structure_children,       /* STAttackHashed */
  &stip_traverse_structure_children,       /* STHelpRoot */
  &stip_traverse_structure_children,       /* STHelpShortcut */
  &stip_traverse_structure_children,       /* STHelpHashed */
  &stip_traverse_structure_children,       /* STSeriesRoot */
  &stip_traverse_structure_children,       /* STSeriesShortcut */
  &stip_traverse_structure_children,       /* STParryFork */
  &stip_traverse_structure_children,       /* STSeriesHashed */
  &stip_traverse_structure_children,       /* STSelfCheckGuardRootSolvableFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardSolvableFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardRootDefenderFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardAttackerFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardDefenderFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardHelpFilter */
  &stip_traverse_structure_children,       /* STSelfCheckGuardSeriesFilter */
  &keepmating_guards_inserter_battle_fork, /* STDirectDefenderFilter */
  &stip_traverse_structure_children,       /* STReflexHelpFilter */
  &stip_traverse_structure_children,       /* STReflexSeriesFilter */
  &keepmating_guards_inserter_battle_fork, /* STReflexRootSolvableFilter */
  &keepmating_guards_inserter_battle_fork, /* STReflexAttackerFilter */
  &keepmating_guards_inserter_battle_fork, /* STReflexDefenderFilter */
  &keepmating_guards_inserter_battle_fork, /* STSelfDefense */
  &stip_traverse_structure_children,       /* STRestartGuardRootDefenderFilter */
  &stip_traverse_structure_children,       /* STRestartGuardHelpFilter */
  &stip_traverse_structure_children,       /* STRestartGuardSeriesFilter */
  &stip_traverse_structure_children,       /* STIntelligentHelpFilter */
  &stip_traverse_structure_children,       /* STIntelligentSeriesFilter */
  &stip_traverse_structure_children,       /* STGoalReachableGuardHelpFilter */
  &stip_traverse_structure_children,       /* STGoalReachableGuardSeriesFilter */
  &stip_traverse_structure_children,       /* STKeepMatingGuardRootDefenderFilter */
  &stip_traverse_structure_children,       /* STKeepMatingGuardAttackerFilter */
  &stip_traverse_structure_children,       /* STKeepMatingGuardDefenderFilter */
  &stip_traverse_structure_children,       /* STKeepMatingGuardHelpFilter */
  &stip_traverse_structure_children,       /* STKeepMatingGuardSeriesFilter */
  &stip_traverse_structure_children,       /* STMaxFlightsquares */
  &stip_traverse_structure_children,       /* STDegenerateTree */
  &stip_traverse_structure_children,       /* STMaxNrNonTrivial */
  &stip_traverse_structure_children,       /* STMaxNrNonTrivialCounter */
  &stip_traverse_structure_children,       /* STMaxThreatLength */
  &stip_traverse_structure_children,       /* STMaxTimeRootDefenderFilter */
  &stip_traverse_structure_children,       /* STMaxTimeDefenderFilter */
  &stip_traverse_structure_children,       /* STMaxTimeHelpFilter */
  &stip_traverse_structure_children,       /* STMaxTimeSeriesFilter */
  &stip_traverse_structure_children,       /* STMaxSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,       /* STMaxSolutionsRootDefenderFilter */
  &stip_traverse_structure_children,       /* STMaxSolutionsHelpFilter */
  &stip_traverse_structure_children,       /* STMaxSolutionsSeriesFilter */
  &stip_traverse_structure_children,       /* STStopOnShortSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,       /* STStopOnShortSolutionsHelpFilter */
  &stip_traverse_structure_children        /* STStopOnShortSolutionsSeriesFilter */
};

/* Instrument stipulation with STKeepMatingGuard slices
 */
void stip_insert_keepmating_guards(void)
{
  keepmating_type km = { false, false };
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceStipulation(root_slice);

  stip_structure_traversal_init(&st,&keepmating_guards_inserters,&km);
  stip_traverse_structure(root_slice,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
