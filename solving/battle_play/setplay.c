#include "solving/battle_play/setplay.h"
#include "solving/battle_play/try.h"
#include "trace.h"

static void filter_output_mode(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (slices[si].u.output_mode_selector.mode==output_mode_tree)
    stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void insert_setplay_solvers_defense_adapter(slice_index si,
                                               stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (st->level==structure_traversal_level_setplay
      && slices[si].u.branch.length>slack_length_battle)
  {
    unsigned int const max_nr_refutations = UINT_MAX;
    branch_insert_try_solvers(si,max_nr_refutations);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitors const setplay_solver_inserters[] =
{
  { STOutputModeSelector, &filter_output_mode                     },
  { STDefenseAdapter,     &insert_setplay_solvers_defense_adapter }
};

enum
{
  nr_setplay_solver_inserters = sizeof setplay_solver_inserters / sizeof setplay_solver_inserters[0]
};

/* Instrument the stipulation structure with slices solving set play
 * @param root_slice root slice of the stipulation
 */
void stip_insert_setplay_solvers(slice_index si)
{
  stip_structure_traversal st;
  output_mode mode = output_mode_none;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,&mode);
  stip_structure_traversal_override(&st,setplay_solver_inserters,nr_setplay_solver_inserters);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}