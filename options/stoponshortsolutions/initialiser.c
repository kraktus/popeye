#include "options/stoponshortsolutions/initialiser.h"
#include "options/stoponshortsolutions/stoponshortsolutions.h"
#include "pypipe.h"
#include "trace.h"

/* Allocate a STStopOnShortSolutionsInitialiser slice.
 * @return allocated slice
 */
slice_index alloc_stoponshortsolutions_initialiser_slice(void)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  result = alloc_pipe(STStopOnShortSolutionsInitialiser);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve a slice
 * @param si slice index
 * @return whether there is a solution and (to some extent) why not
 */
has_solution_type stoponshortsolutions_initialiser_solve(slice_index si)
{
  has_solution_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  reset_short_solution_found_in_phase();

  result = slice_solve(slices[si].u.pipe.next);

  TraceFunctionExit(__func__);
  TraceEnumerator(has_solution_type,result,"");
  TraceFunctionResultEnd();
  return result;
}
