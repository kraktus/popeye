/********************* MODIFICATIONS to py2.c **************************
 **
 ** Date       Who  What
 **
 ** 2006/05/04 NG   Bugfix: wrong rrefcech evaluation
 **
 ** 2006/05/09 SE   New pieces Bouncer, Rookbouncer, Bishopbouncer (invented P.Wong)
 **
 ** 2006/06/28 SE   New condition: Masand (invented P.Petkov)
 **
 ** 2006/06/30 SE   New condition: BGL (invented P.Petkov)
 **
 ** 2006/07/30 SE   New condition: Schwarzschacher  
 **
 ** 2007/01/28 SE   New condition: NormalPawn 
 **
 ** 2008/01/02 NG   New condition: Geneva Chess 
 **
 **************************** End of List ******************************/

#ifdef macintosh	/* is always defined on macintosh's  SB */
#	define SEGM1
#	include "pymac.h"
#endif


#include <stdio.h>
#include "py.h"
#include "pyproc.h"
#include "pydata.h"
#include "pymsg.h"

boolean eval_ortho(square sq_departure, square sq_arrival, square sq_capture) {
  return true;
}

boolean legalsquare(square sq_departure, square sq_arrival, square sq_capture) {
  if (CondFlag[koeko]) {				/* V1.3c  NG */
	if (nocontact(sq_departure,sq_arrival,sq_capture))
      return(false);
  }
  if (CondFlag[gridchess]) {
    if (GridNum(sq_departure) == GridNum(sq_arrival))		/* V3.22  TLi */
      return(false);			/* V1.3c  NG */
  }
  if (CondFlag[blackedge]) {				 /* V2.90  NG */
	if (e[sq_departure] <= roin)
      if (NoEdge(sq_arrival))				/* V3.22  TLi */
		return(false);
  }
  if (CondFlag[whiteedge]) {				/* V2.90  NG */
	if (e[sq_departure] >= roib)
      if (NoEdge(sq_arrival))				/* V3.22  TLi */
		return(false);
  }
  if (CondFlag[bichro]) {
	if (SquareCol(sq_departure) == SquareCol(sq_arrival))		/* V3.22  TLi */
      return(false);				/* V1.3c  NG */
  }
  if (CondFlag[monochro]) {
	if (SquareCol(sq_departure) != SquareCol(sq_arrival))		/* V3.22  TLi */
      return(false);				/* V1.3c  NG */
  }
  if (TSTFLAG(spec[sq_departure], Jigger)) {			/* V3.1  TLi */
	if (nocontact(sq_departure,sq_arrival,sq_capture))
      return(false);
  }
  if (CondFlag[newkoeko]) {				/* V3.1  TLi */
	if (nocontact(sq_departure,sq_arrival,sq_capture)
        != nocontact(initsquare,sq_departure,initsquare))
	{
      return false;
	}
  }
  if (anygeneva) {				 /* V4.38  NG */
	if ((e[sq_capture] <= roin) && (rex_geneva || (sq_departure != rb)))
      if (e[(*genevarenai)(e[sq_departure],spec[sq_departure],sq_departure,sq_departure,sq_arrival,noir)] != vide)
		return(false);
	if ((e[sq_capture] >= roib) && (rex_geneva || (sq_departure != rn)))
      if (e[(*genevarenai)(e[sq_departure],spec[sq_departure],sq_departure,sq_departure,sq_arrival,blanc)] != vide)
		return(false);
  }
  return(true);
} /* end of legalsquare */

boolean imok(square i, square j) {			/* V2.4d  TM */
  /* move i->j ok? */
  smallint	count;
  smallint	diff = j - i;
  square	j2;					/* V2.90  NG */

  for (count= inum[nbply]-1; count >= 0; count--) {
	j2= isquare[count] + diff;
	if ((j2 != i) && (e[j2] != vide)) {
      return false;
	}
  }
  return(true);
}

boolean maooaimok(square i, square j, square pass) {    /* V3.81 SE - good name, huh? */ 
  boolean ret;
  piece p= e[i];
  e[i]= vide;
  ret= imok(i, pass) && imok(i, j);
  e[i]= p;
  return ret;
}

boolean ridimok(square i, square j, smallint diff) {	/* V2.4d  TM */

  /* move i->j in steps of diff ok? */
  square  i2= i;
  boolean ret;
  piece   p= e[i];

  e[i]= vide;/* an imitator might be disturbed by the moving rider! */
  do {
	i2-= diff;
  } while (imok(i, i2) && (i2 != j));

  ret= (i2 == j) && imok (i, j);
  e[i]= p;			 /* only after the last call of imok! */
  return ret;
}

boolean castlingimok(square i, square j) {  /* V3.80  SE */
  piece p= e[i];
  boolean ret= false;
  /* I think this should work - clear the K, and move the Is, but don't clear the rook. */
  /* If the Is crash into the R, the move would be illegal as the K moves first.        */
  /* The only other test here is for long castling when the Is have to be clear to move */
  /* one step right (put K back first)as well as two steps left.                        */
  /* But there won't be an I one sq to the left of a1 (a8) so no need to clear the R    */

  switch (j-i)
  {
  case 2:  /* 00 - can short-circuit here (only follow K, if ok rest will be ok) */
    e[i]= vide;
    ret= imok(i, i+dir_right) && imok(i, i+2*dir_right);
    e[i]= p;
    break;

  case -2:  /* 000 - follow K, (and move K as well), then follow R */
    e[i]= vide;
    ret= imok(i, i+dir_left) && imok(i, i+2*dir_left);
    e[i+2*dir_left]= p;
    ret= ret && imok(i, i+dir_left) && imok (i, i) && imok(i, i+dir_right);
    e[i+2*dir_left]= vide;
    e[i]= p;
    break;
  }
  return ret;
}

            
boolean hopimok(square i, square j, square k, smallint diff) {
  /* V3.12  TM */
  /* hop i->j hopping over k in steps of diff ok? */
  square	i2= i;
  piece	p=e[i];
  smallint	l;
  boolean	ret= true;

  if (TSTFLAG(spec[i], ColourChange)) {		 /* V3.64 SE */
	chop[nbcou+1]= k;
	ret= true;
  }

  if (!CondFlag[imitators]) {
	return ret;
  }

  /* an imitator might be disturbed by the moving hopper! */
  e[i]= vide;

  /* Are the lines from the imitators to the square to hop over free?
   */
  do {
	i2+= diff;
  } while (imok(i, i2) && (i2 != k));

  ret = i2 == k;
  if (ret) {
	/* Are the squares the imitators have to hop over occupied? */
	l= inum[nbply];
	while (l>0) {
      --l;
      if (e[isquare[l]+k-i] == vide) {
		ret= false;
		break;
      }
	}
  }

  if (ret) {
	do {
      i2+= diff;
	} while (imok(i,i2) && (i2 != j));
  }

  ret = ret && i2==j && imok(i,j);

  e[i]= p;
  return ret;
}


void joueim(smallint diff) {				/* V2.4d  TM */
  smallint i;

  for (i=inum[nbply]-1; i >= 0; i--)			/* V2.90  NG */
	isquare[i]+= diff;

}

boolean rmhopech(square	sq_king,
                 numvec	kend,
                 numvec	kanf,
                 angle_t angle,
                 piece	p,
                 evalfunction_t *evaluate)	/* V2.1c, V3.62  NG */
{
  square sq_hurdle;
  numvec k, k1;
  piece hopper;

  /* ATTENTION:
   *	angle==angle_45:  moose    45 degree hopper
   *	angle==angle_90:  eagle    90 degree hopper
   *	angle==angle_135: sparrow 135 degree hopper
   *
   *	kend==vec_queen_end, kanf==vec_queen_start: all types (moose,
   *	                                            eagle, sparrow)
   *	kend==vec_bishop_end, kanf==vec_bishop_start: orthogonal types
   *	                          (rookmoose, rooksparrow, bishopeagle)
   *	kend==vec_rook_end, kanf==vec_rook_start: diagonal types
   *                            (bishopmoose, bishopsparrow, rookeagle)
   *
   *	YES, this is definitely different to generating their moves ...
   *								     NG
   */

  square sq_departure;

  for (k= kend; k>=kanf; k--) {
	sq_hurdle= sq_king+vec[k];
	if (abs(e[sq_hurdle])>=roib) {
      k1= 2*k;
      finligne(sq_hurdle,mixhopdata[angle][k1],hopper,sq_departure);
      if (hopper==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
      finligne(sq_hurdle,mixhopdata[angle][k1-1],hopper,sq_departure);
      if (hopper==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
	}
  }
  return false;
}

boolean rcsech(square  sq_king,
               numvec  k,
               numvec  k1,
               piece   p,
               evalfunction_t *evaluate)		/* V2.1c  NG */
{
  /* ATTENTION: There is a parameter dependency between the
   *		  indexes of k and of k1 !
   *			  p		  index of k (ik) I index of k1
   *		  ----------------------------------------I------------
   *		  Spiralspringer	   9 to 16	  I 25 - ik
   *							  I
   *		  DiagonalSpiralspringer   9 to 14	  I 23 - ik
   *		  DiagonalSpiralspringer  15 to 16	  I 27 - ik
   */

  square sq_departure= sq_king+k;
  square sq_arrival= sq_king;
  square sq_capture= sq_king;

  while (e[sq_departure] == vide) {
    sq_departure+= k1;
	if (e[sq_departure] != vide)
      break;
    else
      sq_departure+= k;
  }

  if (e[sq_departure]==p
      && evaluate(sq_departure,sq_arrival,sq_capture))			/* V3.02  TLi */
    return true;

  sq_departure = sq_king+k;
  while (e[sq_departure]==vide) {
    sq_departure-= k1;
	if (e[sq_departure]!=vide)
      break;
    else
      sq_departure+= k;
  }

  if (e[sq_departure]==p
      && evaluate(sq_departure,sq_arrival,sq_capture))			/* V3.02  TLi */
    return true;

  return false;
}

boolean nevercheck(
  square  i,
  piece   p,
  evalfunction_t *evaluate)		/* V3.81  ElB */
{
  return false;
}

boolean cscheck(
  square  i,
  piece   p,
  evalfunction_t *evaluate)		/* V2.1c  NG */
{
  numvec  k;

  for (k= vec_knight_start; k <= vec_knight_end; k++) {
	if (rcsech(i, vec[k], vec[25 - k], p, evaluate)) {
      return true;
	}
  }
  return false;
}

boolean bscoutcheck(
  square  i,
  piece   p,
  evalfunction_t *evaluate)		/* V3.05  NG */
{
  numvec  k;

  for (k= vec_bishop_start; k <= vec_bishop_end; k++) {
	if (rcsech(i, vec[k], vec[13 - k], p, evaluate)) {
      return true;
	}
  }
  return false;
}

boolean gscoutcheck(
  square  i,
  piece   p,
  evalfunction_t *evaluate)		/* V3.05  NG */
{
  numvec  k;

  for (k= vec_rook_end; k >= vec_rook_start; k--) {
	if (rcsech(i, vec[k], vec[5 - k], p, evaluate)) {
      return true;
	}
  }
  return false;
}

boolean rrefcech(square	sq_king,
                 square	i1,
                 smallint	x,
                 piece	p,
                 evalfunction_t *evaluate)	/* V2.1c  NG */
{
  numvec k;

  /* ATTENTION:   first call of rrefech: x must be 2 !!   */

  square sq_departure;

  for (k= vec_knight_start; k <= vec_knight_end; k++)
	if (x) {
      sq_departure= i1+vec[k];
      if (e[sq_departure]==vide) {
		/* if (boolnoedge[sq_departure]) */
        /*		if (NoEdge(sq_departure)) {			/+ V3.22  TLi */
		if (!NoEdge(sq_departure)) {            /* V3.22  TLi, V4.03  NG */
          if (rrefcech(sq_king,sq_departure,x-1,p,evaluate))
			return true;
		}
      }
      else if (e[sq_departure]==p
               && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
	}
	else
      for (k= vec_knight_start; k <= vec_knight_end; k++) {
        sq_departure= i1+vec[k];
		if (e[sq_departure]==p
            && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
  
  return false;
}

boolean nequicheck(square	sq_king,
                   piece	p,
                   evalfunction_t *evaluate)   /* V2.60  NG */
{
  /* check by non-stop equihopper? */
  numvec delta_horiz, delta_vert;
  numvec vector;
  square sq_hurdle;
  square sq_departure;

  square const coin= coinequis(sq_king);

  for (delta_horiz= 3*dir_right;
       delta_horiz!=dir_left;
       delta_horiz+= dir_left)

	for (delta_vert= 3*dir_up;
         delta_vert!=dir_down;
         delta_vert+= dir_down) {	/* V2.90  NG */

      sq_hurdle= coin+delta_horiz+delta_vert;
      vector= sq_king-sq_hurdle;
      sq_departure= sq_hurdle-vector;

      if (e[sq_hurdle]!=vide	 /* V1.5c, V2.90  NG */
          && e[sq_departure]==p		 /* V1.5c, V2.90  NG */
          && sq_king!=sq_departure					/* V3.54  NG */
          && evaluate(sq_departure,sq_king,sq_king)	/* V3.02  TLi */
          && hopimcheck(sq_departure,
                        sq_king,
                        sq_hurdle,
                        vector))		/* V3.12  TM */
		return true;
	}

  return false;
}

boolean equifracheck(square	sq_king,
                     piece	p,
                     evalfunction_t *evaluate)   /* V2.60  NG */
{
  /* check by non-stop equistopper? */
  square sq_hurdle;
  square *bnp;
  numvec vector;
  square sq_departure;

  for (bnp= boardnum; *bnp; bnp++)
  {
    sq_departure= *bnp;
    vector= sq_king-sq_departure;
    sq_hurdle= sq_king+vector;
    if (e[sq_hurdle]!=vide	 /* V1.5c, V2.90  NG */
        && e[sq_hurdle]!=obs
        && e[sq_departure]==p		 /* V1.5c, V2.90  NG */
        && sq_king!=sq_departure					/* V3.54  NG */
        && evaluate(sq_departure,sq_king,sq_king))	/* V3.02  TLi */
      return true;
  }

  return false;
}

boolean vizircheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_rook_start, vec_rook_end, p, evaluate);		/* V2.60  NG */
}

boolean dabcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_dabbaba_start, vec_dabbaba_end, p, evaluate);		/* V2.60  NG */
}

boolean ferscheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_bishop_start, vec_bishop_end, p, evaluate);		/* V2.60  NG */
}


boolean alfilcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_alfil_start, vec_alfil_end, p, evaluate);		/* V2.60  NG */
}

boolean rccinqcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_rccinq_start, vec_rccinq_end, p, evaluate);		/* V2.60  NG */
}


boolean bucheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_bucephale_start, vec_bucephale_end, p, evaluate);		/* V2.60  NG */
}


boolean gicheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_girafe_start, vec_girafe_end, p, evaluate);		/* V2.60  NG */
}

boolean chcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_chameau_start, vec_chameau_end, p, evaluate);		/* V2.60  NG */
}


boolean zcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_zebre_start, vec_zebre_end, p, evaluate);		/* V2.60  NG */
}

boolean leap16check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.38  NG */
{
  return leapcheck(i, vec_leap16_start, vec_leap16_end, p, evaluate);
}

boolean leap24check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)     /* V3.42  TLi */
{
  return leapcheck(i, vec_leap24_start, vec_leap24_end, p, evaluate);
}

boolean leap35check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.42  TLi */
{
  return leapcheck(i, vec_leap35_start, vec_leap35_end, p, evaluate);
}

boolean leap37check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.34  TLi */
{
  return leapcheck(i, vec_leap37_start, vec_leap37_end, p, evaluate);
}

boolean okapicheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.32  TLi */
{
  return leapcheck(i, vec_okapi_start, vec_okapi_end, p, evaluate);	  /* knight+zebra !!! */
}

boolean bisoncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.60  TLi */
{
  return leapcheck(i, vec_bison_start, vec_bison_end, p, evaluate);	   /* camel+zebra !!! */
}

boolean elephantcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  TLi */
{
  return ridcheck(i, vec_elephant_start, vec_elephant_end, p, evaluate);	/* queen+nightrider  */
}

boolean ncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_knight_start, vec_knight_end, p, evaluate);		/* V2.60  NG */
}

boolean scheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rhopcheck(i, vec_queen_start, vec_queen_end, p, evaluate);		/* V2.60  NG */
}

boolean grasshop2check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.34  TLi */
{
  return rhop2check(i, vec_queen_start, vec_queen_end, p, evaluate);
}

boolean grasshop3check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.34  TLi */
{
  return rhop3check(i, vec_queen_start, vec_queen_end, p, evaluate);
}

/***** V3.44  SE  begin *****/

boolean kinghopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return shopcheck(i, vec_queen_start, vec_queen_end, p, evaluate);		/* SE */
}

boolean doublegrascheck(square	sq_king,
                        piece	p,
                        evalfunction_t *evaluate)	/* V3.44  SE */
{
  /* SE */ /* W.B.Trumper feenschach 1968 - but here
              null moves will not be allowed by Popeye
           */
  piece	doublegras;
  square	sq_hurdle2, sq_hurdle1;
  numvec	k, k1;

  square sq_departure;

  for (k=vec_queen_end; k>=vec_queen_start; k--) {
	sq_hurdle2= sq_king+vec[k];
	if (abs(e[sq_hurdle2])>=roib) {
      sq_hurdle2+= vec[k];
      while (e[sq_hurdle2]==vide) {
		for (k1= vec_queen_end; k1>=vec_queen_start; k1--) {
          sq_hurdle1= sq_hurdle2+vec[k1];
          if (abs(e[sq_hurdle1]) >= roib) {
			finligne(sq_hurdle1,vec[k1],doublegras,sq_departure);
			if (doublegras==p
                && evaluate(sq_departure,sq_king,sq_king))
              return true;
          }
		}
        sq_hurdle2+= vec[k];
      }
	}
  }
  
  return false;
}

/***** V3.44  SE  end *****/

/***** V3.22  TLi ***** begin *****/

boolean contragrascheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return crhopcheck(i, vec_queen_start, vec_queen_end, p, evaluate);
}

/***** V3.22 TLi  *****  end  *****/

boolean nightlocustcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.53  TLi */
{
  return marincheck(i, vec_knight_start, vec_knight_end, p, evaluate);
} /* nightlocustcheck */

boolean loccheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return marincheck(i, vec_queen_start, vec_queen_end, p, evaluate);
}

boolean tritoncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return marincheck(i, vec_rook_start, vec_rook_end, p, evaluate);
}

boolean nereidecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return marincheck(i, vec_bishop_start, vec_bishop_end, p, evaluate);
}

boolean nightriderlioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.64  TLi */
{
  return lrhopcheck(i, vec_knight_start, vec_knight_end, p, evaluate);
} /* nightriderlioncheck */

boolean lioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_queen_start,vec_queen_end, p, evaluate);		/* V2.60  NG */
}

boolean t_lioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean f_lioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);
}

/* see comment in py4.c on how rose and rose based pieces are
 * handled */
static boolean detect_rosecheck_on_line(square sq_king,
                                        piece p,
                                        numvec k, numvec k1,
                                        numvec delta_k,
                                        evalfunction_t *evaluate) {
  square sq_departure= fin_circle_line(sq_king,k,&k1,delta_k);
  return e[sq_departure]==p
    && sq_departure!=sq_king	      /* bug fixed. V1.3c  NG */
    && evaluate(sq_departure,sq_king,sq_king);	/* V3.02  TLi */
}

boolean rosecheck(square	sq_king,
                  piece	p,
                  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;
  for (k= vec_knight_start; k<=vec_knight_end; k++) {
    if (detect_rosecheck_on_line(sq_king,p,
                                 k,0,+1,
                                 evaluate))
      return true;
    if (detect_rosecheck_on_line(sq_king,p,
                                 k,vec_knight_end-vec_knight_start+1,-1,
                                 evaluate))
      return true;
  }

  return false;
}

static boolean detect_roselioncheck_on_line(square sq_king,
                                            piece p,
                                            numvec k, numvec k1,
                                            numvec delta_k,
                                            evalfunction_t *evaluate) {
  square sq_hurdle= fin_circle_line(sq_king,k,&k1,delta_k);
  if (sq_hurdle!=sq_king && e[sq_hurdle]!=obs) {
    square sq_departure= fin_circle_line(sq_hurdle,k,&k1,delta_k);
    if (e[sq_departure]==p
        && sq_departure!=sq_king	      /* bug fixed. V1.3c  NG */
        && evaluate(sq_departure,sq_king,sq_king))/* V3.02  TLi */
      return true;
  }

  return false;
}

boolean roselioncheck(square	sq_king,
                      piece	p,
                      evalfunction_t *evaluate)
{
  /* detects check by a rose lion  -	V3.23  TLi */
  numvec  k;
  for (k= vec_knight_start; k <= vec_knight_end; k++)
    if (detect_roselioncheck_on_line(sq_king,p,
                                     k,0,+1,
                                     evaluate)
        || detect_roselioncheck_on_line(sq_king,p,
                                        k,vec_knight_end-vec_knight_start+1,-1,
                                        evaluate))
      return true;

  return false;
} /* roselioncheck */

static boolean detect_rosehoppercheck_on_line(square sq_king,
                                              square sq_hurdle,
                                              piece p,
                                              numvec k, numvec k1,
                                              numvec delta_k,
                                              evalfunction_t *evaluate) {
  square sq_departure= fin_circle_line(sq_hurdle,k,&k1,delta_k);
  return e[sq_departure]==p
    && sq_departure!=sq_king
    && evaluate(sq_departure,sq_king,sq_king);
}

boolean rosehoppercheck(square	sq_king,
                        piece	p,
                        evalfunction_t *evaluate) {
  /* detects check by a rose hopper  -  V3.23  TLi */
  numvec  k;
  square sq_hurdle;

  for (k= vec_knight_start; k <= vec_knight_end; k++) {
    sq_hurdle= sq_king+vec[k];
    if (e[sq_hurdle]!=vide && e[sq_hurdle]!=obs) {
        /* k1==0 (and the equivalent
         * vec_knight_end-vec_knight_start+1) were already used for
         * sq_hurdle! */
      if (detect_rosehoppercheck_on_line(sq_king,sq_hurdle,p,
                                         k,1,+1,
                                         evaluate))
        return true;
      if (detect_rosehoppercheck_on_line(sq_king,sq_hurdle,p,
                                         k,vec_knight_end-vec_knight_start,-1,
                                         evaluate))
      return true;
    }
  }

  return false;
} /* rosehoppercheck */

boolean maocheck(square	sq_king,
                 piece	p,
                 evalfunction_t *evaluate)	/* V2.60  NG */
{
  square sq_departure;
    
  if (e[sq_king+dir_up+dir_right]==vide) {
    sq_departure= sq_king+dir_up+2*dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_up+2*dir_right,
                            sq_king,
                            sq_king+dir_up+dir_right); /* V3.81 SE */
	}
    sq_departure= sq_king+2*dir_up+dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_up+dir_right,
                            sq_king,
                            sq_king+dir_up+dir_right); /* V3.81 SE */
	}
  }
  
  if (e[sq_king+dir_down+dir_left]==vide) {
    sq_departure= sq_king+dir_down+2*dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_down+2*dir_left,
                            sq_king,
                            sq_king+dir_down+dir_left); /* V3.81 SE */
	}
    sq_departure= sq_king+2*dir_down+dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_down+dir_left,
                            sq_king,
                            sq_king+dir_down+dir_left); /* V3.81 SE */
	}
  }
  
  if (e[sq_king+dir_up+dir_left]==vide) {
    sq_departure= sq_king+dir_up+2*dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_up+2*dir_left,
                            sq_king,
                            sq_king+dir_up+dir_left); /* V3.81 SE */
	}
    sq_departure= sq_king+2*dir_up+dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_up+dir_left,
                            sq_king,
                            sq_king+dir_up+dir_left); /* V3.81 SE */
	}
  }
  
  if (e[sq_king+dir_down+dir_right]==vide) {
    sq_departure= sq_king+2*dir_down+dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_down+dir_right,
                            sq_king,
                            sq_king+dir_down+dir_right); /* V3.81 SE */
	}
    sq_departure= sq_king+dir_down+2*dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_down+2*dir_right,
                            sq_king,
                            sq_king+dir_down+dir_right); /* V3.81 SE */
	}
  }
  
  return false;
}

boolean moacheck(square	sq_king,
                 piece	p,
                 evalfunction_t *evaluate)	/* V2.60  NG */
{

  square sq_departure;
    
  if (e[sq_king+dir_up]==vide) {
    sq_departure= sq_king+2*dir_up+dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_up+dir_left, sq_king, sq_king+dir_up); /* V3.81 SE */
	}
    sq_departure= sq_king+2*dir_up+dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_up+dir_right, sq_king, sq_king+dir_up); /* V3.81 SE */
	}
  }
  if (e[sq_king+dir_down]==vide) {
    sq_departure= sq_king+2*dir_down+dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_down+dir_right, sq_king, sq_king+dir_down); /* V3.81 SE */
	}
    sq_departure= sq_king+2*dir_down+dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+2*dir_down+dir_left, sq_king, sq_king+dir_down); /* V3.81 SE */
	}
  }
  if (e[sq_king+dir_right]==vide) {
    sq_departure= sq_king+dir_up+2*dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_up+2*dir_right, sq_king, sq_king+dir_right); /* V3.81 SE */
	}
    sq_departure= sq_king+dir_down+2*dir_right;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_down+2*dir_right, sq_king, sq_king+dir_right); /* V3.81 SE */
	}
  }
  if (e[sq_king+dir_left]==vide) {
    sq_departure= sq_king+dir_down+2*dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_down+2*dir_left, sq_king, sq_king+dir_left); /* V3.81 SE */
	}
    sq_departure= sq_king+dir_up+2*dir_left;
	if (e[sq_departure]==p) {
      if (evaluate(sq_departure,sq_king,sq_king))  /* V3.02  TLi */
		return maooaimcheck(sq_king+dir_up+2*dir_left, sq_king, sq_king+dir_left); /* V3.81 SE */
	}
  }

  return false;
}

boolean paocheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_rook_start,vec_rook_end, p, evaluate);		/* V2.60  NG */
}

boolean vaocheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);		/* V2.60  NG */
}

boolean naocheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  TLi */
{
  return lrhopcheck(i, vec_knight_start,vec_knight_end, p, evaluate);		/* V3.62  TLi */
}

boolean leocheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return lrhopcheck(i, vec_queen_start,vec_queen_end, p, evaluate);		/* V2.60  NG */
}

boolean pbcheck(square	sq_king,
                piece	p,
                evalfunction_t *evaluate)	/* V2.60  NG */
{
  if (anymars) {	    /* NG  3.47 */
	boolean anymarscheck=
      (p==e[rb] && e[sq_king+dir_down]==p)
      || (p==e[rn] && e[sq_king+dir_up]==p);
	if (!CondFlag[phantom] || anymarscheck)
      return anymarscheck;
  }

  if (p<=roin) {					/* V3.47  NG */
	if (sq_king<=square_h6				/* V3.02  TLi */
        || CondFlag[parrain]
        || CondFlag[normalp]
        || CondFlag[einstein]				 /* V3.2  TLi */
        || p==orphann)				 /* V3.2  TLi */
	{
      square sq_departure= sq_king+dir_up;
            
      if (e[sq_departure]==p
	      && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
		return true;

      if (ep[nbply]!=initsquare
          && RB_[nbply]!=rb
          && (rb==ep[nbply]+dir_up+dir_left
              || rb==ep[nbply]+dir_up+dir_right)) {	/* V3.45  TLi */ /* V3.78  SE */ /* V3.80  NG */
		/* ep captures of royal pawns */
        sq_departure= ep[nbply]+dir_up;
		if (e[sq_departure]==pbn
            && evaluate(sq_departure,ep[nbply],sq_king))
          imech(sq_departure,ep[nbply]);
      }
	}
  }
  else {	  /* hopefully (p >= roib) */		/* V3.47  NG */
	if (sq_king>=square_a3				/* V3.02  TLi */
        || CondFlag[parrain]
        || CondFlag[normalp]
        || CondFlag[einstein]				/* V3.2  TLi */
        || p==orphanb)
	{						 /* V3.2  TLi */
      square sq_departure= sq_king+dir_down;
            
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
		return true;
      
      if (ep[nbply]!=initsquare
          && RN_[nbply]!=rn
          && (rn==ep[nbply]+dir_down+dir_right
              || rn==ep[nbply]+dir_down+dir_left)) {	    /* V3.45  TLi */ /* V3.78  SE */ /* V3.80  NG */
		/* ep captures of royal pawns */
        sq_departure= ep[nbply]+dir_down;
		if (e[sq_departure] == pbb
            && evaluate(sq_departure,ep[nbply],sq_king))
          imech(sq_departure,ep[nbply]);
      }
	}
  }

  return false;
}

boolean bspawncheck(square	sq_king,
                    piece	p,
                    evalfunction_t *evaluate)	/* V2.60  NG */
{
  piece   p1;

  square sq_departure;
    
  if (p==bspawnn
      || (CondFlag[blrefl_king] && p==roin))		/* V3.04  NG */
  {
	if (sq_king<=square_h7) {	       /* it can move from eigth rank */
      finligne(sq_king,+dir_up,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
	}
  }
  else {/* hopefully ((p == bspawnb)
           || (CondFlag[whrefl_king] && p == roib)) */	/* V3.04  NG */

	if (sq_king>=square_a2) {	       /* it can move from first rank */
      finligne(sq_king,+dir_down,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
	}
  }
  
  return false;
}

boolean spawncheck(square	sq_king,
                   piece	p,
                   evalfunction_t *evaluate)	/* V2.60  NG */
{
  piece   p1;

  square sq_departure;
    
  if (p==spawnn
      || (CondFlag[blrefl_king] && p==roin))		/* V3.04  NG */
  {
    if (sq_king<=square_h7) {	       /* it can move from eigth rank */
      finligne(sq_king,dir_up+dir_left,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;

      finligne(sq_king,+dir_up+dir_right,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
    }
  }
  else {/* hopefully ((p == bspawnb)
           || (CondFlag[whrefl_king] && p == roib)) */  /* V3.04  NG */
    if (sq_king>=square_a2) {	       /* it can move from first rank */
      finligne(sq_king,+dir_down+dir_right,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;

      finligne(sq_king,+dir_down+dir_left,p1,sq_departure);
      if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
    }
  }

  return false;
}

boolean amazcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		 /* V2.60  NG */
    || ridcheck(i, vec_queen_start,vec_queen_end, p, evaluate);
}

boolean impcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		/* V2.60  NG */
    || ridcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean princcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		/* V2.60  NG */
    || ridcheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);
}

boolean gnoucheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		/* V2.60  NG */
    || leapcheck(i, vec_chameau_start, vec_chameau_end, p, evaluate);
}

boolean antilcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_antilope_start, vec_antilope_end, p, evaluate);		/* V2.60  NG */
}

boolean ecurcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		/* V2.60  NG */
    || leapcheck(i, vec_ecureuil_start, vec_ecureuil_end, p, evaluate);
}

boolean warancheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  ridcheck(i, vec_knight_start,vec_knight_end, p, evaluate)		/* V2.60  NG */
    || ridcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean dragoncheck(square	sq_king,
                    piece	p,
                    evalfunction_t *evaluate)	/* V2.60  NG */
{
  if (leapcheck(sq_king,vec_knight_start,vec_knight_end,p,evaluate))		/* V2.60  NG */
	return true;

  square sq_departure;
    
  if (p==dragonn
      || (CondFlag[blrefl_king] && p==roin))	/* V3.04  NG */
  {
	if (sq_king<=square_h6				/* V3.04  NG */
        || CondFlag[parrain]
        || CondFlag[normalp])
	{
      sq_departure= sq_king+dir_up+dir_left;
      if (e[sq_departure]==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }

      sq_departure= sq_king+dir_up+dir_right;
      if (e[sq_departure]==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
    }
  }
  else {/* hopefully ((p == dragonb)
           || (CondFlag[whrefl_king] && p == roib)) */	/* V3.04  NG */

	if (sq_king>=square_a3				/* V3.04  NG */
        || CondFlag[parrain]
        || CondFlag[normalp])
	{
      sq_departure= sq_king+dir_down+dir_right;
      if (e[sq_departure]==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }

      sq_departure= sq_king+dir_down+dir_left;
      if (e[sq_departure]==p) {
		if (evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
	}
  }

  return false;
}

boolean kangoucheck(square	sq_king,
                    piece	p,
                    evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;
  piece   p1;
  square sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {
	sq_hurdle= sq_king+vec[k];					/* V2.51  NG */
	if (abs(e[sq_hurdle])>=roib) {
      finligne(sq_hurdle,vec[k],p1,sq_hurdle);
      if (p1!=obs) {
		finligne(sq_hurdle,vec[k],p1,sq_departure);
		if (p1==p && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
          return true;
      }
	}
  }

  return false;
}

boolean rabbitcheck(square	sq_king,
                    piece	p,
                    evalfunction_t *evaluate)	/* V3.76  NG */
{
  /* 2 hurdle lion */
  numvec  k;
  piece   p1;
  square sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {
	finligne(sq_king,vec[k],p1,sq_hurdle);
	if (abs(p1)>=roib) {
      finligne(sq_hurdle,vec[k],p1,sq_hurdle);
      if (p1!=obs) {
		finligne(sq_hurdle,vec[k],p1,sq_departure);
		if (p1==p && evaluate(sq_departure,sq_king,sq_king))
          return true;
      }
	}
  }

  return false;
}

boolean bobcheck(square	sq_king,
                 piece	p,
                 evalfunction_t *evaluate)	/* V3.76  NG */
{
  /* 4 hurdle lion */
  numvec  k;
  piece   p1;
  square sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {
	finligne(sq_king,vec[k],p1,sq_hurdle);
	if (abs(p1)>=roib) {
      finligne(sq_hurdle,vec[k],p1,sq_hurdle);
      if (p1!=obs) {
		finligne(sq_hurdle,vec[k],p1,sq_hurdle);
		if (p1!=obs) {
          finligne(sq_hurdle,vec[k],p1,sq_hurdle);
          if (p1!=obs) {
			finligne(sq_hurdle,vec[k],p1,sq_departure);
            if (p1==p && evaluate(sq_departure,sq_king,sq_king))
              return true;
          }
		}
      }
	}
  }

  return false;
}

boolean ubicheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  square  *bnp;

  if (evaluate == eval_madrasi) {
	for (bnp= boardnum; *bnp; bnp++) {
      e_ubi_mad[*bnp]= e[*bnp];
	}
	return rubiech(i, i, p, e_ubi_mad, evaluate);
  }
  else {
	for (bnp= boardnum; *bnp; bnp++) {
      e_ubi[*bnp]= e[*bnp];
	}
	return rubiech(i, i, p, e_ubi, evaluate);
  }
}

boolean moosecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rmhopech(i, vec_queen_end,vec_queen_start, angle_45, p, evaluate);
}

boolean eaglecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rmhopech(i, vec_queen_end,vec_queen_start, angle_90, p, evaluate);
}

boolean sparrcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rmhopech(i, vec_queen_end,vec_queen_start, angle_135, p, evaluate);
}

boolean margueritecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.64  TLi */
{
  return  sparrcheck(i, p, evaluate)
    || eaglecheck(i, p, evaluate)
    || moosecheck(i, p, evaluate)
    || scheck(i, p, evaluate);
} /* margueritecheck */

boolean leap36check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.64  TLi */
{
  return leapcheck(i, vec_leap36_start, vec_leap36_end, p, evaluate);
} /* leap36check */

boolean rookmoosecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_rook_end,vec_rook_start, angle_45, p, evaluate);
}

boolean rookeaglecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_bishop_end,vec_bishop_start, angle_90, p, evaluate);
}

boolean rooksparrcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_rook_end,vec_rook_start, angle_135, p, evaluate);
}

boolean bishopmoosecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_bishop_end,vec_bishop_start, angle_45, p, evaluate);
}

boolean bishopeaglecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_rook_end,vec_rook_start, angle_90, p, evaluate);
}

boolean bishopsparrcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.62  NG */
{
  return rmhopech(i, vec_bishop_end,vec_bishop_start, angle_135, p, evaluate);
}

boolean archcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;

  for (k= vec_bishop_start; k <= vec_bishop_end; k++) {
	if (rrfouech(i, i, vec[k], p, 1, evaluate)) {	/* V2.4c  NG */
      return true;
	}
  }
  return false;
}

boolean reffoucheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;

  for (k= vec_bishop_start; k <= vec_bishop_end; k++) {
	if (rrfouech(i, i, vec[k], p, 4, evaluate)) {	/* V2.4c  NG */
      return true;
	}
  }
  return false;
}

boolean cardcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;

  for (k= vec_bishop_start; k <= vec_bishop_end; k++) {
	if (rcardech(i, i, vec[k], p, 1, evaluate)) {	/* V2.4c  NG */
      return true;
	}
  }
  return false;
}

boolean nsautcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rhopcheck(i, vec_knight_start,vec_knight_end, p, evaluate);
}

boolean camridcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_chameau_start, vec_chameau_end, p, evaluate);
}

boolean zebridcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_zebre_start, vec_zebre_end, p, evaluate);
}

boolean gnuridcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  ridcheck(i, vec_knight_start,vec_knight_end, p, evaluate)
    || ridcheck(i, vec_chameau_start, vec_chameau_end, p, evaluate);
}

boolean camhopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rhopcheck(i, vec_chameau_end, vec_chameau_end, p, evaluate);
}

boolean zebhopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rhopcheck(i, vec_zebre_start, vec_zebre_end, p, evaluate);
}

boolean gnuhopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return  rhopcheck(i, vec_knight_start,vec_knight_end, p, evaluate)
    || rhopcheck(i, vec_chameau_start, vec_chameau_end, p, evaluate);
}

boolean dcscheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;

  for (k= vec_knight_start; k <= 14; k++) {
	if (rcsech(i, vec[k], vec[23 - k], p, evaluate)) {
      return true;
	}
  }
  for (k= 15; k <= vec_knight_end; k++) {
	if (rcsech(i, vec[k], vec[27 - k], p, evaluate)) {
      return true;
    }
  }
  return false;
}

boolean refccheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return rrefcech(i, i, 2, p, evaluate);
}

boolean equicheck(square	sq_king,
                  piece	p,
                  evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;
  piece   p1;
  square  sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {	    /* 0,2; 0,4; 0,6; 2,2; 4,4; 6,6; */
	finligne(sq_king,vec[k],p1,sq_hurdle);
	if (p1!=obs) {
      finligne(sq_hurdle,vec[k],p1,sq_departure);
      if (p1==p
          && sq_departure-sq_hurdle==sq_hurdle-sq_king
          && evaluate(sq_departure,sq_king,sq_king)		/* V3.02  TLi */
          && hopimcheck(sq_departure,
                        sq_king,
                        sq_hurdle,
                        -vec[k]))	/* V3.12  TM */
        return true;
	}
  }

  for (k= vec_equi_nonintercept_start; k<=vec_equi_nonintercept_end; k++) {      /* 2,4; 2,6; 4,6; */
    sq_departure= sq_king+2*vec[k];
	if (abs(e[sq_king+vec[k]])>=roib
        && e[sq_departure]==p
        && evaluate(sq_departure,sq_king,sq_king)		/* V3.02  TLi */
        && hopimcheck(sq_departure,
                      sq_king,
                      sq_departure-vec[k],
                      -vec[k]))	/* V3.12  TM */
      return true;
  }

  return false;
}

boolean equiengcheck(square	sq_king,
                     piece	p,
                     evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;
  piece   p1;
  square  sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {	    /* 0,2; 0,4; 0,6; 2,2; 4,4; 6,6; */
	finligne(sq_king,vec[k],p1,sq_hurdle);
	if (p1!=obs) {
      finligne(sq_king,-vec[k],p1,sq_departure);
      if (p1==p
          && sq_departure-sq_king==sq_king-sq_hurdle
          && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;
	}
  }

  for (k= vec_equi_nonintercept_start; k<=vec_equi_nonintercept_end; k++) {      /* 2,4; 2,6; 4,6; */
    sq_departure= sq_king-vec[k];
    sq_hurdle= sq_king+vec[k];
	if (abs(e[sq_hurdle])>=roib
        && e[sq_departure]==p
        && evaluate(sq_departure,sq_king,sq_king))			/* V3.02  TLi */
      return true;
  }
  
  return false;
}

boolean catcheck(square	sq_king,
                 piece	p,
                 evalfunction_t *evaluate)	/* V2.60  NG */
{
  numvec  k;
  square  middle_square;

  square sq_departure;
    
  if (leapcheck(sq_king,vec_knight_start,vec_knight_end,p,evaluate)) {
	return true;
  }

  for (k= vec_dabbaba_start; k<=vec_dabbaba_end; k++) {
	middle_square= sq_king+vec[k];
	while (e[middle_square]==vide) {
      sq_departure= middle_square+mixhopdata[3][k-60];
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king))
        return true;

      sq_departure= middle_square+mixhopdata[3][k-56];
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king))		/* V3.02  TLi */
        return true;

      middle_square+= vec[k];
	}
  }

  return false;
}

boolean roicheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_queen_start,vec_queen_end, p, evaluate);
}

boolean cavcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return leapcheck(i, vec_knight_start,vec_knight_end, p, evaluate);
}

boolean damecheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_queen_start,vec_queen_end, p, evaluate);
}

boolean tourcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean foucheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.60  NG */
{
  return ridcheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);
}

boolean pioncheck(square sq_king,
                  piece	p,
                  evalfunction_t *evaluate)	/* V2.60  NG */
{
  square sq_departure;

  if (anymars) {				      /* SE/TLi  3.46 */
	boolean anymarscheck=				/* V3.47  NG */
      (p==e[rb]
       && (e[sq_king+dir_down+dir_right]==p || e[sq_king+dir_down+dir_left]==p))
      || (p==e[rn]
          && (e[sq_king+dir_up+dir_left]==p || e[sq_king+dir_up+dir_right]==p));
	if (!CondFlag[phantom] || anymarscheck) {		/* V3.47  NG */
      return anymarscheck;
	}
  }

  if (p<=roin) {					/* V3.2  TLi */
	if (sq_king<=square_h6				/* V3.02  TLi */
        || CondFlag[parrain]
        || CondFlag[normalp]
        || CondFlag[einstein]			/* V3.2  TLi */
        || p==orphann 				/* V3.2  TLi */
        || p<=hunter0n)
	{
      sq_departure= sq_king+dir_up+dir_left;
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king)) 	/* V3.02  TLi */
        return true;

      sq_departure= sq_king+dir_up+dir_right;
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king)) 	/* V3.02  TLi */
        return true;
	}
  }
  else {	  /* hopefully (p >= roib) */		/* V3.21  NG */
	if (sq_king>=square_a3				/* V3.02  TLi */
        || CondFlag[parrain]
        || CondFlag[normalp]
        || CondFlag[einstein]			/* V3.2  TLi */
        || p==orphanb 				/* V3.2  TLi */
        || p>=hunter0b)
	{
      sq_departure= sq_king+dir_down+dir_right;
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king)) 	/* V3.02  TLi */
        return true;

      sq_departure= sq_king+dir_down+dir_left;
      if (e[sq_departure]==p
          && evaluate(sq_departure,sq_king,sq_king)) 	/* V3.02  TLi */
        return true;
	}
  }

  return false;
}

boolean ep_not_libre(
  piece	p,
  square	sq,
  boolean	generating,   /* V3.44	TLi */
  checkfunction_t	*checkfunc)
{
  /* Returns true if a pawn who has just crossed the square sq is
     paralysed by a piece p due to the ugly Madrasi-ep-rule by a
     pawn p.
     ---------------------------------------------------------
     Dear inventors of fairys:
     Make it as sophisticated and inconsistent as possible!
     ---------------------------------------------------------

     Checkfunc must be the corresponding checking function.

     pawn just moved	    p	    checkfunc
     --------------------------------------
     white pawn	    pn	    pioncheck
     black pawn	    pb	    pioncheck
     white berolina pawn  pbn     pbcheck
     black berolina pawn  pbb     pbcheck
  */

  ply ply_dblstp= generating ? nbply-1 : nbply;

  return (ep[ply_dblstp]==sq || ep2[ply_dblstp]==sq)
    && nbpiece[p]
    && (*checkfunc)(sq,
                    p,
                    flaglegalsquare ? legalsquare : eval_ortho);
} /* end eplibre */

boolean libre(square sq, boolean generating) {
  piece   p= e[sq];		/* V3.51  NG */
  boolean flag= true, neutcoul_sic= neutcoul;

  if ((CondFlag[madras] || CondFlag[isardam])		/* V3.76  TLi */
      && (!rex_mad) && ((sq == rb) || (sq == rn)))
    return true;

  if (TSTFLAG(spec[sq], Neutral)) {		/* V3.76  TLi */
	if (generating)
      p= -p;
	else
      initneutre(advers(neutcoul));
  }

  if (CondFlag[madras] || CondFlag[isardam]) {	/* V3.60  TLi */

	/* The ep capture needs special handling. */
	switch (p) {					/* V3.22  TLi */

    case pb: /* white pawn */
      if (ep_not_libre(pn, sq+dir_down, generating, pioncheck)) {
		flag= False;
      }
      break;

    case pn: /* black pawn */
      if (ep_not_libre(pb, sq+dir_up, generating, pioncheck)) {
		flag= False;
      }
      break;

    case pbb: /* white berolina pawn */
      if ( ep_not_libre(pbn, sq+dir_down+dir_right, generating, pbcheck)
           || ep_not_libre(pbn, sq+dir_down+dir_left, generating, pbcheck))
      {
		flag= False;
      }
      break;

    case pbn: /* black berolina pawn */
      if (ep_not_libre(pbb, sq+dir_up+dir_left, generating, pbcheck)
          || ep_not_libre(pbb, sq+dir_up+dir_right, generating, pbcheck))
      {
		flag= False;
      }
      /* NB: Super (Berolina) pawns cannot neither be captured
         ep nor capture ep themselves.
      */
      break;
	}

	flag = flag
      && (!nbpiece[-p]
          || !(*checkfunctions[abs(p)])(sq, -p,
                                        flaglegalsquare ? legalsquare : eval_ortho));
  } /* if (CondFlag[madrasi] ... */

  if (CondFlag[eiffel]) {				/* V3.60  TLi */
	boolean test= true;
	piece eiffel_piece;

	switch (p) {					/* V3.22  TLi */
    case pb: eiffel_piece= dn; break;
    case db: eiffel_piece= tn; break;
    case tb: eiffel_piece= fn; break;
    case fb: eiffel_piece= cn; break;
    case cb: eiffel_piece= pn; break;
    case pn: eiffel_piece= db; break;
    case dn: eiffel_piece= tb; break;
    case tn: eiffel_piece= fb; break;
    case fn: eiffel_piece= cb; break;
    case cn: eiffel_piece= pb; break;
    default:
      test= false;
      eiffel_piece= 0;	 /* avoid compiler warning. ElB, 2001-12-16. */
      break;
	}

	if (test) {
      flag = flag
        && (!nbpiece[eiffel_piece]
            || !(*checkfunctions[abs(eiffel_piece)])(sq, eiffel_piece,
                                                     flaglegalsquare ? legalsquare : eval_ortho));
	}
  } /* CondFlag[eiffel] */

  if (TSTFLAG(spec[sq], Neutral) && !generating)	/* V3.76  TLi */
	initneutre(neutcoul_sic);

  return flag;
} /* libre */

boolean soutenu(square sq_departure, square sq_arrival, square sq_capture) {
  /*  V3.02  TLi */
  piece	p= 0;	    /* avoid compiler warning ElB, 2001-12-16 */
  boolean	Result;
  evalfunction_t *evaluate;

  if (CondFlag[central]) {				 /* V3.50 SE */
	if ( sq_departure == rb || sq_departure == rn) {
      return true;
	}
	nbpiece[p= e[sq_departure]]--;
	e[sq_departure]= (p > vide) ? dummyb : dummyn;
	evaluate= soutenu;
  }
  else if (flaglegalsquare) {
	if (!legalsquare(sq_departure,sq_arrival,sq_capture)) {
      return false;
	}
	evaluate= legalsquare;
  }
  else if (flag_madrasi) {			/* V3.32, V3.60  TLi */
	if (!eval_madrasi(sq_departure,sq_arrival,sq_capture)) {
      return false;
	}
	evaluate= eval_madrasi;
  }
  else if (TSTFLAG(PieSpExFlags,Paralyse)) {		/* V3.32  TLi */
	if (!paraechecc(sq_departure,sq_arrival,sq_capture)) {
      return false;
	}
	evaluate= paraechecc;
  }
  else {
	evaluate= eval_ortho;
  }

  if ((color(sq_departure)==blanc)			 /* V3.32, V3.53  TLi */
      != (CondFlag[beamten] || TSTFLAG(PieSpExFlags, Beamtet)))
  {
	if (  TSTFLAG(PieSpExFlags, Beamtet)
          && !TSTFLAG(spec[sq_departure], Beamtet))		/* V3.53  TLi */
	{
      Result= True;
	}
	else {
      sq_arrival= rn;
      rn= sq_departure;
      Result= rnechec(evaluate);
      rn= sq_arrival;
	}
  }
  else {
	if ( TSTFLAG(PieSpExFlags, Beamtet)
         && !TSTFLAG(spec[sq_departure], Beamtet))		/* V3.53  TLi */
	{
      Result= True;
	}
	else {
      sq_arrival= rb;
      rb= sq_departure;
      Result= rbechec(evaluate);
      rb= sq_arrival;
	}
  }

  if (CondFlag[central])				 /* V3.50 SE */
	nbpiece[e[sq_departure]= p]++;

  return(Result);
} /* soutenu */

boolean eval_madrasi(square sq_departure, square sq_arrival, square sq_capture) {
  if (flaglegalsquare
      && !legalsquare(sq_departure,sq_arrival,sq_capture)) {	/* V3.02  TLi */
	return false;
  }
  else {
    return libre(sq_departure, false) && 
      (!CondFlag[BGL] || eval_2(sq_departure,sq_arrival,sq_capture)); /* V4.06 SE */
    /* is this just appropriate for BGL? in verifieposition eval_2 is set when madrasi is true,
       but never seems to be used here or in libre */
  }
} /* eval_madrasi */

boolean eval_shielded(square sq_departure, square sq_arrival, square sq_capture) {  /* V3.62 SE */
  /* V3.02  TLi */
  if ((sq_departure==rn && sq_capture==rb)
      || (sq_departure==rb && sq_capture==rn)) {
	return !soutenu(sq_capture,sq_departure,sq_departure);  /* won't work for locust Ks etc.*/
  }
  else {
	return true;
  }
} /* eval_shielded */

boolean edgehcheck(square	sq_king,
                   piece	p,
                   evalfunction_t *evaluate)
{
  /* detect "check" of edgehog p */
  piece p1;
  numvec  k;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {				/* V3.00  NG */
	finligne(sq_king,vec[k],p1,sq_departure);
	if (p1==p					/* V3.22  TLi */
        && NoEdge(sq_king)!=NoEdge(sq_departure)
        && evaluate(sq_departure,sq_king,sq_king))			/* V3.02  TLi */
      return true;
  }

  return false;
}

/***************  V3.1	TLi  ***************/

boolean maooaridercheck(square	sq_king,
                        piece	p,
                        numvec	fir,
                        numvec	sec,
                        evalfunction_t *evaluate)
{
  square  middle_square;

  square sq_departure= sq_king+sec;
    
  middle_square = sq_king+fir;
  while (e[middle_square]==vide && e[sq_departure]==vide) {
	middle_square+= sec;
	sq_departure+= sec;
  }

  return e[middle_square]==vide
    && e[sq_departure]==p
    && evaluate(sq_departure,sq_king,sq_king);
} /* end of maooaridercheck */

boolean moaridercheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  if (maooaridercheck(i, p, +dir_up,+2*dir_up+dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p, +dir_up,+2*dir_up+dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down,+2*dir_down+dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down,+2*dir_down+dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,	+dir_right,+dir_up+2*dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,	+dir_right,+dir_down+2*dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_left,+dir_down+2*dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_left,+dir_up+2*dir_left, evaluate)) {
	return true;
  }
  return false;
}

boolean maoridercheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  if (maooaridercheck(i, p,+dir_up+dir_right,+2*dir_up+dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_up+dir_right,+dir_up+2*dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down+dir_right,+dir_down+2*dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down+dir_right,+2*dir_down+dir_right, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down+dir_left,+2*dir_down+dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,+dir_down+dir_left,+dir_down+2*dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,  dir_up+dir_left,+dir_up+2*dir_left, evaluate)) {
	return true;
  }
  if (maooaridercheck(i, p,  dir_up+dir_left,+2*dir_up+dir_left, evaluate)) {
	return true;
  }
  return false;
}

boolean maooariderlioncheck(square	sq_king,
                            piece	p,
                            numvec	fir,
                            numvec	sec,
                            evalfunction_t *evaluate)	/* V3.64  TLi */
{
  square middle_square= sq_king+fir;

  square sq_departure= sq_king+sec;
    
  while (e[middle_square]==vide && e[sq_departure]==vide) {
	middle_square+= sec;
	sq_departure+= sec;
  }
  if (e[middle_square]!=vide
      && e[sq_departure]==p
      && evaluate(sq_departure,sq_king,sq_king))
	return true;

  if (e[middle_square]!=obs
      && e[sq_departure]!=obs
      && (e[middle_square]==vide || e[sq_departure]==vide))
  {
	middle_square+= sec;
	sq_departure+= sec;
	while (e[middle_square]==vide && e[sq_departure]==vide) {
      middle_square+= sec;
      sq_departure+= sec;
	}
	if (e[middle_square]==vide
        && e[sq_departure]==p
        && evaluate(sq_departure,sq_king,sq_king))
      return true;
  }

  return false;
} /* maooariderlioncheck */

boolean maoriderlioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  if (maooariderlioncheck(i, p,+dir_up+dir_right,	+2*dir_up+dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_up+dir_right,	+dir_up+2*dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down+dir_right,+dir_down+2*dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down+dir_right,+2*dir_down+dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down+dir_left,+2*dir_down+dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down+dir_left,+dir_down+2*dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,  dir_up+dir_left,	+dir_up+2*dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,  dir_up+dir_left,	+2*dir_up+dir_left, evaluate)) {
	return true;
  }
  return false;
} /* maoriderlioncheck */

boolean moariderlioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.65  TLi */
{
  if (maooariderlioncheck(i, p, +dir_up,+2*dir_up+dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p, +dir_up,+2*dir_up+dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down,+2*dir_down+dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_down,+2*dir_down+dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_right,+dir_up+2*dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_right,+dir_down+2*dir_right, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_left,+dir_down+2*dir_left, evaluate)) {
	return true;
  }
  if (maooariderlioncheck(i, p,+dir_left,+dir_up+2*dir_left, evaluate)) {
	return true;
  }
  return false;
} /* moariderlioncheck */

boolean r_hopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return rhopcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean b_hopcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return rhopcheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);
}

/***************  V3.1	TLi  ***************/

boolean pos_legal(void) {			     /* V3.44  SE/TLi */
  /* could be used for other genres e.g. Ohneschach */

  if (CondFlag[isardam]) {
	square z;
	int i,j;

	initneutre(trait[nbply]);			 /* V3.50 SE */
    /* for e.p. captures */
	z=haut;
	for (i=8; i>0; i--, z-=16)
      for (j=8; j>0; j--, z--) {
	    if (e[z]!=vide) {
          if (!libre(z, false)) {
		    return false;
          }
	    }
      }
  }

  /* To avoid messing up the ???[nbply] arrays during output of
     the solution */
  if (flag_writinglinesolution) {		       /* V3.55  TLi */
	return true;
  }

  if (CondFlag[ohneschach]) {
	couleur camp= trait[nbply];
	couleur ad= advers(camp);

	if (nbply > maxply-1)  {
      FtlMsg(ChecklessUndecidable);
	}

	if (echecc(camp)) {
      return false;
	}

	if (echecc(ad) && !patt(ad)) {
      return false;
	}
  }

  if (CondFlag[exclusive]) {				/* V3.45  TLi */
	if (nbply > maxply-1) {
      FtlMsg(ChecklessUndecidable);
	}

	if (!mateallowed[nbply] && (*stipulation)(trait[nbply])) {
      return false;
	}
  }

  return true;
} /* pos_legal */

boolean eval_isardam(square sq_departure, square sq_arrival, square sq_capture) {
  /* V3.44  SE/TLi */
  boolean flag=false;
  couleur camp;

  /* the following does not suffice if we have neutral kings,
     but we have no chance to recover the information who is to
     move from sq_departure, sq_arrival and sq_capture.
     TLi
  */
  if (flag_nk) {	    /* V3.50 SE will this do for neutral Ks? */
	camp= neutcoul;
  }
  else if (sq_capture == rn) {
    camp=blanc;
  }
  else if (sq_capture == rb) {
    camp=noir;
  }
  else {
    camp= e[sq_departure]<0 ? noir : blanc;
  }

  nextply();
  trait[nbply]= camp;

  init_move_generation_optimizer();
  k_cap=true;		  /* set to allow K capture in e.g. AntiCirce */
  empile(sq_departure,sq_arrival,sq_capture);	  /* generate only the K capture */
  k_cap=false;
  finish_move_generation_optimizer();

  while (encore() && !flag) {
	/* may be several K capture moves e.g. PxK=S,B,R,Q */
	if (CondFlag[brunner]) {			 /* V3.50 SE */
      /* For neutral Ks will need to return true always */
      flag= jouecoup()
		&& (camp==blanc ? !echecc(blanc) : !echecc(noir));
	}
	else if (CondFlag[isardam]) {
      flag= jouecoup();
	}
	/* Isardam + Brunner may be possible! in which case this logic
	   is correct
    */
	repcoup();
  }

  finply();
  return flag;
} /* eval_isardam */


boolean orixcheck(square sq_king,
                  piece	p,
                  evalfunction_t *evaluate)	/* V3.44  NG */
{
  numvec  k;
  piece   p1;
  square  sq_hurdle;

  square sq_departure;
    
  for (k= vec_queen_end; k>=vec_queen_start; k--) {	    /* 0,2; 0,4; 0,6; 2,2; 4,4; 6,6; */
	finligne(sq_king,vec[k],p1,sq_hurdle);
	if (p1!=obs) {
      finligne(sq_hurdle,vec[k],p1,sq_departure);
      if (p1==p
          && sq_departure-sq_hurdle==sq_hurdle-sq_king
          && evaluate(sq_departure,sq_king,sq_king)
          && hopimcheck(sq_departure,
                        sq_king,
                        sq_hurdle,
                        -vec[k]))
        return true;
	}
  }

  return false;
}

boolean leap15check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.46  NG */
{
  return leapcheck(i, vec_leap15_start, vec_leap15_end, p, evaluate);
}

boolean leap25check(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.46  NG */
{
  return leapcheck(i, vec_leap25_start, vec_leap25_end, p, evaluate);
}

boolean gralcheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.46  NG */
{
  return leapcheck(i, vec_alfil_start, vec_alfil_end, p, evaluate)
	|| rhopcheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

/*** woozles + heffalumps ***/

square	sq_woo_from;
square	sq_woo_to;
couleur col_woo;

boolean aux_whx(square sq_departure, square sq_arrival, square sq_capture) {	/* V3.55 TLi */
  if (sq_departure != sq_woo_from)
	return false;

  /* sq_departure == sq_woo_from */
  if (CondFlag[heffalumps]) {
	smallint cd1= sq_departure%onerow - sq_arrival%onerow;
	smallint rd1= sq_departure/onerow - sq_arrival/onerow;
	smallint cd2= sq_woo_to%onerow - sq_departure%onerow;
	smallint rd2= sq_woo_to/onerow - sq_departure/onerow;
	smallint t= 7;

	if (cd1 != 0)
      t= abs(cd1);
	if (rd1 != 0 && t > abs(rd1))
      t= abs(rd1);

	while (!(cd1%t == 0 && rd1%t == 0))
      t--;
	cd1= cd1/t;
	rd1= rd1/t;

	t= 7;
	if (cd2 != 0)
      t= abs(cd2);
	if (rd2 != 0 && t > abs(rd2))
      t= abs(rd2);

	while (!(cd2%t == 0 && rd2%t == 0))
      t--;

	cd2= cd2/t;
	rd2= rd2/t;

	if (!(	(cd1 == cd2 && rd1 == rd2)
            || (cd1 == -cd2 && rd1 == -rd2)))
	{
      return false;
	}
  }

  return (flaglegalsquare ? legalsquare : eval_ortho)(sq_departure,sq_arrival,sq_capture);
} /* aux_whx */

boolean aux_wh(square sq_departure, square sq_arrival, square sq_capture) {	/* V3.55  TLi */
  if ((flaglegalsquare ? legalsquare : eval_ortho)(sq_departure,sq_arrival,sq_capture)) {
    piece const p= e[sq_woo_from];
    return nbpiece[p]
      && (*checkfunctions[abs(p)])(sq_departure, e[sq_woo_from], aux_whx);
  }
  else
    return false;
} /* aux_wh */

boolean woohefflibre(square to, square from) {		/* V3.55  TLi */

  piece   *pcheck, p;

  if (rex_wooz_ex && (from == rb || from == rn)) {
	return true;
  }

  sq_woo_from= from;
  sq_woo_to= to;
  col_woo= e[from] > vide ? blanc : noir;

  pcheck = transmpieces;
  if (rex_wooz_ex)
	pcheck++;

  while (*pcheck) {
	if (CondFlag[biwoozles] != (col_woo==noir)) {
      p= -*pcheck;
	}
	else {
      p= *pcheck;
	}
	if (nbpiece[p] && (*checkfunctions[*pcheck])(from, p, aux_wh)) {
      return false;
	}
	pcheck++;
  }

  return true;
} /* woohefflibre */

boolean eval_wooheff(square sq_departure, square sq_arrival, square sq_capture) {
  if (flaglegalsquare && !legalsquare(sq_departure,sq_arrival,sq_capture)) {	/* V3.02  TLi */
	return false;
  }
  else {
	return woohefflibre(sq_arrival, sq_departure);
  }
} /* eval_wooheff */


boolean scorpioncheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V2.63  NG */
{
  return  leapcheck(i, vec_queen_start,vec_queen_end, p, evaluate)
    || rhopcheck(i, vec_queen_start,vec_queen_end, p, evaluate);
}

boolean dolphincheck(
  square	i,
  piece	p,
  evalfunction_t *evaluate)	/* V3.70  TLi */
{
  return  rhopcheck(i, vec_queen_start,vec_queen_end, p, evaluate)
    || kangoucheck(i, p, evaluate);
} /* dolphincheck */

boolean querquisitecheck(square sq_king,
                         piece p,
                         evalfunction_t *evaluate)   /* V3.78  SE */
{
  numvec k;
  smallint file_departure;
  piece p1;

  square sq_departure;
    
  for (k= vec_rook_start; k<=vec_rook_end; k++) {
    finligne(sq_king,vec[k],p1,sq_departure);
    file_departure= sq_departure%onerow - nr_of_slack_files_left_of_board;
    if ((file_departure==file_rook_queenside
         || file_departure==file_queen
         || file_departure==file_rook_kingside)
        && p1==p
        && evaluate(sq_departure,sq_king,sq_king)
        && ridimcheck(sq_departure,sq_king,vec[k]))
      return true;
  }
  
  for (k= vec_bishop_start; k<=vec_bishop_end; k++) {
    finligne(sq_king,vec[k],p1,sq_departure);
    file_departure= sq_departure%onerow - nr_of_slack_files_left_of_board;
    if ((file_departure==file_bishop_queenside
         || file_departure==file_queen
         || file_departure==file_bishop_kingside)
        && p1==p
        && evaluate(sq_departure,sq_king,sq_king)
        && ridimcheck(sq_departure,sq_king,vec[k]))
      return true;
  }
  
  for (k= vec_knight_start; k<=vec_knight_end; k++) {
    sq_departure= sq_king+vec[k];
    file_departure= sq_departure%onerow - nr_of_slack_files_left_of_board;
    if (e[sq_departure]==p
        && (file_departure==file_knight_queenside
            || file_departure==file_knight_kingside)
        && evaluate(sq_departure,sq_king,sq_king)
        && imcheck(sq_departure,sq_king))
      return true;
  }
  
  for (k= vec_queen_start; k<=vec_queen_end; k++) {
    sq_departure= sq_king+vec[k];
    file_departure= sq_departure%onerow - nr_of_slack_files_left_of_board;
    if (e[sq_departure]==p
        && file_departure==file_king
        && evaluate(sq_departure,sq_king,sq_king)
        && imcheck(sq_departure,sq_king))
      return true;
  }

  return false;
}

boolean bouncerfamilycheck(square sq_king,  /* V4.03 */
                           numvec kbeg,
                           numvec kend,
                           piece	p,
                           evalfunction_t *evaluate)
{
  numvec  k;
  piece   p1,p2;
  square  sq_hurdle;

  square sq_departure;
    
  for (k= kend; k>=kbeg; k--) {
	finligne(sq_king,vec[k],p1,sq_departure);
	finligne(sq_departure,vec[k],p2,sq_hurdle);  /* p2 can be obs - bounces off edges */
    if (sq_departure-sq_king==sq_hurdle-sq_departure
        && p1==p
        && evaluate(sq_departure,sq_king,sq_king))
      return true;
  }

  return false;
}

boolean bouncercheck(  /* V4.03 */
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return bouncerfamilycheck(i, vec_queen_start,vec_queen_end, p, evaluate);
}

boolean rookbouncercheck(  /* V4.03 */
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return bouncerfamilycheck(i, vec_rook_start,vec_rook_end, p, evaluate);
}

boolean bishopbouncercheck(  /* V4.03 */
  square	i,
  piece	p,
  evalfunction_t *evaluate)
{
  return bouncerfamilycheck(i, vec_bishop_start,vec_bishop_end, p, evaluate);
}

boolean pchincheck(square sq_king,  /* V4.03 */
                   piece	p,
                   evalfunction_t *evaluate)
{
  square sq_departure;
    
  boolean const is_black= p<=roin;

  sq_departure= sq_king + (is_black ? dir_up :dir_down);
  if (e[sq_departure]==p
      && evaluate(sq_departure,sq_king,sq_king))	/* V3.02  TLi */
    return true;

  /* chinese pawns can capture side-ways if standing on the half of
   * the board farther away from their camp's base line (i.e. if
   * black, on the lower half, if white on the upper half) */
  /* TODO (tm 2006-07-16) Stephen: is this correct?*/
  if ((sq_king*2<(haut+bas)) == is_black) {
    sq_departure= sq_king+dir_right;
    if (e[sq_departure]==p
        && evaluate(sq_departure,sq_king,sq_king))	/* V3.02  TLi */
      return true;

    sq_departure= sq_king+dir_left;
    if (e[sq_departure]==p
        && evaluate(sq_departure,sq_king,sq_king))	/* V3.02  TLi */
      return true;
  }

  return false;
}

square masand_square; 
boolean	eval_masand(square sq_departure, square sq_arrival, square sq_capture) { /* V4.06 SE */
  return
    sq_departure==masand_square
    && (e[sq_departure]>vide ? eval_white : eval_black)(sq_departure,sq_arrival,sq_capture);
}

boolean observed(square on_this, square by_that) { /* V4.06 SE */
  boolean flag;
  square k;

  masand_square= by_that;
  if (e[by_that] > vide)
  {
    k= rn;
    rn= on_this;
    flag= rnechec(eval_masand);
    rn= k;
  }
  else
  {
    k= rb;
    rb= on_this;
    flag= rbechec(eval_masand);
    rb= k;
  }
  return flag;
}

void change_observed(square z) /* V4.06 SE */
{
  square* bnp;

  for (bnp= boardnum; *bnp; bnp++)
  {
    if (e[*bnp] != vide && *bnp != rn && *bnp != rb && *bnp != z)
    {
      if (observed(*bnp, z))
      {
        change(*bnp);
        CHANGECOLOR(spec[*bnp]);
        if (e[*bnp] == tb && *bnp == square_a1)
          SETFLAGMASK(castling_flag[nbply],ra1_cancastle);  
        if (e[*bnp] == tb && *bnp == square_h1)
          SETFLAGMASK(castling_flag[nbply],rh1_cancastle);  
        if (e[*bnp] == tn && *bnp == square_a8)
          SETFLAGMASK(castling_flag[nbply],ra8_cancastle);  
        if (e[*bnp] == tn && *bnp == square_h8)
          SETFLAGMASK(castling_flag[nbply],rh8_cancastle);  
      }
    }
  }
}

boolean eval_BGL(square sq_departure, square sq_arrival, square sq_capture) {
  return
    BGL_move_diff_code[abs(sq_departure-sq_arrival)]
    <= (e[sq_capture]<vide ? BGL_white : BGL_black);
}
