#include "pieces/attributes/total_invisible.h"
#include "pieces/walks/classification.h"
#include "position/position.h"
#include "position/move_diff_code.h"
#include "position/effects/piece_creation.h"
#include "position/effects/null_move.h"
#include "stipulation/structure_traversal.h"
#include "stipulation/branch.h"
#include "stipulation/pipe.h"
#include "stipulation/proxy.h"
#include "stipulation/slice_insertion.h"
#include "stipulation/goals/slice_insertion.h"
#include "solving/check.h"
#include "solving/has_solution_type.h"
#include "solving/machinery/solve.h"
#include "solving/machinery/slack_length.h"
#include "solving/move_generator.h"
#include "solving/pipe.h"
#include "solving/move_effect_journal.h"
#include "output/plaintext/plaintext.h"
#include "output/plaintext/pieces.h"
#include "optimisations/orthodox_square_observation.h"
#include "optimisations/orthodox_check_directions.h"
#include "debugging/assert.h"
#include "debugging/trace.h"

#include <stdlib.h>
#include <string.h>

unsigned int total_invisible_number;

static unsigned int nr_placed_victims = 0;
static unsigned int nr_placed_interceptors = 0;

static ply ply_replayed;

static stip_length_type combined_result;

static square square_order_for_non_interceptors[65];

static unsigned int taboo[nr_sides][maxsquare];

static struct
{
    Side side;
    piece_walk_type walk;
    square pos;
} piece_choice[nr_squares_on_board];

static enum
{
  regular_play,
  replaying_moves
} play_phase = regular_play;

static unsigned long nr_tries = 0;

static void play_with_placed_invisibles(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TracePosition(being_solved.board,being_solved.spec);

  if (is_in_check(advers(SLICE_STARTER(si))))
    solve_result = previous_move_is_illegal;
  else
  {
    play_phase = replaying_moves;
    ++nr_tries;
    pipe_solve_delegate(si);
    play_phase = regular_play;
  }

  if (solve_result>combined_result)
    combined_result = solve_result;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean is_rider_check_uninterceptable_on_vector(Side side_checking, square king_pos,
                                                        vec_index_type k, piece_walk_type rider_walk)
{
  boolean result = false;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_checking);
  TraceSquare(king_pos);
  TraceValue("%u",k);
  TraceWalk(rider_walk);
  TraceFunctionParamListEnd();

  {
    square s = king_pos+vec[k];
    while (is_square_empty(s) && taboo[White][s]>0 && taboo[Black][s]>0)
      s += vec[k];

    {
      piece_walk_type const walk = get_walk_of_piece_on_square(s);
      result = ((walk==rider_walk || walk==Queen)
                && TSTFLAG(being_solved.spec[s],side_checking));
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean is_rider_check_uninterceptable(Side side_checking, square king_pos,
                                              vec_index_type kanf, vec_index_type kend, piece_walk_type rider_walk)
{
  boolean result = false;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_checking);
  TraceSquare(king_pos);
  TraceValue("%u",kanf);
  TraceValue("%u",kend);
  TraceWalk(rider_walk);
  TraceFunctionParamListEnd();

  {
    vec_index_type k;
    for (k = kanf; !result && k<=kend; k++)
      if (is_rider_check_uninterceptable_on_vector(side_checking,king_pos,k,rider_walk))
      {
        result = true;
        break;
      }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean is_check_uninterceptable(Side side_in_check)
{
  boolean result = false;
  Side const side_checking = advers(side_in_check);
  square const sq_king = being_solved.king_square[side_in_check];

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_in_check);
  TraceFunctionParamListEnd();

  result = result || (being_solved.number_of_pieces[side_checking][King]>0
                      && king_check_ortho(side_checking,sq_king));

  result = result || (being_solved.number_of_pieces[side_checking][Pawn]>0
                      && pawn_check_ortho(side_checking,sq_king));

  result = result || (being_solved.number_of_pieces[side_checking][Knight]>0
                      && knight_check_ortho(side_checking,sq_king));

  result = result || (being_solved.number_of_pieces[side_checking][Rook]>0
                      && is_rider_check_uninterceptable(side_checking,sq_king, vec_rook_start,vec_rook_end, Rook));

  result = result || (being_solved.number_of_pieces[side_checking][Bishop]>0
                      && is_rider_check_uninterceptable(side_checking,sq_king, vec_bishop_start,vec_bishop_end, Bishop));

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static void place_invisible_non_interceptor(slice_index si,
                                            square *pos_start,
                                            unsigned int idx,
                                            unsigned int base,
                                            unsigned int top)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",idx);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",top);
  TraceFunctionParamListEnd();

  if (idx==top)
    play_with_placed_invisibles(si);
  else
  {
    Side const side = piece_choice[idx].side;
    piece_walk_type const walk = piece_choice[idx].walk;
    SquareFlags PromSq = side==White ? WhPromSq : BlPromSq;
    SquareFlags BaseSq = side==White ? WhBaseSq : BlBaseSq;
    square *pos;

    for (pos = pos_start;
         *pos && combined_result!=previous_move_has_not_solved;
         ++pos)
      if (is_square_empty(*pos))
        if (taboo[side][*pos]==0
            && !(is_pawn(walk)
                 && (TSTFLAG(sq_spec[*pos],PromSq) || TSTFLAG(sq_spec[*pos],BaseSq))))
        {
          square const s = *pos;
          ++being_solved.number_of_pieces[side][walk];
          occupy_square(s,walk,BIT(side));

          if (idx+1==top || !is_check_uninterceptable(advers(side)))
          {
            piece_choice[idx].pos = s;

            *pos = 0;
            place_invisible_non_interceptor(si,pos_start-1,idx+1,base,top);
            *pos = s;
          }

          empty_square(s);
          --being_solved.number_of_pieces[side][walk];
        }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void walk_invisible_non_interceptor(slice_index si,
                                           unsigned int idx,
                                           unsigned int base,
                                           unsigned int top)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",idx);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",top);
  TraceFunctionParamListEnd();

  if (idx==top)
    place_invisible_non_interceptor(si,square_order_for_non_interceptors+top-base-1,base,base,top);
  else
    for (piece_choice[idx].walk = Pawn;
         piece_choice[idx].walk<=Bishop && combined_result!=previous_move_has_not_solved;
         ++piece_choice[idx].walk)
        walk_invisible_non_interceptor(si,idx+1,base,top);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void colour_invisible_non_interceptor(slice_index si,
                                             unsigned int idx,
                                             unsigned int base,
                                             unsigned int top)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",idx);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",top);
  TraceFunctionParamListEnd();

  if (idx==top)
    walk_invisible_non_interceptor(si,base,base,top);
  else
  {
    piece_choice[idx].side = Black;
    colour_invisible_non_interceptor(si,idx+1,base,top);

    if (combined_result!=previous_move_has_not_solved)
    {
      piece_choice[idx].side = White;
      colour_invisible_non_interceptor(si,idx+1,base,top);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void walk_interceptor(slice_index si,
                             unsigned int idx,
                             unsigned int base,
                             unsigned int top)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",idx);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",top);
  TraceFunctionParamListEnd();

  if (idx==top)
    colour_invisible_non_interceptor(si,top,top,total_invisible_number);
  else
  {
    square const place = piece_choice[idx].pos;
    TraceSquare(place);TraceEOL();
    if (is_square_empty(place))
    {
      for (piece_choice[idx].walk = Pawn;
           piece_choice[idx].walk<=Bishop && combined_result!=previous_move_has_not_solved;
           ++piece_choice[idx].walk)
      {
        ++being_solved.number_of_pieces[piece_choice[idx].side][piece_choice[idx].walk];
        occupy_square(place,piece_choice[idx].walk,BIT(piece_choice[idx].side));
        if (idx+1==total_invisible_number || !is_check_uninterceptable(advers(piece_choice[idx].side)))
          walk_interceptor(si,idx+1,base,top);
        empty_square(place);
        --being_solved.number_of_pieces[piece_choice[idx].side][piece_choice[idx].walk];
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void colour_interceptor(slice_index si,
                               unsigned int idx,
                               unsigned int base,
                               unsigned int top)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",idx);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",top);
  TraceFunctionParamListEnd();

  if (idx==top)
    walk_interceptor(si,base,base,top);
  else
  {
    /* remove the dummy */
    empty_square(piece_choice[idx].pos);

    if (taboo[piece_choice[idx].side][piece_choice[idx].pos]==0)
      colour_interceptor(si,idx+1,base,top);

    if (combined_result!=previous_move_has_not_solved)
    {
      piece_choice[idx].side = advers(piece_choice[idx].side);
      if (taboo[piece_choice[idx].side][piece_choice[idx].pos]==0)
        colour_interceptor(si,idx+1,base,top);
    }

    /* re-place the dummy */
    occupy_square(piece_choice[idx].pos,Dummy,BIT(White)|BIT(Black));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void deal_with_check_to_be_intercepted(ply current_ply,
                                              slice_index si,
                                              unsigned int base);

static void unwrap_move_effects(ply current_ply,
                                slice_index si,
                                unsigned int base)
{
  ply const save_nbply = nbply;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",current_ply);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  ply_replayed = nbply;

  undo_move_effects();

  nbply = parent_ply[nbply];
  deal_with_check_to_be_intercepted(current_ply,si,base);
  nbply = save_nbply;

  redo_move_effects();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void deal_with_check_to_be_intercepted_diagonal(ply current_ply,
                                                       slice_index si,
                                                       unsigned int base,
                                                       vec_index_type kanf, vec_index_type kcurr, vec_index_type kend)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",current_ply);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",kanf);
  TraceFunctionParam("%u",kcurr);
  TraceFunctionParam("%u",kend);
  TraceFunctionParamListEnd();

  if (kcurr>kend)
  {
    if (nbply==ply_retro_move)
    {
      ply const save_nbply = nbply;
      nbply = current_ply;
      colour_interceptor(si,base,base,base+nr_placed_interceptors);
      nbply = save_nbply;
    }
    else
      unwrap_move_effects(current_ply,si,base);
  }
  else
  {
    Side const side_in_check = trait[nbply];
    Side const side_checking = advers(side_in_check);
    square const king_pos = being_solved.king_square[side_in_check];

    square const sq_end = find_end_of_line(king_pos,vec[kcurr]);
    piece_walk_type const walk_at_end = get_walk_of_piece_on_square(sq_end);
    if ((walk_at_end==Bishop || walk_at_end==Queen)
        && TSTFLAG(being_solved.spec[sq_end],side_checking))
    {
      if (base+nr_placed_interceptors<total_invisible_number)
      {
        square s;
        for (s = king_pos+vec[kcurr];
             is_square_empty(s) && combined_result!=previous_move_has_not_solved;
             s += vec[kcurr])
          if (taboo[White][s]==0 || taboo[Black][s]==0)
          {
            assert(!is_rider_check_uninterceptable_on_vector(side_checking,king_pos,kcurr,walk_at_end));
            piece_choice[base+nr_placed_interceptors].pos = s;
            piece_choice[base+nr_placed_interceptors].side = side_in_check;
            TraceSquare(s);TraceEnumerator(Side,side_in_check);TraceEOL();

            /* occupy the square to avoid intercepting it again "2 half moves ago" */
            occupy_square(s,Dummy,BIT(White)|BIT(Black));
            ++nr_placed_interceptors;
            deal_with_check_to_be_intercepted_diagonal(current_ply,si,base,kanf,kcurr+1,kend);
            --nr_placed_interceptors;
            empty_square(s);
          }
      }
      else
      {
        /* there are not enough total invisibles to intercept all checks */
      }
    }
    else
      deal_with_check_to_be_intercepted_diagonal(current_ply,
                                                 si,
                                                 base,
                                                 kanf,kcurr+1,kend);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void deal_with_check_to_be_intercepted_orthogonal(ply current_ply,
                                                         slice_index si,
                                                         unsigned int base,
                                                         vec_index_type kanf, vec_index_type kcurr, vec_index_type kend)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",current_ply);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",base);
  TraceFunctionParam("%u",kanf);
  TraceFunctionParam("%u",kcurr);
  TraceFunctionParam("%u",kend);
  TraceFunctionParamListEnd();

  if (kcurr>kend)
    deal_with_check_to_be_intercepted_diagonal(current_ply,
                                               si,
                                               base,
                                               vec_bishop_start,vec_bishop_start,vec_bishop_end);
  else
  {
    Side const side_in_check = trait[nbply];
    Side const side_checking = advers(side_in_check);
    square const king_pos = being_solved.king_square[side_in_check];
    square const sq_end = find_end_of_line(king_pos,vec[kcurr]);
    piece_walk_type const walk_at_end = get_walk_of_piece_on_square(sq_end);

    TraceEnumerator(Side,side_in_check);
    TraceEnumerator(Side,side_checking);
    TraceSquare(king_pos);
    TraceSquare(sq_end);
    TraceWalk(walk_at_end);
    TraceEOL();

    if ((walk_at_end==Rook || walk_at_end==Queen)
        && TSTFLAG(being_solved.spec[sq_end],side_checking))
    {
      if (base+nr_placed_interceptors<total_invisible_number)
      {
        square s;
        for (s = king_pos+vec[kcurr];
             is_square_empty(s) && combined_result!=previous_move_has_not_solved;
             s += vec[kcurr])
          if (taboo[White][s]==0 || taboo[Black][s]==0)
          {
            assert(!is_rider_check_uninterceptable_on_vector(side_checking,king_pos,kcurr,walk_at_end));
            piece_choice[base+nr_placed_interceptors].pos = s;
            piece_choice[base+nr_placed_interceptors].side = side_in_check;
            TraceSquare(s);TraceEnumerator(Side,side_in_check);TraceEOL();

            /* occupy the square to avoid intercepting it again "2 half moves ago" */
            occupy_square(s,Dummy,BIT(White)|BIT(Black));
            ++nr_placed_interceptors;
            deal_with_check_to_be_intercepted_orthogonal(current_ply,si,base,kanf,kcurr+1,kend);
            --nr_placed_interceptors;
            empty_square(s);
          }
      }
      else
      {
        /* there are not enough total invisibles to intercept all checks */
      }
    }
    else
      deal_with_check_to_be_intercepted_orthogonal(current_ply,
                                                   si,
                                                   base,
                                                   kanf,kcurr+1,kend);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void deal_with_check_to_be_intercepted(ply current_ply,
                                              slice_index si,
                                              unsigned int base)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",current_ply);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",base);
  TraceFunctionParamListEnd();

  deal_with_check_to_be_intercepted_orthogonal(current_ply,
                                               si,
                                               base,
                                               vec_rook_start,vec_rook_start,vec_rook_end);
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void update_taboo(int delta)
{
  numecoup const curr = CURRMOVE_OF_PLY(nbply);
  move_generation_elmt const * const move_gen_top = move_generation_stack+curr;
  square const sq_capture = move_gen_top->capture;
  square const sq_departure = move_gen_top->departure;
  square const sq_arrival = move_gen_top->arrival;

  move_effect_journal_index_type const base = move_effect_journal_base[nbply];
  move_effect_journal_index_type const movement = base+move_effect_journal_index_offset_movement;
  piece_walk_type const walk = move_effect_journal[movement].u.piece_movement.moving;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%d",delta);
  TraceFunctionParamListEnd();

  taboo[White][sq_departure] += delta;
  taboo[Black][sq_departure] += delta;

  taboo[trait[nbply]][sq_arrival] += delta;

  /* en passant etc. */
  if (!is_no_capture(sq_capture))
  {
    taboo[White][sq_capture] += delta;
    taboo[Black][sq_capture] += delta;
  }

  // TODO: castling

  if (is_rider(walk))
  {
    int const diff_move = sq_arrival-sq_departure;
    int const dir_move = CheckDir[walk][diff_move];

    square s;
    assert(dir_move!=0);
    for (s = sq_departure+dir_move; s!=sq_arrival; s += dir_move)
    {
      taboo[White][s] += delta;
      taboo[Black][s] += delta;
    }
  }
  else if (is_pawn(walk))
  {
    taboo[White][sq_arrival] += delta;
    taboo[Black][sq_arrival] += delta;

    if (sq_capture==pawn_multistep)
    {
      /* outside the board if no double step */
      taboo[White][(sq_departure+sq_arrival)/2] += delta;
      taboo[Black][(sq_departure+sq_arrival)/2] += delta;
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
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
void total_invisible_move_sequence_tester_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceValue("%u",nbply-ply_retro_move);TraceEOL();

  update_taboo(+1);

  /* necessary for detecting checks by pawns and leapers */
  if (is_check_uninterceptable(trait[nbply]))
    solve_result = previous_move_is_illegal;
  else
  {
    /* make sure that our length corresponds to the length of the tested move sequence
     * (which may vary if STFindShortest is used)
     */
    assert(slices[SLICE_NEXT2(si)].type==STHelpAdapter);
    slices[SLICE_NEXT2(si)].u.branch.length = slack_length+(nbply-ply_retro_move);

    TraceValue("%u",SLICE_NEXT2(si));
    TraceValue("%u",slices[SLICE_NEXT2(si)].u.branch.length);
    TraceEOL();

    combined_result = previous_move_is_illegal;
    deal_with_check_to_be_intercepted(nbply,si,nr_placed_victims);
    solve_result = combined_result==immobility_on_next_move ? previous_move_has_not_solved : combined_result;

    if ((combined_result==previous_move_is_illegal && nr_tries>0)
        || nr_tries>10000)
    {
      printf("\n");
      move_generator_write_history();
      printf(" tri:%lu",nr_tries);
    }
    nr_tries = 0;
  }

  update_taboo(-1);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean is_move_still_playable(slice_index si)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  {
    square const sq_departure = move_generation_stack[CURRMOVE_OF_PLY(ply_replayed)].departure;
    square const sq_arrival = move_generation_stack[CURRMOVE_OF_PLY(ply_replayed)].arrival;

    TraceValue("%u",ply_replayed);
    TraceSquare(sq_departure);
    TraceSquare(sq_arrival);
    TraceSquare(move_generation_stack[CURRMOVE_OF_PLY(ply_replayed)].capture);
    TraceEOL();

    assert(TSTFLAG(being_solved.spec[sq_departure],SLICE_STARTER(si)));
    generate_moves_for_piece(sq_departure);

    {
      numecoup start = MOVEBASE_OF_PLY(nbply);
      numecoup i;
      numecoup new_top = start;
      for (i = start+1; i<=CURRMOVE_OF_PLY(nbply); ++i)
      {
        assert(move_generation_stack[i].departure==sq_departure);
        if (move_generation_stack[i].arrival==sq_arrival)
        {
          ++new_top;
          move_generation_stack[new_top] = move_generation_stack[i];
          break;
        }
      }

      SET_CURRMOVE(nbply,new_top);
    }

    result = CURRMOVE_OF_PLY(nbply)>MOVEBASE_OF_PLY(nbply);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static void copy_move_effects(void)
{
  move_effect_journal_index_type replayed_curr = move_effect_journal_base[ply_replayed];
  move_effect_journal_index_type const replayed_top = move_effect_journal_base[ply_replayed+1];
  move_effect_journal_index_type curr = move_effect_journal_base[nbply+1];

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceValue("%u",ply_replayed);
  TraceValue("%u",move_effect_journal_base[ply_replayed]);
  TraceValue("%u",move_effect_journal_base[ply_replayed+1]);
  TraceValue("%u",nbply);
  TraceEOL();

  // TODO use memmove()?
  while (replayed_curr!=replayed_top)
  {
    move_effect_journal[curr] = move_effect_journal[replayed_curr];
    ++replayed_curr;
    ++curr;
  }

  move_effect_journal_base[nbply+1] = curr;

  TraceValue("%u",move_effect_journal_base[nbply]);
  TraceValue("%u",move_effect_journal_base[nbply+1]);
  TraceEOL();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
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
void total_invisible_move_repeater_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  nextply(SLICE_STARTER(si));

  assert(is_move_still_playable(si));

  /* With the current placement algorithm, the move is always playable */

  /*if (is_move_still_playable(si))*/
  {
    copy_move_effects();
    redo_move_effects();
    ++ply_replayed;
    pipe_solve_delegate(si);
    --ply_replayed;
    undo_move_effects();
  }
  /*else
    solve_result = previous_move_is_illegal;*/

  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
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
void total_invisible_uninterceptable_selfcheck_guard_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  update_taboo(+1);

  if (is_check_uninterceptable(trait[nbply]))
    solve_result = previous_move_is_illegal;
  else
    pipe_solve_delegate(si);

  update_taboo(-1);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
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
void total_invisible_goal_guard_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (is_in_check(advers(SLICE_STARTER(si))))
    solve_result = previous_move_is_illegal;
  else
  {
    /* make sure that we don't generate pawn captures total invisible */
    assert(play_phase==replaying_moves);
    pipe_solve_delegate(si);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Generate moves for a single piece
 * @param identifies generator slice
 */
void total_invisible_pawn_generate_pawn_captures(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  pipe_move_generation_delegate(si);

  if (play_phase==regular_play)
  {
    square const sq_departure = curr_generation->departure ;
    Side const side = trait[nbply];
    if (TSTFLAG(being_solved.spec[sq_departure],side) && being_solved.board[sq_departure]==Pawn)
    {
      int const dir_vertical = trait[nbply]==White ? dir_up : dir_down;

      curr_generation->arrival = curr_generation->departure+dir_vertical+dir_left;
      if (is_square_empty(curr_generation->arrival))
        push_move_capture_extra(curr_generation->arrival);

      curr_generation->arrival = curr_generation->departure+dir_vertical+dir_right;
      if (is_square_empty(curr_generation->arrival))
        push_move_capture_extra(curr_generation->arrival);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
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
void total_invisible_special_moves_player_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    numecoup const curr = CURRMOVE_OF_PLY(nbply);
    move_generation_elmt * const move_gen_top = move_generation_stack+curr;
    square const sq_capture = move_gen_top->capture;
    Side const side_victim = advers(SLICE_STARTER(si));

    if (!is_no_capture(sq_capture) && is_square_empty(sq_capture))
    {
      /* sneak the creation of a dummy piece into the previous ply - very dirty...,
       * but this prevents the dummy from being un-created when we undo this move's
       * effects
       */
      move_effect_journal_do_piece_creation(move_effect_reason_removal_of_invisible,
                                            sq_capture,
                                            Dummy,
                                            BIT(side_victim),
                                            side_victim);
      ++move_effect_journal_base[nbply];

      piece_choice[nr_placed_victims].pos = sq_capture;

      ++nr_placed_victims;
      pipe_solve_delegate(si);
      --nr_placed_victims;

      --move_effect_journal_base[nbply];
    }
    else
      pipe_solve_delegate(si);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void subsitute_generator(slice_index si,
                                stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%p",st);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(si,st);

  {
    slice_index const prototype = alloc_pipe(STTotalInvisibleMoveSequenceMoveRepeater);
    SLICE_STARTER(prototype) = SLICE_STARTER(si);
    slice_insertion_insert_contextually(si,st->context,&prototype,1);
  }

  pipe_remove(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remove_the_pipe(slice_index si,
                             stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);
  pipe_remove(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remember_self_check_guard(slice_index si,
                                      stip_structure_traversal *st)
{
  slice_index * const remembered = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  *remembered = si;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void subsitute_goal_guard(slice_index si,
                                 stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(si,st);

  {
    slice_index remembered = no_slice;

    stip_structure_traversal st_nested;
    stip_structure_traversal_init_nested(&st_nested,st,&remembered);
    stip_structure_traversal_override_single(&st_nested,
                                             STSelfCheckGuard,
                                             &remember_self_check_guard);
    stip_traverse_structure(SLICE_NEXT2(si),&st_nested);

    if (remembered!=no_slice)
    {
      /* self check is impossible with the current optimisations for orthodox pieces
      slice_index prototype = alloc_pipe(STTotalInvisibleGoalGuard);
      SLICE_STARTER(prototype) = SLICE_STARTER(remembered);
      goal_branch_insert_slices(SLICE_NEXT2(si),&prototype,1); */
      pipe_remove(remembered);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void instrument_replay_branch(slice_index si,
                                     stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    stip_structure_traversal st_nested;

    stip_structure_traversal_init_nested(&st_nested,st,0);
    // TODO prevent instrumentation in the first place?
    stip_structure_traversal_override_single(&st_nested,
                                             STFindShortest,
                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STFindAttack,
                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STMoveEffectJournalUndoer,
                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STPostMoveIterationInitialiser,
                                             &remove_the_pipe);
    // TODO like this, this would cause a slice leak (STCastlingPlayer is a fork type!)
//    stip_structure_traversal_override_single(&st_nested,
//                                             STCastlingPlayer,
//                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STMovePlayer,
                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STPawnPromoter,
                                             &remove_the_pipe);
    stip_structure_traversal_override_single(&st_nested,
                                             STMoveGenerator,
                                             &subsitute_generator);
    stip_structure_traversal_override_single(&st_nested,
                                             STGoalReachedTester,
                                             &subsitute_goal_guard);
    /* self check is impossible with the current optimisations for orthodox pieces */
    stip_structure_traversal_override_single(&st_nested,
                                             STSelfCheckGuard,
                                             &remove_the_pipe);
    stip_traverse_structure(si,&st_nested);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remove_self_check_guard(slice_index si,
                                    stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  /* This iteration ends at STTotalInvisibleMoveSequenceTester. We can therefore
   * blindly tamper with all STSelfCheckGuard slices that we meet.
   */
  stip_traverse_structure_children_pipe(si,st);

  if (st->context==stip_traversal_context_intro)
    pipe_remove(si);
  else
    SLICE_TYPE(si) = STTotalInvisibleUninterceptableSelfCheckGuard;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static int square_compare_around_both_kings(void const *v1, void const *v2)
{
  int result;
  square const *s1 = v1;
  square const *s2 = v2;
  square const kpos = being_solved.king_square[Black];

  result = move_diff_code[abs(kpos-*s1)]-move_diff_code[abs(kpos-*s2)];

  if (being_solved.king_square[White]!=initsquare)
  {
    square const kpos = being_solved.king_square[White];
    result += move_diff_code[abs(kpos-*s1)]-move_diff_code[abs(kpos-*s2)];
  }

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
void total_invisible_instrumenter_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    stip_structure_traversal st;

    stip_structure_traversal_init(&st,0);
    stip_structure_traversal_override_single(&st,
                                             STTotalInvisibleMoveSequenceTester,
                                             &instrument_replay_branch);
    stip_structure_traversal_override_single(&st,
                                             STSelfCheckGuard,
                                             &remove_self_check_guard);
    stip_traverse_structure(si,&st);
  }

  TraceStipulation(si);

  output_plaintext_check_indication_disabled = true;

  memmove(square_order_for_non_interceptors, boardnum, sizeof boardnum);
  qsort(square_order_for_non_interceptors, 64, sizeof square_order_for_non_interceptors[0], &square_compare_around_both_kings);

  solving_instrument_move_generation(si,nr_sides,STTotalInvisiblePawnCaptureGenerator);

  pipe_solve_delegate(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

typedef struct
{
    boolean instrumenting;
    slice_index the_copy;
    stip_length_type length;
} insertion_state_type;

static void insert_copy(slice_index si,
                        stip_structure_traversal *st)
{
  insertion_state_type * const state = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (state->the_copy==no_slice)
    stip_traverse_structure_children(si,st);
  else
  {
    slice_index const proxy = alloc_proxy_slice();
    slice_index const substitute = alloc_pipe(STTotalInvisibleMoveSequenceTester);
    pipe_link(proxy,substitute);
    link_to_branch(substitute,state->the_copy);
    SLICE_NEXT2(substitute) = state->the_copy;
    state->the_copy = no_slice;
    dealloc_slices(SLICE_NEXT2(si));
    SLICE_NEXT2(si) = proxy;

    assert(state->length!=no_stip_length);
    if (state->length%2!=0)
      pipe_append(proxy,alloc_pipe(STMoveInverter));
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void copy_help_branch(slice_index si,
                             stip_structure_traversal *st)
{
  insertion_state_type * const state = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceValue("%u",state->instrumenting);
  TraceEOL();

  state->length = slices[si].u.branch.length;

  if (state->instrumenting)
    stip_traverse_structure_children(si,st);
  else
  {
    state->instrumenting = true;
    state->the_copy = stip_deep_copy(si);
    stip_traverse_structure_children(si,st);
    assert(state->the_copy==no_slice);

    {
      slice_index const prototypes[] = {
          alloc_pipe(STTotalInvisibleSpecialMovesPlayer),
          alloc_pipe(STTotalInvisibleSpecialMovesPlayer)
      };
      enum { nr_protypes = sizeof prototypes / sizeof prototypes[0] };
      slice_insertion_insert(si,prototypes,nr_protypes);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Instrument the solvers with support for Total Invisible pieces
 * @param si identifies the root slice of the stipulation
 */
void solving_instrument_total_invisible(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceStipulation(si);

  // later:
  // - in original
  //   - insert revelation logic
  // - in copy
  //   - logic for iteration over all possibilities of invisibles
  //     - special case of invisible king
  //     - special case: position has no legal placement of all invisibles may have to be dealt with:
  //       - not enough empty squares :-)
  //   - substitute for STFindShortest

  // bail out at STAttackAdapter

  // input for total_invisible_number, initialize to 0

  // what about:
  // - structured stipulations?
  // - goals that don't involve immobility
  // ?

  // we shouldn't need to set the starter of
  // - STTotalInvisibleMoveSequenceMoveRepeater
  // - STTotalInvisibleGoalGuard

  // check indication should also be deactivated in tree output

  {
    slice_index const prototype = alloc_pipe(STTotalInvisibleInstrumenter);
    slice_insertion_insert(si,&prototype,1);
  }

  {
    stip_structure_traversal st;
    insertion_state_type state = { false, no_slice, no_stip_length };

    stip_structure_traversal_init(&st,&state);
    stip_structure_traversal_override_single(&st,
                                             STHelpAdapter,
                                             &copy_help_branch);
    stip_structure_traversal_override_single(&st,
                                             STGoalReachedTester,
                                             &insert_copy);
    stip_traverse_structure(si,&st);
  }

  TraceStipulation(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
