/* This is the bot contributed by Don Dailey and modified by Omar
   to add some additional options. See the README file for details.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/time.h>

#define  version "1.1"


typedef unsigned long long u64;

#define NOT_A_FILE  0xfefefefefefefefeULL
#define NOT_H_FILE  0x7f7f7f7f7f7f7f7fULL
#define NOT_1_RANK  0x00ffffffffffffffULL
#define NOT_8_RANK  0xffffffffffffff00ULL
#define MSB         0x8000000000000000ULL
#define TRAPS       0x0000240000240000ULL
#define FPARITY     0x5555555555555555ULL


#define RING0  0x0000001818000000ULL   /* same as small center */
#define RING1  0x00003c24243c0000ULL   
#define RING2  0x007e424242427e00ULL
#define RING3  0xff818181818181ffULL   


#define RABBIT      1
#define CAT         2
#define DOG         3
#define HORSE       4
#define CAMEL       5
#define ELEPHANT    6

#define INF         9999999
#define WIN_SCORE   99999
#define REDUNDANT   99999999

#define EXACT_SCORE  0
#define LOWER_BOUND  1
#define UPPER_BOUND  2

u64   adv[8][2] = { {0xff00000000000000ULL, 0x00000000000000ffULL},
		    {0x00ff000000000000ULL, 0x000000000000ff00ULL},
		    {0x0000ff0000000000ULL, 0x0000000000ff0000ULL},
		    {0x000000ff00000000ULL, 0x00000000ff000000ULL},
		    {0x00000000ff000000ULL, 0x000000ff00000000ULL},
		    {0x0000000000ff0000ULL, 0x0000ff0000000000ULL},
		    {0x000000000000ff00ULL, 0x00ff000000000000ULL},
		    {0x00000000000000ffULL, 0xff00000000000000ULL } };


#define bit_on(n) (1ULL << (n))

#define  WALL_TIME  1    // WALL_TIME = 0  means processor time


FILE     *flog;
FILE     *fgme;


u64   win_rank[2] = { 0x00000000000000ffULL, 0xff00000000000000ULL };

u64   nodec = 0ULL;

double  start_time = 0.0;
int     abort_flag = 0;

/* These global variables are set by command line options; look in main() */
double  time_level = 0.0; /* controls how many seconds to search; -t option */
int     step_level = 0; /* controls how deep to search; -d option */
unsigned int rseed = 0; /* seed for random number generator; -r option */
int     playrand = 0; /* set to 1 if we are to play randomly; -d 0 given */
char    sfile[512]; /* input setup file; -1 option */

/* ----------------------------------------------------------
   How a move is coded.

   A "move" consists of 1 to 4 steps stored as a move_t type.

   Each step is sub-coded into a step_t.   The move generators
   can return 1 or 2 step moves,  but the search can return
   4 step moves.
   ----------------------------------------------------------- */
typedef  struct
{
  int    fsq:8;   // 'from' square of piece being moved
  int    tsq:8;   // 'to' square of piece being moved
  int    typ:4;   // type of piece being moved
  int    col:2;   // color of piece being moved

} step_t;



typedef  struct
{
  int     steps;
  step_t  list[4];    

} move_t;

move_t kill[100];

typedef  struct
{
  int     count;
  move_t  pv[32];   

} pv_t;


/* STUFF FOR LOCAL TRANSPOSITIONS TABLES */
/* ------------------------------------- */

/* ------------------------------------------
   age - It takes too long to initialize the
   local transposition tables and brings the
   program to a crawl.  Instead we do not 
   bother and simply update a global age in
   lieu of that.  An age is stored in each
   table entry to compare against this.
   ------------------------------------------ */
int      age[8] = {0, 0, 0, 0, 0, 0, 0, 0 };



/* -------------------------------------------
   TRANS_SIZE must be power of 2. 

   There is an optimal size where less slows 
   down too much and more doesn't help much.
   0x10000 seems to be close to optimal.  
   0x20000 may be slighly better if your machine
   has enough memory and cache.   Your mileage
   may vary.
   --------------------------------------------- */
#define  TRANS_SIZE 0x10000    


typedef  struct
{
  u64   tkey;
  int   age;
} trans_t;


trans_t   trans_tabs[8][TRANS_SIZE];




/* ------------------------------------------------
   This structure encapsulates a complete position. 
   ------------------------------------------------ */
typedef struct ptag 
{
  u64          bd[2][7];        /* bitboard indexed by color,type        */
  u64          signature;       /* position signature (hash key)         */
  struct ptag  *his;            /* pointer to last position              */
  int          ply;             /* number of full moves into search      */
  int          steps;           /* number of executed steps in this move */
  move_t       last_move;       /* move that created this position       */
  trans_t      *trans;          /* table of transpositions               */

} position;




u64  zobrist[2][7][64];         /* table of 64 bit psuedo random numbers */
u64  move_offset[2][7][64];     /* precomputed move offsets              */


/* prototype */
void  bot_notate( position *p, char *buf,  pv_t mv );
int pick_setup(char *s, char side, char *file);
int shuffle_pieces(char *s);


/* -------------------------------------
   Routines to make it easy to keep time
   ------------------------------------- */

static u64 wall_time(){
  struct timeval t;
  struct timezone tz;
  u64 result;

  gettimeofday(&t,&tz);

  result = 1000 * (u64) t.tv_sec;
  result += ((u64) t.tv_usec)/1000;

  return(result);
}



/* return an absolute process time (user+system) counter in milliseconds  */
static u64 process_time(){
  struct rusage ru;
  u64 result;

  getrusage(RUSAGE_SELF,&ru);

  result  = 1000 * (u64) ru.ru_utime.tv_sec;
  result += ((u64) ru.ru_utime.tv_usec)/1000;
  result += 1000 * (u64) ru.ru_stime.tv_sec;
  result += ((u64) ru.ru_stime.tv_usec)/1000;

  return(result);
}


double  seconds()
{
#if WALL_TIME
    return( ((double)wall_time())/1000);
#else
    return( ((double)process_time())/1000);
#endif
}





/* -------------------------------------
   Convert algebraic notation like 'f3' 
   to a bit index
   ------------------------------------- */
int  alg2index( const char *alg )
{
  return( (7 ^ ('h' - alg[0]))  +  (('8' - alg[1]) * 8) );
}


/* ------------------------------------------------ 
   Get index of leftmost bit the most naive way.
   This is a very slow routine and there are many 
   faster implementations possible.    

   Some processors have native instructions to do this fast.

   NOTE: lsb==0,  msb==63
   ------------------------------------------------- */
static inline int  lix( u64 b )
{
  int  x=63;

  while (b)
  {
    if (b & MSB) return(x);
    x--;
    b <<= 1;
  }

  return( -1 );
}



int  bit_count(u64 b)
{
  int c = 0;

  while (b) {
    b -= (b & (-b));
    c++;
  }
  return(c);
}


u64  compute_signature( position *p )
{
  u64  sig  = 0ULL;
  int  col;
  int  typ;
  

  /* build position signature from scratch */

  for (col=0; col<2; col++)
    for (typ=1; typ<7; typ++)
    {
      u64  bb = p->bd[col][typ];

      while(bb)
      {
	int squ = lix(bb);
	bb ^= bit_on(squ);  /* clear out bit */
	sig ^= zobrist[col][typ][squ];
      }
    }

  return(sig);
}


void   build_zobrist_table()
{
  int  col;
  int  squ;
  int  typ;

  if (rseed==0){ /* means we should pick a random seed */
    srandom((unsigned int)wall_time());
  }
  if (rseed>0){ /* means we are given the random seed */
    srandom(rseed);
  }

  for (col=0; col<2; col++)
    for (typ=0; typ<7; typ++)
      for (squ=0; squ<64; squ++)
      {
	zobrist[col][typ][squ] = 
	  (((u64) random()) << 40) ^ 
	  (((u64) random()) << 20) ^ 
	  ((u64) random() );
      }
}



int setup_board( position *p,  const char *ps )
{
  int     i;
  int     x;
  int     col;
  int     typ;
  move_t  no_move;

  no_move.steps = 0;

  /* clear board */
  for (i=0; i<7; i++) {
    p->bd[0][i] = 0ULL;
    p->bd[1][i] = 0ULL;
  }

  /* set up bit boards */
  for (i=0; i<64; i++) {
    x = 16 - strlen( strchr( "-PNBRQK--pnbrqk-", (int) ps[i]) );

    if (x && (x < 16))
    {
      p->bd[x/8][x & 7] |= bit_on(i);
      p->bd[x/8][0] |= bit_on(i);
    }
  }

  p->ply = 0;
  p->his = NULL;
  p->steps = 0;
  p->last_move = no_move;   // No history here.


  /* build position signature from scratch */
  p->signature = 0ULL;
  for (col=0; col<2; col++)
    for (typ=1; typ<7; typ++)
    {
      u64  bb = p->bd[col][typ];

      while(bb)
      {
	int squ = lix(bb);
	bb ^= bit_on(squ);  /* clear out bit */
	p->signature ^= zobrist[col][typ][squ];
      }
    }

  return(0);
}



/* -----------------------------------------------------------------
   return a mask of all squares immediately next to target bit board
   ------------------------------------------------------------------ */
u64   neighbors_of( u64 target )
{
  u64   x;

  x =  (target & NOT_H_FILE) << 1;
  x |= (target & NOT_A_FILE) >> 1;
  x |= (target & NOT_1_RANK) << 8;
  x |= (target & NOT_8_RANK) >> 8;

  return(x);
}

int  moves_equal( move_t *a,  move_t *b )
{
  int  i;

  if (a->steps > 3) return(0);
  if (a->steps != b->steps) return(0);

  for (i=0; i<a->steps; i++)
  {
    if (a->list[i].fsq != b->list[i].fsq) return(0);
    if (a->list[i].tsq != b->list[i].tsq) return(0);
    if (a->list[i].typ != b->list[i].typ) return(0);
    if (a->list[i].col != b->list[i].col) return(0);
  }

  return(1);
}

/* -------------------------------------------
   return type of piece on square.  
   ----------------------------------------- */
int  type_of( position *p, int col, int sq )
{
  int   i;
  u64   target = bit_on(sq);

  for (i=1; i<7; i++)
    if (p->bd[col][i] & target)
      return(i);

  return(0);
}


/* -----------------------------------------------------
   make_move()  

   The move must be 100% legal or the program is broken 
   because this routine assumes that to be the case.
   (The only exception is for repetition issues which 
   are checked in the search routine.)

   RETURNS:  0 in most cases.
             1 if side to move runs a rabbit.   
            -1 if we push enemy rabbit to victory.
   --------------------------------------------------- */
int  make_move( position *cur, position *nxt, move_t mv )
{
  int   i;
  int   c;
  int   ctm = cur->ply & 1; 

  *nxt = *cur;

// Process a move to pass the remaining steps 
//   doesn't change the position, but sets steps taken to 4
  if (mv.steps == 0){
//fprintf(stderr, "hit a passing move\n");
    nxt->steps = 4;
    return(0);
  }

  for (i=0; i<mv.steps; i++)
  {
    step_t  s = mv.list[i];


    /* execute move on board */
    nxt->bd[s.col][s.typ] ^= bit_on( s.fsq );
    nxt->bd[s.col][s.typ] ^= bit_on( s.tsq );
    nxt->bd[s.col][0] ^= bit_on( s.fsq );
    nxt->bd[s.col][0] ^= bit_on( s.tsq );

    /* update zobrist hash key */
    nxt->signature ^= zobrist[ s.col ][ s.typ ][ s.fsq ];
    nxt->signature ^= zobrist[ s.col ][ s.typ ][ s.tsq ];

    /* see if ANY pieces are lost in trap */
    for (c=0; c<2; c++)
    {
      if (nxt->bd[c][0] & TRAPS)
      {
	u64  x = nxt->bd[c][0] & TRAPS;
	
	while (x)
	{
	  int  t = lix(x);
	  u64  ton = bit_on(t);
	  
	  x ^= ton;  // clear out

	  if ( !(neighbors_of(ton) & nxt->bd[c][0]) )
	  {
	    int typ = type_of( nxt, c, t );
	    nxt->bd[c][typ] ^= ton;
	    nxt->bd[c][0] ^= ton;
	    nxt->signature ^= zobrist[c][typ][t];   // repair hash
	  }
	}
      }
    }
  }
  nodec+= mv.steps;
  nxt->steps += mv.steps;  // number of steps
  nxt->last_move = mv;     // record of move that created this state
  nxt->his = cur;          // pointer to previous position

  
  /* detect rabbit run wins */
  if ( nxt->bd[ctm][1] & win_rank[ctm] ) return(1);
  if ( nxt->bd[ctm^1][1] & win_rank[ctm^1] ) return(-1);
  
  return(0);
}
  



int   gen_pass_step( position *p, move_t *move_list ){
  if ((p->steps > 0) && (p->steps < 4)){
    move_list[0].steps = 0;
    return(1);
  }
  return(0);
}



/* generate all one step moves for side to move */
int   gen_one_step( position *p, move_t *move_list )
{
  int  ctm = p->ply & 1;
  u64  mt = ~(p->bd[0][0] | p->bd[1][0]);   /* mask of unoccupied squares    */
  int  lc = 0;                              /* list count                    */
  int  typ;                                 /* type of piece being moved     */
  u64  stronger = p->bd[ctm^1][0];          /* mask of stronger enemy pcs    */

  for (typ=1; typ<7; typ++)
  {
    u64   good_squares;                     // square which are not frozen
    u64   pm = p->bd[ctm][typ];             // get all pieces of this type
    
    stronger ^= p->bd[ctm^1][typ];          // remove from consideration
    good_squares = 
      neighbors_of( p->bd[ctm][0] ) | (~neighbors_of(stronger));

    pm = pm & good_squares;                 // bitmap of unfrozen pieces

    while (pm)
    {
      u64    pot;               /* potential squares to move to         */
      int    fsq = lix(pm);     /* consider moves from this square next */

      pm ^= bit_on(fsq);        /* cancel out for next pass             */
      pot = mt & move_offset[ctm][typ][fsq];

      while (pot) 
      {
	int  tsq = lix(pot);    /* get next potential move  */
	pot ^= bit_on(tsq);     /* cancel out               */

	move_list[lc].list[0].fsq = fsq;
	move_list[lc].list[0].tsq = tsq;
	move_list[lc].list[0].typ = typ;
	move_list[lc].list[0].col = ctm;
	move_list[lc].steps = 1;
	lc++;
      }
    }
  }
  return(lc);
}





/* generate push moves for side to move */
int   gen_push_moves( position *p, move_t *move_list )
{
  int  ctm = p->ply & 1;
  int  opp = ctm ^ 1;
  u64  mt = ~(p->bd[0][0] | p->bd[1][0]);   /* mask of unoccupied squares    */
  int  lc = 0;                              /* list count                    */
  int  typ;                                 /* type of piece being moved     */
  u64  stronger = p->bd[ctm^1][0];          /* mask of stronger enemy pcs    */
  u64  victims = 0ULL;                      /* victims to be pushed          */


  stronger ^= p->bd[ctm^1][1];

  for (typ=2; typ<7; typ++)
  {
    u64   good_squares;                     // square which are not frozen
    u64   pm = p->bd[ctm][typ];             // get all pieces of this type

    victims |= p->bd[ctm^1][typ-1];         // victims we can push around
    
    stronger ^= p->bd[ctm^1][typ];          // remove from consideration
    good_squares = 
      neighbors_of( p->bd[ctm][0] ) | (~neighbors_of(stronger));

    pm = pm & good_squares;                 // bitmap of unfrozen pieces

    while (pm)
    {
      u64    vsl;               /* potential victim square list         */
      int    fsq = lix(pm);     /* consider moves from this square next */

      pm ^= bit_on(fsq);        /* cancel out for next pass             */
      vsl = victims & move_offset[ctm][typ][fsq];

      
      while (vsl)
      {
	u64  ptobb;             /* push to bit board        */
	int  tsq = lix(vsl);    /* a victim!                */
	vsl ^= bit_on(tsq);     /* cancel out               */

	ptobb = mt & move_offset[0][2][tsq];   

	while (ptobb) 
	{
	  int  pto = lix(ptobb);
	  ptobb ^= bit_on(pto);

	  move_list[lc].list[0].fsq = tsq;
	  move_list[lc].list[0].tsq = pto;
	  move_list[lc].list[0].typ = type_of( p, opp, tsq );
	  move_list[lc].list[0].col = opp;

	  move_list[lc].list[1].fsq = fsq;
	  move_list[lc].list[1].tsq = tsq;
	  move_list[lc].list[1].typ = typ;
	  move_list[lc].list[1].col = ctm;

	  move_list[lc].steps = 2;
	  lc++;
	}
	
      }
    }
  }
  return(lc);
}


/* generate pull moves for side to move */
int   gen_pull_moves( position *p, move_t *move_list )
{
  int  ctm = p->ply & 1;
  int  opp = ctm ^ 1;
  u64  mt = ~(p->bd[0][0] | p->bd[1][0]);   /* mask of unoccupied squares    */
  int  lc = 0;                              /* list count                    */
  int  typ;                                 /* type of piece being moved     */
  u64  stronger = p->bd[ctm^1][0];          /* mask of stronger enemy pcs    */
  u64  victims = 0ULL;                      /* victims to be pushed          */


  stronger ^= p->bd[ctm^1][1];

  for (typ=2; typ<7; typ++)
  {
    u64   good_squares;                     // square which are not frozen
    u64   pm = p->bd[ctm][typ];             // get all pieces of this type

    victims |= p->bd[ctm^1][typ-1];         // victims we can push around
    
    stronger ^= p->bd[ctm^1][typ];          // remove from consideration
    good_squares = 
      neighbors_of( p->bd[ctm][0] ) | (~neighbors_of(stronger));

    pm = pm & good_squares;                 // bitmap of unfrozen pieces

    while (pm)
    {
      u64    vsl;               /* potential victim square list         */
      int    fsq = lix(pm);     /* consider moves from this square next */

      pm ^= bit_on(fsq);        /* cancel out for next pass             */
      vsl = victims & move_offset[ctm][typ][fsq];

      
      while (vsl)
      {
	u64  ptobb;             /* push to bit board        */
	int  tsq = lix(vsl);    /* a victim!                */
	vsl ^= bit_on(tsq);     /* cancel out               */

	ptobb = mt & move_offset[0][2][fsq];   

	while (ptobb) 
	{
	  int  pto = lix(ptobb);

	  ptobb ^= bit_on(pto);

	  move_list[lc].list[0].fsq = fsq;
	  move_list[lc].list[0].tsq = pto;
	  move_list[lc].list[0].typ = typ;
	  move_list[lc].list[0].col = ctm;

	  move_list[lc].list[1].fsq = tsq;
	  move_list[lc].list[1].tsq = fsq;
	  move_list[lc].list[1].typ = type_of( p, opp, tsq );
	  move_list[lc].list[1].col = opp;


	  move_list[lc].steps = 2;
	  lc++;
	}
	
      }
    }
  }
  return(lc);
}




void display_position( position *p, FILE *f )
{
  int   i;
  int   c;
  int   t;

  for (i=0; i<64; i++) {
    u64  pat = bit_on(i);
    char pce = '-';

    if ((i % 8) == 0) fprintf(f, "\n");

    for (c=0; c<2; c++)
      for (t=1; t<7; t++)
	if (pat & p->bd[c][t]) 
	{
	  pce = "-RCDHME--rcdhme-"[c * 8 + t ];
	  break;
	}

    fprintf(f, " %c", pce );
  }
  fprintf(f, "\n");
  fflush(f);
  return;
}



void alg( char *buf, int squ )
{
  buf[0] = "abcdefgh"[squ & 7];
  buf[1] = "87654321"[squ >> 3];
  buf[2] = 0;
}




void  notate( char *buf, move_t move )
{
  int  i;

  for (i=0; i<move.steps; i++)
  {
    if (i != 0) { *buf = ' ' ; buf++; }
    alg( buf, move.list[i].fsq );
    buf += 2;
    *buf = '-';
    buf++;
    alg( buf, move.list[i].tsq );
    buf += 2;
  }
}

/* --------------------------------------------------------------------
   Builds the move_offset[color][type][square] array.

   This table returns a bitmap of legal squares for a
   given piece on a given color on a given square.
   -------------------------------------------------------------------- */
void build_move_offsets()
{
  int  col;
  int  pce;
  int  squ;

  // do rabbits as a special case
  // --
  for (col=0; col<2; col++)
    for (squ=0; squ<64; squ++)
    {
      u64  ts = 0ULL;
      
      ts |= (bit_on(squ) & NOT_H_FILE) << 1;  // east
      ts |= (bit_on(squ) & NOT_A_FILE) >> 1;  // west
      if (col == 0)
	ts |= (bit_on(squ) & NOT_8_RANK) >> 8;  // north
      else
	ts |= (bit_on(squ) & NOT_1_RANK) << 8;  // south

      move_offset[col][RABBIT][squ] = ts;    
    }

  // Now do the rest
  for (col=0; col<2; col++)
    for (pce=2; pce<7; pce++)
      for (squ=0; squ<64; squ++)
      {
	u64  ts = 0ULL;

	ts |= (bit_on(squ) & NOT_H_FILE) << 1;  // east
	ts |= (bit_on(squ) & NOT_A_FILE) >> 1;  // west
	ts |= (bit_on(squ) & NOT_1_RANK) << 8;  // south
	ts |= (bit_on(squ) & NOT_8_RANK) >> 8;  // north

	move_offset[col][pce][squ] = ts;
      }

}

#include "eval.c"

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
int  evalx( position *p )
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


/* ----------------------------------------------------- 
   Note to computer scientists:  The scramble algorithm 
   is slightly biased, but who cares.   We use this only
   to start with randomized move list for some variety.
   ----------------------------------------------------- */
void scramble( int lc,  move_t *mvlist )
{
  int  i;
  
  for (i=0; i<lc; i++)
  {
    move_t  tmp;
    int  r = rand() % lc;

    tmp = mvlist[i];
    mvlist[i] = mvlist[r];
    mvlist[r] = tmp;
  }
}




void   pv_append( pv_t *a,  pv_t *b )
{
  int   i;

  for (i=0; i<b->count; i++)
    a->pv[ a->count++ ] = b->pv[i];
}


//fprintf(stderr, "in root depth=%d lc=%d\n", depth, lc); exit(0);

int  srch( position     *p, 
	   int          alpha, 
	   int          beta, 
	   int          depth, 
	   int          ssteps, 
	   move_t       move, 
	   pv_t         *line, 
	   int       ply_of_search )
{
  pv_t         wpv;
  int          best_score = -INF;
  int          sc;
  int          i;
  int          lc = 0;
  int          mv_stat = 0;
  static int   poll = 0;
  move_t       mvlist[128];
  position     cur;

  if (abort_flag) return(0);

  /* check time every once in a while */
  poll++;
  if ((poll & 0xffff) == 0) 
  {
    double  et = seconds() - start_time;

    if (et >= time_level) {
      abort_flag = 1;
      return(0);
    }
  }
  

  line->count = 0;
  
  mv_stat = make_move( p, &cur, move );
// if it was a pass steps move we need to reset the depth
  if (move.steps == 0){
//fprintf(stderr, "steps for move is 0\n");
    depth = depth - (depth%4);
  }

  if (1)
  {
    u64   psig = cur.signature ^ zobrist[0][0][cur.steps];
    int   add = cur.signature & (TRANS_SIZE-1);

    if ( (cur.trans[add].age == age[ply_of_search]) && 
	 (cur.trans[add].tkey == psig) )
    {
      return( REDUNDANT );
    }
    cur.trans[add].tkey = psig;
    cur.trans[add].age = age[ply_of_search];
  }


  if (cur.steps == 4)   // let's switch sides now.
  {
    if (ply_of_search == 0) {  // test for repeated positions.
      position  *h = cur.his;

      while ( h != NULL ) {
	if ( cur.signature == h->signature ) return( REDUNDANT );
	h = h->his;
      }
    }

    cur.steps = 0;
    cur.ply++;            // ply is number of full moves into game.
    ply_of_search++;
    age[ply_of_search]++;
    mv_stat = -mv_stat;

    // initialize local transpositions
    cur.trans = trans_tabs[ ply_of_search ];
  }


  if (mv_stat)
  {
    if (mv_stat == 1) 
      return(WIN_SCORE - ply_of_search); 
    else 
      return(-(WIN_SCORE - ply_of_search));
  }

  if (depth == 0)           // search over now.  Apply static evaluation
  {
// To avoid a move which does not change the board, we also need to
//   check for repeated positions when depth==0
// This was added by Omar
    if (ply_of_search == 0) {  // test for repeated positions.
      position  *h = cur.his;

      while ( h != NULL ) {
	if ( cur.signature == h->signature ) return( REDUNDANT );
	h = h->his;
      }
    }

    line->count = 0;        // no main line to report here.
    return( eval(&cur) );
  }



  if (cur.steps < 3 && depth > 1)
  {
    lc += gen_push_moves( &cur, mvlist + lc );
    lc += gen_pull_moves( &cur, mvlist + lc );
  }
  lc += gen_one_step( &cur, mvlist + lc );
//  lc += gen_pass_step( &cur, mvlist + lc );

// Omar changed this
// Do this only if we are not playing randomly; playrand!=1
//  if (depth > 2)
  if ((depth > 2) && (playrand != 1))
  {
    // find killer if it exists
    // -------------------------
    for (i=0; i<lc; i++) 
      if (moves_equal(&mvlist[i], &kill[ssteps]))
      {
	move_t  tmp = mvlist[i];
	
	mvlist[i] = mvlist[0];
	mvlist[0] = tmp;
	break;
      }
  }


  for (i=0; i<lc; i++)
  {
    if (cur.steps + mvlist[i].steps == 4)
      sc = -srch( &cur, 
		  -beta, 
		  -alpha, 
		  depth-mvlist[i].steps, 
		  ssteps+mvlist[i].steps, 
		  mvlist[i], 
		  &wpv, 
		  ply_of_search );
    else      
      sc = srch( &cur, 
		 alpha, 
		 beta, 
		 depth-mvlist[i].steps, 
		 ssteps+mvlist[i].steps, 
		 mvlist[i], 
		 &wpv, 
		 ply_of_search );

    if (abs(sc) == REDUNDANT) continue;

    if (sc > best_score) 
    {
      best_score = sc;

      line->pv[0] = mvlist[i];
      line->count = 1;
      pv_append( line, &wpv );
      
      if (best_score >= beta)  {
	kill[ssteps] = mvlist[i];
	break;   // beta cutoff
      }

      if (best_score > alpha)
	alpha = best_score;
    }
  }


  return( best_score );
}





int   root( position *p,  int depth,  pv_t *main_line )
{
  int     alpha = -INF;
  int     beta  = INF;
  pv_t    wpv;
  int     best_score = -INF;
  int     sc;
  int     i;
  int     ssteps = 0;
  int     ply_of_search = 0;   // full moves into search, not steps
  static
    move_t  prev_best;
  static
    int     lc = 0;
  static 
    move_t  mvlist[128];

  p->steps = 0;

  
  {
    age[ ply_of_search ]++;
    p->trans = trans_tabs[ ply_of_search ];

    for (i=0; i<TRANS_SIZE; i++) {
      p->trans[i].tkey = 0ULL;
      p->trans[i].age = -999;
    }
  }


  if (depth == 2)
  {
    lc = 0;
    lc += gen_push_moves( p, mvlist + lc );
    lc += gen_pull_moves( p, mvlist + lc );
    lc += gen_one_step( p, mvlist + lc );
    scramble( lc, mvlist );
  } 
  else
  {
    for (i=0; i<lc; i++) if ( moves_equal(&mvlist[i], &prev_best) ) break;

    for ( ; i>0; i--) mvlist[i] = mvlist[i-1];
    mvlist[0] = prev_best;
  }
  
  for (i=0; i<lc; i++)
  {
    if (abort_flag) return(0);
    sc = srch( p, 
	       alpha, 
	       beta, 
	       depth-mvlist[i].steps, 
	       ssteps+mvlist[i].steps, 
	       mvlist[i], 
	       &wpv, 
	       ply_of_search );

    if (abs(sc) == REDUNDANT) continue;

    if (sc > best_score) 
    {
      best_score = sc;
      prev_best = mvlist[i];

      main_line->pv[0] = mvlist[i];
      main_line->count = 1;

      pv_append( main_line, &wpv );
      
      if (best_score >= beta)  break;   // beta cutoff

      if (best_score > alpha)
	alpha = best_score;
    }

  }

  return( best_score );
}



void get_pv( char *buf,  pv_t *x )
{
  int   i;
  char  mvs[32];

  strcpy(mvs, "");
  strcpy(buf, "");

  for (i=0; i<x->count; i++)
  {
    notate( mvs, x->pv[i] );
    strcat( buf, mvs );
    strcat( buf, " " );
  }

}




int ParseString( char *ps, char *fld[] )
{
  int   field_count = 0;
  char  *tok;
  int   i;

  
  field_count = 0;    
  for ( i=0,tok=strtok(ps, " \t\n\r");
        tok!=NULL; 
        tok=strtok(NULL, " \t\n\r"), i++ ) {
     fld[i] = tok;  
     field_count++; 
  }
  
  fld[ field_count ] = NULL;
  return( field_count );
}



/* ----------------------------------------------------------
   bot_make - 

   . get move as a list of tokens from interface.
   . convert to native format.
   . execute move

   No checking is done.  Move better be correctly formatted!
   ---------------------------------------------------------- */

void  bot_make( position *cur,  position *nxt, int tc,  char **tok )
{
  int     i;
  move_t  m;
  int     typ = 0;
  int     col = 0;
  int     squ = 0;
  int     tsq = 0;


  m.steps = 0;

  for (i=1; i<tc; i++)
  {
    char  *s = tok[i];

    if (s[3] == 'x') continue;

    typ = 16 - strlen( strchr( "-RCDHME--rcdhme-", (int) s[0] ) );
    col = typ / 8;
    typ = typ & 7;
    squ = alg2index(s + 1);

    switch( s[3] ) 
    {
      case 'n' :  tsq = squ - 8; break;
      case 's' :  tsq = squ + 8; break;
      case 'e' :  tsq = squ + 1; break;
      case 'w' :  tsq = squ - 1; break;
    }

    // printf("mvdata = %d %d %d %d\n", col, typ, squ, tsq );

    m.list[m.steps].fsq = squ;
    m.list[m.steps].tsq = tsq;
    m.list[m.steps].typ = typ;
    m.list[m.steps].col = col;
    m.steps++;
  }

  make_move( cur, nxt, m );
}




int  notate_make( position *cur, position *nxt, move_t mv, char *buf )
{
  int   i;
  int   c;
  char  pce[2];
  char  sqbuf[8];

  strcpy( buf, "" );

  *nxt = *cur;


  for (i=0; i<mv.steps; i++)
  {
    step_t  s = mv.list[i];

    if (s.fsq == 0 && s.tsq == 0) continue;

    pce[0] = "-RCDHME--rcdhme-"[ s.col * 8 + s.typ ]; 
    pce[1] = 0;

    strcat(buf, pce);

    alg(sqbuf, s.fsq);
    strcat(buf, sqbuf);

    // direction
    // -------------------
    switch( s.tsq - s.fsq )
    {
      case -8 :  strcat(buf, "n "); break;
      case  8 :  strcat(buf, "s "); break;
      case  1 :  strcat(buf, "e "); break;
      case -1 :  strcat(buf, "w "); break;
    }



    /* execute move on board */
    nxt->bd[s.col][s.typ] ^= bit_on( s.fsq );
    nxt->bd[s.col][s.typ] ^= bit_on( s.tsq );
    nxt->bd[s.col][0] ^= bit_on( s.fsq );
    nxt->bd[s.col][0] ^= bit_on( s.tsq );

    /* see if ANY pieces are lost in trap */
    for (c=0; c<2; c++)
    {
      if (nxt->bd[c][0] & TRAPS)
      {
	u64  x = nxt->bd[c][0] & TRAPS;
	
	while (x)
	{
	  int  t = lix(x);
	  u64  ton = bit_on(t);
	  
	  x ^= ton;  // clear out

	  if ( !(neighbors_of(ton) & nxt->bd[c][0]) )
	  {
	    int typ = type_of( nxt, c, t );
	    nxt->bd[c][typ] ^= ton;
	    nxt->bd[c][0] ^= ton;

	    pce[0] = "-RCDHME--rcdhme-"[ c * 8 + typ ]; 
	    pce[1] = 0;
	    strcat(buf, pce);
	    alg(sqbuf, t);
	    strcat(buf, sqbuf);
	    strcat(buf, "x");
	    strcat(buf, " ");
	  }
	}
      }
    }
  }

  nxt->steps += mv.steps;  // number of steps
  nxt->last_move = mv;     // record of move that created this state
  nxt->his = cur;          // pointer to previous position

  if (nxt->steps == 4) 
  {
    nxt->steps = 0;
    nxt->ply++;
  }

  {
    int  x = strlen(buf);

    if (x) 
      buf[x-1] = 0;
  }
  
  return(0);
}




/* --------------------------------------------------------
   bot_notate - 

   Take a main line and produce arimaa bot notation.
   This routine is a big ugly hack and could be 
   greatly simplified.   But it seems to work.   I would
   look here first if you find bot interface bugs.
   --------------------------------------------------------- */


void  bot_notate( position *p, char *buf,  pv_t mv )
{
  int        i;
  position   fut[8];
  int        pc = 0;

  fut[0] = *p;

  strcpy(buf, "");


  for (i=0; i<mv.count; i++)
  {
    char  subuf[16];
    move_t  m;
    
    m = mv.pv[i];

    notate_make( &fut[pc], &fut[pc+1], m, subuf );
    pc++;

    strcat(buf, subuf);
    strcat(buf, " ");

    if ((fut[pc].ply & 1) != (fut[pc-1].ply & 1)) break;
  }

  
  
  buf[ strlen(buf)-1 ] = '\0';
}





void  clear_position( position *p )
{
  int     i;
  move_t  no_move;

  no_move.steps = 0;

  /* clear board */
  for (i=0; i<7; i++) {
    p->bd[0][i] = 0ULL;
    p->bd[1][i] = 0ULL;
  }

  p->ply = 0;
  p->his = NULL;
  p->steps = 0;
  p->last_move = no_move;   // No history here.

  return;
}


/* insert moves onto board for opening setup */
void  bot_insert( position *p, char *s )
{
  int     i;
  int     tc;
  char    *tok[200];
  int     col;
  int     typ;
  int     squ;

  tc = ParseString( s, tok );

  for (i=0; i<tc; i++)
  {
    typ = 16 - strlen( strchr( "-RCDHME--rcdhme-", (int) tok[i][0] ) );
    squ = alg2index( tok[i] + 1 );

    p->bd[typ/8][typ & 7] |= bit_on(squ);
    p->bd[typ/8][0] |= bit_on(squ);
  }
  
  /* build position signature */
  p->signature = 0ULL;
  for (col=0; col<2; col++)
    for (typ=1; typ<7; typ++)
    {
      u64  bb = p->bd[col][typ];

      while(bb)
      {
	int squ = lix(bb);
	bb ^= bit_on(squ);  /* clear out bit */
	p->signature ^= zobrist[col][typ][squ];
      }
    }

  return;
}







pv_t  begin_search( position *p )
{
  int       d;
  pv_t      main_line;  
  pv_t      cur_main;

  
  // display_position( &gme[0], flog );


  start_time = seconds();   // current time

  fprintf(flog, "\n");
  fprintf(flog, "Depth   Elapsed      Knps    Eval  Main Line\n");
  fprintf(flog, "-----  --------  --------  ------  ------------------------------------------\n");
  
  nodec = 0ULL;
  

  abort_flag = 0;

  for (d=2; d<=20 ; d++)    
  {
    char   buf[512];
    int    sc;
    double   et;
    double   knps;

    if (step_level>0){
      if (d>step_level){ abort_flag = 1; }
    }
    if (! abort_flag){
      sc = root( p, d, &main_line );
    }
    if (abort_flag)
    {
      strcpy(buf, "-");
      get_pv(buf, &cur_main);
      et = seconds() - start_time;
      if (et < 0.01)  et = 0.01;     // avoid divide by zero errors
      knps = nodec / et / 1000.0;
      fprintf(flog, "   --  %8.1f  %8.1f      --  %s\n",  et, knps,  buf );
      fprintf(flog, "   --    node count = %d\n",  nodec );
      fflush(flog);
    }
    else
    {
      cur_main = main_line;
      get_pv( buf, &cur_main);
      
      et = seconds() - start_time;
      if (et < 0.01)  et = 0.01;     // avoid divide by zero errors
      knps = nodec / et / 1000.0;

      fprintf(flog, "%5d  %8.1f  %8.1f  %6d  %s\n", d, et, knps, sc, buf );
      fflush(flog);
    }

    if (abort_flag) break;
    if (sc > 99900) break;
    if (sc < -99900) break;
  }

  return( cur_main );
}



void  do_bot( char *file )
{
  // pv_t      main_line;
  char      s[512];
  char      ps[512];
  FILE      *f;
  int       tc;
  char      *tok[512];
  position  gme[500];
  int       ply = 0;


  clear_position( &gme[0] );
  

  f = fopen( file, "r" );

  while (fgets(s, 511, f) != NULL) {
    s[511] = '\0';
    strcpy( ps, s );

    if (ps[0] == '1' && ps[1] == 'w') {
      fprintf(fgme, "\n\n");
    }
    fprintf( fgme, ps );
    fflush(fgme);

    // fprintf(flog, "recieving: %s\n", ps);
    // fflush(flog);

    tc = ParseString(s, tok);

    if ( !strcmp(tok[0], "1w") ) 
    {
      if (tc == 1) 
      {
        s[0] = '\0';
        if (strlen(sfile) > 0){
          pick_setup(s, 'w', sfile);
        }
        if (strlen(s) <= 0){
	  strcpy(s, "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2");
          if (playrand){
            shuffle_pieces(s);
          }
        }
	printf("%s\n", s);
	// fprintf(flog, "Applying this move: %s\n", s);
	// fflush(flog);
	bot_insert(&gme[0], s);
	// display_position(&gme[0], flog);
	return;
      }
      else
      {
	bot_insert(&gme[0], ps + 3);
	// display_position(&gme[0], flog);
      }
      continue;
    }

    if ( !strcmp(tok[0], "1b") ) 
    {
      if (tc == 1) 
      {
/*
	if (gme[0].bd[0][6] & FPARITY)
	  strcpy(s, "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 md7 ee7 cf7 dg7 hh7");
	else
	  strcpy(s, "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ee7 md7 cf7 dg7 hh7");
*/

        s[0] = '\0';
        if (strlen(sfile) > 0){
          pick_setup(s, 'b', sfile);
        }
        if (strlen(s) <= 0){
	  strcpy(s, "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7");
          if (playrand){
            shuffle_pieces(s);
          }
        }
	printf("%s\n", s);
	// fprintf(flog, "Applying this move: %s\n", s);
	bot_insert(&gme[0], s);

	return;
      }
      else
      {
	bot_insert(&gme[0], ps + 3);
      }
      continue;
    }

    if (tc == 1)  // need to actually produce a move here!
    {
      pv_t  main_line;
      char  buf[64];

      display_position(&gme[ply], flog);
      main_line = begin_search( &gme[ply] );
      bot_notate( &gme[ply], buf, main_line );
      fprintf(flog, "\nApplying this move: %s\n", buf);
      fflush(flog);
      printf("%s\n", buf);
      return;
    }

    // Apply move to the board 
    // ------------------------
    bot_make( &gme[ply], &gme[ply+1], tc, tok );
    ply++;
    gme[ply].ply++;

  }


  return;
}

void strrep(char *s, char f, char t){
  int i;

  for(i=0;s[i]!='\0';i++){   // s =~ s/f/t/g;
    if (s[i]==f){ s[i]=t; }
  }
}

void strlwr(char *s){
  int i;

  for(i=0;s[i]!='\0';i++){   // s =~ s/f/t/g;
    if ((s[i]>='A') && (s[i]<='Z')){ s[i] = s[i] - 'A' + 'a'; }
  }
}


/*
Randomly pick a setup from the input file.
The format of the setup should be from golds point of view.
It will be converted to silvers point of view if needed.

For example, it should be entered like this:
Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2

If needed for silver (side=='b') it will be converted to this:
rh8 rg8 rf8 re8 rd8 rc8 rb8 ra8 hh7 dg7 cf7 ee7 md7 cc7 db7 ha7

*/

pick_setup(char *s, char side, char *file){
  FILE *f;
  char b[512];
  int i, count, r;

  s[0] = '\0';

  count = 0;
  f = fopen( file, "r" );
  if (f == NULL){ return(0); }
  while (fgets(b, 511, f) != NULL) {
    b[511] = '\0';
    for(i=0;b[i]!='\0';i++){   // b =~ s/\n/\0/g;
      if (b[i]=='\n'){ b[i]='\0'; }
    }
    if (strlen(b) >= 63){ 
      if ((b[0]>='A') && (b[0]<='Z')){
        count += 1; 
      }
    }
    b[0] = '\0';
  }
  fclose(f);
  if (count == 0){ return(0); }
  
  r = rand() % count;
  count = 0;
  f = fopen( file, "r" );
  while (fgets(b, 511, f) != NULL) {
    b[511] = '\0';
    for(i=0;b[i]!='\0';i++){   // b =~ s/\n/\0/g;
      if (b[i]=='\n'){ b[i]='\0'; }
    }
    if (strlen(b) >= 63){ 
      if ((b[0]>='A') && (b[0]<='Z')){
        if (r == count){
          b[63] = '\0';
          strcpy(s, b);
        }
        count += 1; 
      }
    }
    b[0] = '\0';
  }
  fclose(f);

  if (side == 'b'){
    strrep(s, '1', '8');
    strrep(s, '2', '7');
    strrep(s, 'a', 'H');
    strrep(s, 'b', 'G');
    strrep(s, 'c', 'F');
    strrep(s, 'd', 'E');
    strrep(s, 'e', 'D');
    strrep(s, 'f', 'C');
    strrep(s, 'g', 'B');
    strrep(s, 'h', 'A');
    strlwr(s);
  }
}

shuffle_pieces(char *s){
  int i, p1, p2;
  char tmp;

  for(i=0;i<100;i++){
    p1 = rand() % 16;
    p2 = rand() % 16;
    tmp = s[4*p1];
    s[4*p1] = s[4*p2];
    s[4*p2] = tmp;
  }
}

/* for now we only look for the tcmove field and use that
   to set the number of seconds per move */
read_gamestate(char *file){
  FILE *f;
  char b[512];
  int t;

  f = fopen( file, "r" );
  if (f == NULL){ return(0); }
  while (fgets(b, 511, f) != NULL) {
    b[511] = '\0';
    if (strncmp(b,"tcmove=", 7) == 0){
      sscanf(&b[7], "%d", &t);
      if (t > 0){
        time_level = t;
        fclose(f);
        return(1);
      }
    }
    b[0] = '\0';
  }
  fclose(f);
  return(0);
}






#define argcmp(s) if ( strcmp(argv[i], s) == 0 )
#define woops if ( i >= argc-1 ) { fprintf(stderr, "woops!\n"); return(1); }


int main( int argc, char **argv )
{
  char      extra[3][PATH_MAX];
  int       ec = 0;
  int       i;
  int       bot = 1;


  flog = fopen( "occ.log", "a" );
  fgme = fopen( "game.log", "a" );

  for (i=1; i<argc; i++) {

    argcmp("-v") {   // print version number
      fprintf(stderr, "%s\n", version);
      exit(0);
    }

/*
    argcmp("-b") {
      bot = 1;
      continue;
    }
*/

    argcmp("-t") {
      woops;
      i++;
      time_level = strtod( argv[i], NULL );  

      if (time_level < 1.0) {
	fprintf( stderr, "Illegal -time argument given.\n" );
	return(1);
      }
      continue;
    }

/* This was added by Omar; option to limit search by number of steps */
    argcmp("-d") {
      woops;
      i++;
      step_level = atoi( argv[i] );  
      time_level = 999999.0; /* if step_level is given; set time to unlimited */

// if step level is 0 then make random 4 step moves
      if (step_level == 0) {
        step_level = 4;
        playrand = 1;
      }
      if (step_level < 0) {
	fprintf( stderr, "Illegal -step argument given.\n" );
	return(1);
      }
      continue;
    }

/* This was added by Omar; option to provide a random seed */
    argcmp("-r") {
      woops;
      i++;
      rseed = atoi( argv[i] );  

      if (rseed < 1) {
	fprintf( stderr, "Illegal -randomSeed argument given.\n" );
	return(1);
      }
      continue;
    }

/* This was added by Omar; option to provide a setup file for 1st move */
    argcmp("-1") {
      woops;
      i++;
      strcpy(sfile, argv[i]);

      if (strlen(sfile) < 1) {
	fprintf( stderr, "No setup file given.\n" );
	return(1);
      }
      continue;
    }
    
    if (ec < 3) {
      strcpy( extra[ec], argv[i] );
      ec++;
      continue;
    }
    
  }

/* if no -d or -t option was given see if we can find a tcmove field in
   the gamestate file and limit the search based on that. Otherwise
   we will default to '-d 8'.
*/
  if ((step_level==0) && (time_level == 0.0)){
    read_gamestate( extra[2] );
    if (time_level == 0.0){
      step_level = 8;
      time_level = 999999.0;
    }
  } 
  
/* need to put this after parsing the arguments */
  build_zobrist_table();
  build_move_offsets();


  if (bot) {
    do_bot( extra[1] );
  }

  return(0);

}







