#if !defined(CONDITIONS_ANNAN_H)
#define CONDITIONS_ANNAN_H

#include "py.h"
#include "position/board.h"
#include "utilities/boolean.h"
#include "pyproc.h"

/* This module implements the condition Annan Chess */

typedef enum
{
  annan_type_A,
  annan_type_B,
  annan_type_C,
  annan_type_D
} annan_type_type;

extern annan_type_type annan_type;

/* Determine whether a piece annanises another
 * @param side side of potentially annanised piece
 * @param rear potential annaniser
 * @param front potential annanisee
 */
boolean annanises(Side side, square rear, square front);

boolean annan_is_square_attacked(Side side_in_check,
                                 square sq_target,
                                 evalfunction_t *evaluate);

#endif
