#include "eval.h"

//---------------------------------------------------------------------
//  section Eval
//---------------------------------------------------------------------- 

u64 adv[8][2] = { 
        {0x00000000000000ffULL, 0xff00000000000000ULL},
		    {0x000000000000ff00ULL, 0x00ff000000000000ULL},
		    {0x0000000000ff0000ULL, 0x0000ff0000000000ULL},
		    {0x00000000ff000000ULL, 0x000000ff00000000ULL},
		    {0x000000ff00000000ULL, 0x00000000ff000000ULL},
		    {0x0000ff0000000000ULL, 0x0000000000ff0000ULL},
		    {0x00ff000000000000ULL, 0x000000000000ff00ULL},
		    {0xff00000000000000ULL, 0x00000000000000ffULL} 
      };

//static piece values rabbit 100, cat 250, ...
const int pieceValue[7] = { 0, 100, 250, 300, 800, 1100, 1800 };

//less rabbits => lesser chance for winning
const int rabbitPenalty[8] = { -10000, -500, -400 ,-200, -150, -50, 0, 0};

//penalty for being frozen per piece percentage from value of piece
const float frozenPenaltyRatio = -0.2; 

//penalty for being frozen per piece rabbit 10, cat 20, ...
//const int frozenPenalty[7] = { 0, 10, 20, 25};

#define EVAL_MAX 1500
#define EVAL_MIN (-10000)

float Eval::evaluateInPercent(const Board* b) const
{
  int evaluation = evaluate(b);

  float p = (evaluation - EVAL_MIN) / float(EVAL_MAX - EVAL_MIN);
  if (p < 0)
    return 0;
  if (p > 1)
    return 1;
  return p;
}

//--------------------------------------------------------------------- 

int Eval::evaluate(const Board* b) const
{
  int   tot[2] = { 0, 0 };

  assert(b->getStepCount() == 0);

  for (int player = 0; player < 2; player++) {
    logDDebug("Player %d", player); 
    logDDebug("=================================="); 
    
    //pieces
    u64 pieces = b->bitboard_[player][0]; 
    u64 movable = b->calcMovable(player);
    coord_t pos = -1;
    logDDebug("============"); 

    while ((pos = bits::lix(pieces)) != -1){
      piece_t piece = b->getPiece(pos, player);
      tot[player] += pieceValue[piece];
      string s = pieceToStr(player, piece, pos);
      logDDebug("material bonus %4d for %s", pieceValue[piece], s.c_str());

    //frozen
      if (! bits::getBit(movable, pos)){
        tot[player] += pieceValue[piece] * frozenPenaltyRatio;
        logDDebug("material penalty %4.2f for %s being frozen", pieceValue[piece] * frozenPenaltyRatio, s.c_str());
      } 
    }

    //traps 
    /*
    u64 traps = TRAPS;   
    coord_t trap = -1;
    trapType_e trapTypes[2][4];
    while ((trap = bits::lix(traps)) != -1){
      u64 guards = bits::neighborsOne(trap) & bitboard_[player][0];
      if (! guards)  {
        trapTypes[player][TRAP_TO_INDEX(i)] = TT_UNSAFE;
      }
      //if (bitC
      
      //frame
      //if (bits::neighborsOne(trap) ^ ( bitboard_[GOLD][0] || ) 
    }
  */
    
    //rabbits 
    int rabbitsNum = bits::bitCount(b->bitboard_[player][RABBIT]);
    logDDebug("rabbit penalty %4d for %d rabbits left", rabbitPenalty[rabbitsNum], rabbitsNum);
    tot[player] += rabbitPenalty[rabbitsNum];

    //mobility

    //influence
    
    
    //special shapes: pins, forks, hostages
    

  }

  return tot[0] - tot[1];
}
//--------------------------------------------------------------------- 

int Eval::evaluateDailey(const Board* b) const
{
  int   tot[2] = { 0, 0 };
  u64   dom;                   // bit board of dominant pieces.
  u64   ndom;                   // bb of non-dominant pieces 

//  if (playrand){ return (rand()%10000 - 5000); } /* act like a random bot */
//  if (playrand){ return (10000 - (p->steps)*1000); } /* act like a random bot */
  assert(b->getStepCount() == 0);

  for (int c = 0; c < 2; c++)  // c = color, 0 or 1
  {

    for (int i = 1; i < 7; i++){
      tot[c] += bits::bitCount(b->bitboard_[c][i]) * i * 100;
    }

    // small connectivity bonus
    tot[c] += bits::bitCount(b->bitboard_[c][0] & 
              bits::neighbors(b->bitboard_[c][0]));

    // bonus for rabbit advancement - advance if a few enemy (non-pawn) 
    // pieces have left the board.
    if ( bits::bitCount(b->bitboard_[OPP(c)][0] ^ b->bitboard_[OPP(c)][1] ) < 5 ) 
    {
      tot[c] += bits::bitCount(b->bitboard_[c][1] & adv[4][c] ) * 2;
      tot[c] += bits::bitCount(b->bitboard_[c][1] & adv[5][c] ) * 3;
      tot[c] += bits::bitCount(b->bitboard_[c][1] & adv[6][c] ) * 5;
      tot[c] += bits::bitCount(b->bitboard_[c][1] & adv[7][c] ) * 8;
    }

    // figure out which pieces are dominant
    // ------------------------------------
    {
      int  strongestEnemy = 0;   
      for (int i = 6; i > 0; i--) {
        if (b->bitboard_[OPP(c)][i]) 
        {
          strongestEnemy = i;
          break;
        }
      }

      dom = 0ULL;
      for (int i = strongestEnemy; i <=6 ; i++) {
	      dom |= b->bitboard_[c][i];
      }

      ndom = b->bitboard_[c][0] ^ dom;
    }

    // get dominant piece out to center
    // --------------------------------
    tot[c] += bits::bitCount(dom & RING0) * 10;
    tot[c] += bits::bitCount(dom & RING1) * 4;

    // don't hang out in traps.
    // ------------------------
    tot[c] -= bits::bitCount(dom & TRAPS) * 12;    // penalty 12 for dominant
    tot[c] -= bits::bitCount((ndom & TRAPS)) * 7;   // penalty  7 for non
  }

  return(tot[0] - tot[1]);
}

//--------------------------------------------------------------------- 


float Eval::evaluateStep(const Board* b, const Step& step) const
{

  return 0;
  /*
  float eval = 0; 

  if (step.isPass()){
    eval -= 0.5 * (STEPS_IN_MOVE - stepCount_);
    return eval;
  }

  assert(step.pieceMoved());
  assert(IS_PLAYER(board_[step.from_]));
  
  //we don't like inverse steps 
  //TODO ... inverse step is reasonable when something dies in the trap
  if (step.inversed(lastStep_)){
    eval -= 5;
    return eval;
  }

  if (step.piece_ == ELEPHANT ) {
    eval += 0.1;
  }

  if (step.isPushPull()){
    //push opponent to the goal :( not impossible ? )
    if (step.oppPiece_ == PIECE_RABBIT && 
        ROW(step.oppTo_) == rabbitWinRow[PLAYER_TO_INDEX(OPP(step.player_))]){
      eval -= 10;
    }
    //otherwise push/pulls are encouraged
    else{
      eval += 0.5; 
    }
  } 
  
  //check self-kill
  if (checkKillForward(step.from_, step.to_)){
    //allow self-kills to allow rabbits move to the goal
    //in the opponent's part of the board
    if (step.piece_ == PIECE_RABBIT && ! IS_TRAP(step.to_) && 
        ((toMove_ == GOLD && ROW(step.from_) >= 4) ||
        (toMove_ == SILVER && ROW(step.from_) <= 5))){
      eval -= 1;
    }
    else{
      eval -= 5;   
    }
  }
  else{

    //push opp to trap is good 
    if (step.isPushPull() && IS_TRAP(step.oppTo_)){
      eval += 3;
    }

    //check opp-kill
    if (step.isPushPull() && checkKillForward(step.oppFrom_, step.oppTo_)){
      eval += 5;   
    }
  }

  //rabbit movements 
  if (step.piece_ == PIECE_RABBIT){
    //moves in opponent's part of the board are encouraged
    if ((toMove_ == GOLD && ROW(step.from_) >= 4) ||
        (toMove_ == SILVER && ROW(step.from_) <= 5)){
      //empty space ahead is good 
      bool empty = true;
      int emptyNum = 0;
      int row = ROW(step.from_);
      int toEdge = toMove_ == GOLD ? 
          TOP_ROW - row : 
          row - BOTTOM_ROW;
      while (emptyNum < toEdge){
        row += toMove_ == GOLD ? +1 : -1 ;
        if (board_[row * 10 + COL(step.from_)] != EMPTY_SQUARE){
          empty = false;
          assert(toEdge > emptyNum);
          break;
        }
        emptyNum++;
      } 
      //moving forward
      if (step.to_ - step.from_ == rabbitForward[toMoveIndex_]) {
        if (empty){
          eval += 5;
        }
        eval += emptyNum * 1;
      }
      //TODO add evaluation for moves left right

    }
    else { //move in player's part
      eval += -2;
    }
  } //rabbits

  //locality 
  if (cfg.localPlayout() && 
      lastStep_.stepType_ != STEP_NULL){
    int d = SQUARE_DISTANCE(lastStep_.to_, step.from_);
    eval += d <= 3 ? (3 - d) * 0.5 : 0;
  }

  return eval;
  */
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

