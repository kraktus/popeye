/********************* MODIFICATIONS to pyhash.c ***********************
 **
 ** Date       Who  What
 **
 ** 2003/05/12 TLi  hashing bug fixed: h= + intel did not find all solutions .
 **
 ** 2004/03/22 TLi  hashing for exact-* stipulations improved
 **
 ** 2005/02/01 TLi  function hashdefense is not used anymore...
 **
 ** 2005/02/01 TLi  in branch_d_does_attacker_win and invref exchanged the inquiry into the hash
 **                 table for "white can mate" and "white cannot mate" because
 **                 it is more likely that a position has no solution
 **                 This yields an incredible speedup of .5-1%  *fg*
 **
 ** 2006/06/30 SE   New condition: BGL (invented P.Petkov)
 **
 ** 2008/02/10 SE   New condition: Cheameleon Pursuit (invented? : L.Grolman)
 **
 ** 2009/01/03 SE   New condition: Disparate Chess (invented: R.Bedoni)
 **
 **************************** End of List ******************************/

/**********************************************************************
 ** We hash.
 **
 ** SmallEncode and LargeEncode are functions to encode the current
 ** position. SmallEncode is used when the starting position contains
 ** less than or equal to eight pieces. LargeEncode is used when more
 ** pieces are present. The function TellSmallEncode and TellLargeEncode
 ** are used to decide what encoding to use. Which function to use is
 ** stored in encode.
 ** SmallEncode encodes for each piece the square where it is located.
 ** LargeEncode uses the old scheme introduced by Torsten: eight
 ** bytes at the beginning give in 64 bits the locations of the pieces
 ** coded after the eight bytes. Both functions give for each piece its
 ** type (1 byte) and specification (2 bytes). After this information
 ** about ep-captures, Duellants and Imitators are coded.
 **
 ** The hash table uses a dynamic hashing scheme which allows dynamic
 ** growth and shrinkage of the hashtable. See the relevant dht* files
 ** for more details. Two procedures are used:
 **   dhtLookupElement: This procedure delivers
 ** a nil pointer, when the given position is not in the hashtable,
 ** or a pointer to a hashelement.
 **   dhtEnterElement:  This procedure enters an encoded position
 ** with its values into the hashtable.
 **
 ** When there is no more memory, or more than MaxPositions positions
 ** are stored in the hash-table, then some positions are removed
 ** from the table. This is done in the compress procedure.
 ** This procedure uses a little improved scheme introduced by Torsten.
 ** The selection of positions to remove is based on the value of
 ** information gathered about this position. The information about
 ** a position "unsolvable in 2 moves" is less valuable than "unsolvable
 ** in 5 moves", since the former can be recomputed faster. For the other
 ** type of information ("solvable") the comparison is the other way round.
 ** The compression of the table is an expensive operation, in a lot
 ** of exeperiments it has shown to be quite effective in keeping the
 ** most valuable information, and speeds up the computation time
 ** considerably. But to be of any use, there must be enough memory to
 ** to store more than 800 positions.
 ** Now Torsten changed popeye so that all stipulations use hashing.
 ** There seems to be no real penalty in using hashing, even if the
 ** hit ratio is very small and only about 5%, it speeds up the
 ** computation time by 30%.
 ** I changed the output of hashstat, since its really informative
 ** to see the hit rate.
 **
 ** inithash()
 **   -- enters the startposition into the hash-table.
 **   -- determines which encode procedure to use
 **   -- Check's for the MaxPostion/MaxMemory settings
 **
 ** closehash()
 **   -- deletes the hashtable and gives back allocated storage.
 **
 ***********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/* TurboC and BorlandC  TLi */
#if defined(__TURBOC__)
# include <mem.h>
# include <alloc.h>
# include <conio.h>
#else
# include <memory.h>
#endif  /* __TURBOC__ */

#include "py.h"
#include "pyproc.h"
#include "pydata.h"
#include "pymsg.h"
#include "optimisations/hash.h"
#include "DHT/dhtvalue.h"
#include "DHT/dht.h"
#include "pyproof.h"
#include "pystip.h"
#include "stipulation/battle_play/branch.h"
#include "stipulation/help_play/branch.h"
#include "solving/solving.h"
#include "solving/avoid_unsolvable.h"
#include "conditions/bgl.h"
#include "options/nontrivial.h"
#include "stipulation/branch.h"
#include "pypipe.h"
#include "platform/maxtime.h"
#include "platform/maxmem.h"
#include "debugging/trace.h"

typedef unsigned int hash_value_type;

static struct dht *pyhash;

static char    piece_nbr[PieceCount];
static boolean one_byte_hash;
static unsigned int bytes_per_spec;
static unsigned int bytes_per_piece;

/* Minimal value of a hash table element.
 * compresshash() will remove all elements with a value less than
 * minimalElementValueAfterCompression, and increase
 * minimalElementValueAfterCompression if necessary.
 */
static hash_value_type minimalElementValueAfterCompression;


/* Container of indices of hash slices
 */
static unsigned int nr_hash_slices;
static slice_index hash_slices[max_nr_slices];


HashBuffer hashBuffers[maxply+1];

stip_length_type hashBufferValidity[maxply+1];

enum
{
  HASHBUFFER_INVALID = 0
};

void validateHashBuffer(stip_length_type validity_value)
{
  hashBufferValidity[nbply] = validity_value;
}

void invalidateHashBuffer(void)
{
  hashBufferValidity[nbply] = HASHBUFFER_INVALID;
}

#if defined(TESTHASH)
#define ifTESTHASH(x)   x
#if defined(__unix)
#include <unistd.h>
static void *OldBreak;
extern int dhtDebug;
#endif /*__unix*/
#else
#define ifTESTHASH(x)
#endif /*TESTHASH*/

#if defined(HASHRATE)
#define ifHASHRATE(x)   x
static unsigned long use_pos, use_all;
#else
#define ifHASHRATE(x)
#endif /*HASHRATE*/

/* New Version for more ply's */
enum
{
  ByteMask = (1u<<CHAR_BIT)-1,
  BitsForPly = 10      /* Up to 1023 ply possible */
};

static void (*encode)(stip_length_type validity_value);

typedef unsigned int data_type;

/* hash table element type defining the data member as we use it in
 * this module
 */
typedef struct
{
	dhtValue Key;
    data_type data;
} element_t;

/* Grand union of "element" type and the generic one used by the hash
 * table implementation.
 * Using this union type rather than casting frm dhtElement * to
 * element_t * avoids aliasing issues.
 */
typedef union
{
    dhtElement d;
    element_t e;
} hashElement_union_t;

/* Hashing properties of stipulation slices
 */
typedef struct
{
    unsigned int size;
    unsigned int valueOffset;

    union
    {
        struct
        {
            unsigned int offsetSucc;
            unsigned int maskSucc;
            unsigned int offsetNoSucc;
            unsigned int maskNoSucc;
        } d;
        struct
        {
            unsigned int offsetNoSucc;
            unsigned int maskNoSucc;
        } h;
    } u;
} slice_properties_t;

static slice_properties_t slice_properties[max_nr_slices];

/* Determine the number of bits necessary to represent a range of numbers
 * @param value maximum value of the range
 * @return number of bits necessary to represent the numbers
 *         from 0 to value (inclusive)
 */
static unsigned int bit_width(unsigned int value)
{
  unsigned int result = 0;

  while (value!=0)
  {
    ++result;
    value /= 2;
  }

  return result;
}

typedef struct
{
    unsigned int nrBitsLeft;
    unsigned int valueOffset;
} slice_initializer_state;

/* Initialise the slice_properties array according to a subtree of the
 * current stipulation slices whose root is a pipe for which we don't
 * have a more specialised function
 * @param leaf root slice of subtree
 * @param st address of structure defining traversal
 */
static void init_slice_properties_pipe(slice_index pipe,
                                       stip_structure_traversal *st)
{
  slice_index const next = slices[pipe].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",pipe);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(pipe,st);
  slice_properties[pipe].valueOffset = slice_properties[next].valueOffset;
  TraceValue("%u",pipe);
  TraceValue("%u\n",slice_properties[pipe].valueOffset);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Shift the value offset of one slice, then continue the traversal
 * @param si identifies the slice currently traversed
 * @param st points to the structure holding the traversal state
 */
static void slice_property_offset_shifter(slice_index si,
                                          stip_structure_traversal *st)
{
  unsigned int const * const delta = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  slice_properties[si].valueOffset -= *delta;

  TraceValue("%u",*delta);
  TraceValue("->%u\n",slice_properties[si].valueOffset);

  stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Shift the value offsets of slices reachable from a particular slice
 * @param si identifies slice
 * @param delta indicates how much to shift the value offsets
 */
static void shift_offsets(slice_index si, unsigned int delta)
{
  unsigned int i;
  stip_structure_traversal st;

  stip_structure_traversal_init(&st,&delta);
  for (i = 0; i!=nr_slice_structure_types; ++i)
    stip_structure_traversal_override_by_structure(&st,
                                                   i,
                                                   &slice_property_offset_shifter);
  stip_traverse_structure(si,&st);
}

/* Initialise the slice_properties array according to a subtree of the
 * current stipulation slices whose root is a fork
 * @param si root slice of subtree
 * @param st address of structure defining traversal
 */
static void init_slice_properties_binary(slice_index fork,
                                         stip_structure_traversal *st)
{
  slice_initializer_state * const sis = st->param;

  unsigned int const save_valueOffset = sis->valueOffset;

  slice_index const op1 = slices[fork].u.binary.op1;
  slice_index const op2 = slices[fork].u.binary.op2;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",fork);
  TraceFunctionParamListEnd();

  slice_properties[fork].valueOffset = sis->valueOffset;

  stip_traverse_structure(op1,st);
  sis->valueOffset = save_valueOffset;
  stip_traverse_structure(op2,st);

  TraceValue("%u",op1);
  TraceValue("%u",slice_properties[op1].valueOffset);
  TraceValue("%u",op2);
  TraceValue("%u\n",slice_properties[op2].valueOffset);

  /* both operand slices must have the same valueOffset, or the
   * shorter one will dominate the longer one */
  if (slice_properties[op1].valueOffset>slice_properties[op2].valueOffset)
  {
    unsigned int const delta = (slice_properties[op1].valueOffset
                                -slice_properties[op2].valueOffset);
    shift_offsets(op1,delta);
  }
  else if (slice_properties[op2].valueOffset>slice_properties[op1].valueOffset)
  {
    unsigned int const delta = (slice_properties[op2].valueOffset
                                -slice_properties[op1].valueOffset);
    shift_offsets(op2,delta);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Initialise the slice_properties array
 * @param si root slice of subtree
 * @param st address of structure defining traversal
 */
static void init_slice_properties_attack_hashed(slice_index si,
                                                stip_structure_traversal *st)
{
  slice_initializer_state * const sis = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;
  unsigned int const size = bit_width((length-min_length+1)/2);
  data_type const mask = (1<<size)-1;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceValue("%u",length);
  TraceValue("%u",sis->nrBitsLeft);
  TraceValue("%u\n",sis->valueOffset);

  sis->valueOffset -= size;
  TraceValue("%u",size);
  TraceValue("->%u\n",sis->valueOffset);

  slice_properties[si].size = size;
  slice_properties[si].valueOffset = sis->valueOffset;
  TraceValue("%u\n",slice_properties[si].valueOffset);

  assert(sis->nrBitsLeft>=size);
  sis->nrBitsLeft -= size;
  slice_properties[si].u.d.offsetNoSucc = sis->nrBitsLeft;
  slice_properties[si].u.d.maskNoSucc = mask << sis->nrBitsLeft;

  assert(sis->nrBitsLeft>=size);
  sis->nrBitsLeft -= size;
  slice_properties[si].u.d.offsetSucc = sis->nrBitsLeft;
  slice_properties[si].u.d.maskSucc = mask << sis->nrBitsLeft;

  hash_slices[nr_hash_slices++] = si;
  stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Initialise a slice_properties element representing help play
 * @param si root slice of subtree
 * @param length number of half moves of help slice
 * @param sis state of slice properties initialisation
 */
static void init_slice_property_help(slice_index si,
                                     slice_initializer_state *sis)
{
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;
  unsigned int const size = bit_width((length-min_length+1)/2+1);
  data_type const mask = (1<<size)-1;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  slice_properties[si].size = size;
  slice_properties[si].valueOffset = sis->valueOffset;
  TraceValue("%u",size);
  TraceValue("%08x",mask);
  TraceValue("%u\n",slice_properties[si].valueOffset);

  assert(sis->nrBitsLeft>=size);
  sis->nrBitsLeft -= size;
  slice_properties[si].u.h.offsetNoSucc = sis->nrBitsLeft;
  slice_properties[si].u.h.maskNoSucc = mask << sis->nrBitsLeft;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Initialise the slice_properties array
 * @param si root slice of subtree
 * @param st address of structure defining traversal
 */
static void init_slice_properties_hashed_help(slice_index si,
                                              stip_structure_traversal *st)
{
  slice_initializer_state * const sis = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    slice_index const sibling = branch_find_slice(STHelpHashed,si);

    stip_length_type const length = slices[si].u.branch.length;
    unsigned int const width = bit_width((length-slack_length+1)/2);

    sis->valueOffset -= width;

    if (sibling!=no_slice
        && slices[sibling].u.branch.length>slack_length
        && get_stip_structure_traversal_state(sibling,st)==slice_not_traversed)
    {
      assert(sibling!=si);

      /* 1 bit more because we have two slices whose values are added
       * for computing the value of this branch */
      --sis->valueOffset;

      stip_traverse_structure(sibling,st);
    }

    init_slice_property_help(si,sis);
  }

  stip_traverse_structure_children(si,st);

  hash_slices[nr_hash_slices++] = si;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitors slice_properties_initalisers[] =
{
  { STAnd,          &init_slice_properties_binary        },
  { STOr,           &init_slice_properties_binary        },
  { STAttackHashed, &init_slice_properties_attack_hashed },
  { STHelpHashed,   &init_slice_properties_hashed_help   }
};

enum
{
  nr_slice_properties_initalisers = (sizeof slice_properties_initalisers
                                     / sizeof slice_properties_initalisers[0])
};

/* Reduce the value offsets for the hash slices to the minimal
 * possible value. This is important in order for
 * minimalElementValueAfterCompression not to grow too high in
 * compresshash().
 */
static void minimiseValueOffset(void)
{
  unsigned int minimalValueOffset = sizeof(data_type)*CHAR_BIT;
  unsigned int i;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  for (i = 0; i<nr_hash_slices; ++i)
  {
    slice_index const si = hash_slices[i];
    unsigned int const valueOffset = slice_properties[si].valueOffset;
    if (valueOffset<minimalValueOffset)
      minimalValueOffset = valueOffset;
  }

  TraceValue("%u\n",minimalValueOffset);

  for (i = 0; i<nr_hash_slices; ++i)
  {
    slice_index const si = hash_slices[i];
    slice_properties[si].valueOffset -= minimalValueOffset;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Initialise the slice_properties array according to the current
 * stipulation slices.
 */
static void init_slice_properties(slice_index si)
{
  stip_structure_traversal st;
  slice_initializer_state sis = {
    sizeof(data_type)*CHAR_BIT,
    sizeof(data_type)*CHAR_BIT
  };

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  nr_hash_slices = 0;

  stip_structure_traversal_init(&st,&sis);
  stip_structure_traversal_override_by_structure(&st,
                                            slice_structure_pipe,
                                            &init_slice_properties_pipe);
  stip_structure_traversal_override_by_function(&st,
                                                slice_function_testing_pipe,
                                                &init_slice_properties_pipe);
  stip_structure_traversal_override(&st,
                                    slice_properties_initalisers,
                                    nr_slice_properties_initalisers);
  stip_traverse_structure(si,&st);

  minimiseValueOffset();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


/* Pseudo hash table element - template for fast initialization of
 * newly created actual table elements
 */
static hashElement_union_t template_element;


static void set_value_attack_nosuccess(hashElement_union_t *hue,
                                       slice_index si,
                                       hash_value_type val)
{
  unsigned int const offset = slice_properties[si].u.d.offsetNoSucc;
  unsigned int const bits = val << offset;
  unsigned int const mask = slice_properties[si].u.d.maskNoSucc;
  element_t * const e = &hue->e;
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",val);
  TraceFunctionParamListEnd();
  TraceValue("%u",slice_properties[si].size);
  TraceValue("%u",offset);
  TraceValue("%08x ",mask);
  TracePointerValue("%p ",&e->data);
  TraceValue("pre:%08x ",e->data);
  TraceValue("%08x\n",bits);
  assert((bits&mask)==bits);
  e->data &= ~mask;
  e->data |= bits;
  TraceValue("post:%08x\n",e->data);
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void set_value_attack_success(hashElement_union_t *hue,
                                     slice_index si,
                                     hash_value_type val)
{
  unsigned int const offset = slice_properties[si].u.d.offsetSucc;
  unsigned int const bits = val << offset;
  unsigned int const mask = slice_properties[si].u.d.maskSucc;
  element_t * const e = &hue->e;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",val);
  TraceFunctionParamListEnd();

  TraceValue("%u",slice_properties[si].size);
  TraceValue("%u",offset);
  TraceValue("%08x ",mask);
  TracePointerValue("%p ",&e->data);
  TraceValue("pre:%08x ",e->data);
  TraceValue("%08x\n",bits);
  assert((bits&mask)==bits);
  e->data &= ~mask;
  e->data |= bits;
  TraceValue("post:%08x\n",e->data);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void set_value_help(hashElement_union_t *hue,
                           slice_index si,
                           hash_value_type val)
{
  unsigned int const offset = slice_properties[si].u.h.offsetNoSucc;
  unsigned int const bits = val << offset;
  unsigned int const mask = slice_properties[si].u.h.maskNoSucc;
  element_t * const e = &hue->e;
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",val);
  TraceFunctionParamListEnd();
  TraceValue("%u",slice_properties[si].size);
  TraceValue("%u",offset);
  TraceValue("0x%08x ",mask);
  TraceValue("0x%08x ",&e->data);
  TraceValue("pre:0x%08x ",e->data);
  TraceValue("0x%08x\n",bits);
  assert((bits&mask)==bits);
  e->data &= ~mask;
  e->data |= bits;
  TraceValue("post:0x%08x\n",e->data);
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static hash_value_type get_value_attack_success(hashElement_union_t const *hue,
                                                slice_index si)
{
  unsigned int const offset = slice_properties[si].u.d.offsetSucc;
  unsigned int const mask = slice_properties[si].u.d.maskSucc;
  element_t const * const e = &hue->e;
  data_type const result = (e->data & mask) >> offset;
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceValue("%08x ",mask);
  TracePointerValue("%p ",&e->data);
  TraceValue("%08x\n",e->data);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static hash_value_type get_value_attack_nosuccess(hashElement_union_t const *hue,
                                                  slice_index si)
{
  unsigned int const offset = slice_properties[si].u.d.offsetNoSucc;
  unsigned int const mask = slice_properties[si].u.d.maskNoSucc;
  element_t const * const e = &hue->e;
  data_type const result = (e->data & mask) >> offset;
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceValue("%08x ",mask);
  TracePointerValue("%p ",&e->data);
  TraceValue("%08x\n",e->data);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static hash_value_type get_value_help(hashElement_union_t const *hue,
                                      slice_index si)
{
  unsigned int const offset = slice_properties[si].u.h.offsetNoSucc;
  unsigned int const  mask = slice_properties[si].u.h.maskNoSucc;
  element_t const * const e = &hue->e;
  data_type const result = (e->data & mask) >> offset;
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceValue("%u",offset);
  TraceValue("0x%08x ",mask);
  TraceValue("0x%08x ",&e->data);
  TraceValue("0x%08x\n",e->data);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine the contribution of an attacking move to the value of a
 * hash table element node.
 * @param he address of hash table element to determine value of
 * @param si slice index of slice
 * @return value of contribution of slice si to *he's value
 */
static hash_value_type own_value_of_data_attack(hashElement_union_t const *hue,
                                                slice_index si)
{
  stip_length_type const length = slices[si].u.branch.length;
  hash_value_type result;
  hash_value_type success;
  hash_value_type nosuccess;
  hash_value_type success_neg;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%p",hue);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  success = get_value_attack_success(hue,si);
  nosuccess = get_value_attack_nosuccess(hue,si);

  assert(success<=length);
  success_neg = length-success;

  result = success_neg>nosuccess ? success_neg : nosuccess;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine the contribution of a help slice (or leaf slice with help
 * end) to the value of a hash table element node.
 * @param he address of hash table element to determine value of
 * @param si slice index of help slice
 * @return value of contribution of slice si to *he's value
 */
static hash_value_type own_value_of_data_help(hashElement_union_t const *hue,
                                              slice_index si)
{
  hash_value_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%p",hue);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = get_value_help(hue,si);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine the contribution of a slice to the value of a hash table
 * element node.
 * @param he address of hash table element to determine value of
 * @param si slice index
 * @return value of contribuation of the slice to *he's value
 */
static hash_value_type value_of_data_from_slice(hashElement_union_t const *hue,
                                                slice_index si)
{
  hash_value_type result;
  unsigned int const offset = slice_properties[si].valueOffset;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%p ",hue);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceEnumerator(slice_type,slices[si].type," ");
  TraceValue("%u\n",slice_properties[si].valueOffset);

  switch (slices[si].type)
  {
    case STAttackHashed:
      result = own_value_of_data_attack(hue,si) << offset;
      break;

    case STHelpHashed:
      result = own_value_of_data_help(hue,si) << offset;
      break;

    default:
      assert(0);
      result = 0;
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%08x",result);
  TraceFunctionResultEnd();
  return result;
}

/* How much is element *he worth to us? This information is used to
 * determine which elements to discard from the hash table if it has
 * reached its capacity.
 * @param he address of hash table element to determine value of
 * @return value of *he
 */
static hash_value_type value_of_data(hashElement_union_t const *hue)
{
  hash_value_type result = 0;
  unsigned int i;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%p",hue);
  TraceFunctionParamListEnd();

  for (i = 0; i<nr_hash_slices; ++i)
    result += value_of_data_from_slice(hue,hash_slices[i]);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%08x",result);
  TraceFunctionResultEnd();
  return result;
}

#if defined(TESTHASH)
static unsigned long totalRemoveCount = 0;
#endif

static void compresshash (void)
{
  dhtElement const *he;
  unsigned long targetKeyCount;
#if defined(TESTHASH)
  unsigned long RemoveCnt = 0;
  unsigned long initCnt;
  unsigned long visitCnt;
  unsigned long runCnt;
#endif

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  targetKeyCount = dhtKeyCount(pyhash);
  targetKeyCount -= targetKeyCount/16;

  TraceValue("%u",minimalElementValueAfterCompression);
  TraceValue("%u",dhtKeyCount(pyhash));
  TraceValue("%u\n",targetKeyCount);

#if defined(TESTHASH)
  printf("\nminimalElementValueAfterCompression: %08x\n",
         minimalElementValueAfterCompression);
  fflush(stdout);
  initCnt= dhtKeyCount(pyhash);
  runCnt= 0;
#endif  /* TESTHASH */

  while (true)
  {
#if defined(TESTHASH)
    printf("minimalElementValueAfterCompression: %08x\n",
           minimalElementValueAfterCompression);
    printf("RemoveCnt: %ld\n", RemoveCnt);
    fflush(stdout);
    visitCnt= 0;
#endif  /* TESTHASH */

#if defined(TESTHASH)
    for (he = dhtGetFirstElement(pyhash);
         he!=0;
         he = dhtGetNextElement(pyhash))
      printf("%08x\n",value_of_data(he));
    exit (0);
#endif  /* TESTHASH */

    for (he = dhtGetFirstElement(pyhash);
         he!=0;
         he = dhtGetNextElement(pyhash))
    {
      hashElement_union_t const * const hue = (hashElement_union_t const *)he;
      if (value_of_data(hue)<minimalElementValueAfterCompression)
      {
#if defined(TESTHASH)
        RemoveCnt++;
        totalRemoveCount++;
#endif  /* TESTHASH */

        dhtRemoveElement(pyhash, hue->d.Key);

#if defined(TESTHASH)
        if (RemoveCnt + dhtKeyCount(pyhash) != initCnt)
        {
          fprintf(stdout,
                  "dhtRemove failed on %ld-th element of run %ld. "
                  "This was the %ld-th call to dhtRemoveElement.\n"
                  "RemoveCnt=%ld, dhtKeyCount=%ld, initCnt=%ld\n",
                  visitCnt, runCnt, totalRemoveCount,
                  RemoveCnt, dhtKeyCount(pyhash), initCnt);
          exit(1);
        }
#endif  /* TESTHASH */
      }
    }

#if defined(TESTHASH)
    visitCnt++;
#endif  /* TESTHASH */
#if defined(TESTHASH)
    runCnt++;
    printf("run=%ld, RemoveCnt: %ld, missed: %ld\n",
           runCnt, RemoveCnt, initCnt-visitCnt);
    {
      int l, counter[16];
      int KeyCount=dhtKeyCount(pyhash);
      dhtBucketStat(pyhash, counter, 16);
      for (l=0; l< 16-1; l++)
        fprintf(stdout, "%d %d %d\n", KeyCount, l+1, counter[l]);
      printf("%d %d %d\n\n", KeyCount, l+1, counter[l]);
      if (runCnt > 9)
        printf("runCnt > 9 after %ld-th call to  dhtRemoveElement\n",
               totalRemoveCount);
      dhtDebug= runCnt == 9;
    }
    fflush(stdout);
#endif  /* TESTHASH */

    if (dhtKeyCount(pyhash)<=targetKeyCount)
      break;
    else
      ++minimalElementValueAfterCompression;
  }
#if defined(TESTHASH)
  printf("%ld;", dhtKeyCount(pyhash));
#if defined(HASHRATE)
  printf(" usage: %ld", use_pos);
  printf(" / %ld", use_all);
  printf(" = %ld%%", (100 * use_pos) / use_all);
#endif
#if defined(FREEMAP) && defined(FXF)
  PrintFreeMap(stdout);
#endif /*FREEMAP*/
#if defined(__TURBOC__)
  gotoxy(1, wherey());
#else
  printf("\n");
#endif /*__TURBOC__*/
#if defined(FXF)
  printf("\n after compression:\n");
  fxfInfo(stdout);
#endif /*FXF*/
#endif /*TESTHASH*/

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
} /* compresshash */

#if defined(HASHRATE)
/* Level = 0: No output of HashStat
 * Level = 1: Output with every trace output
 * Level = 2: Output at each table compression
 * Level = 3: Output at every 1000th hash entry
 * a call to HashStats with a value of 0 will
 * always print
 */
static unsigned int HashRateLevel = 0;

void IncHashRateLevel(void)
{
  ++HashRateLevel;
  StdString("  ");
  PrintTime();
  logIntArg(HashRateLevel);
  Message(IncrementHashRateLevel);
  HashStats(0, "\n");
}

void DecHashRateLevel(void)
{
  if (HashRateLevel>0)
    --HashRateLevel;
  StdString("  ");
  PrintTime();
  logIntArg(HashRateLevel);
  Message(DecrementHashRateLevel);
  HashStats(0, "\n");
}

#else

void IncHashRateLevel(void)
{
  /* intentionally nothing */
}

void DecHashRateLevel(void)
{
  /* intentionally nothing */
}

#endif

void HashStats(unsigned int level, char *trailer)
{
#if defined(HASHRATE)
  int pos=dhtKeyCount(pyhash);
  char rate[60];

  if (level<=HashRateLevel)
  {
    StdString("  ");
    pos= dhtKeyCount(pyhash);
    logIntArg(pos);
    Message(HashedPositions);
    if (use_all > 0)
    {
      if (use_all < 10000)
        sprintf(rate, " %ld/%ld = %ld%%",
                use_pos, use_all, (use_pos*100) / use_all);
      else
        sprintf(rate, " %ld/%ld = %ld%%",
                use_pos, use_all, use_pos / (use_all/100));
    }
    else
      sprintf(rate, " -");
    StdString(rate);
    if (HashRateLevel > 3)
    {
      unsigned long msec;
      unsigned long Seconds;
      StopTimer(&Seconds,&msec);
      if (Seconds > 0)
      {
        sprintf(rate, ", %lu pos/s", use_all/Seconds);
        StdString(rate);
      }
    }
    if (trailer)
      StdString(trailer);
  }
#endif /*HASHRATE*/
}

static unsigned int estimateNumberOfHoles(void)
{
  unsigned int result = 0;
  unsigned int i;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  for (i = 0; i<nr_hash_slices && !result; ++i)
  {
    slice_index const si = hash_slices[i];
    stip_length_type const length = slices[si].u.branch.length;
    switch (slices[si].type)
    {
      case STAttackHashed:
        result += (length-slack_length)/2;
        break;

      case STHelpHashed:
        result += (length-slack_length+1)/2;
        break;

      default:
        assert(0);
        break;
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static unsigned int TellCommonEncodePosLeng(unsigned int len,
                                            unsigned int nbr_p)
{
  len += 2; /* Castling_Flag, validity_value */

  if (CondFlag[haanerchess])
  {
    unsigned int nbr_holes = estimateNumberOfHoles();
    if (nbr_holes>(nr_files_on_board*nr_rows_on_board-nbr_p)/2)
      nbr_holes = (nr_files_on_board*nr_rows_on_board-nbr_p)/2;
    len += bytes_per_piece*nbr_holes;
  }

  if (CondFlag[messigny])
    len+= 2;

  if (CondFlag[duellist])
    len+= 2;

  if (CondFlag[blfollow] || CondFlag[whfollow] || CondFlag[champursue])
    len++;

  if (flag_synchron)
    len++;

  if (CondFlag[imitators])
  {
    unsigned int imi_idx;
    for (imi_idx = 0; imi_idx<inum[nbply]; imi_idx++)
      len++;

    /* coding of no. of imitators and average of one
       imitator-promotion assumed.
    */
    len+=2;
  }

  if (anyparrain)
    /*
    ** only one out of three positions with a capture
    ** assumed.
    */
    len++;

  if (OptFlag[nontrivial])
    len++;

  if (CondFlag[disparate])
    len++;

  return len;
} /* TellCommonEncodePosLeng */

static unsigned int TellLargeEncodePosLeng(void)
{
  square const *bnp;
  unsigned int nbr_p = 0;
  unsigned int len = 8;

  for (bnp= boardnum; *bnp; bnp++)
    if (e[*bnp] != vide)
    {
      len += bytes_per_piece;
      nbr_p++;  /* count no. of pieces and holes */
    }

  if (CondFlag[BGL])
    len+= sizeof BGL_values[White][1] + sizeof BGL_values[Black][1];

  len += nr_ghosts*bytes_per_piece;

  return TellCommonEncodePosLeng(len, nbr_p);
} /* TellLargeEncodePosLeng */

static unsigned int TellSmallEncodePosLeng(void)
{
  square const *bnp;
  unsigned int nbr_p = 0;
  unsigned int len = 0;

  for (bnp= boardnum; *bnp; bnp++)
  {
    /* piece    p;
    ** Flags    pspec;
    */
    if (e[*bnp] != vide)
    {
      len += 1 + bytes_per_piece;
      nbr_p++;            /* count no. of pieces and holes */
    }
  }

  len += nr_ghosts*bytes_per_piece;

  return TellCommonEncodePosLeng(len, nbr_p);
} /* TellSmallEncodePosLeng */

static byte *CommonEncode(byte *bp, stip_length_type validity_value)
{
  if (CondFlag[messigny]) {
    if (move_generation_stack[nbcou].capture == messigny_exchange) {
      *bp++ = (byte)(move_generation_stack[nbcou].arrival - square_a1);
      *bp++ = (byte)(move_generation_stack[nbcou].departure - square_a1);
    }
    else {
      *bp++ = (byte)(0);
      *bp++ = (byte)(0);
    }
  }
  if (CondFlag[duellist]) {
    *bp++ = (byte)(whduell[nbply] - square_a1);
    *bp++ = (byte)(blduell[nbply] - square_a1);
  }

  if (CondFlag[blfollow] || CondFlag[whfollow] || CondFlag[champursue])
    *bp++ = (byte)(move_generation_stack[nbcou].departure - square_a1);

  if (flag_synchron)
    *bp++= (byte)(sq_num[move_generation_stack[nbcou].departure]
                  -sq_num[move_generation_stack[nbcou].arrival]
                  +64);

  if (CondFlag[imitators])
  {
    unsigned int imi_idx;

    /* The number of imitators has to be coded too to avoid
     * ambiguities.
     */
    *bp++ = (byte)inum[nbply];
    for (imi_idx = 0; imi_idx<inum[nbply]; imi_idx++)
      *bp++ = (byte)(isquare[imi_idx]-square_a1);
  }

  if (OptFlag[nontrivial])
    *bp++ = (byte)(max_nr_nontrivial);

  if (anyparrain) {
    /* a piece has been captured and can be reborn */
    *bp++ = (byte)(move_generation_stack[nbcou].capture - square_a1);
    if (one_byte_hash) {
      *bp++ = (byte)(pprispec[nbply])
          + ((byte)(piece_nbr[abs(pprise[nbply])]) << (CHAR_BIT/2));
    }
    else {
      *bp++ = pprise[nbply];
      *bp++ = (byte)(pprispec[nbply]>>CHAR_BIT);
      *bp++ = (byte)(pprispec[nbply]&ByteMask);
    }
  }

  assert(validity_value<=(1<<CHAR_BIT));
  *bp++ = (byte)(validity_value);

  if (ep[nbply]!=initsquare)
    *bp++ = (byte)(ep[nbply] - square_a1);

  *bp++ = castling_flag[nbply];     /* Castling_Flag */

  if (CondFlag[BGL]) {
    memcpy(bp, &BGL_values[White][nbply], sizeof BGL_values[White][nbply]);
    bp += sizeof BGL_values[White][nbply];
    memcpy(bp, &BGL_values[Black][nbply], sizeof BGL_values[Black][nbply]);
    bp += sizeof BGL_values[Black][nbply];
  }

  if (CondFlag[disparate]) {
    *bp++ = (byte)(nbply>=2?pjoue[nbply]:vide);
  }

  return bp;
} /* CommonEncode */

static byte *LargeEncodePiece(byte *bp, byte *position,
                              int row, int col,
                              piece p, Flags pspec)
{
  if (!TSTFLAG(pspec, Neutral))
    SETFLAG(pspec, (p < vide ? Black : White));
  p = abs(p);
  if (one_byte_hash)
    *bp++ = (byte)pspec + ((byte)piece_nbr[p] << (CHAR_BIT/2));
  else
  {
    unsigned int i;
    *bp++ = p;
    for (i = 0; i<bytes_per_spec; i++)
      *bp++ = (byte)((pspec>>(CHAR_BIT*i)) & ByteMask);
  }

  position[row] |= BIT(col);

  return bp;
}

static void LargeEncode(stip_length_type validity_value)
{
  HashBuffer *hb = &hashBuffers[nbply];
  byte *position = hb->cmv.Data;
  byte *bp = position+nr_rows_on_board;
  int row, col;
  square a_square = square_a1;
  ghost_index_type gi;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  /* detect cases where we encode the same position twice */
  assert(hashBufferValidity[nbply]!=validity_value);

  /* clear the bits for storing the position of pieces */
  memset(position,0,nr_rows_on_board);

  for (row=0; row<nr_rows_on_board; row++, a_square+= onerow)
  {
    square curr_square = a_square;
    for (col=0; col<nr_files_on_board; col++, curr_square+= dir_right)
    {
      piece const p = e[curr_square];
      if (p!=vide)
        bp = LargeEncodePiece(bp,position,row,col,p,spec[curr_square]);
    }
  }

  for (gi = 0; gi<nr_ghosts; ++gi)
  {
    square s = (ghosts[gi].ghost_square
                - nr_of_slack_rows_below_board*onerow
                - nr_of_slack_files_left_of_board);
    row = s/onerow;
    col = s%onerow;
    bp = LargeEncodePiece(bp,position,
                          row,col,
                          ghosts[gi].ghost_piece,ghosts[gi].ghost_flags);
  }

  /* Now the rest of the party */
  bp = CommonEncode(bp,validity_value);

  assert(bp-hb->cmv.Data<=UCHAR_MAX);
  hb->cmv.Leng = (unsigned char)(bp-hb->cmv.Data);

  validateHashBuffer(validity_value);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
} /* LargeEncode */

static byte *SmallEncodePiece(byte *bp,
                              int row, int col,
                              piece p, Flags pspec)
{
  if (!TSTFLAG(pspec,Neutral))
    SETFLAG(pspec, (p < vide ? Black : White));
  p= abs(p);
  *bp++= (byte)((row<<(CHAR_BIT/2))+col);
  if (one_byte_hash)
    *bp++ = (byte)pspec + ((byte)piece_nbr[p] << (CHAR_BIT/2));
  else
  {
    unsigned int i;
    *bp++ = p;
    for (i = 0; i<bytes_per_spec; i++)
      *bp++ = (byte)((pspec>>(CHAR_BIT*i)) & ByteMask);
  }

  return bp;
}

static void SmallEncode(stip_length_type validity_value)
{
  HashBuffer *hb = &hashBuffers[nbply];
  byte *bp = hb->cmv.Data;
  square a_square = square_a1;
  int row;
  int col;
  ghost_index_type gi;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  /* detect cases where we encode the same position twice */
  assert(hashBufferValidity[nbply]!=validity_value);

  for (row=0; row<nr_rows_on_board; row++, a_square += onerow)
  {
    square curr_square= a_square;
    for (col=0; col<nr_files_on_board; col++, curr_square += dir_right)
    {
      piece const p = e[curr_square];
      if (p!=vide)
        bp = SmallEncodePiece(bp,row,col,p,spec[curr_square]);
    }
  }

  for (gi = 0; gi<nr_ghosts; ++gi)
  {
    square s = (ghosts[gi].ghost_square
                - nr_of_slack_rows_below_board*onerow
                - nr_of_slack_files_left_of_board);
    row = s/onerow;
    col = s%onerow;
    bp = SmallEncodePiece(bp,
                          row,col,
                          ghosts[gi].ghost_piece,ghosts[gi].ghost_flags);
  }

  /* Now the rest of the party */
  bp = CommonEncode(bp,validity_value);

  assert(bp-hb->cmv.Data<=UCHAR_MAX);
  hb->cmv.Leng = (unsigned char)(bp-hb->cmv.Data);

  validateHashBuffer(validity_value);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Initialise the bits representing all slices in a hash table
 * element's data field with null values
 * @param he address of hash table element
 */
static void init_elements(hashElement_union_t *hue)
{
  unsigned int i;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  for (i = 0; i!=nr_hash_slices; ++i)
  {
    slice_index const si = hash_slices[i];
    switch (slices[si].type)
    {
      case STAttackHashed:
      {
        stip_length_type const length = slices[si].u.branch.length;
        stip_length_type const min_length = slices[si].u.branch.min_length;
        set_value_attack_nosuccess(hue,si,0);
        set_value_attack_success(hue,si,(length-min_length+1)/2);
        break;
      }

      case STHelpHashed:
        set_value_help(hue,si,0);
        break;

      default:
        assert(0);
        break;
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* (attempt to) allocate a hash table element - compress the current
 * hash table if necessary; exit()s if allocation is not possible
 * in spite of compression
 * @param hb has value (basis for calculation of key)
 * @return address of element
 */
static dhtElement *allocDHTelement(dhtConstValue hb)
{
  dhtElement *result = dhtEnterElement(pyhash,hb,template_element.d.Data);
  while (result==dhtNilElement)
  {
    unsigned long const nrKeysBeforeCompression = dhtKeyCount(pyhash);
    compresshash();
    if (dhtKeyCount(pyhash)==nrKeysBeforeCompression)
    {
      dhtDestroy(pyhash);
#if defined(FXF)
      fxfReset();
#endif
      pyhash = dhtCreate(dhtBCMemValue,dhtCopy,dhtSimpleValue,dhtNoCopy);
      assert(pyhash!=0);
      result = dhtEnterElement(pyhash,hb,template_element.d.Data);
      break;
    }
    else
      result = dhtEnterElement(pyhash,hb,template_element.d.Data);
  }

  if (result==dhtNilElement)
  {
    fprintf(stderr,
            "Sorry, cannot enter more hashelements "
            "despite compression\n");
    exit(-2);
  }

  return result;
}

static unsigned long hashtable_kilos;

/* Allocate memory for the hash table. If the requested amount of
 * memory isn't available, reduce the amount until allocation
 * succeeds.
 * @param nr_kilos number of kilo-bytes to allocate
 * @return number of kilo-bytes actually allocated
 */
unsigned long allochash(unsigned long nr_kilos)
{
#if defined(FXF)
  size_t const one_kilo = 1<<10;
  while (fxfInit(nr_kilos*one_kilo)==-1)
    /* we didn't get hashmemory ... */
    nr_kilos /= 2;
  ifTESTHASH(fxfInfo(stdout));
#endif /*FXF*/

  hashtable_kilos = nr_kilos;

  pyhash = dhtCreate(dhtBCMemValue,dhtCopy,dhtSimpleValue,dhtNoCopy);
  if (pyhash==0)
  {
    TraceValue("%s\n",dhtErrorMsg());
    return 0;
  }
  else
    return nr_kilos;
}

static void proof_goal_found(slice_index si, stip_structure_traversal *st)
{
  boolean * const result = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  *result = true;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean is_proofgame(slice_index si)
{
  boolean result = false;
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,&result);
  if (slices[si].type==STIntelligentStalemateFilter
      || slices[si].type==STIntelligentMateFilter)
    st.context = stip_traversal_context_help;
  stip_structure_traversal_override_single(&st,
                                           STGoalProofgameReachedTester,
                                           &proof_goal_found);
  stip_structure_traversal_override_single(&st,
                                           STGoalAToBReachedTester,
                                           &proof_goal_found);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the hash table has been successfully allocated
 * @return true iff the hashtable has been allocated
 */
boolean is_hashtable_allocated(void)
{
  return pyhash!=0;
}

/* Initialise the hashing machinery for the current stipulation
 * @param si identifies the root slice of the stipulation
 */
void inithash(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  ifTESTHASH(
      sprintf(GlobalStr, "calling inithash\n");
      StdString(GlobalStr)
      );

  if (pyhash!=0)
  {
    int i, j;

#if defined(__unix) && defined(TESTHASH)
    OldBreak= sbrk(0);
#endif /*__unix,TESTHASH*/

    minimalElementValueAfterCompression = 2;

    init_slice_properties(si);

    template_element.d.Data = 0;
    init_elements(&template_element);

    dhtRegisterValue(dhtBCMemValue,0,&dhtBCMemoryProcs);
    dhtRegisterValue(dhtSimpleValue,0,&dhtSimpleProcs);

    ifHASHRATE(use_pos = use_all = 0);

    /* check whether a piece can be coded in a single byte */
    j = 0;
    for (i = PieceCount; Empty < i; i--)
      if (exist[i])
        piece_nbr[i] = j++;

    if (CondFlag[haanerchess])
      piece_nbr[obs]= j++;

    one_byte_hash = j<(1<<(CHAR_BIT/2)) && PieSpExFlags<(1<<(CHAR_BIT/2));

    bytes_per_spec= 1;
    if ((PieSpExFlags >> CHAR_BIT) != 0)
      bytes_per_spec++;
    if ((PieSpExFlags >> 2*CHAR_BIT) != 0)
      bytes_per_spec++;

    bytes_per_piece= one_byte_hash ? 1 : 1+bytes_per_spec;

    if (is_proofgame(si))
    {
      encode = ProofEncode;
      if (hashtable_kilos>0 && MaxPositions==0)
        MaxPositions= hashtable_kilos/(24+sizeof(char *)+1);
    }
    else
    {
      unsigned int const Small = TellSmallEncodePosLeng();
      unsigned int const Large = TellLargeEncodePosLeng();
      if (Small<=Large)
      {
        encode = SmallEncode;
        if (hashtable_kilos>0 && MaxPositions==0)
          MaxPositions= hashtable_kilos/(Small+sizeof(char *)+1);
      }
      else
      {
        encode = LargeEncode;
        if (hashtable_kilos>0 && MaxPositions==0)
          MaxPositions= hashtable_kilos/(Large+sizeof(char *)+1);
      }
    }

#if defined(FXF)
    ifTESTHASH(printf("MaxPositions: %7lu\n", MaxPositions));
    assert(hashtable_kilos/1024<UINT_MAX);
    ifTESTHASH(printf("hashtable_kilos:    %7u KB\n",
                      (unsigned int)(hashtable_kilos/1024)));
#else
    ifTESTHASH(
        printf("room for up to %lu positions in hash table\n", MaxPositions));
#endif /*FXF*/

    dhtDestroy(pyhash);

#if defined(TESTHASH) && defined(FXF)
    fxfInfo(stdout);
#endif /*TESTHASH,FXF*/

#if defined(FXF)
    fxfReset();
#endif

    pyhash = dhtCreate(dhtBCMemValue,dhtCopy,dhtSimpleValue,dhtNoCopy);
    assert(pyhash!=0);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
} /* inithash */

/* Uninitialise the hashing machinery
 */
void closehash(void)
{
  if (pyhash!=0)
  {
#if defined(TESTHASH)
    sprintf(GlobalStr, "calling closehash\n");
    StdString(GlobalStr);

#if defined(HASHRATE)
    sprintf(GlobalStr, "%ld enquiries out of %ld successful. ",
            use_pos, use_all);
    StdString(GlobalStr);
    if (use_all) {
      sprintf(GlobalStr, "Makes %ld%%\n", (100 * use_pos) / use_all);
      StdString(GlobalStr);
    }
#endif
#if defined(__unix)
    {
#if defined(FXF)
      unsigned long const HashMem = fxfTotal();
#else
      unsigned long const HashMem = sbrk(0)-OldBreak;
#endif /*FXF*/
      unsigned long const HashCount = pyhash==0 ? 0 : dhtKeyCount(pyhash);
      if (HashCount>0)
      {
        unsigned long const BytePerPos = (HashMem*100)/HashCount;
        sprintf(GlobalStr,
                "Memory for hash-table: %ld, "
                "gives %ld.%02ld bytes per position\n",
                HashMem, BytePerPos/100, BytePerPos%100);
      }
      else
        sprintf(GlobalStr, "Nothing in hashtable\n");
      StdString(GlobalStr);
#endif /*__unix*/
    }
#endif /*TESTHASH*/
    invalidateHashBuffer();
  }
} /* closehash */

/* Spin a tester slice off a STAttackHashedTester slice
 * @param base_slice identifies the STAttackHashedTester slice
 * @return id of allocated slice
 */
void spin_off_testers_attack_hashed(slice_index si, stip_structure_traversal *st)
{
  spin_off_tester_state_type * const state = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (state->spinning_off)
  {
    state->spun_off[si] = alloc_pipe(STAttackHashedTester);
    slices[state->spun_off[si]].u.derived_pipe.base = si;
    stip_traverse_structure_children(si,st);
    link_to_branch(state->spun_off[si],state->spun_off[slices[si].u.pipe.next]);
  }
  else
    stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Spin a tester slice off a STHelpHashed slice
 * @param base_slice identifies the STHelpHashed slice
 * @return id of allocated slice
 */
void spin_off_testers_help_hashed(slice_index si, stip_structure_traversal *st)
{
  spin_off_tester_state_type * const state = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (state->spinning_off)
  {
    state->spun_off[si] = alloc_pipe(STHelpHashedTester);
    slices[state->spun_off[si]].u.derived_pipe.base = si;
    stip_traverse_structure_children(si,st);
    link_to_branch(state->spun_off[si],state->spun_off[slices[si].u.pipe.next]);
  }
  else
    stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traverse a slice while inserting hash elements
 * @param si identifies slice
 * @param st address of structure holding status of traversal
 */
static void insert_hash_element_attack(slice_index si,
                                       stip_structure_traversal *st)
{
  slice_index const * const previous_move_slice = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (*previous_move_slice!=no_slice && length>slack_length)
  {
    slice_index const prototype = alloc_branch(STAttackHashed,length,min_length);
    attack_branch_insert_slices(si,&prototype,1);
  }

  stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Traverse a slice while inserting hash elements
 * @param si identifies slice
 * @param st address of structure holding status of traversal
 */
static void insert_hash_element_help(slice_index si,
                                     stip_structure_traversal *st)
{
  slice_index const * const previous_move_slice = st->param;
  stip_length_type const length = slices[si].u.branch.length;
  stip_length_type const min_length = slices[si].u.branch.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (*previous_move_slice!=no_slice && length>slack_length)
  {
    slice_index const prototype = alloc_branch(STHelpHashed,length,min_length);
    help_branch_insert_slices(si,&prototype,1);
  }

  stip_traverse_structure_children(si,st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remember_move(slice_index si, stip_structure_traversal *st)
{
  slice_index * const previous_move_slice = st->param;
  slice_index const save_previous_move_slice = *previous_move_slice;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  *previous_move_slice = si;
  stip_traverse_structure_children(si,st);
  *previous_move_slice = save_previous_move_slice;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitors const hash_element_inserters[] =
{
  { STReadyForAttack,   &insert_hash_element_attack   },
  { STReadyForHelpMove, &insert_hash_element_help     },
  { STMove,             &remember_move                },
  { STConstraintSolver, &stip_traverse_structure_children_pipe }
};

enum
{
  nr_hash_element_inserters
  = sizeof hash_element_inserters / sizeof hash_element_inserters[0]
};

/* Instrument stipulation with hashing slices
 * @param si identifies slice where to start
 */
void stip_insert_hash_slices(slice_index si)
{
  stip_structure_traversal st;
  slice_index previous_move_slice = no_slice;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceStipulation(si);

  stip_structure_traversal_init(&st,&previous_move_slice);
  stip_structure_traversal_override_by_function(&st,
                                                slice_function_conditional_pipe,
                                                &stip_traverse_structure_children_pipe);
  stip_structure_traversal_override(&st,
                                    hash_element_inserters,
                                    nr_hash_element_inserters);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Try to solve in n half-moves after a defense.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @return length of solution found and written, i.e.:
 *            slack_length-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type attack_hashed_attack(slice_index si, stip_length_type n)
{
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert((slices[si].u.branch.length-n)%2==0);

  result = attack(slices[si].u.pipe.next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Remember that the current position does not have a solution in a
 * number of half-moves
 * @param si index of slice where the current position was reached
 * @param n number of half-moves
 */
static void addtohash_battle_nosuccess(slice_index si,
                                       stip_length_type n,
                                       stip_length_type min_length_adjusted)
{
  HashBuffer const * const hb = &hashBuffers[nbply];
#if !defined(NDEBUG)
  stip_length_type const validity_value = min_length_adjusted/2+1;
#endif
  hash_value_type const val = (n+1-min_length_adjusted)/2;
  dhtElement *he;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",min_length_adjusted);
  TraceFunctionParamListEnd();

  assert(hashBufferValidity[nbply]==validity_value);

  he = dhtLookupElement(pyhash,hb);
  if (he==dhtNilElement)
  {
    hashElement_union_t * const hue = (hashElement_union_t *)allocDHTelement(hb);
    set_value_attack_nosuccess(hue,si,val);
  }
  else
  {
    hashElement_union_t * const hue = (hashElement_union_t *)he;
    if (get_value_attack_nosuccess(hue,si)<val)
      set_value_attack_nosuccess(hue,si,val);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();

#if defined(HASHRATE)
  if (dhtKeyCount(pyhash)%1000 == 0)
    HashStats(3, "\n");
#endif /*HASHRATE*/
}

/* Remember that the current position has a solution in a number of
 * half-moves
 * @param si index of slice where the current position was reached
 * @param n number of half-moves
 */
static void addtohash_battle_success(slice_index si,
                                     stip_length_type n,
                                     stip_length_type min_length_adjusted)
{
  HashBuffer const * const hb = &hashBuffers[nbply];
#if !defined(NDEBUG)
  stip_length_type const validity_value = min_length_adjusted/2+1;
#endif
  hash_value_type const val = (n+1-min_length_adjusted)/2 - 1;
  dhtElement *he;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",min_length_adjusted);
  TraceFunctionParamListEnd();

  assert(hashBufferValidity[nbply]==validity_value);

  he = dhtLookupElement(pyhash,hb);
  if (he==dhtNilElement)
  {
    hashElement_union_t * const
        hue = (hashElement_union_t *)allocDHTelement(hb);
    set_value_attack_success(hue,si,val);
  }
  else
  {
    hashElement_union_t * const hue = (hashElement_union_t *)he;
    if (get_value_attack_success(hue,si)>val)
      set_value_attack_success(hue,si,val);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();

#if defined(HASHRATE)
  if (dhtKeyCount(pyhash)%1000 == 0)
    HashStats(3, "\n");
#endif /*HASHRATE*/
}

static
stip_length_type delegate_can_attack_in_n(slice_index si,
                                          stip_length_type n,
                                          stip_length_type min_length_adjusted)
{
  stip_length_type result;
  slice_index const base = slices[si].u.derived_pipe.base;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",min_length_adjusted);
  TraceFunctionParamListEnd();

  result = attack(next,n);

  if (result<=n)
    addtohash_battle_success(base,result,min_length_adjusted);
  else
    addtohash_battle_nosuccess(base,n,min_length_adjusted);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to solve in n half-moves after a defense.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @return length of solution found and written, i.e.:
 *            slack_length-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type attack_hashed_tester_attack(slice_index si, stip_length_type n)
{
  stip_length_type result;
  dhtElement const *he;
  slice_index const base = slices[si].u.derived_pipe.base;
  stip_length_type const min_length = slices[base].u.branch.min_length;
  stip_length_type const played = slices[base].u.branch.length-n;
  stip_length_type const min_length_adjusted = (min_length<played+slack_length-1
                                                ? slack_length-(min_length-slack_length)%2
                                                : min_length-played);
  stip_length_type const validity_value = min_length_adjusted/2+1;
  stip_length_type const save_max_unsolvable = max_unsolvable;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert((n-slices[base].u.branch.length)%2==0);

  if (hashBufferValidity[nbply]!=validity_value)
    (*encode)(validity_value);

  he = dhtLookupElement(pyhash,&hashBuffers[nbply]);
  if (he==dhtNilElement)
    result = delegate_can_attack_in_n(si,n,min_length_adjusted);
  else
  {
    hashElement_union_t const * const hue = (hashElement_union_t const *)he;
    stip_length_type const parity = (n-min_length_adjusted)%2;

    /* It is more likely that a position has no solution. */
    /* Therefore let's check for "no solution" first.  TLi */
    hash_value_type const val_nosuccess = get_value_attack_nosuccess(hue,base);
    stip_length_type const n_nosuccess = 2*val_nosuccess + min_length_adjusted-parity;
    if (n_nosuccess>=n)
      result = n+2;
    else
    {
      hash_value_type const val_success = get_value_attack_success(hue,base);
      stip_length_type const n_success = 2*val_success + min_length_adjusted+2-parity;
      if (n_success<=n)
        result = n_success;
      else
      {
        if (max_unsolvable<n_nosuccess)
        {
          max_unsolvable = n_nosuccess;
          TraceValue("->%u\n",max_unsolvable);
        }
        result = delegate_can_attack_in_n(si,n,min_length_adjusted);
      }
    }
  }

  max_unsolvable = save_max_unsolvable;
  TraceValue("->%u\n",max_unsolvable);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Look up whether the current position in the hash table to find out
 * if it has a solution in a number of half-moves
 * @param si index slice where current position was reached
 * @param n number of half-moves
 * @return true iff we know that the current position has no solution
 *         in n half-moves
 */
static boolean inhash_help(slice_index si, stip_length_type n)
{
  boolean result;
  HashBuffer *hb = &hashBuffers[nbply];
  dhtElement const *he;
  stip_length_type const min_length = slices[si].u.branch.min_length;
  stip_length_type const validity_value = (min_length+2-slack_length)/2+1;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  if (hashBufferValidity[nbply]!=validity_value)
    (*encode)(validity_value);

  ifHASHRATE(use_all++);

  he = dhtLookupElement(pyhash,hb);
  if (he==dhtNilElement)
    result = false;
  else
  {
    hashElement_union_t const * const hue = (hashElement_union_t const *)he;
    hash_value_type const val = (n-(min_length+2-slack_length))/2+1;
    hash_value_type const nosuccess = get_value_help(hue,si);
    TraceValue("%u",min_length);
    TraceValue("%u\n",val);
    if (nosuccess>=val)
    {
      ifHASHRATE(use_pos++);
      result = true;
    }
    else
      result = false;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Remember that the current position does not have a solution in a
 * number of half-moves
 * @param si index of slice where the current position was reached
 * @param n number of half-moves
 */
static void addtohash_help(slice_index si, stip_length_type n)
{
  HashBuffer const * const hb = &hashBuffers[nbply];
  stip_length_type const min_length = slices[si].u.branch.min_length;
  hash_value_type const val = (n-(min_length+2-slack_length))/2+1;
  dhtElement *he;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  he = dhtLookupElement(pyhash,hb);
  if (he==dhtNilElement)
  {
    hashElement_union_t * const hue = (hashElement_union_t *)allocDHTelement(hb);
    set_value_help(hue,si,val);
  }
  else
  {
    hashElement_union_t * const hue = (hashElement_union_t *)he;
    if (get_value_help(hue,si)<val)
      set_value_help(hue,si,val);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();

#if defined(HASHRATE)
  if (dhtKeyCount(pyhash)%1000 == 0)
    HashStats(3, "\n");
#endif /*HASHRATE*/
}

/* Try to solve in n half-moves after a defense.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @return length of solution found and written, i.e.:
 *            slack_length-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type help_hashed_attack(slice_index si, stip_length_type n)
{
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length);

  if (inhash_help(si,n))
    result = n+2;
  else
  {
    if (slices[si].u.branch.min_length>slack_length+1)
    {
      slices[si].u.branch.min_length -= 2;
      result = attack(slices[si].u.pipe.next,n);
      slices[si].u.branch.min_length += 2;
    }
    else
      result = attack(slices[si].u.pipe.next,n);

    if (result==n+2)
      addtohash_help(si,n);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to solve in n half-moves after a defense.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @return length of solution found and written, i.e.:
 *            slack_length-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type help_hashed_tester_attack(slice_index si, stip_length_type n)
{
  stip_length_type result;
  slice_index const base = slices[si].u.derived_pipe.base;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length);

  if (inhash_help(base,n))
    result = n+2;
  else
  {
    if (slices[base].u.branch.min_length>slack_length+1)
    {
      slices[base].u.branch.min_length -= 2;
      result = attack(slices[si].u.pipe.next,n);
      slices[base].u.branch.min_length += 2;
    }
    else
      result = attack(slices[si].u.pipe.next,n);

    if (result>n)
      addtohash_help(base,n);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* assert()s below this line must remain active even in "productive"
 * executables. */
#undef NDEBUG
#include <assert.h>

/* Check assumptions made in the hashing module. Abort if one of them
 * isn't met.
 * This is called from checkGlobalAssumptions() once at program start.
 */
void check_hash_assumptions(void)
{
  /* SmallEncode uses 1 byte for both row and file of a square */
  assert(nr_rows_on_board<1<<(CHAR_BIT/2));
  assert(nr_files_on_board<1<<(CHAR_BIT/2));

  /* LargeEncode() uses 1 bit per square */
  assert(nr_files_on_board<=CHAR_BIT);

  /* the encoding functions encode Flags as 2 bytes */
  assert(PieSpCount<=2*CHAR_BIT);
}