/* ---------------------------------------------------
   Trivial evaluation.  

     count 100 for rabbits,
           200 for cats,
           300 for dogs,
     etc ...     

     DISCLAIMER:

     This evaluation is just an example.  All chosen 
     evaluation terms and the weights that go with them
     were chosen arbitrarily, with little thought, and
     absolutely no testing or tuning whatsoever!

     Great improvement is possible by improving on this.
   ---------------------------------------------------- */
int  eval( position *p )
{
  int   i;
  int   c;
  int   tot[2] = { 0, 0 };
  u64   dom;                   // bit board of dominant pieces.
  u64   non;                   // bb of non-dominant pieces 

//  if (playrand){ return (rand()%10000 - 5000); } /* act like a random bot */
//  if (playrand){ return (10000 - (p->steps)*1000); } /* act like a random bot */
if (p->steps == 2){
//fprintf(stderr, "steps is 2\n");
}
//  if (playrand){ return ((p->steps==2)?-1:1); } /* act like a random bot */
  if (playrand){ return (0 - p->steps); } /* act like a random bot */

  for (c=0; c<2; c++)  // c = color, 0 or 1
  {
    int  e = c^1;  // enemy for convienence.

    for (i=1; i<7; i++)
      tot[c] += bit_count( p->bd[c][i] ) * i * 100;

    // small connectivity bonus
    tot[c] += bit_count(p->bd[c][0] & neighbors_of(p->bd[c][0]) );

    // bonus for rabbit advancement - advance if a few enemy (non-pawn) 
    // pieces have left the board.
    if ( bit_count(p->bd[e][0] ^ p->bd[e][1] ) < 5 ) 
    {
      tot[c] += bit_count( p->bd[c][1] & adv[4][c] ) * 2;
      tot[c] += bit_count( p->bd[c][1] & adv[5][c] ) * 3;
      tot[c] += bit_count( p->bd[c][1] & adv[6][c] ) * 5;
      tot[c] += bit_count( p->bd[c][1] & adv[7][c] ) * 8;
    }

    // figure out which pieces are dominant
    // ------------------------------------
    {
      int  be = 0;   
      for (i=6; i>0; i--) 
	if (p->bd[e][i]) 
	{
	  be = i;
	  break;
	}

      dom = 0ULL;
      for ( i=be; i<=6; i++) 
	dom |= p->bd[c][i];

      non = p->bd[c][0] ^ dom;
    }

    // get dominant piece out to center
    // --------------------------------
    tot[c] += bit_count( dom & RING0 ) * 10;
    tot[c] += bit_count( dom & RING1 ) * 4;

    // don't hang out in traps.
    // ------------------------
    tot[c] -= bit_count( dom & TRAPS ) * 12;    // penalty 12 for dominant
    tot[c] -= bit_count( (non & TRAPS) ) * 7;   // penalty  7 for non
  }

  if (p->ply & 1) 
    return(tot[1] - tot[0]);

  return(tot[0] - tot[1]);
}

