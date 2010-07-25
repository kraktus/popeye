#include "pyreflxg.h"
#include "stipulation/branch.h"
#include "stipulation/battle_play/branch.h"
#include "stipulation/battle_play/attack_play.h"
#include "stipulation/help_play/branch.h"
#include "stipulation/help_play/play.h"
#include "stipulation/series_play/branch.h"
#include "stipulation/series_play/play.h"
#include "pypipe.h"
#include "pyslice.h"
#include "pybrafrk.h"
#include "pypipe.h"
#include "pydata.h"
#include "stipulation/proxy.h"
#include "trace.h"

#include <assert.h>


/* **************** Initialisation ***************
 */

/* Substitute links to proxy slices by the proxy's target
 * @param si root of sub-tree where to resolve proxies
 * @param st address of structure representing the traversal
 */
void reflex_filter_resolve_proxies(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  pipe_resolve_proxies(si,st);
  proxy_slice_resolve(&slices[si].u.reflex_guard.avoided);
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Allocate a STReflexRootFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice that leads towards goal from
 *                      the branch
 * @param proxy_to_avoided prototype of slice that must not be solvable
 * @return index of allocated slice
 */
static slice_index alloc_reflex_root_filter(slice_index proxy_to_avoided)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  /* ab(use) the fact that .avoided and .towards_goal are collocated */
  result = alloc_branch_fork(STReflexRootFilter,0,0,proxy_to_avoided);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a STReflexHelpFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice that leads towards goal from
 *                      the branch
 * @param proxy_to_avoided prototype of slice that must not be solvable
 * @return index of allocated slice
 */
static slice_index alloc_reflex_help_filter(stip_length_type length,
                                            stip_length_type min_length,
                                            slice_index proxy_to_avoided)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  /* ab(use) the fact that .avoided and .towards_goal are collocated */
  result = alloc_branch_fork(STReflexHelpFilter,
                             length,min_length,
                             proxy_to_avoided);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}


/* **************** Implementation of interface attacker_filter **************
 */

/* Allocate a STReflexAttackerFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_avoided prototype of slice that must not be solvable
 * @return index of allocated slice
 */
static slice_index alloc_reflex_attacker_filter(stip_length_type length,
                                                stip_length_type min_length,
                                                slice_index proxy_to_avoided)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  /* ab(use) the fact that .avoided and .towards_goal are collocated */
  result = alloc_branch_fork(STReflexAttackerFilter,
                             length,min_length,
                             proxy_to_avoided);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Recursively make a sequence of root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void reflex_attacker_filter_make_root(slice_index si,
                                      stip_structure_traversal *st)
{
  slice_index * const root = st->param;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index guard;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  guard = alloc_reflex_root_filter(stip_make_root_slices(avoided));

  stip_traverse_structure_pipe(si,st);
  pipe_link(guard,*root);
  *root = guard;
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traversal of the moves beyond a reflex attacker filter slice 
 * @param si identifies root of subtree
 * @param st address of structure representing traversal
 */
void stip_traverse_moves_reflex_attack_filter(slice_index si,
                                              stip_move_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_moves_branch_init_full_length(si,st);

  if (st->remaining==slices[si].u.reflex_guard.length)
    stip_traverse_moves_branch(slices[si].u.reflex_guard.avoided,st);

  stip_traverse_moves_pipe(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Determine whether there is a solution in n half moves, by trying
 * n_min, n_min+2 ... n half-moves.
 * @param si slice index of slice being solved
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimal number of half moves to try
 * @param n_max_unsolvable maximum number of half-moves that we
 *                         know have no solution
 * @return length of solution found, i.e.:
 *            slack_length_battle-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type
reflex_attacker_filter_has_solution_in_n(slice_index si,
                                         stip_length_type n,
                                         stip_length_type n_min,
                                         stip_length_type n_max_unsolvable)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_min);
  TraceFunctionParam("%u",n_max_unsolvable);
  TraceFunctionParamListEnd();

  switch (slice_has_solution(avoided))
  {
    case opponent_self_check:
      result = slack_length_battle-2;
      break;

    case has_no_solution:
      result = n+2;
      break;

    case has_solution:
      result = attack_has_solution_in_n(next,n,n_min,n_max_unsolvable);
      break;

    default:
      assert(0);
      result = n+2;
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve a slice
 * @param si slice index
 * @return true iff >=1 solution was found
 */
has_solution_type reflex_root_filter_solve(slice_index si)
{
  has_solution_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (slice_solve(avoided)==has_solution)
    result = slice_solve(next);
  else
    result = has_no_solution;

  TraceFunctionExit(__func__);
  TraceEnumerator(has_solution_type,result,"");
  TraceFunctionResultEnd();
  return result;
}

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
reflex_attacker_filter_solve_in_n(slice_index si,
                                  stip_length_type n,
                                  stip_length_type n_max_unsolvable)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_max_unsolvable);
  TraceFunctionParamListEnd();

  switch (slice_has_solution(avoided))
  {
    case opponent_self_check:
      result = slack_length_battle-2;
      break;

    case has_solution:
      result = attack_solve_in_n(next,n,n_max_unsolvable);
      break;

    case has_no_solution:
      result = n+2;
      break;

    default:
      assert(0);
      result = n+2;
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Find the first postkey slice and deallocate unused slices on the
 * way to it
 * @param si slice index
 * @param st address of structure capturing traversal state
 */
void reflex_root_filter_reduce_to_postkey_play(slice_index si,
                                               stip_structure_traversal *st)
{
  slice_index *postkey_slice = st->param;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_pipe(si,st);

  if (*postkey_slice!=no_slice)
  {
    dealloc_slices(avoided);
    dealloc_slice(si);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traverse the moves beyond a reflex root filter
 * @param branch root slice of subtree
 * @param st address of structure defining traversal
 */
void stip_traverse_moves_reflex_root_filter(slice_index si,
                                            stip_move_traversal *st)
{
  stip_traverse_moves_pipe(si,st);
  stip_traverse_moves(slices[si].u.reflex_guard.avoided,st);
}


/* **************** Implementation of interface defender_filter **********
 */

/* Allocate a STReflexDefenderFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice that leads towards goal from
 *                      the branch
 * @param proxy_to_avoided prototype of slice that must not be solvable
 * @return index of allocated slice
 */
static slice_index alloc_reflex_defender_filter(stip_length_type length,
                                                stip_length_type min_length,
                                                slice_index proxy_to_avoided)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  /* ab(use) the fact that .avoided and .towards_goal are collocated */
  result = alloc_branch_fork(STReflexDefenderFilter,
                             length,min_length,
                             proxy_to_avoided);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Recursively make a sequence of root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void reflex_defender_filter_make_root(slice_index si,
                                      stip_structure_traversal *st)
{
  slice_index * const root = st->param;
  slice_index root_filter;
  stip_length_type const length = slices[si].u.reflex_guard.length;
  stip_length_type const min_length = slices[si].u.reflex_guard.min_length;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_pipe(si,st);

  root_filter = alloc_reflex_defender_filter(length,min_length,avoided);
  pipe_link(root_filter,*root);
  *root = root_filter;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

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
stip_length_type
reflex_defender_filter_defend_in_n(slice_index si,
                                   stip_length_type n,
                                   stip_length_type n_max_unsolvable)
{
  stip_length_type result;
  slice_index const next = slices[si].u.reflex_guard.next;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  stip_length_type const length = slices[si].u.branch_fork.length;
  stip_length_type const min_length = slices[si].u.branch_fork.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_max_unsolvable);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_battle);

  if (n_max_unsolvable<=slack_length_battle
      && length-min_length+1>=n-slack_length_battle
      && slice_solve(avoided)==has_no_solution)
    result = slack_length_battle+1;
  else
    result = defense_defend_in_n(next,n,n_max_unsolvable);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

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
stip_length_type
reflex_defender_filter_can_defend_in_n(slice_index si,
                                       stip_length_type n,
                                       stip_length_type n_max_unsolvable,
                                       unsigned int max_nr_refutations)
{
  stip_length_type result;
  slice_index const next = slices[si].u.pipe.next;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  stip_length_type const length = slices[si].u.branch_fork.length;
  stip_length_type const min_length = slices[si].u.branch_fork.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",n_max_unsolvable);
  TraceFunctionParam("%u",max_nr_refutations);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_battle);

  if (n_max_unsolvable<=slack_length_battle
      && length-min_length+1>=n-slack_length_battle
      && slice_has_solution(avoided)==has_no_solution)
    result = slack_length_battle+1;
  else
    result = defense_can_defend_in_n(next,
                                     n,n_max_unsolvable,
                                     max_nr_refutations);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Produce slices representing set play
 * @param si slice index
 * @param st state of traversal
 */
void
reflex_guard_defender_filter_make_setplay_slice(slice_index si,
                                                stip_structure_traversal *st)
{
  slice_index * const result = st->param;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  stip_length_type const length = slices[si].u.reflex_guard.length;
  stip_length_type const length_h = (length-slack_length_battle
                                     +slack_length_help);

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_pipe(si,st);

  {
    slice_index const guard = alloc_reflex_help_filter(length_h,length_h,
                                                       avoided);
    pipe_link(guard,*result);
    *result = guard;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Spin off set play
 * @param si slice index
 * @param st state of traversal
 */
void reflex_defender_filter_apply_setplay(slice_index si,
                                          stip_structure_traversal *st)
{
  slice_index * const setplay_slice = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    slice_index const avoided = slices[si].u.reflex_guard.avoided;
    slice_index const length = slices[si].u.reflex_guard.length;
    stip_length_type const length_h = (length-slack_length_battle
                                       +slack_length_help);
    slice_index const min_length = slices[si].u.reflex_guard.min_length;
    stip_length_type const min_length_h = (min_length-slack_length_battle
                                           +slack_length_help);
    slice_index const filter = alloc_reflex_help_filter(length_h,min_length_h,
                                                        avoided);
    pipe_link(filter,*setplay_slice);
    *setplay_slice = filter;
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Find the first postkey slice and deallocate unused slices on the
 * way to it
 * @param si slice index
 * @param st address of structure capturing traversal state
 */
void reflex_defender_filter_reduce_to_postkey_play(slice_index si,
                                                   stip_structure_traversal *st)
{
  slice_index *postkey_slice = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  *postkey_slice = si;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


/* **************** Implementation of interface help_filter ************
 */

/* Recursively make a sequence of root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void reflex_help_filter_make_root(slice_index si,
                                    stip_structure_traversal *st)
{
  slice_index * const root = st->param;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index guard;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  guard = alloc_reflex_root_filter(stip_make_root_slices(avoided));

  stip_traverse_structure_pipe(si,st);
  pipe_link(guard,*root);
  *root = guard;

  if (slices[si].u.pipe.next==no_slice)
    /* we are obsolete and are going to be deallocated */
    slices[si].u.reflex_guard.avoided = no_slice;
  else
    help_branch_shorten_slice(si);
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Solve in a number of half-moves
 * @param si identifies slice
 * @param n exact number of half moves until end state has to be reached
 * @return length of solution found, i.e.:
 *         n+4 the move leading to the current position has turned out
 *             to be illegal
 *         n+2 no solution found
 *         n   solution found
 */
stip_length_type reflex_help_filter_solve_in_n(slice_index si,
                                               stip_length_type n)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  /* TODO exact - but what does it mean??? */
  if (slice_has_solution(avoided)==has_solution)
    result = help_solve_in_n(next,n);
  else 
    result = n+2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 * @return length of solution found, i.e.:
 *         n+4 the move leading to the current position has turned out
 *             to be illegal
 *         n+2 no solution found
 *         n   solution found
 */
stip_length_type reflex_help_filter_has_solution_in_n(slice_index si,
                                                      stip_length_type n)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_help);

  /* TODO exact - but what does it mean??? */
  if (slice_has_solution(avoided)==has_solution)
    result = help_has_solution_in_n(next,n);
  else
    result = n+2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* **************** Implementation of interface series_filter ************
 */

/* Allocate a STReflexSeriesFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice that leads towards goal from
 *                      the branch
 * @param proxy_to_avoided prototype of slice that must not be solvable
 * @return index of allocated slice
 */
static slice_index alloc_reflex_series_filter(stip_length_type length,
                                              stip_length_type min_length,
                                              slice_index proxy_to_avoided)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  /* ab(use) the fact that .avoided and .towards_goal are collocated */
  result = alloc_branch_fork(STReflexSeriesFilter,
                             length,min_length,
                             proxy_to_avoided);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Recursively make a sequence of root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void reflex_series_filter_make_root(slice_index si,
                                      stip_structure_traversal *st)
{
  slice_index * const root = st->param;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_pipe(si,st);

  {
    slice_index const guard = alloc_reflex_root_filter(avoided);
    pipe_link(guard,*root);
    *root = guard;
    shorten_series_pipe(si);
  }
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traversal of the moves beyond a reflex attacker filter slice 
 * @param si identifies root of subtree
 * @param st address of structure representing traversal
 */
void stip_traverse_moves_reflex_series_filter(slice_index si,
                                              stip_move_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_moves_branch_init_full_length(si,st);

  if (st->remaining==slack_length_series+1)
    stip_traverse_moves_branch(slices[si].u.reflex_guard.avoided,st);
  else
    stip_traverse_moves_pipe(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Solve in a number of half-moves
 * @param si identifies slice
 * @param n exact number of half moves until end state has to be reached
 * @return length of solution found, i.e.:
 *         n+2 the move leading to the current position has turned out
 *             to be illegal
 *         n+1 no solution found
 *         n   solution found
 */
stip_length_type reflex_series_filter_solve_in_n(slice_index si,
                                                 stip_length_type n)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_series);

  /* TODO exact - but what does it mean??? */
  if (slice_has_solution(avoided)==has_solution)
    result = series_solve_in_n(next,n);
  else
    result = n+1;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n exact number of half moves until end state has to be reached
 * @return length of solution found, i.e.:
 *         n+2 the move leading to the current position has turned out
 *             to be illegal
 *         n+1 no solution found
 *         n   solution found
 */
stip_length_type reflex_series_filter_has_solution_in_n(slice_index si,
                                                        stip_length_type n)
{
  stip_length_type result;
  slice_index const avoided = slices[si].u.reflex_guard.avoided;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_series);

  /* TODO exact - but what does it mean??? */
  if (slice_has_solution(avoided)==has_solution)
    result = series_has_solution_in_n(next,n);
  else
    result = n+1;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}


/* **************** Stipulation instrumentation ***************
 */

typedef struct
{
    slice_index to_be_avoided[2];
} init_param;

/* In battle play, insert a STReflexAttackFilter slice before a
 * slice where the reflex stipulation might force the side at the move
 * to reach the goal
 */
static void reflex_guards_inserter_attack(slice_index si,
                                          stip_structure_traversal *st)
{
  init_param * const param = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    stip_length_type const idx = (length-slack_length_battle-1)%2;
    slice_index const proxy_to_avoided = param->to_be_avoided[idx];
    pipe_append(slices[si].prev,
                alloc_reflex_attacker_filter(length,min_length,
                                             proxy_to_avoided));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* In battle play, insert a STReflexDefenseFilter slice before a slice
 * where the reflex stipulation might force the side at the move to
 * reach the goal
 */
static void reflex_guards_inserter_defense(slice_index si,
                                           stip_structure_traversal *st)
{
  init_param const * const param = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    stip_length_type const idx = (length-slack_length_battle-1)%2;
    slice_index const proxy_to_avoided = param->to_be_avoided[idx];
    pipe_append(slices[si].prev,
                alloc_reflex_defender_filter(length,min_length,
                                             proxy_to_avoided));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Prevent STReflex* slice insertion from recursing into the following
 * branch
 */
static void reflex_guards_inserter_branch_fork(slice_index si,
                                               stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  /* don't traverse .avoided! */
  stip_traverse_structure_pipe(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


static stip_structure_visitor const reflex_guards_inserters[] =
{
  &stip_traverse_structure_children,   /* STProxy */
  &reflex_guards_inserter_attack,      /* STAttackMove */
  &reflex_guards_inserter_defense,     /* STDefenseMove */
  &stip_traverse_structure_children,   /* STDefenseMoveAgainstGoal */
  &stip_traverse_structure_children,   /* STHelpMove */
  &stip_traverse_structure_children,   /* STHelpMoveToGoal */
  &reflex_guards_inserter_branch_fork, /* STHelpFork */
  &stip_traverse_structure_children,   /* STSeriesMove */
  &stip_traverse_structure_children,   /* STSeriesMoveToGoal */
  &reflex_guards_inserter_branch_fork, /* STSeriesFork */
  &stip_structure_visitor_noop,        /* STGoalReachedTester */
  &stip_structure_visitor_noop,        /* STLeaf */
  &stip_traverse_structure_children,   /* STReciprocal */
  &stip_traverse_structure_children,   /* STQuodlibet */
  &stip_traverse_structure_children,   /* STNot */
  &stip_traverse_structure_children,   /* STMoveInverterRootSolvableFilter */
  &stip_traverse_structure_children,   /* STMoveInverterSolvableFilter */
  &stip_traverse_structure_children,   /* STMoveInverterSeriesFilter */
  &stip_traverse_structure_children,   /* STAttackRoot */
  &stip_traverse_structure_children,   /* STDefenseRoot */
  &stip_traverse_structure_children,   /* STPostKeyPlaySuppressor */
  &stip_traverse_structure_children,   /* STContinuationSolver */
  &stip_traverse_structure_children,   /* STContinuationWriter */
  &stip_traverse_structure_children,   /* STBattlePlaySolver */
  &stip_traverse_structure_children,   /* STBattlePlaySolutionWriter */
  &stip_traverse_structure_children,   /* STThreatSolver */
  &stip_traverse_structure_children,   /* STZugzwangWriter */
  &stip_traverse_structure_children,   /* STThreatEnforcer */
  &stip_traverse_structure_children,   /* STThreatCollector */
  &stip_traverse_structure_children,   /* STRefutationsCollector */
  &stip_traverse_structure_children,   /* STVariationWriter */
  &stip_traverse_structure_children,   /* STRefutingVariationWriter */
  &stip_traverse_structure_children,   /* STNoShortVariations */
  &stip_traverse_structure_children,   /* STAttackHashed */
  &stip_traverse_structure_children,   /* STHelpRoot */
  &stip_traverse_structure_children,   /* STHelpShortcut */
  &stip_traverse_structure_children,   /* STHelpHashed */
  &stip_traverse_structure_children,   /* STSeriesRoot */
  &stip_traverse_structure_children,   /* STSeriesShortcut */
  &stip_traverse_structure_children,   /* STParryFork */
  &stip_traverse_structure_children,   /* STSeriesHashed */
  &stip_traverse_structure_children,   /* STSelfCheckGuardRootSolvableFilter */
  &stip_traverse_structure_children,   /* STSelfCheckGuardSolvableFilter */
  &stip_traverse_structure_children,   /* STSelfCheckGuardAttackerFilter */
  &stip_traverse_structure_children,   /* STSelfCheckGuardDefenderFilter */
  &stip_traverse_structure_children,   /* STSelfCheckGuardHelpFilter */
  &stip_traverse_structure_children,   /* STSelfCheckGuardSeriesFilter */
  &stip_traverse_structure_children,   /* STDirectDefenderFilter */
  &stip_traverse_structure_children,   /* STReflexRootFilter */
  &stip_traverse_structure_children,   /* STReflexHelpFilter */
  &stip_traverse_structure_children,   /* STReflexSeriesFilter */
  &stip_traverse_structure_children,   /* STReflexAttackerFilter */
  &stip_traverse_structure_children,   /* STReflexDefenderFilter */
  &stip_traverse_structure_children,   /* STSelfDefense */
  &stip_traverse_structure_children,   /* STDefenseFork */
  &stip_traverse_structure_children,   /* STRestartGuardRootDefenderFilter */
  &stip_traverse_structure_children,   /* STRestartGuardHelpFilter */
  &stip_traverse_structure_children,   /* STRestartGuardSeriesFilter */
  &stip_traverse_structure_children,   /* STIntelligentHelpFilter */
  &stip_traverse_structure_children,   /* STIntelligentSeriesFilter */
  &stip_traverse_structure_children,   /* STGoalReachableGuardHelpFilter */
  &stip_traverse_structure_children,   /* STGoalReachableGuardSeriesFilter */
  &stip_traverse_structure_children,   /* STIntelligentDuplicateAvoider */
  &stip_traverse_structure_children,   /* STKeepMatingGuardAttackerFilter */
  &stip_traverse_structure_children,   /* STKeepMatingGuardDefenderFilter */
  &stip_traverse_structure_children,   /* STKeepMatingGuardHelpFilter */
  &stip_traverse_structure_children,   /* STKeepMatingGuardSeriesFilter */
  &stip_traverse_structure_children,   /* STMaxFlightsquares */
  &stip_traverse_structure_children,   /* STDegenerateTree */
  &stip_traverse_structure_children,   /* STMaxNrNonTrivial */
  &stip_traverse_structure_children,   /* STMaxNrNonChecks */
  &stip_traverse_structure_children,   /* STMaxNrNonTrivialCounter */
  &stip_traverse_structure_children,   /* STMaxThreatLength */
  &stip_traverse_structure_children,   /* STMaxTimeRootDefenderFilter */
  &stip_traverse_structure_children,   /* STMaxTimeDefenderFilter */
  &stip_traverse_structure_children,   /* STMaxTimeHelpFilter */
  &stip_traverse_structure_children,   /* STMaxTimeSeriesFilter */
  &stip_traverse_structure_children,   /* STMaxSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,   /* STMaxSolutionsSolvableFilter */
  &stip_traverse_structure_children,   /* STMaxSolutionsRootDefenderFilter */
  &stip_traverse_structure_children,   /* STMaxSolutionsHelpFilter */
  &stip_traverse_structure_children,   /* STMaxSolutionsSeriesFilter */
  &stip_traverse_structure_children,   /* STStopOnShortSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,   /* STStopOnShortSolutionsHelpFilter */
  &stip_traverse_structure_children,   /* STStopOnShortSolutionsSeriesFilter */
  &stip_traverse_structure_children,   /* STEndOfPhaseWriter */
  &stip_traverse_structure_children,   /* STEndOfSolutionWriter */
  &stip_traverse_structure_children,   /* STRefutationWriter */
  &stip_traverse_structure_children,   /* STOutputPlaintextTreeCheckDetectorAttackerFilter */
  &stip_traverse_structure_children,   /* STOutputPlaintextTreeCheckDetectorDefenderFilter */
  &stip_traverse_structure_children,   /* STOutputPlaintextLineLineWriter */
  &stip_traverse_structure_children,   /* STOutputPlaintextTreeGoalWriter */
  &stip_traverse_structure_children,   /* STOutputPlaintextTreeMoveInversionCounter */
  &stip_traverse_structure_children,   /* STOutputPlaintextLineMoveInversionCounter */
  &stip_traverse_structure_children    /* STOutputPlaintextLineEndOfIntroSeriesMarker */
};

/* In alternate play, insert a STReflexHelpFilter slice before a slice
 * where the reflex stipulation might force the side at the move to
 * reach the goal
 */
static void reflex_guards_inserter_help(slice_index si,
                                        stip_structure_traversal *st)
{
  init_param * const param = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;
  stip_length_type const idx = (length+1-slack_length_help)%2;
  slice_index const proxy_to_avoided = param->to_be_avoided[idx];

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  if (proxy_to_avoided!=no_slice)
    pipe_append(slices[si].prev,
                alloc_reflex_help_filter(length,min_length,proxy_to_avoided));

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Instrument a branch with STReflex* slices for a (non-semi)
 * reflex stipulation 
 * @param si root of branch to be instrumented
 * @param proxy_to_avoided_attack identifies branch that the
 *                                attacker attempts to avoid
 * @param proxy_to_avoided_defense identifies branch that the
 *                                 defender attempts to avoid
 */
void slice_insert_reflex_filters(slice_index si,
                                 slice_index proxy_to_avoided_attack,
                                 slice_index proxy_to_avoided_defense)
{
  stip_structure_traversal st;
  init_param param = { { proxy_to_avoided_defense, proxy_to_avoided_attack } };

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",proxy_to_avoided_attack);
  TraceFunctionParam("%u",proxy_to_avoided_defense);
  TraceFunctionParamListEnd();

  TraceStipulation(si);

  assert(slices[proxy_to_avoided_attack].type==STProxy);
  assert(slices[proxy_to_avoided_defense].type==STProxy);

  stip_structure_traversal_init(&st,&reflex_guards_inserters,&param);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* In series play, insert a STReflexSeriesFilter slice before a slice where
 * the reflex stipulation might force the side at the move to reach
 * the goal
 */
static void reflex_guards_inserter_series(slice_index si,
                                          stip_structure_traversal *st)
{
  init_param * const param = st->param;
  slice_index const proxy_to_avoided = param->to_be_avoided[0];
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;
  slice_index const prev = slices[si].prev;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    slice_index const filter = alloc_reflex_series_filter(length,min_length,
                                                          proxy_to_avoided);
    pipe_append(prev,filter);
    pipe_append(filter,alloc_proxy_slice());
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* In battle play, insert a STReflexDefenderFilter slice before a
 * defense slice
 * @param si identifies defense slice
 * @param address of structure representing the traversal
 */
static void reflex_guards_inserter_defense_semi(slice_index si,
                                                stip_structure_traversal *st)
{
  init_param const * const param = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    stip_length_type const length = slices[si].u.branch.length;
    stip_length_type const min_length = slices[si].u.branch.min_length;
    stip_length_type const idx = (length-slack_length_battle-1)%2;
    pipe_append(slices[si].prev,
                alloc_reflex_defender_filter(length,min_length,
                                             param->to_be_avoided[idx]));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static stip_structure_visitor const reflex_guards_inserters_semi[] =
{
  &stip_traverse_structure_children,    /* STProxy */
  &stip_traverse_structure_children,    /* STAttackMove */
  &reflex_guards_inserter_defense_semi, /* STDefenseMove */
  &stip_traverse_structure_children,    /* STDefenseMoveAgainstGoal */
  &reflex_guards_inserter_help,         /* STHelpMove */
  &reflex_guards_inserter_help,         /* STHelpMoveToGoal */
  &reflex_guards_inserter_branch_fork,  /* STHelpFork */
  &reflex_guards_inserter_series,       /* STSeriesMove */
  &stip_traverse_structure_children,    /* STSeriesMoveToGoal */
  &reflex_guards_inserter_branch_fork,  /* STSeriesFork */
  &stip_structure_visitor_noop,         /* STGoalReachedTester */
  &stip_structure_visitor_noop,         /* STLeaf */
  &stip_traverse_structure_children,    /* STReciprocal */
  &stip_traverse_structure_children,    /* STQuodlibet */
  &stip_traverse_structure_children,    /* STNot */
  &stip_traverse_structure_children,    /* STMoveInverterRootSolvableFilter */
  &stip_traverse_structure_children,    /* STMoveInverterSolvableFilter */
  &stip_traverse_structure_children,    /* STMoveInverterSeriesFilter */
  &stip_traverse_structure_children,    /* STAttackRoot */
  &stip_traverse_structure_children,    /* STDefenseRoot */
  &stip_traverse_structure_children,    /* STPostKeyPlaySuppressor */
  &stip_traverse_structure_children,    /* STContinuationSolver */
  &stip_traverse_structure_children,    /* STContinuationWriter */
  &stip_traverse_structure_children,    /* STBattlePlaySolver */
  &stip_traverse_structure_children,    /* STBattlePlaySolutionWriter */
  &stip_traverse_structure_children,    /* STThreatSolver */
  &stip_traverse_structure_children,    /* STZugzwangWriter */
  &stip_traverse_structure_children,    /* STThreatEnforcer */
  &stip_traverse_structure_children,    /* STThreatCollector */
  &stip_traverse_structure_children,    /* STRefutationsCollector */
  &stip_traverse_structure_children,    /* STVariationWriter */
  &stip_traverse_structure_children,    /* STRefutingVariationWriter */
  &stip_traverse_structure_children,    /* STNoShortVariations */
  &stip_traverse_structure_children,    /* STAttackHashed */
  &stip_traverse_structure_children,    /* STHelpRoot */
  &stip_traverse_structure_children,    /* STHelpShortcut */
  &stip_traverse_structure_children,    /* STHelpHashed */
  &stip_traverse_structure_children,    /* STSeriesRoot */
  &stip_traverse_structure_children,    /* STSeriesShortcut */
  &stip_traverse_structure_children,    /* STParryFork */
  &stip_traverse_structure_children,    /* STSeriesHashed */
  &stip_traverse_structure_children,    /* STSelfCheckGuardRootSolvableFilter */
  &stip_traverse_structure_children,    /* STSelfCheckGuardSolvableFilter */
  &stip_traverse_structure_children,    /* STSelfCheckGuardAttackerFilter */
  &stip_traverse_structure_children,    /* STSelfCheckGuardDefenderFilter */
  &stip_traverse_structure_children,    /* STSelfCheckGuardHelpFilter */
  &stip_traverse_structure_children,    /* STSelfCheckGuardSeriesFilter */
  &stip_traverse_structure_children,    /* STDirectDefenderFilter */
  &stip_traverse_structure_children,    /* STReflexRootFilter */
  &stip_traverse_structure_children,    /* STReflexHelpFilter */
  &stip_traverse_structure_children,    /* STReflexSeriesFilter */
  &stip_traverse_structure_children,    /* STReflexAttackerFilter */
  &stip_traverse_structure_children,    /* STReflexDefenderFilter */
  &stip_traverse_structure_children,    /* STSelfDefense */
  &stip_traverse_structure_children,    /* STDefenseFork */
  &stip_traverse_structure_children,    /* STRestartGuardRootDefenderFilter */
  &stip_traverse_structure_children,    /* STRestartGuardHelpFilter */
  &stip_traverse_structure_children,    /* STRestartGuardSeriesFilter */
  &stip_traverse_structure_children,    /* STIntelligentHelpFilter */
  &stip_traverse_structure_children,    /* STIntelligentSeriesFilter */
  &stip_traverse_structure_children,    /* STGoalReachableGuardHelpFilter */
  &stip_traverse_structure_children,    /* STGoalReachableGuardSeriesFilter */
  &stip_traverse_structure_children,    /* STIntelligentDuplicateAvoider */
  &stip_traverse_structure_children,    /* STKeepMatingGuardAttackerFilter */
  &stip_traverse_structure_children,    /* STKeepMatingGuardDefenderFilter */
  &stip_traverse_structure_children,    /* STKeepMatingGuardHelpFilter */
  &stip_traverse_structure_children,    /* STKeepMatingGuardSeriesFilter */
  &stip_traverse_structure_children,    /* STMaxFlightsquares */
  &stip_traverse_structure_children,    /* STDegenerateTree */
  &stip_traverse_structure_children,    /* STMaxNrNonTrivial */
  &stip_traverse_structure_children,    /* STMaxNrNonChecks */
  &stip_traverse_structure_children,    /* STMaxNrNonTrivialCounter */
  &stip_traverse_structure_children,    /* STMaxThreatLength */
  &stip_traverse_structure_children,    /* STMaxTimeRootDefenderFilter */
  &stip_traverse_structure_children,    /* STMaxTimeDefenderFilter */
  &stip_traverse_structure_children,    /* STMaxTimeHelpFilter */
  &stip_traverse_structure_children,    /* STMaxTimeSeriesFilter */
  &stip_traverse_structure_children,    /* STMaxSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,    /* STMaxSolutionsSolvableFilter */
  &stip_traverse_structure_children,    /* STMaxSolutionsRootDefenderFilter */
  &stip_traverse_structure_children,    /* STMaxSolutionsHelpFilter */
  &stip_traverse_structure_children,    /* STMaxSolutionsSeriesFilter */
  &stip_traverse_structure_children,    /* STStopOnShortSolutionsRootSolvableFilter */
  &stip_traverse_structure_children,    /* STStopOnShortSolutionsHelpFilter */
  &stip_traverse_structure_children,    /* STStopOnShortSolutionsSeriesFilter */
  &stip_traverse_structure_children,    /* STEndOfPhaseWriter */
  &stip_traverse_structure_children,    /* STEndOfSolutionWriter */
  &stip_traverse_structure_children,    /* STRefutationWriter */
  &stip_traverse_structure_children,    /* STOutputPlaintextTreeCheckDetectorAttackerFilter */
  &stip_traverse_structure_children,    /* STOutputPlaintextTreeCheckDetectorDefenderFilter */
  &stip_traverse_structure_children,    /* STOutputPlaintextLineLineWriter */
  &stip_traverse_structure_children,    /* STOutputPlaintextTreeGoalWriter */
  &stip_traverse_structure_children,    /* STOutputPlaintextTreeMoveInversionCounter */
  &stip_traverse_structure_children,    /* STOutputPlaintextLineMoveInversionCounter */
  &stip_traverse_structure_children     /* STOutputPlaintextLineEndOfIntroSeriesMarker */
};

/* Instrument a branch with STReflex* slices for a semi-reflex
 * stipulation 
 * @param si root of branch to be instrumented
 * @param proxy_to_avoided identifies branch that needs to be guarded from
 */
void slice_insert_reflex_filters_semi(slice_index si,
                                      slice_index proxy_to_avoided)
{
  stip_structure_traversal st;
  init_param param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",proxy_to_avoided);
  TraceFunctionParamListEnd();

  TraceStipulation(si);

  assert(slices[proxy_to_avoided].type==STProxy);

  param.to_be_avoided[0] = proxy_to_avoided;
  param.to_be_avoided[1] = no_slice;

  stip_structure_traversal_init(&st,&reflex_guards_inserters_semi,&param);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traverse a subtree
 * @param branch root slice of subtree
 * @param st address of structure defining traversal
 */
void stip_traverse_structure_reflex_filter(slice_index branch,
                                           stip_structure_traversal *st)
{
  slice_index const avoided = slices[branch].u.reflex_guard.avoided;
  stip_traverse_structure_pipe(branch,st);
  stip_traverse_structure(avoided,st);
}
