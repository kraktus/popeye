#if !defined(PIECES_ATTRIBUTES_TOTAL_INVISIBLE_REVELATIONS_H)
#define PIECES_ATTRIBUTES_TOTAL_INVISIBLE_REVELATIONS_H

#include "position/position.h"
#include "pieces/attributes/total_invisible.h"
#include "solving/move_effect_journal.h"

#include <limits.h>

typedef struct
{
    square first_on;
    piece_walk_type walk;
    Flags spec;
    action_type first;
    action_type last;
} revelation_status_type;

typedef unsigned int decision_level_type;

enum
{
  decision_level_uninitialised = 0,
  decision_level_forever = 1,
  decision_level_latest = UINT_MAX
};

typedef struct
{
    decision_level_type side;
    decision_level_type walk;
    decision_level_type from;
    decision_level_type to;
} decision_levels_type;

// TODO this doesn't belong here
typedef struct
{
    action_type first;
    action_type last;
    decision_levels_type levels;
} motivation_type;
extern motivation_type motivation[MaxPieceId+1];

extern boolean revelation_status_is_uninitialised;
extern unsigned int nr_potential_revelations;
extern revelation_status_type revelation_status[nr_squares_on_board];
extern decision_level_type curr_decision_level;
extern decision_level_type max_decision_level;
extern move_effect_journal_index_type top_before_relevations[maxply+1];

void reveal_new(move_effect_journal_entry_type const *entry);
void unreveal_new(move_effect_journal_entry_type const *entry);

void reveal_placed(move_effect_journal_entry_type const *entry);
void unreveal_placed(move_effect_journal_entry_type const *entry);

void undo_revelation_of_new_invisible(move_effect_journal_entry_type const *entry);
void redo_revelation_of_new_invisible(move_effect_journal_entry_type const *entry);

void undo_revelation_of_castling_partner(move_effect_journal_entry_type const *entry);
void redo_revelation_of_castling_partner(move_effect_journal_entry_type const *entry);

void adapt_id_of_existing_to_revealed(move_effect_journal_entry_type const *entry);
void unadapt_id_of_existing_to_revealed(move_effect_journal_entry_type const *entry);

void undo_revelation_of_placed_invisible(move_effect_journal_entry_type const *entry);
void redo_revelation_of_placed_invisible(move_effect_journal_entry_type const *entry);

void replace_moving_piece_ids_in_past_moves(PieceIdType from, PieceIdType to, ply up_to_ply);

void setup_revelations(void);
void initialise_revelations(void);
void update_revelations(void);
void evaluate_revelations(void);

void do_revelation_bookkeeping(void);

void undo_revelation_effects(move_effect_journal_index_type curr);

#endif
