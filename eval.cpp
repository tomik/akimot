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
const int rabbitPenalty[9] = { -10000, -500, -400 ,-200, -150, -50, -20, 0, 0};

//penalty for being frozen per piece percentage from value of piece
const float frozenPenaltyRatio = -0.2; 

const int domRadius = 2;

//dangerous
const int trapSoleVal = 50;  
const int trapSafeeVal = 50;  
const int trapActiveVal = 150;
const int trapPotVal = 100;
//ratio substracted from piece value if framed 
const float framePenaltyRatio = -0.3;  
//ratio substracted from piece value if supports framed piece (not mobile)
const float pinnedPenaltyRatio = frozenPenaltyRatio;  

//penalty for being frozen per piece rabbit 10, cat 20, ...
//const int frozenPenalty[7] = { 0, 10, 20, 25};

#define EVAL_MAX 2000
#define EVAL_MIN (-2000)

#define EVAL_MAX_DAILEY 2000
#define EVAL_MIN_DAILEY (-2000)

//--------------------------------------------------------------------- 

Eval::Eval() 
{
  evalTT_ = new EvalTT();
}

//--------------------------------------------------------------------- 
 
float Eval::evaluateInPercent(const Board* b) const
{
  /*
  if (evalTT_->hasItem(b->getSignature())){
    float p = 0; 
    evalTT_->loadItem(b->getSignature(), p); 
    return p;
  }
  */

  int evaluation;
  float p; 

  if (cfg.useBestEval()){
    evaluation = evaluate(b);
    p = (evaluation - EVAL_MIN) / float(EVAL_MAX - EVAL_MIN);
  }else{ 
    evaluation = evaluateDailey(b);
    p = (evaluation - EVAL_MIN_DAILEY) / float(EVAL_MAX_DAILEY - EVAL_MIN_DAILEY);
  }
  p = p < 0 ? 0 : (p > 1 ? 1 : p);

  //evalTT_->insertItem(b->getSignature(), p);

  return p;
}

//--------------------------------------------------------------------- 

int Eval::evaluate(const Board* b) const
{
  float  tot[2] = { 0, 0 };

  logDDebug("========================="); 
  logDDebug("Tomik's Arimaa Evaluation"); 
  logDDebug("========================="); 
  logDDebug(""); 

  assert(b->getStepCount() == 0);
  
  u64 movable = b->calcMovable(GOLD) | b->calcMovable(SILVER);

  for (int player = 0; player < 2; player++) {
    logDDebug("Player %d", player); 
    logDDebug("=================================="); 
    
    //pieces
    u64 pieces = b->bitboard_[player][0]; 
    coord_t pos = -1;
    logDDebug("============"); 

    while ((pos = bits::lix(pieces)) != BIT_EMPTY){
      piece_t piece = b->getPiece(pos, player);
      tot[player] += pieceValue[piece];
      
      logDDebug("material bonus %4d for %s", pieceValue[piece], pieceToStr(player, piece, pos).c_str());

    //frozen
      if (! bits::getBit(movable, pos)){
        tot[player] += pieceValue[piece] * frozenPenaltyRatio;
        logDDebug("material penalty %4.2f for %s being frozen", pieceValue[piece] * frozenPenaltyRatio, pieceToStr(player, piece, pos).c_str());
      } 
    }

    //rabbits 
    int rabbitsNum = bits::bitCount(b->bitboard_[player][RABBIT]);
    logDDebug("rabbit penalty %4d for %d rabbits left", rabbitPenalty[rabbitsNum], rabbitsNum);
    tot[player] += rabbitPenalty[rabbitsNum];

    //blockade

    //influence
     
    //hostages
    
    //special shapes: forks
    
  }

  //traps 
  u64 traps = TRAPS;   
  coord_t trap = BIT_EMPTY;
  //trapType_e trapTypes[2][4];
  u64 guards[2];
  int guardsNum[2] = {0, 0};

  while ((trap = bits::lix(traps)) != BIT_EMPTY){
    guards[GOLD] = bits::neighborsOne(trap) & b->bitboard_[GOLD][0];
    guards[SILVER] = bits::neighborsOne(trap) & b->bitboard_[SILVER][0];

    guardsNum[GOLD] = bits::neighborsOneNum(trap, guards[GOLD]);
    guardsNum[SILVER] = bits::neighborsOneNum(trap, guards[SILVER]);
    
    u64 guardsArea = guards[GOLD] | guards[SILVER];
    u64 victimsArea = bits::sphere(trap, 2);
    u64 influenceArea = bits::sphere(trap, 3);

    player_t guardDom = b->strongestPieceOwner(guardsArea);
    player_t influenceDom = b->strongestPieceOwner(influenceArea & movable);

    logDDebug("trap %s - dominant %d/%d guards g/s : %d/%d",
      coordToStr(trap).c_str(), guardDom, influenceDom, 
      guardsNum[GOLD], guardsNum[SILVER]); 

    for (int player = 0; player < 2; player++) {
      if (guardsNum[player] > 0 && guardsNum[OPP(player)] == 0){
        tot[player] += trapSoleVal;
        logDDebug("trap sole %d to %d player for %s trap " , 
          trapSoleVal, player, coordToStr(trap).c_str()); 
        //bonus for opponent pieces in victims Area
        if (influenceDom == player && 
            victimsArea & b->bitboard_[OPP(player)][0] & ~movable){
          tot[player] += trapPotVal;
          logDDebug("trap pot bonus %d to %d player for %s trap " , 
            trapPotVal, player, coordToStr(trap).c_str()); 
        }
      }
      if (guardsNum[player] > 0 && guardsNum[OPP(player) > 0]){
        //2 guards or locally unbeatable piece => safe
        //(guardDom == player cannot be used => absolutely strongest piece )
        if (guardsNum[player] >= 2 || 
            (guardDom != OPP(player) && influenceDom != OPP(player))){

          //dominance in trap
          if (guardsNum[player] >= guardsNum[OPP(player)] && guardDom == player){
            tot[player] += trapActiveVal;
            logDDebug("trap active %d to %d player for %s trap " , 
                      trapActiveVal, player, coordToStr(trap).c_str()); 
          }
          else{
            tot[player] += trapSafeeVal;
            logDDebug("trap safe %d to %d player for %s trap " , 
                      trapSafeeVal, player, coordToStr(trap).c_str()); 
          }
        }
      }


      //check frame
      if ((BIT_ON(trap) & b->bitboard_[player][0]) && guardsNum[OPP(player)] == 3) {
        u64 all = (b->bitboard_[GOLD][0] | b->bitboard_[SILVER][0] );
        //squares that must be blocked for trapped piece not to break free
        piece_t framedPiece = b->getPiece(trap, player); 
        u64 mustBlock = 
          bits::neighbors(guards[OPP(player)] & b->weaker(OPP(player), framedPiece));
        
        if (guardsArea == bits::neighborsOne(trap) && 
           ( mustBlock == 0ULL || (mustBlock & all) == mustBlock )){
          //frame penalty
          tot[player] += framePenaltyRatio *  pieceValue[framedPiece];
          logDDebug("full frame penalty %4.2f to %d player for %s framed" , 
                    framePenaltyRatio *  pieceValue[framedPiece],
                    player, pieceToStr(player, framedPiece, trap).c_str()); 
          //pin penalty
          u64 u = guards[player];
          assert(u);
          int pinnedPos = bits::lix(u);
          piece_t pinnedPiece = b->getPiece(pinnedPos, player);
          assert(IS_PIECE(pinnedPiece));
          assert(!u);
          tot[player] += pinnedPenaltyRatio *  pieceValue[pinnedPiece];
              logDDebug("full pin penalty %4.2f to %d player for %s pinned" , 
                        pinnedPenaltyRatio *  pieceValue[pinnedPiece], 
                        player, pieceToStr(player, pinnedPiece, pinnedPos).c_str()); 
          int sd = b->strongerDistance(player, pinnedPiece, pinnedPos);
          if (sd != BIT_EMPTY && sd <= 2) {
            tot[player] += framePenaltyRatio * pieceValue[framedPiece];
            logDDebug("extra pin penalty %4.2f to %d player for %s pinned in danger",
                  framePenaltyRatio *  pieceValue[framedPiece],
                  player, pieceToStr(player, pinnedPiece, pinnedPos).c_str()); 
          }
        }
      }
    }
  }
      
/*
      switch(guardsNum[player]){
        case 0 :  trapTypes[player][TRAP_TO_INDEX(trap)] = TT_UNSAFE;
                  break;
        case 1 :  if (influenceDom == player){
                    trapTypes[player][TRAP_TO_INDEX(trap)] = TT_SAFE;
                  } else {
                    trapTypes[player][TRAP_TO_INDEX(trap)] = TT_HALF_SAFE;
                  }
                  break;
        default : trapTypes[player][TRAP_TO_INDEX(trap)] = TT_SAFE;
                  if (dominant == player) {
                    if (b->bitboard_[OPP(player)][0] & area) {
                      trapTypes[player][TRAP_TO_INDEX(trap)] = TT_ACTIVE;
                    }
                  }
      }

    
    logDDebug("trap status %s for %s trap - dominant %d/%d" , 
            trapTypeToStr(trapTypes[player][TRAP_TO_INDEX(trap)]).c_str(), 
            coordToStr(trap).c_str(), guardDom, influenceDom); 
    }
*/
    
    //frame
    //if (bits::neighborsOne(trap) ^ ( bitboard_[GOLD][0] || ) 
  

  logDDebug("total for gold %d", int(tot[0] - tot[1]));
  return int(tot[0] - tot[1]);
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

  float eval = 0; 

  if (step.isPass()){
    eval -= 0.5 * (STEPS_IN_MOVE - b->stepCount_);
    return eval;
  }
    return 0;
  /*

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
    
string Eval::trapTypeToStr(trapType_e trapType)
{
  switch (trapType) {
    case TT_UNSAFE: return string("unsafe");
    case TT_HALF_SAFE: return string("half safe");
    case TT_SAFE: return string("safe");
    case TT_ACTIVE: return string("active");
    default: assert(false);
  }
  return "";
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

