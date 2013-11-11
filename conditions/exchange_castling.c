#include "conditions/exchange_castling.h"
#include "conditions/castling_chess.h"
#include "pieces/walks/classification.h"
#include "solving/move_generator.h"
#include "solving/castling.h"
#include "solving/move_effect_journal.h"
#include "stipulation/stipulation.h"
#include "debugging/trace.h"
#include "pieces/pieces.h"

#include <assert.h>

static boolean castling_right_used_up[nr_sides];

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            previous_move_is_illegal the move just played (or being played)
 *                                     is illegal
 *            immobility_on_next_move  the moves just played led to an
 *                                     unintended immobility on the next move
 *            <=n+1 length of shortest solution found (n+1 only if in next
 *                                     branch)
 *            n+2 no solution found in this branch
 *            n+3 no solution found in next branch
 */
stip_length_type exchange_castling_move_player_solve(slice_index si,
                                                      stip_length_type n)
{
  stip_length_type result;
  numecoup const curr = CURRMOVE_OF_PLY(nbply);
  move_generation_elmt const * const move_gen_top = move_generation_stack+curr;
  square const sq_capture = move_gen_top->capture;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  if (sq_capture==offset_platzwechsel_rochade)
  {
    Side const trait_ply = trait[nbply];

    square const sq_departure = move_gen_top->departure;
    square const sq_arrival = move_gen_top->arrival;

    assert(sq_arrival!=nullsquare);

    move_effect_journal_do_no_piece_removal();
    move_effect_journal_do_piece_exchange(move_effect_reason_exchange_castling_exchange,
                                          sq_departure,sq_arrival);

    castling_right_used_up[trait_ply] = true;

    result = solve(slices[si].next2,n);

    castling_right_used_up[trait_ply] = false;
  }
  else
    result = solve(slices[si].next1,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Generate moves for a single piece
 * @param identifies generator slice
 * @param p walk to be used for generating
 */
void exchange_castling_generate_moves_for_piece(slice_index si, PieNam p)
{
  generate_moves_for_piece(slices[si].next1,p);

  if (p==King && !castling_right_used_up[trait[nbply]])
  {
    int i;
    square square_a = square_a1;
    for (i = nr_rows_on_board; i>0; --i, square_a += onerow)
    {
      int j;
      curr_generation->arrival = square_a;
      for (j = nr_files_on_board; j>0; --j, curr_generation->arrival += dir_right)
        if (curr_generation->arrival!=curr_generation->departure
            && TSTFLAG(spec[curr_generation->arrival],trait[nbply])
            && !is_pawn(get_walk_of_piece_on_square(curr_generation->arrival))) /* not sure if "castling" with Ps forbidden */
          push_special_move(offset_platzwechsel_rochade);
    }
  }
}

/* Instrument slices with move tracers
 */
void exchange_castling_initialise_solving(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  solving_instrument_move_generation(si,nr_sides,STPlatzwechselRochadeMovesForPieceGenerator);
  insert_alternative_move_players(si,STExchangeCastlingMovePlayer);
  solving_disable_castling(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
