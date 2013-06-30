#include "pieces/vectors.h"
#include "position/board.h"

/* This are the vectors for the CAT (a special concatenation of knight and
 * dabbabba-rider)
 */
numvec const cat_vectors[17] =
{ 0,
  dir_down+2*dir_right, 2*dir_up+dir_left,
  dir_up+2*dir_left,    2*dir_down+dir_right,
  dir_up+2*dir_right,   2*dir_up+dir_right,
  dir_down+2*dir_left,  2*dir_down+dir_left,
  2*dir_left,           2*dir_up,
  2*dir_up,             2*dir_right,
  2*dir_right,          2*dir_down,
  2*dir_down,           2*dir_left
};

/* don't try to delete something like "duplicates" or change
  the order of the vectors.
  they are all necessary and need this order !!
*/
numvec vec[maxvec + 1] = { 0,
/*   1 -   4 | 0,1 */    1,   24,   -1,  -24,
/*   5 -   8 | 1,1 */   23,   25,  -23,  -25,
/*   9 -  16 | 1,2 */   22,   47,   49,   26,  -22,  -47,  -49,  -26,
/*  17 -  24 | 1,2 */   22,   47,   49,   26,  -22,  -47,  -49,  -26,
/*  25 -  32 | 2,3 */   45,   70,   74,   51,  -45,  -70,  -74,  -51,
/*  33 -  40 | 1,3 */   21,   71,   73,   27,  -21,  -71,  -73,  -27,
/*  41 -  48 | 1,4 */   20,   95,   97,   28,  -20,  -95,  -97,  -28,
/*  49 -  56 | 3,4 */   68,   93,   99,   76,  -68,  -93,  -99,  -76,
/*  57 -  60 | 0,5 */    5,  120,   -5, -120,
/*  61 -  64 | 0,2 */    2,   48,   -2,  -48,
/*  65 -  68 | 2,2 */   46,   50,  -46,  -50,
/*  69 -  76 | 1,7 */   17,  167,  169,   31,  -17, -167, -169,  -31,
/*  77 -  80 | 5,5 */  115,  125, -115, -125,
/*  81 -  88 | 3,7 */   65,  165,  171,   79,  -65, -165, -171,  -79,
/*  89 -  96 | 1,6 */   18,  143,  145,   30,  -18, -143, -145,  -30,
/*  97 - 104 | 2,4 */   44,   94,   98,   52,  -44,  -94,  -98,  -52,
/* 105 - 112 | 3,5 */   67,  117,  123,   77,  -67, -117, -123,  -77,
/* 113 - 120 | 1,5 */   19,  119,  121,   29,  -19, -119, -121,  -29,
/* 121 - 128 | 2,5 */   43,  118,  122,   53,  -43, -118, -122,  -53,
/* 129 - 136 | 3,6 */   66,  141,  147,   78,  -66, -141, -147,  -78,
/* 137 - 140 | 0,3 */    3,   72,   -3,  -72,
/* 141 - 144 | 0,4 */    4,   96,   -4,  -96,
/* 145 - 148 | 0,6 */    6,  144,   -6, -144,
/* 149 - 152 | 0,7 */    7,  168,   -7, -168,
/* 153 - 156 | 3,3 */   69,   75,  -69,  -75,
/* 157 - 160 | 4,4 */   92,  100,  -92, -100,
/* 161 - 164 | 6,6 */  138,  150, -138, -150,
/* 165 - 168 | 7,7 */  161,  175, -161, -175,
/* 169 - 176 | 2,6 */   42,  142,  146,   54,  -42, -142, -146,  -54,
/* 177 - 184 | 4,5 */   91,  116,  124,  101,  -91, -116, -124, -101,
/* 185 - 192 | 4,6 */   90,  140,  148,  102,  -90, -140, -148, -102,
/* 193 - 200 | 4,7 */   89,  164,  172,  103,  -89, -164, -172, -103,
/* 201 - 208 | 5,6 */  114,  139,  149,  126, -114, -139, -149, -126,
/* 209 - 216 | 5,7 */  113,  163,  173,  127, -113, -163, -173, -127,
/* 217 - 224 | 6,7 */  137,  162,  174,  151, -137, -162, -174, -151,
/* 225 - 232 | 2,7 */   41,  166,  170,   55,  -41, -166, -170,  -55,
};
