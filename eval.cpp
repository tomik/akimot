#include "eval.h"

extern string implicit_eval_cfg;

//---------------------------------------------------------------------
//  section ValueItem
//---------------------------------------------------------------------- 

ValueItem::ValueItem(string name, itemType_e type, void* item, int num) : 
  name_(name), type_(type), item_(item), num_(num)
{
}

//--------------------------------------------------------------------- 

bool ValueItem::isSingleValue() const
{
  return num_ == SINGLE_VALUE;
}

//---------------------------------------------------------------------
//  section Values
//---------------------------------------------------------------------- 

Values::Values(){
  init();
  loadFromString(implicit_eval_cfg);
  mirrorPiecePositions();
}

//--------------------------------------------------------------------- 

Values::Values(string config) { 
  init();
  loadFromString(config);
  mirrorPiecePositions();
}

//--------------------------------------------------------------------- 

void Values::init(){ 
  values.clear();
  values.push_back(ValueItem("piece_values", IT_INT_AR, (void*)&pieceValue, PIECE_NUM + 1));
  values.push_back(ValueItem("rabbit_penalty", IT_INT_AR, (void*)&rabbitPenalty, RABBITS_NUM + 1));
  values.push_back(ValueItem("frozen_penalty_ratio", IT_FLOAT, (void*)&frozenPenaltyRatio, SINGLE_VALUE));
  values.push_back(ValueItem("trap_sole_val", IT_INT, (void*)&trapSoleVal, SINGLE_VALUE));
  values.push_back(ValueItem("trap_more_than_one_val", IT_INT, (void*)&trapMoreThanOneVal, SINGLE_VALUE));
  values.push_back(ValueItem("trap_safe_val", IT_INT, (void*)&trapSafeVal, SINGLE_VALUE));
  values.push_back(ValueItem("trap_active_val", IT_INT, (void*)&trapActiveVal, SINGLE_VALUE));
  values.push_back(ValueItem("trap_pot_val", IT_INT, (void*)&trapPotVal, SINGLE_VALUE));
  values.push_back(ValueItem("active_trap_blocked_penalty", IT_INT, (void*)&activeTrapBlockedPenalty, SINGLE_VALUE));
  values.push_back(ValueItem("frame_penalty_ratio", IT_FLOAT, (void*)&framePenaltyRatio, SINGLE_VALUE));
  values.push_back(ValueItem("pinned_penalty_ratio", IT_FLOAT, (void*)&pinnedPenaltyRatio, SINGLE_VALUE));
  values.push_back(ValueItem("camel_hostage_penalty", IT_INT, (void*)&camelHostagePenalty, SINGLE_VALUE));
  values.push_back(ValueItem("elephant_blockade_penalty", IT_INT, (void*)&elephantBlockadePenalty, SINGLE_VALUE));
  values.push_back(ValueItem("elephant_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][ELEPHANT], BIT_LEN));
  values.push_back(ValueItem("elephant_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][ELEPHANT], BIT_LEN));
  values.push_back(ValueItem("elephant_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][ELEPHANT], BIT_LEN));
  values.push_back(ValueItem("camel_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][CAMEL], BIT_LEN));
  values.push_back(ValueItem("camel_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][CAMEL], BIT_LEN));
  values.push_back(ValueItem("camel_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][CAMEL], BIT_LEN));
  values.push_back(ValueItem("horse_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][HORSE], BIT_LEN));
  values.push_back(ValueItem("horse_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][HORSE], BIT_LEN));
  values.push_back(ValueItem("horse_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][HORSE], BIT_LEN));
  values.push_back(ValueItem("dog_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][DOG], BIT_LEN));
  values.push_back(ValueItem("dog_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][DOG], BIT_LEN));
  values.push_back(ValueItem("dog_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][DOG], BIT_LEN));
  values.push_back(ValueItem("cat_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][CAT], BIT_LEN));
  values.push_back(ValueItem("cat_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][CAT], BIT_LEN));
  values.push_back(ValueItem("cat_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][CAT], BIT_LEN));
  values.push_back(ValueItem("rabbit_position", IT_INT_AR, (void*)&piecePos[GS_BEGIN][SILVER][RABBIT], BIT_LEN));
  values.push_back(ValueItem("rabbit_position", IT_INT_AR, (void*)&piecePos[GS_MIDDLE][SILVER][RABBIT], BIT_LEN));
  values.push_back(ValueItem("rabbit_late_position", IT_INT_AR, (void*)&piecePos[GS_LATE][SILVER][RABBIT], BIT_LEN));

}

//--------------------------------------------------------------------- 

void Values::baseMirrorIndexes(int& player, int& index) 
{
  int c = index % 8; 
  int r = index / 8;

  if (c >= 4) {
    c = 7 - c;
  }

  if (player == GOLD) {
    r = 7 - r; 
    player = SILVER;
  }

  index = 8 * r + c;
  
}

//--------------------------------------------------------------------- 

void Values::mirrorPiecePositions()
{

  for (int gs = 0; gs < GS_NUM; gs++){
    for (int i = 0; i < 2; i++){
      for (int j = 0; j < PIECE_NUM + 1; j++){
        for (int k = 0; k < BIT_LEN; k++ ){
          int basepl = i; 
          int baseindex = k;
          baseMirrorIndexes(basepl, baseindex);
          piecePos[gs][i][j][k] = piecePos[gs][basepl][j][baseindex];
        }
      }
    }
  }

}


//--------------------------------------------------------------------- 

bool Values::loadFromString(string config) { 
  
  //remove = 
  config = replaceAllChars(config, CFG_SEP, ' ');
  stringstream sss;
  sss.str(config);
  //strip comments
  string noComments;
  string line;
  while (getline(sss, line)){
    if (trimLeft(line).find('#', 0) != 0){
      noComments += line + '\n';
    }
  }
  config = noComments;

  //proceed
  //cerr << config;
  string value;
  string s;
  ValueItem vi; 
  stringstream ss; 
   
  for (ValueList::const_iterator it = values.begin(); it != values.end(); it++){
    bool found = false;
    ss.str(config);
    while (ss.good() && ! found) {
      ss >> s;

      if (s == it->name_){ 
        vi = *it;
        int max = vi.isSingleValue() ? 1 : vi.num_;
        for (int i = 0; i < max; i++){
          ss >> value;
          if (! fillItemFromString( vi.item_, vi.type_, value, 
                                    vi.isSingleValue() ? -1 : i)){
              assert(false);
              logError("Unknown option type.");
              return false; 
          }
        
          if (! ss.good()) {
            logError("Incomplete evaluation confinguration.");
            return false;
          }
        }
        found = true;
      }
    }
    if (! found){
      logError("Item not found %s ... ", it->name_.c_str());
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------- 

string Values::toString() const 
{
  stringstream ss; 

  for (ValueList::const_iterator it = values.begin(); it != values.end(); it++){
    ss << (*it).name_ << " = ";
    if ((*it).isSingleValue()) {
      ss.width(6);
      ss << getItemAsString((*it).item_, (*it).type_, (*it).num_); 
    } else { 

      for (int i = 0; i < (*it).num_; i++){
        if ( (*it).num_ == BIT_LEN && i % 8 == 0 ){
          ss << endl;
        }
        ss.width(6);
        ss << getItemAsString((*it).item_, (*it).type_, i); 
      }
    }
    ss << endl << endl;
  }

  return ss.str();
}

//--------------------------------------------------------------------- 

bool Values::getItemForToken(string token, ValueItem& valueItem) const
{ 

  for (ValueList::const_iterator it = values.begin(); it != values.end(); it++){
    
    if ((*it).name_ == token) {
      valueItem = (*it);
      return true;
    }
  }
  return false;
}

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

//penalty for being frozen per piece rabbit 10, cat 20, ...
//const int frozenPenalty[7] = { 0, 10, 20, 25};

//#define EVAL_MAX 1900

#define EVAL_MAX 5000

#define EVAL_MAX_DAILEY 2000

//--------------------------------------------------------------------- 

Eval::Eval() 
{
 init(); 
}

//--------------------------------------------------------------------- 

Eval::Eval(const Board* board) 
{
  init();
  //base_eval_ = cfg.useBestEval() ? evaluate(board) : evaluateDailey(board);
  
}

//--------------------------------------------------------------------- 

void Eval::init() 
{
  evalTT_ = new EvalTT();
  vals_ = new Values(*cfg.evaluationValues());
  base_eval_ = 0;
  eval_max_ = cfg.useBestEval() ? EVAL_MAX : EVAL_MAX_DAILEY;
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

  if (cfg.extensionsInEval() && b->goalCheck(b->getPlayerToMove(), 3)) {
    return 1 - b->getPlayerToMove();
  }
  
  float evaluation;
  float p; 

  evaluation = cfg.useBestEval() ? evaluate(b) : evaluateDailey(b);
  //evaluation -= base_eval_;
  p = log_sig(double(6)/eval_max_, evaluation);
  logDDebug("%f ~ %f ", evaluation, p);

  //evalTT_->insertItem(b->getSignature(), p);

  return p;
}

//--------------------------------------------------------------------- 
 
float Eval::getPieceValue(piece_t piece) const
{
  return vals_->pieceValue[piece];
}

//--------------------------------------------------------------------- 

gameStage_e Eval::determineGameStage(const Bitboard & bitboard) const 
{
  int piecesNum = bits::bitCount(bitboard[GOLD][0] | bitboard[SILVER][0]);
  return piecesNum > 28 ? GS_BEGIN : (piecesNum > 20 ? GS_MIDDLE : GS_LATE );
}

//--------------------------------------------------------------------- 

bool Eval::blocked(player_t player, piece_t piece, coord_t coord, const Board* b) const 
{
  
  u64 nbg  = bits::neighborsOne(coord); 
  u64 mine = nbg & b->bitboard_[player][0];
  u64 opps = nbg & b->bitboard_[OPP(player)][0];
  u64 all  = b->bitboard_[GOLD][0] | b->bitboard_[SILVER][0];
  //unguarded traps
  u64 dieTrap = nbg & (TRAPS ^ (TRAPS & bits::neighbors(b->bitboard_[player][0] ^ BIT_ON(coord))));
  //TODO safe trap ... but blocked anyway ? 
  if (nbg ^ ((mine | opps | dieTrap) & nbg)) {
    return false;
  }
  u64 mustBlock = bits::neighbors(opps & (b->weaker(OPP(player), piece) ^ dieTrap));
  if (mustBlock == 0ULL || (mustBlock & all) == mustBlock ){
    return true;
  }

  return false;
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
  u64 all = (b->bitboard_[GOLD][0] | b->bitboard_[SILVER][0] );

  //determine game stage
  gameStage_e gs = determineGameStage(b->getBitboard());

  logDDebug("Game stage is : %d    [0 - begin, 1 - middle, 2 - end]", gs);

  for (int player = 0; player < 2; player++) {
    logDDebug("Player %d", player); 
    logDDebug("=================================="); 
    
    //pieces
    u64 pieces = b->bitboard_[player][0]; 
    coord_t pos = -1;
    logDDebug("============"); 

    while ((pos = bits::lix(pieces)) != BIT_EMPTY){
      piece_t piece = b->getPiece(pos, player);
      tot[player] += vals_->pieceValue[piece];
      
      logDDebug("material bonus %4d for %s", vals_->pieceValue[piece], Soldier(player, piece, pos).toString().c_str());

      tot[player] += vals_->piecePos[gs][player][piece][pos];
      logDDebug("positional bonus %4d for %s", vals_->piecePos[gs][player][piece][pos], Soldier(player, piece, pos).toString().c_str());

    //frozen
      if (! bits::getBit(movable, pos)){
        tot[player] += vals_->pieceValue[piece] * vals_->frozenPenaltyRatio;
        logDDebug("material penalty %4.2f for %s being frozen", vals_->pieceValue[piece] * vals_->frozenPenaltyRatio, Soldier(player, piece, pos).toString().c_str());
      } 

    //hostage situation TODO IMPROVE
      if (piece == CAMEL && ! bits::getBit(movable, pos)) {
        //get trap in distance 2 
        u64 m = bits::circle(pos, 2) & TRAPS;
        int trap = bits::lix(m);
        if (trap != BIT_EMPTY) {
          u64 guards[2];
          int guardsNum[2] = {0, 0};
          guards[GOLD] = bits::neighborsOne(trap) & b->bitboard_[GOLD][0];
          guards[SILVER] = bits::neighborsOne(trap) & b->bitboard_[SILVER][0];
          guardsNum[GOLD] = bits::neighborsOneNum(trap, guards[GOLD]);
          guardsNum[SILVER] = bits::neighborsOneNum(trap, guards[SILVER]);
          //little guards && empty trap
          if (guardsNum[player] < 2 && ! bits::getBit(all, trap)){
            tot[player] += vals_->camelHostagePenalty;
            logDDebug("hostage penalty %4d for %s being hostage", 
               vals_->camelHostagePenalty, Soldier(player, piece, pos).toString().c_str());
          }
        }
      }
    
    //blockade situation
    if (piece == ELEPHANT && blocked(player, piece, pos, b)) {
      tot[player] += vals_->elephantBlockadePenalty;
      logDDebug("elephant blockade penalty %4d for %s being blocked", 
         vals_->elephantBlockadePenalty, Soldier(player, piece, pos).toString().c_str());
    }
    
     

    //distance to stronger
    }

    //rabbits 
    int rabbitsNum = bits::bitCount(b->bitboard_[player][RABBIT]);
    logDDebug("rabbit penalty %4d for %d rabbits left", vals_->rabbitPenalty[rabbitsNum], rabbitsNum);
    tot[player] += vals_->rabbitPenalty[rabbitsNum];

    //influence
    
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
    
    u64 guardsAll = guards[GOLD] | guards[SILVER];
    u64 victimsArea = bits::sphere(trap, 2);
    u64 influenceArea = bits::sphere(trap, 3);

    player_t guardDom = b->strongestPieceOwner(guardsAll);
    player_t influenceDom = b->strongestPieceOwner(influenceArea & movable);

    logDDebug("trap %s - dominant %d/%d guards g/s : %d/%d",
      coordToStr(trap).c_str(), guardDom, influenceDom, 
      guardsNum[GOLD], guardsNum[SILVER]); 

    for (int player = 0; player < 2; player++) {
      
      if (guardsNum[player] > 1) {
        tot[player] += vals_->trapMoreThanOneVal;
        logDDebug("trap more than one %d to %d player for %s trap " , 
          vals_->trapMoreThanOneVal, player, coordToStr(trap).c_str()); 
      }

      //sole traps
      if (guardsNum[player] > 0 && guardsNum[OPP(player)] == 0){
        tot[player] += vals_->trapSoleVal;
        logDDebug("trap sole %d to %d player for %s trap " , 
          vals_->trapSoleVal, player, coordToStr(trap).c_str()); 
        //bonus for opponent pieces in victims Area
        if (influenceDom == player && 
            victimsArea & b->bitboard_[OPP(player)][0] & ~movable){
          tot[player] += vals_->trapPotVal;
          logDDebug("trap pot bonus %d to %d player for %s trap " , 
            vals_->trapPotVal, player, coordToStr(trap).c_str()); 
        }
      }

      //fighting traps
      if (guardsNum[player] > 0 && guardsNum[OPP(player) > 0]){
        //2 guards or locally unbeatable piece => safe
        //(guardDom == player cannot be used => absolutely strongest piece )
        if (guardsNum[player] >= 2 || 
            (guardDom != OPP(player) && influenceDom != OPP(player))){

          //dominance in trap ... >= 2 ? 
          if (guardsNum[player] >= guardsNum[OPP(player)] && guardDom == player){
            tot[player] += vals_->trapActiveVal;
            logDDebug("trap active %d to %d player for %s trap " , 
                      vals_->trapActiveVal, player, coordToStr(trap).c_str()); 

            //active, but trap is full
            if (bits::getBit(b->bitboard_[player][0], trap)) {
              tot[player] += vals_->activeTrapBlockedPenalty;
              logDDebug("active trap blocked penalty %d to %d player for %s trap " , 
                       vals_->activeTrapBlockedPenalty, player, coordToStr(trap).c_str()); 
            }
          }
          else{
            tot[player] += vals_->trapSafeVal;
            logDDebug("trap safe %d to %d player for %s trap " , 
                      vals_->trapSafeVal, player, coordToStr(trap).c_str()); 
          }
        }
      }


      //check frame
      if ((BIT_ON(trap) & b->bitboard_[player][0]) && guardsNum[OPP(player)] == 3) {
        //squares that must be blocked for trapped piece not to break free
        piece_t framedPiece = b->getPiece(trap, player); 
        u64 mustBlock = 
          bits::neighbors(guards[OPP(player)] & b->weaker(OPP(player), framedPiece));
        
        if (guardsAll == bits::neighborsOne(trap) && 
           ( mustBlock == 0ULL || (mustBlock & all) == mustBlock )){
          //frame penalty
          tot[player] += vals_->framePenaltyRatio *  vals_->pieceValue[framedPiece];
          logDDebug("full frame penalty %4.2f to %d player for %s framed" , 
                    vals_->framePenaltyRatio *  vals_->pieceValue[framedPiece],
                    player, Soldier(player, framedPiece, trap).toString().c_str()); 
          //pin penalty
          u64 u = guards[player];
          assert(u);
          int pinnedPos = bits::lix(u);
          piece_t pinnedPiece = b->getPiece(pinnedPos, player);
          assert(IS_PIECE(pinnedPiece));
          assert(!u);
          tot[player] += vals_->pinnedPenaltyRatio *  vals_->pieceValue[pinnedPiece];
              logDDebug("full pin penalty %4.2f to %d player for %s pinned" , 
                        vals_->pinnedPenaltyRatio *  vals_->pieceValue[pinnedPiece], 
                        player, Soldier(player, pinnedPiece, pinnedPos).toString().c_str()); 

          int sd = b->strongerDistance(player, pinnedPiece, pinnedPos);
          if (sd != BIT_EMPTY && sd <= 2) {
            tot[player] += vals_->framePenaltyRatio * vals_->pieceValue[framedPiece];
            logDDebug("extra pin penalty %4.2f to %d player for %s pinned in danger",
                  vals_->framePenaltyRatio *  vals_->pieceValue[framedPiece],
                  player, Soldier(player, pinnedPiece, pinnedPos).toString().c_str()); 
          }
        }
      }
    }
  }
      
  logDDebug("total for gold %d", int(tot[0] - tot[1]));
  //cerr << int(tot[0] - tot[1]) << " ";
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
    eval -= 0.2 * (STEPS_IN_MOVE - b->stepCount_);
    return eval;
  }

  assert(step.pieceMoved());
  
  //we don't like inverse steps 
  if (step.inversed(b->lastStep())){
    eval -= 0.5;
    return eval;
  }
  

  if (step.piece_ == ELEPHANT ) {
    eval += 0.1;
    //elephant the savior :)
    /*
    if (glob.grand()->get01() < 0.25){
      coord_t trap = TRAP_INDEX_TO_TRAP(glob.mostLosesTrapIndex[step.player_]);
      if (SQUARE_DISTANCE(trap, step.from_) < SQUARE_DISTANCE(trap, step.to_)){
        //cerr << trap << " " << step.toString() << endl;
        eval -= 0.15;
      }
    }
    */
  }
/*
  switch (step.piece_) { 
    case ELEPHANT :   eval += 0.15;
                      break;
    case CAMEL :   eval += 0.1;
                      break;
    case HORSE :   eval += 0.1;
                      break;
    default : break;
  }
  */

  if (step.isPushPull()){
    //push opponent to the goal :( not impossible ? )
    if (step.oppPiece_ == RABBIT && 
        BIT_ON(step.oppTo_) & bits::winRank[(step.player_)]) {
      eval -= 1;
    }
    //otherwise push/pulls are encouraged
    else{
      eval += 0.1; 
    }
  } 

  //check self-kill
  if (b->checkKillForward(step.from_, step.to_)){
    //leave buddy in opponent trap
    if (! IS_TRAP(step.to_) && step.piece_ == RABBIT && 
        ((step.player_ == GOLD && ROW(step.from_) >= 4) ||
        (step.player_ == SILVER && ROW(step.from_) <= 5))){
      eval -= 0.2;
    }
    else{

      eval -= 0.9;   
    }
  }

  //step into potentially dangerous trap
  /*
  if ( IS_TRAP(step.to_) && bits::bitCount(bits::neighborsOne(step.to_) & b->getBitboard()[step.player_][0]) <= 2){
    eval -= 0.5;
  }

  //push opp to trap is good 
  if (step.isPushPull() && IS_TRAP(step.oppTo_)){
    eval += 0.2;
  }
  */
  
  //check killing opponent
  if (step.isPushPull() && b->checkKillForward(step.oppFrom_, step.oppTo_)){
   // cerr << step.toString() << endl;
    eval += 0.7;   
  }

  //rabbit movements 
  
  gameStage_e gs = determineGameStage(b->getBitboard());
  if (step.piece_ == RABBIT){
    switch (gs) { 
      case GS_BEGIN: eval += -0.3;
                break;
      case GS_MIDDLE: eval += -0.1; 
                break;
      case GS_LATE: eval += 0.2; 
                break;
    }
  }

  //locality 
  if (cfg.localPlayout() && 
      b->lastStep().stepType_ != STEP_NULL){
    int d = SQUARE_DISTANCE(b->lastStep().to_, step.from_);
    eval += d <= 2 ? (2 - d) * 0.1 : 0;
  }

  return eval;
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

//implicit evaluation 
string implicit_eval_cfg   = " \
 \
 piece_values   0 100 250 300 800 1100 1800\
 \
 frozen_penalty_ratio   -0.1\
 \
 rabbit_penalty   -10000 -500 -400 -200 -150 -50 -20 0 0\
 \
 trap_sole_val   50\
 \
 trap_safe_val   50\
 \
 trap_active_val   150\
 \
 active_trap_blocked_penalty -40\
 \
 trap_pot_val   100\
 \
 frame_penalty_ratio   -0.3\
 \
 pinned_penalty_ratio   -0.2\
 \
 camel_hostage_penalty -250\
 \
 elephant_position  \
 \
     0    1    1    2   0    0    0    0\
     1    2    2    3   0    0    0    0\
     2    4   -4   10   0    0    0    0\
     3    8   12   12   0    0    0    0\
     3    8   10   10   0    0    0    0\
     2    4   -4   10   0    0    0    0\
     1    2    2    3   0    0    0    0\
     0    1    1    2   0    0    0    0\
 \
 camel_position  \
 \
    -2   -1    0    1   0    0    0    0\
    -1    0    1    2   0    0    0    0\
     0    1   -5    3   0    0    0    0\
     1    2    3    4   0    0    0    0\
     2    3    4    5   0    0    0    0\
     1    2    3    4   0    0    0    0\
     0    1    2    3   0    0    0    0\
    -1    0    1    2   0    0    0    0\
 \
 horse_position  \
 \
    -2   -1    0    1   0    0    0    0\
    -1    0    1    2   0    0    0    0\
     0    1   -5    3   0    0    0    0\
     1    2    3    4   0    0    0    0\
     2    3    4    5   0    0    0    0\
     1    2    3    4   0    0    0    0\
     0    1    2    3   0    0    0    0\
    -1    0    1    2   0    0    0    0\
 \
 dog_position  \
 \
    -4   -6   -6   -6   0    0    0    0\
    -7   -8  -10   -9   0    0    0    0\
    -9  -10  -12  -11   0    0    0    0\
    -6   -8   -9   -9   0    0    0    0\
    -3   -5   -6   -6   0    0    0    0\
    -1   -1   -2   -3   0    0    0    0\
     1    2    4    2   0    0    0    0\
     0    1    1    1   0    0    0    0\
 \
 cat_position  \
 \
    -4   -6   -6   -6   0    0    0    0\
    -7   -8  -10   -9   0    0    0    0\
    -9  -10  -12  -11   0    0    0    0\
    -6   -8   -9   -9   0    0    0    0\
    -3   -5   -6   -6   0    0    0    0\
    -1   -1   -2   -3   0    0    0    0\
     1    2    4    2   0    0    0    0\
     0    1    1    1   0    0    0    0\
 \
 rabbit_position  \
 \
     0    0    0    0   0    0    0    0\
   -14  -16   -8  -16   0    0    0    0\
   -16  -10  -16  -10   0    0    0    0\
    -8  -12  -12  -12   0    0    0    0\
    -5   -8   -8   -8   0    0    0    0\
    -3   -5   -6   -6   0    0    0    0\
    -1   -2   -1   -4   0    0    0    0\
     0    0    0    0   0    0    0    0\
 \
 rabbit_late_position  \
 \
     0    0    0    0   0    0    0    0\
    25   25   25   25   0    0    0    0\
    20   20   20   20   0    0    0    0\
    16   15   14   14   0    0    0    0\
     8    7    6    6   0    0    0    0\
     4    3    2    2   0    0    0    0\
     2    1    1    1   0    0    0    0\
     0    0    0    0   0    0    0    0\
 ";
