#include "conditions/volage.h"
#include "pydata.h"
#include "stipulation/pipe.h"
#include "stipulation/has_solution_type.h"
#include "stipulation/stipulation.h"
#include "stipulation/move_player.h"
#include "solving/move_effect_journal.h"
#include "debugging/trace.h"

#include <assert.h>
#include <stdlib.h>

static void change_side(void)
{
  square const sq_departure = move_generation_stack[current_move[nbply]].departure;
  square const sq_arrival = move_generation_stack[current_move[nbply]].arrival;

  if (TSTFLAG(spec[sq_arrival],Volage)
      && SquareCol(sq_departure)!=SquareCol(sq_arrival))
  {
    move_effect_journal_do_side_change(move_effect_reason_volage_side_change,
                                       sq_arrival,
                                       e[sq_arrival]<vide ? White : Black);

    if (!CondFlag[hypervolage])
    {
      Flags flags = spec[sq_arrival];
      CLRFLAG(flags,Volage);
      move_effect_journal_do_flags_change(move_effect_reason_volage_side_change,
                                          sq_arrival,
                                          flags);
    }
  }
}

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type volage_side_changer_solve(slice_index si, stip_length_type n)
{
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  change_side();
  result = solve(slices[si].next1,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Instrument slices with move tracers
 */
void stip_insert_volage_side_changers(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  stip_instrument_moves(si,STVolageSideChanger);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}