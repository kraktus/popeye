#include "output/plaintext/tree/threat_writer.h"
#include "output/plaintext/message.h"
#include "output/plaintext/plaintext.h"
#include "stipulation/stipulation.h"
#include "stipulation/pipe.h"
#include "solving/battle_play/threat.h"
#include "utilities/table.h"
#include "solving/pipe.h"
#include "debugging/trace.h"

/* Allocate a STOutputPlainTextThreatWriter defender slice.
 * @return index of allocated slice
 */
slice_index alloc_output_plaintext_tree_threat_writer_slice(void)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  result = alloc_pipe(STOutputPlainTextThreatWriter);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to solve in solve_nr_remaining half-moves.
 * @param si slice index
 * @note assigns solve_result the length of solution found and written, i.e.:
 *            previous_move_is_illegal the move just played is illegal
 *            this_move_is_illegal     the move being played is illegal
 *            immobility_on_next_move  the moves just played led to an
 *                                     unintended immobility on the next move
 *            <=n+1 length of shortest solution found (n+1 only if in next
 *                                     branch)
 *            n+2 no solution found in this branch
 *            n+3 no solution found in next branch
 *            (with n denominating solve_nr_remaining)
 */
void output_plaintext_tree_threat_writer_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (table_length(threats[parent_ply[parent_ply[nbply]]])==0)
  {
    ply const save_nbply = nbply;
    output_plaintext_message(Threat);
    nbply = parent_ply[nbply];
    output_plaintext_write_dummy_move_effects(&output_plaintext_engine,
                                              stdout,
                                              &output_plaintext_symbol_table);
    nbply = save_nbply;
  }

  pipe_solve_delegate(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
