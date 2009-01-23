
/** 
 *  @file board.cpp
 *  @brief Board implementation. 
 *  @full Integer board, able to evaluate itself, compute it's hash, etc. 
 */

#include "board.h"

// zobrist base table for signature creating 
//u64  Board::zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     

//thirdRepetition class
ThirdRep*  Board::thirdRep_;

// switch to know when to init static variables in class Board
bool Board::classInit = false;

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int rabbitForward[2]={NORTH,SOUTH};
const int rabbitWinRow[2]={8,1};
const int trap[4]={33,36,63,66};

/*
extern const u64 not_h_file(string("0111111101111111011111110111111101111111011111110111111101111111"));
extern const u64 not_a_file(string("1111111011111110111111101111111011111110111111101111111011111110"));
extern const u64 not_1_rank(string("0000000011111111111111111111111111111111111111111111111111111111"));
extern const u64 not_8_rank(string("1111111111111111111111111111111111111111111111111111111100000000"));
*/



//---------------------------------------------------------------------
//  section PieceArray
//---------------------------------------------------------------------

PieceArray::PieceArray()
{
  len = 0;
}

//---------------------------------------------------------------------

void PieceArray::add(square_t elem)
{
  elems[len++] = elem;
  assert(MAX_PIECES >= len);
}

//---------------------------------------------------------------------

void PieceArray::del(square_t elem)
{

  for (uint i = 0; i < len;i++)
    if (elems[i] == elem){
      elems[i] = elems[--len];
      return;
    }
  assert(false);
}

//---------------------------------------------------------------------
 
void PieceArray::clear() 
{
  len = 0; 
}

//---------------------------------------------------------------------

string PieceArray::toString() const
{
  stringstream ss;
  ss.clear();

  for (uint i = 0; i < len; i++)
    ss << elems[i] << " ";  
  
  ss << endl;
  return ss.str();
}

//--------------------------------------------------------------------- 

 
uint PieceArray::getLen() const
{
  return len;
}

//---------------------------------------------------------------------
 
square_t PieceArray::operator[](uint index) const
{
  assert( index >= 0 && index < len );
  return elems[index];
}

//--------------------------------------------------------------------- 

square_t PieceArray::getRandom() const
{
  assert(len);
  return elems[rand() % len];
}

//---------------------------------------------------------------------
//  section Step
//---------------------------------------------------------------------

Step::Step( )
{
  stepType_ = STEP_NULL;
}

//---------------------------------------------------------------------
/* this constructor is mainly used for 
 * STEP_NULL or step_pass which don't use other values than stepType */
Step::Step( stepType_t stepType, player_t player )
{
  stepType_ = stepType;
  player_   = player;
}

//---------------------------------------------------------------------


Step::Step( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to){
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}

//---------------------------------------------------------------------

Step::Step( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to, 
            piece_t oppPiece, square_t oppFrom, square_t oppTo)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
  oppPiece_ = oppPiece;
  oppFrom_  = oppFrom;
  oppTo_    = oppTo;
}

//---------------------------------------------------------------------

void Step::setValues( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}

//---------------------------------------------------------------------

void Step::setValues( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to, 
            piece_t oppPiece, square_t oppFrom, square_t oppTo)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
  oppPiece_ = oppPiece;
  oppFrom_  = oppFrom;
  oppTo_    = oppTo;

}

//---------------------------------------------------------------------

player_t Step::getPlayer() const 
{
  return player_;
}

//---------------------------------------------------------------------

bool Step::isPass() const
{
  return stepType_ == STEP_PASS;
}

//---------------------------------------------------------------------

bool Step::isSingleStep() const
{
  return (stepType_ == STEP_SINGLE);
}

//---------------------------------------------------------------------

bool Step::isPushPull() const
{
  return (stepType_ == STEP_PUSH || stepType_ == STEP_PULL);
}

//---------------------------------------------------------------------

int Step::count() const
{
  switch (stepType_){
    case STEP_SINGLE: return 1;
    case STEP_PUSH: return 2;
    case STEP_PULL: return 2;
    default : return 0;
  }

}

//---------------------------------------------------------------------

Step Step::toOld() const
{
  Step s = (*this);
  s.from_ = INDEX_64_TO_SQUARE(s.from_);
  s.to_ = INDEX_64_TO_SQUARE(s.to_);
  s.oppTo_ = INDEX_64_TO_SQUARE(s.oppTo_);
  s.oppFrom_ = INDEX_64_TO_SQUARE(s.oppFrom_);
  s.player_ = s.player_ == BGOLD ? GOLD : SILVER;
  return s;
}

//---------------------------------------------------------------------

bool Step::inversed(const Step& s) const 
{
  if (from_ == s.to_ && to_ == s.from_ && 
      ( (stepType_ == s.stepType_ && stepType_ == STEP_SINGLE) || 
        (isPushPull() && s.isPushPull() && 
        oppFrom_ == s.oppTo_ && oppTo_ == s.oppFrom_ ))){
    assert(((stepType_ == s.stepType_) == STEP_SINGLE) || 
          (isPushPull() && s.isPushPull()));
    assert(pieceMoved() && s.pieceMoved());
    assert(player_ == s.player_ && piece_ == s.piece_);
    return true;
  }
  return false;
}


//---------------------------------------------------------------------

/* returns true if any piece was moved
 * i.e. returns false if pass or no_step */
bool Step::pieceMoved() const 
{
  return (stepType_ == STEP_SINGLE || stepType_ == STEP_PUSH || stepType_ == STEP_PULL);
}

//---------------------------------------------------------------------

bool Step::operator== ( const Step& other) const
{
  if  ( stepType_ == other.stepType_ ){ // necessary condition ! 
    if ( stepType_ == STEP_PASS || stepType_ == STEP_NULL ) 
      return true;
    if ( other.player_ == player_ && other.piece_ == piece_ && other.from_ == from_ && other.to_ == to_ ) {
      if ( stepType_ == STEP_SINGLE ) 
        return true;
      else
        if ( other.oppPiece_ == oppPiece_ && other.oppFrom_ == oppFrom_ && other.oppTo_ == oppTo_ )
          return true;
    }
  }

  return false;
}


//---------------------------------------------------------------------

bool Step::operator< ( const Step& other) const
{
  //TODO optimize - add laziness ! 
  int itemsNum = 8;
  int items[] = {stepType_ - other.stepType_, from_ - other.from_, to_ - other.to_, 
            piece_ - other.piece_, player_ - other.player_, 
            oppFrom_ - other.oppFrom_, oppTo_ - other.oppTo_, 
            oppPiece_ - other.oppPiece_};
  for (int i = 0; i < itemsNum; i++){
    if (items[i] < 0 ) {
     return true;
    }
    if (items[i] > 0 ) {
     return false;
    }
  }
  return false; //equality
}

//---------------------------------------------------------------------

string Step::toString() const
{
  stringstream ss;
  ss.str(""); 

  //dirty
  if (player_ == 0 || player_ == 1 ){
    return toOld().toString();
  }

  switch (stepType_) {
    case STEP_PASS: 
      break;
    case STEP_SINGLE: 
      ss << oneSteptoString(player_, piece_, from_, to_); 
      break;
    case STEP_PUSH: 
      ss << oneSteptoString(OPP(player_), oppPiece_, oppFrom_, oppTo_)
             << oneSteptoString(player_, piece_, from_, to_ );
      break;
    case STEP_PULL: 
      ss << oneSteptoString(player_, piece_, from_, to_ )
             << oneSteptoString(OPP(player_), oppPiece_, oppFrom_, oppTo_);
      break;
    case STEP_NULL:
      ss << "NULL";  
      break;
    default:
      assert(false);
      break;

  }

  return ss.str();
}

//---------------------------------------------------------------------

const string Step::oneSteptoString(player_t player, piece_t piece, square_t from, square_t to) const
{
  stringstream s;
  s.str("");
  string pieceRefStr(" RCDHMErcdhme");
  string columnRefStr("abcdefgh");

  //dirty
  //if (player == 0 || player == 1 ) 
    //s << pieceRefStr[piece + 6 * player] << columnRefStr[from % 8] << from / 8 + 1; 
  //else
    s << pieceRefStr[piece + 6 * PLAYER_TO_INDEX(player)] << columnRefStr[from % 10 - 1] << from / 10; 
  switch (to - from)
  {
    case NORTH : s << "n"; break;
    case WEST :  s << "w"; break;
    case EAST :  s << "e"; break;
    case SOUTH : s << "s"; break;

    //case BNORTH : s << "n"; break;
    //case BWEST :  s << "w"; break;
    //case BEAST :  s << "e"; break;
    //case BSOUTH : s << "s"; break;
    default :
      assert(false);
      break;
  }
  s << " ";

  return s.str();
}

//---------------------------------------------------------------------

void Step::dump()
{
  logRaw(toString().c_str());
}

//--------------------------------------------------------------------- 
//  section KillInfo
//--------------------------------------------------------------------- 

KillInfo::KillInfo()
{
  active_ = false;
}

//--------------------------------------------------------------------- 

KillInfo::KillInfo( player_t player, piece_t piece, square_t square):
  player_ (player), piece_ (piece), square_(square)
{
  active_ = false;
}

//--------------------------------------------------------------------- 

void KillInfo::setValues( player_t player, piece_t piece, square_t square)
{
  active_ = true;
  player_ = player;
  piece_  = piece;
  square_ = square;
}

//--------------------------------------------------------------------- 

const string KillInfo::toString() const
{
  if (! active_ ) 
    return "";
  stringstream s;
  string pieceRefStr(" RCDHMErcdhme");
  string columnRefStr("abcdefgh");

  s << pieceRefStr[piece_ + 6 * PLAYER_TO_INDEX(player_)] << columnRefStr[square_ % 10 - 1] << square_ / 10; 
  s << "x ";
  return s.str();
}

//---------------------------------------------------------------------
//  section StepWithKills
//---------------------------------------------------------------------

StepWithKills::StepWithKills()
{
  assert(false);
}

//--------------------------------------------------------------------- 

StepWithKills::StepWithKills(Step step, const Board* board):
  Step(step)
{
  addKills(board); 
}

//--------------------------------------------------------------------- 

void StepWithKills::addKills(const Board* board)
{
  switch (stepType_) {
    case STEP_SINGLE:
      board->checkKillForward(from_, to_, &kills[0]);
      break;
    case STEP_PUSH:
      board->checkKillForward(oppFrom_, oppTo_, &kills[0]);
      board->checkKillForward(from_, to_, &kills[1]);
      break;
    case STEP_PULL:
      board->checkKillForward(from_, to_, &kills[0]);
      board->checkKillForward(oppFrom_, oppTo_, &kills[1]);
      break;
  }
  
}

//--------------------------------------------------------------------- 

string StepWithKills::toString() const
{
  stringstream ss;
  stringstream ssOut;
  ss << Step::toString();

  string firstPart, secondPart;
  ss >> firstPart; 
  ss >> secondPart;

  ssOut << firstPart << " " << kills[0].toString() << secondPart << " " << kills[1].toString();
  return ssOut.str();
}

//---------------------------------------------------------------------
//  section Move
//---------------------------------------------------------------------

void Move::appendStep(Step step)
{
  stepList_.push_back(Step(step));
}

//--------------------------------------------------------------------- 

void Move::prependStep(Step step)
{
  stepList_.push_front(Step(step));
}

//--------------------------------------------------------------------- 

void Move::appendStepList(StepList stepList)
{
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    stepList_.push_back(*it);
  }
}

//--------------------------------------------------------------------- 

StepList Move::getStepList() const
{
  return stepList_;
}

//--------------------------------------------------------------------- 

int Move::getStepCount() const
{
  return stepList_.size();
}

//--------------------------------------------------------------------- 

string Move::toString()
{
  string s;
  for (StepListIter it = stepList_.begin(); it != stepList_.end(); it++){
    s = s + (*it).toString();
  }
  return s;
}

//--------------------------------------------------------------------- 

string Move::toStringWithKills(const Board* board)
{
  //TODO a bit dirty ... must update board after each step ...
  Board * playBoard = new Board(*board);
  string s;
  for (StepListIter it = stepList_.begin(); it != stepList_.end(); it++){
    s = s + StepWithKills((*it), playBoard).toString();
    playBoard->makeStepTryCommitMove(*it);
  }
  delete playBoard;
  return s;
}

//---------------------------------------------------------------------
//  section BBoard
//---------------------------------------------------------------------


const int bdirection[4]={BNORTH,BEAST,BSOUTH,BWEST};
u64   zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     

u64   bits::stepOffset_[2][7][64]; 
u64   bits::winRank[2] = { 0xff00000000000000ULL ,0x00000000000000ffULL};

int bits::lix(u64& b){

  /*if (!b) {
    return -1;
  }
  */

   static const char LogTable256[] = 
  {
   -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
  };

  unsigned r;     
  register u64 t, tt, ttt; // temporaries

  if ((ttt = b >> 32)){
    if ((tt = ttt >> 16))
    {
      r = (t = tt >> 8) ? 56 + LogTable256[t] : 48 + LogTable256[tt];
    }
    else 
    {
      r = (t = ttt >> 8) ? 40 + LogTable256[t] : 32 + LogTable256[ttt];
    }
  }else{
    if ((tt = b >> 16))
    {
      r = (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
    }
    else 
    {
      r = (t = b >> 8) ? 8 + LogTable256[t] : LogTable256[b];
    }
  }

  //TODO what happens for BIT_ON(-1) ? :(
  b ^= BIT_ON(r);
  return r;

}

//---------------------------------------------------------------------

int bits::lixslow(u64& b){

  if (!b) {
    return -1;
  }

  u64 v = b; 
  const u64 lix_mask[] = 
      {0x2ULL, 0xCULL, 0xF0ULL, 0xFF00ULL, 0xFFFF0000ULL, 0xFFFFFFFF00000000ULL};
  const unsigned int mask_ones[] = {1, 2, 4, 8, 16, 32};
  int i;

  register unsigned int r = 0; 
  for (i = 5; i >= 0; i--) 
  {
      if (v & lix_mask[i])
      {
        v >>= mask_ones[i];
        r |= mask_ones[i];
      } 
  }

  b ^= BIT_ON(r);
  return r;
}

//--------------------------------------------------------------------- 

u64 bits::neighbors( u64 target )
{
  u64 x;

  x =  (target & NOT_H_FILE) << 1;
  x |= (target & NOT_A_FILE) >> 1;
  x |= (target /*& NOT_1_RANK*/) << 8;
  x |= (target /*& NOT_8_RANK*/) >> 8;
  //TODO ..... not_*_rank not neccessary ? 

  return(x);
}

//---------------------------------------------------------------------

u64 bits::str2bits(string str){
  bitset<BIT_LEN> bs(str);
  u64 b = 0ULL;
  for (int i = 0; i < BIT_LEN; i++){
    if (bs[i]){
      b |= BIT_ON(i);
    }
  }

  return b;
}


//---------------------------------------------------------------------

u64 bits::sphere(int center, int radius)
{
  u64 b = BIT_ON(center);
  for (int i = 0; i < radius; i++){
    b |= neighbors(b);
  }
  return b;
}

//---------------------------------------------------------------------

u64 bits::circle(int center, int radius)
{
  return radius == 0 ? 
         BIT_ON(center) : 
         sphere(center, radius) ^ sphere(center, radius - 1);
}

//---------------------------------------------------------------------

bool bits::getBit(const u64& b, int n){
  return b & BIT_ON(n); 
}

//---------------------------------------------------------------------

void bits::buildStepOffsets()
{
  bplayer_t  player;
  bpiece_t  piece;
  bcoord_t  square;

  // do rabbits as a special case
  for (player = 0; player < 2; player++)
    for (square = 0; square < BIT_LEN; square++) {
      u64  ts = 0ULL;
      
      ts |= ((BIT_ON(square)) & NOT_H_FILE) << 1;  // east
      ts |= ((BIT_ON(square)) & NOT_A_FILE) >> 1;  // west
      if (player == 0)    
        ts |= ((BIT_ON(square)) & NOT_1_RANK) << 8;  // north
      else
        ts |= ((BIT_ON(square)) & NOT_8_RANK) >> 8;  // south

      stepOffset_[player][BRABBIT][square] = ts;    
    }

  // Now do the rest
  for (player = 0; player < 2; player++)
    for (piece = 2; piece < 7; piece++)
      for (square = 0; square < BIT_LEN; square++) {
        u64  ts = bits::neighbors(BIT_ON(square));
        stepOffset_[player][piece][square] = ts;
      }
 }

//---------------------------------------------------------------------

ostream& bits::print(ostream& o, const u64& b){ 
  for (int i = 7; i >= 0; i--){
    for (int j = 0; j < 8; j++){
      o << bits::getBit(b, i*8 + j) << " ";
    }
    o << endl;
  }
  o << endl;
  return o; 
}

//--------------------------------------------------------------------- 

BBoard::BBoard(const Board& b)
{
  /* clear board */
  for (int i=0; i<7; i++) {
    bitboard_[0][i] = 0;;
    bitboard_[1][i] = 0;
  }

  stepCount_ = b.stepCount_;
  toMove_ = b.toMoveIndex_;
  winner_ = BNO_PLAYER;
  signature_ = 0;

  const board_t & board = b.board_;

  for (int square = 11; square < 89; square++){
    if (! IS_PLAYER(board[square])){
      continue;
    }
    bcoord_t bcoord = SQUARE_TO_INDEX_64(square);
    bplayer_t bplayer = OWNER(board[square]) == GOLD ? BGOLD : BSILVER; 
    bpiece_t bpiece = 0;
    
    switch PIECE(board[square]){
      case PIECE_ELEPHANT : bpiece = BELEPHANT;  break;
      case PIECE_CAMEL : bpiece = BCAMEL;  break;
      case PIECE_HORSE : bpiece = BHORSE;  break;
      case PIECE_DOG : bpiece = BDOG;  break;
      case PIECE_CAT : bpiece = BCAT;  break;
      case PIECE_RABBIT : bpiece = BRABBIT;  break;
      default : assert(false); break;
    }

    setSquare(bcoord, bplayer, bpiece);
  }

  bits::buildStepOffsets();

}

//--------------------------------------------------------------------- 

Step BBoard::findMCstep() 
{
  Step step;

  //it's not possible to have 0 rabbits in the beginning of move
  assert( bitboard_[toMove_][1] || stepCount_ > 0); 

  //no piece for player to move 
  if (! bitboard_[toMove_][0]) {
    //step_pass since the player with no pieces still might win 
    //if he managed to kill opponent's last rabbit before he lost his last piece
    return Step(STEP_PASS, toMove_);          
  }
  
  int len = genSteps(toMove_, stepArray);
  assert(len < MAX_STEPS);

  //player to move has no step to play - not even pass
  if (len == 0 ){ 
    winner_ = BOPP(toMove_);
    return Step(STEP_NULL,toMove_); 
  }

  assert(len > 0);
  int index = rand() % len;
  assert( index >= 0 && index < len );

  return( stepArray[index]); 
} 

//--------------------------------------------------------------------- 

bool BBoard::makeStepTryCommit(Step& step)
{
  makeStep(step);
	if (stepCount_ >= 4 || ! step.pieceMoved()) {
    updateWinner();
    commit();
    return true;
  }
  return false;

}

//--------------------------------------------------------------------- 

void BBoard::commit()
{
  assert(toMove_ == BGOLD || toMove_ == BSILVER);
  /*if (toMove_ == BSILVER) {
    moveCount_++;
  }
  */
  toMove_ = BOPP(toMove_);
  stepCount_ = 0;

  //preMoveSignature_ = signature_;
}

//--------------------------------------------------------------------- 

void BBoard::makeStep(Step& step){

    if (step.stepType_ == STEP_NULL){
      return;
    }

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

    //handle push/pull steps
    if (step.isPushPull()) {  
      assert( stepCount_ < 3 ); 
      delSquare(step.oppFrom_, BOPP(step.player_), step.oppPiece_);
      setSquare(step.oppTo_, BOPP(step.player_), step.oppPiece_);
      stepCount_++;
    }

    //update board
    delSquare(step.from_, step.player_, step.piece_);
    setSquare(step.to_, step.player_, step.piece_);
    stepCount_++;

    u64 fullTraps;
    u64 dieHard; 

    //traps stuff 
    for (int player = 0; player < 2; player++){
      fullTraps = (TRAPS & bitboard_[player][0]);
      dieHard = fullTraps ^ (fullTraps & bits::neighbors(bitboard_[player][0]));
      //full traps gold now contains to-be removed pieces 

      if (dieHard){
        int trap = bits::lix(dieHard);             
        if ( trap != -1){
          delSquare(trap, player);
          //no more than one dead per player
          assert(bits::lix(dieHard) == -1); 
        }
      }
    } 

}

//--------------------------------------------------------------------- 

void BBoard::updateWinner()
{
  //check goal
  if (bitboard_[toMove_][1] & bits::winRank[toMove_]) {
    winner_ = toMove_;
    return;
  }
  //check self goal
  if (bitboard_[BOPP(toMove_)][1] & bits::winRank[BOPP(toMove_)]){
    winner_ = BOPP(toMove_);
    return;
  }
  //opp has no rabbit 
  if (! bitboard_[BOPP(toMove_)][1]){
    winner_ = toMove_;
    return;
  }
  //player has no rabbit 
  if (! bitboard_[toMove_][1]){
    winner_ = BOPP(toMove_);
    return;
  }
}

//--------------------------------------------------------------------- 

int BBoard::genSteps(bplayer_t player, StepArray& steps) const
{
  int stepsNum = 0;
  u64 movable = calcMovable(player);

  //small opt ... prefill victims for piece types
  u64 victims[7];
  calcWeaker(player, victims);

  bcoord_t coord = BIT_LEN;
  while ( (coord = bits::lix(movable)) != -1) { 
    assert(getPlayer(coord) == player); 
    piece_t piece = getPiece(coord, player);
    genStepsForOneTuned(coord, player, piece, steps, stepsNum, victims[piece]);
  }

  if ( stepCount_ >= 1 ){
    steps[stepsNum++] = Step(STEP_PASS, player);
  }

  return stepsNum;
}
   
//--------------------------------------------------------------------- 

bool BBoard::goalCheck(player_t player, int stepLimit, Move * move) 
{
  
  u64 rabbits = bitboard_[player][BRABBIT];
  int from;
  while ( (from = bits::lix(rabbits)) != -1){
    
    u64 goals = bits::winRank[player] & bits::sphere(from, stepLimit);
    int to;
    while ( (to = bits::lix(goals)) != -1){
      //cerr << "rabbit run" << from << "->" << to;
      //cerr << "goal " << BSQUARE_DISTANCE(from, to) << "far away" << endl;
      //cerr << "=================" << endl;
      if (reachability(from, to, player, stepLimit, 0, move) != -1){
        return true;
      }
    } 
    
  }
  return false;
}

//---------------------------------------------------------------------

int BBoard::reachability(int from, int to, bplayer_t player, int limit, int used, Move * move) 
{

  int reserve = limit - used - BSQUARE_DISTANCE(from, to);
  //distance limit for pieces lookup
  //TODO optimize ! to + 1 and "referer from"
  int distLimit = reserve + 3; 

  if (from == to){
    return used;
  }

  int pseudoReserve = reserve;
  //someone is blocking the to field 
  if (getPlayer(to) != BNO_PLAYER){
    //might be friend
    pseudoReserve -= 1;
    //or opponent
    if (getPlayer(to) == BOPP(player)){
      //generalize not only for rabbits
      pseudoReserve -= 1;
    }
  }

  if (pseudoReserve < 0 || getPlayer(from) == BNO_PLAYER){
    //cerr << "PSEUDORESERVE/DEAD CUTOFF" << endl;
    return -1;
  }

  //cerr.width(2 * (used+1));
  //cerr << ' ';
  //cerr << "used " << used << " reserve " << reserve << endl;

  for (int i = 0; i <= distLimit; i ++){
    u64 iFarAway = bitboard_[player][0] & bits::circle(from, i);
    //unguarded trap in neighborhood -> increase the distLimit 
    //TODO distinguish piece
   // u64 trapHelpers = bits::stepOffset_[player][BRABBIT][from] & bits::TRAPS 
                      

    int bit;
    while ( (bit = bits::lix(iFarAway)) != -1){
      //cerr << "piece at " << bit << " is " << i << " far away" << endl;
      assert(BIS_PLAYER(getPlayer(bit)));
      assert(player == BGOLD || player == BSILVER);
      stepArrayLen = genStepsForOne(bit, player, stepArray);

      for (int j = 0; j < stepArrayLen; j++){
       // cerr.width(2 * (used+1));
       // cerr << ' ';
       // cerr << "trying " << stepArray[j].toString() << endl;

        int newfrom = from;
        if (stepArray[j].from_ == from){
          newfrom = stepArray[j].to_;
        }
        //moving another piece => must have reserve
        if (from == newfrom && ! reserve){
           // cerr << "RESERVE CUTOFF" << endl;
            continue;
        }
        //new board - makestep - recurse 
        BBoard* bb = new BBoard(*this);
        bb->makeStep(stepArray[j]);
        int r = bb->reachability(newfrom, to, player, limit, 
                                  used + stepArray[j].count(), move);
        delete(bb);
        if (r != -1){
          //assert(stepArray[j].player_ == 0 || stepArray[j].player_ == 1);
          if (move != NULL){
            move->prependStep(stepArray[j]);
          }
          return r;
        }
      }
    }
  }
  return -1;

}

//--------------------------------------------------------------------- 

int BBoard::genStepsForOne(bcoord_t coord, bplayer_t player,
                                          StepArray& steps) const
{
  int stepsNum = 0;
  if (bits::getBit(calcMovable(player), coord)){
    u64 victims[7];
    calcWeaker(player, victims);

    assert(getPlayer(coord) == player); 
    piece_t piece = getPiece(coord, player);
    genStepsForOneTuned(coord, player, piece, steps, stepsNum, victims[piece]);
  }
  return stepsNum;
}

//--------------------------------------------------------------------- 

void BBoard::genStepsForOneTuned(bcoord_t from, bplayer_t player, bpiece_t piece,
                                StepArray& steps, int & stepsNum, u64 victims) const
{
  assert(getPlayer(from) == player);
  assert(getPiece(from, player)  == piece);

  u64 empty = ~(bitboard_[0][0] | bitboard_[1][0]);
  u64 whereStep = empty & bits::stepOffset_[player][piece][from];
    
  //single steps
/*
  bcoord_t to = BIT_LEN;
  while ((to = bits::lix(whereStep)) != -1) {
      steps[stepsNum++].setValues(STEP_SINGLE, player, piece, from, to);
  }
*/
  for (int i = 0; i < 4; i++) {
    bcoord_t to = from + bdirection[i];
    if (whereStep & BIT_ON(to)) {
      steps[stepsNum++].setValues(STEP_SINGLE, player, piece, from, to);
    }
  }

  //push/pull steps
  if ( piece == BRABBIT || stepCount_ >= 3 || 
       ! (bits::stepOffset_[player][piece][from] & bitboard_[BOPP(player)][0])) { 
    return;
  }

  victims &= bits::stepOffset_[player][piece][from];
   
  /*
  bcoord_t victimFrom = BIT_LEN;
  while ((victimFrom = bits::lix(victims)) != -1) {
  */
  for (int i = 0; i < 4; i++) {
    bcoord_t victimFrom = from + bdirection[i];
    if (! (victims & BIT_ON(victimFrom))) {
      continue;
    }

    //pull
    u64 wherePull = empty & bits::stepOffset_[player][piece][from];          
    /*
    bcoord_t pullerTo = BIT_LEN; 
    while ((pullerTo = bits::lix(wherePull)) != -1) {
    */
    for (int j = 0; j < 4; j++) {
      bcoord_t pullerTo = from + bdirection[j];
      if (! (wherePull & BIT_ON(pullerTo))) {
        continue;
      }

      steps[stepsNum++].setValues(STEP_PULL, player, piece, from, pullerTo, 
                            getPiece(victimFrom, BOPP(player)), victimFrom, from); 
    }

    //push
    u64 wherePush = empty & bits::stepOffset_[player][piece][victimFrom]; 
    /*
    bcoord_t victimTo = BIT_LEN; 
    while ((victimTo = bits::lix(wherePush)) != -1) {
    */
    for (int k = 0; k < 4; k++) {
      bcoord_t victimTo = victimFrom + bdirection[k];
      if (! (wherePush & BIT_ON(victimTo))) {
        continue;
      }

      steps[stepsNum++].setValues(STEP_PUSH, player, piece, from, victimFrom,
                        getPiece(victimFrom, BOPP(player)), victimFrom, victimTo);
    }
  } 
}

//--------------------------------------------------------------------- 

string BBoard::toString() const
{

	stringstream ss;

  ss << endl;
  ss << "Signature " << signature_ << endl;
  ss << "Move ";

  if (toMove_ == BGOLD) 
    ss << "g" << endl;
  else
    ss << "s" << endl;

  assert(toMove_ == BGOLD || toMove_ == BSILVER );

  ss << " +-----------------+\n";
    
  bcoord_t coord;
  string refStr(".123456rcdhmeRCDHME");
  for (int i = BSIDE_SIZE - 1; i >= 0; i--) {
    ss << i + 1 <<"| ";
    for (int j = 0; j <  BSIDE_SIZE; j++) {
      coord = i * BSIDE_SIZE + j;
      assert(getPlayer(coord) == BNO_PLAYER || 
             getPiece(coord,getPlayer(coord)) + ((2 - getPlayer(coord)) * 6) 
             <= int(refStr.length()));
			if (getPlayer(coord) == BNO_PLAYER ){
			  if (bits::getBit(TRAPS, coord)){
				  ss << "X ";
        }
        else{
				  ss << ". ";
        }
      }
			else{
				ss << refStr[getPiece(coord, getPlayer(coord)) + 
                              ((2 - getPlayer(coord)) * 6)] << " ";
      }
    }
    ss << "| " << endl;
  }

  ss << " +-----------------+" << endl;
  ss << "   a b c d e f g h" << endl;

	return ss.str();
} 

//--------------------------------------------------------------------- 

u64 BBoard::getSignature() const 
{
  return signature_;
}

//--------------------------------------------------------------------- 

bplayer_t BBoard::getWinner() const 
{
  return winner_;
}

//--------------------------------------------------------------------- 

bplayer_t BBoard::gameOver() const 
{
  return winner_ != BNO_PLAYER;
}

//--------------------------------------------------------------------- 

bplayer_t BBoard::getPlayerToMove() const
{
  return toMove_;
}

//--------------------------------------------------------------------- 

u64 BBoard::calcMovable(bplayer_t player) const
{
  u64 ngb[2]; 
  ngb[0] = bits::neighbors(bitboard_[0][0]);
  ngb[1] = bits::neighbors(bitboard_[1][0]);
  u64 stronger = bitboard_[BOPP(player)][0];
  u64 movable = 0ULL; 
  
  for (int piece = 1; piece < 7; piece++) {
    movable |= bitboard_[player][piece];                           
    stronger ^= bitboard_[BOPP(player)][piece];                              
    movable &= ngb[player] | (~bits::neighbors(stronger)); 
  }
  return movable;
}

//--------------------------------------------------------------------- 

void BBoard::calcWeaker(bplayer_t player, u64 (&weaker)[7]) const
{
  weaker[1] = 0ULL;
  for (int i = 2; i < 7; i++){
    weaker[i] = weaker[i-1] | bitboard_[BOPP(player)][i-1];
  }
}

//--------------------------------------------------------------------- 

void BBoard::setSquare(bcoord_t coord, player_t player, piece_t piece) 
{
  assert(player == BGOLD || player == BSILVER);
  assert(piece >= BRABBIT && piece <= BELEPHANT );
  assert(coord >= 0 && coord < BIT_LEN );

  bitboard_[player][piece] |= BIT_ON(coord);
  bitboard_[player][0] |= BIT_ON(coord);

  signature_ ^= zobrist[player][piece][INDEX_64_TO_SQUARE(coord)]; 
}

void BBoard::delSquare(bcoord_t coord, player_t player)
{
  assert(bits::getBit(bitboard_[player][0], coord));
  bitboard_[player][0] ^= BIT_ON(coord);
  for (int i = 1; i < 7; i++ ){
    if (bits::getBit(bitboard_[player][i], coord)){
      bitboard_[player][i] ^= BIT_ON(coord);
      signature_ ^=  zobrist[player][i][INDEX_64_TO_SQUARE(coord)]; 
      return;
    }
  }
}

void BBoard::delSquare(bcoord_t coord, player_t player, piece_t piece) 
{
  assert(bits::getBit(bitboard_[player][0],coord));
  assert(bits::getBit(bitboard_[player][piece],coord));
  bitboard_[player][0] ^= BIT_ON(coord);
  bitboard_[player][piece] ^= BIT_ON(coord);
  signature_ ^=  zobrist[player][piece][INDEX_64_TO_SQUARE(coord)]; 
}


bpiece_t BBoard::getPiece(bcoord_t coord, bplayer_t player) const
{
  assert(bits::getBit(bitboard_[player][0], coord));
  for (int i = 1; i < 7; i++ ){
    if (bits::getBit(bitboard_[player][i],coord)){
      return i;
    }
  } 
  assert(false);

  return BEMPTY;
}


bplayer_t BBoard::getPlayer(bcoord_t coord) const
{
  if (bits::getBit(bitboard_[BGOLD][0], coord))
    return BGOLD;

  if (bits::getBit(bitboard_[BSILVER][0], coord))
    return BSILVER;

  return BNO_PLAYER;
}


uint BBoard::getStepCount() 
{
  return stepCount_;
}

//---------------------------------------------------------------------
//  section Board
//---------------------------------------------------------------------
//

Board::Board()
{
}

//--------------------------------------------------------------------- 

void Board::initNewGame()
{
  init(true);
}

//--------------------------------------------------------------------- 

bool Board::initFromRecord(const char* fn)
{

  init();
  
  fstream f;
  string line;
  stringstream ss;
  string token;
  uint moveCount;

  try { 
    f.open(fn, fstream::in);
    if (! f.good()){
      f.close();
      return false;
    }

    
    while (f.good()){
      
      getline(f, line);
      ss.clear();
      ss.str(line);

      ss >> token; 
      moveCount = str2int(token.substr(0, token.length() - 1));
      assert(moveCount == moveCount_);
      makeMove(getStreamRest(ss));
    }

  }
  catch(int e) {
    return false;     //initialization from file failed
  }

  return true;

} 

//--------------------------------------------------------------------- 

bool Board::operator== ( const Board& board) const
{
  return (signature_ == board.signature_ && moveCount_ == board.moveCount_ ) ;
}

//---------------------------------------------------------------------

bool Board::initFromPosition(const char* fn)
{
  fstream f;

  f.open(fn,fstream::in);

  if (! f.good()){
    f.close();
     return false;
   }

  bool result = initFromPositionStream(f);
  f.close();

  return result;
}

//---------------------------------------------------------------------

bool Board::initFromPositionCompactString(const string& s)
{
  stringstream ss;
  ss.str(s);
  init();
  char side;
  ss >> side;

  toMove_ = sideCharToPlayer(side);
  if (! IS_PLAYER(toMove_))
    return false;
  string boardStr = getStreamRest(ss);
  
  //check format '[' .. 64 characters for position ... ']'
  if (boardStr[0] != '[' || boardStr[65] != ']') 
    return false;

  uint k=1;
  for (int i = 8; i > 0; i--) 
    for (int j = 1; j < 9; j++) {
      assert(k < boardStr.length());
      char c = boardStr[k++];
      if (c == ' ' || c=='X' || c=='x')
        board_[i*10+j] = (EMPTY_SQUARE); 
      else {
        try{
          PiecePair piecePair = parsePieceChar(c);
          board_[i*10+j] = (piecePair.first | piecePair.second);
        }catch (int e){
          logError("Unknown character %c encountered while reading board at [%d, %d]\n", c, i, j);
          return false;
        }
      }
    }

  afterPositionLoad();
  return true;
}

//--------------------------------------------------------------------- 

Step Board::findMCstep() 
{
  Step step;

  //it's not possible to have 0 rabbits in the beginning of move
  assert( rabbitsNum[toMoveIndex_] > 0 || stepCount_ > 0); 

  //no piece for player to move 
  if (pieceArray[toMoveIndex_].getLen() == 0) {
    //step_pass since the player with no pieces still might win 
    //if he managed to kill opponent's last rabbit before he lost his last piece
    return Step(STEP_PASS, toMove_);          
  }

  if (! cfg.knowledgeInPlayout()){
    if (findRandomStep(step))
      return step;
  }

  stepArrayLen = 
      generateAllStepsNoPass(toMove_, stepArray);
  uint len = stepArrayLen;

  assert(stepArrayLen < MAX_STEPS);

  if (len == 0 ){ //player to move has no step to play - not even pass
    winner_ = OPP(toMove_);
    return Step(STEP_NULL,toMove_); 
  }

  if (cfg.knowledgeInPlayout()){
    return chooseStepWithKnowledge(stepArray, stepArrayLen);
  }

  assert(len > 0);
  uint index = rand() % len;
  assert( index >= 0 && index < len );

  return( stepArray[index]); 
} 

//--------------------------------------------------------------------- 

void Board::findMCmoveAndMake()
{
  //TODO area selection 
  
  if (pieceArray[toMoveIndex_].getLen() == 0){
    makeStepTryCommitMove(Step(STEP_PASS, toMove_));
    return;
  }

  Step step;
  PieceArray p;

  for (int i = 0; i < 2; i++){
    p.add(pieceArray[toMoveIndex_].getRandom());
  }

  do { 
    /*if (options.localMode()){
      cerr << toString();
      cerr << lastStep().toString();
    }
    */

    stepArrayLen = 1;
    stepArray[0] = Step(STEP_PASS, toMove_);

    for (uint i = 0; i < p.getLen(); i++) { 
      if (OWNER(board_[p[i]]) == toMove_){
        generateStepsForPiece(p[i], stepArray, stepArrayLen);
      }
    }
    if (cfg.knowledgeInPlayout()){
      step = chooseStepWithKnowledge(stepArray, stepArrayLen);
    }
    else{
      step = stepArray[rand() % stepArrayLen];
    }
    if (! step.isPass()){
      p.del(step.from_);
      p.add(step.to_);
    }
  } while ( ! makeStepTryCommitMove(step));

  /*
  do {
    step = findMCstep();
  } while (! makeStepTryCommitMove(step));
  */

}

//--------------------------------------------------------------------- 

void Board::getHeuristics(const StepArray& steps, uint stepsNum, HeurArray& heurs) const
{
  for (uint i = 0; i < stepsNum; i++){
    heurs[i] = evaluateStep(steps[i]); 
    //assert(int(heurs[i]) == heurs[i]);
  }
}

//--------------------------------------------------------------------- 

void Board::init(bool newGame)
{

  stepArrayLen = 0;

  assert(OPP(GOLD) == SILVER);
  assert(OPP(SILVER) == GOLD);

  //game initialization - done in the first run or when newgame is specified
  if (! classInit || newGame) {
    classInit = true;
    initZobrist();  
    thirdRep.clear();
    thirdRep_ = &thirdRep;
  }

  for (int i = 0; i < 100; i++)  
    board_[i] = OFF_BOARD_SQUARE;

  for (int i = 1; i < 9; i++)
    for (int j = 1; j < 9; j++){
      board_[10*i+j]       = EMPTY_SQUARE;
      //frozenBoard is not in use now
      //frozenBoard_[10*i+j] = false;           //implicitly we consider everything not frozen
    }

  toMove_    = GOLD;
  toMoveIndex_ = PLAYER_TO_INDEX(toMove_);
  stepCount_ = 0;
  moveCount_ = 1;
  lastStep_  = Step();
  winner_    = EMPTY;

  //init pieceArray and rabbitsNum
  pieceArray[0].clear();
  pieceArray[1].clear();
  rabbitsNum[0] = 0;
  rabbitsNum[1] = 0;

  assert(PLAYER_TO_INDEX(GOLD) == 0 && PLAYER_TO_INDEX(SILVER) == 1);

  signature_ = 0;
  preMoveSignature_ = 0; 
  //signature is generated after the position is loaded
}

//---------------------------------------------------------------------


bool Board::initFromPositionStream(istream& is)
{

  init();

  char side;
  char c;
  PiecePair piecePair;
  
  try { 

    is >> moveCount_; 
    is >> side;
    is.ignore(1024,'\n'); //ignores the rest of initial line 
    toMove_ = sideCharToPlayer(side);
    if (! IS_PLAYER(toMove_))
      return false;
    toMoveIndex_ = PLAYER_TO_INDEX(toMove_);
    is.ignore(1024,'\n'); //ignores the top of the border till EOF 

    for (int i = 8; i > 0; i--) { // do this for each of the 8 lines of the board
      is.ignore(2); //ignore trailing characters

      for (int j = 1; j < 9; j++) {
        is.ignore(1); //ignore a white space 
        c=is.get();

        if (c == ' ' || c=='X' || c=='x' || c == '.')
          board_[i*10+j] = (EMPTY_SQUARE); 
        else {
          try{
            piecePair = parsePieceChar(c);
            board_[i*10+j] = (piecePair.first | piecePair.second);
          }catch (int e){
            logError("Unknown character %c encountered while reading board at [%d, %d]\n", c, i, j);
            return false;
          }
        } 
      } 
      is.ignore(1024,'\n'); //finish the line
    } 
  } 
  catch(int e) {
    return false; //initialization from file failed
  }

  afterPositionLoad();

  return true;
}

//--------------------------------------------------------------------- 

player_t Board::sideCharToPlayer(char side) const
{
  player_t player;
  if (! (side == 'w' || side == 'b' || side == 'g' || side =='s'))
    return EMPTY;


  if (side == 'w' || side == 'g')
    player = GOLD;
  else {
    player = SILVER;
  }

  return player;
}

//--------------------------------------------------------------------- 

void Board::afterPositionLoad()
{
  makeSignature();    //(unique) position identification
  preMoveSignature_ = signature_;

  for (int square = 11; square < 89; square++){
    if (IS_PLAYER(board_[square]))
      pieceArray[PLAYER_TO_INDEX(OWNER(board_[square]))].add(square);
    if (PIECE(board_[square]) == PIECE_RABBIT)
      rabbitsNum[PLAYER_TO_INDEX(OWNER(board_[square]))]++; 
  }
}

//--------------------------------------------------------------------- 

recordAction_e  Board::parseRecordActionToken(const string& token, player_t& player, 
                                              piece_t& piece, square_t& from, square_t& to)
{
  assert(token.length() == 3 || token.length() == 4);
  recordAction_e recordAction = ACTION_STEP;

  if (token.length() == 3) 
    recordAction = ACTION_PLACEMENT;
  else if (token[3] == 'x') //kill in trap
    recordAction = ACTION_TRAP_FALL; 

  //parse player/piece
  PiecePair piecePair = parsePieceChar(token[0]);
  player = piecePair.first;
  piece = piecePair.second;

  //parse from
  string columnRefStr("abcdefgh");
  string::size_type col = columnRefStr.find(token[1], 0);
  assert(col != string::npos);
  uint row = token[2] - '0';
  assert(row > 0 && row < 9);
  from = (row) * 10 + (col + 1);

  if (recordAction == ACTION_STEP){
    square_t direction = 0; 
    switch  (token[3]){
      case 'n': direction = NORTH;  
        break;  
      case 's': direction = SOUTH;  
        break;  
      case 'e': direction = EAST;  
        break;  
      case 'w': direction = WEST;  
        break;  
      default:
        assert(false);
        break;
    }
    to = from + direction;
  }

  return recordAction;
}

//--------------------------------------------------------------------- 

PiecePair Board::parsePieceChar(char pieceChar) 
{
  player_t player;
  piece_t piece;

  player = GOLD;
  if (islower(pieceChar))
    player = SILVER;
  pieceChar = tolower(pieceChar); 

  switch(pieceChar) {
    case 'e' : piece = PIECE_ELEPHANT; break;
    case 'm' : piece = PIECE_CAMEL;    break;
    case 'h' : piece = PIECE_HORSE;    break;
    case 'd' : piece = PIECE_DOG;      break;
    case 'c' : piece = PIECE_CAT;      break;
    case 'r' : piece = PIECE_RABBIT;   break;
    default:
      throw "Incorrect piece Character encountered.";
      break;
  }
  return PiecePair(player, piece); 
}


//---------------------------------------------------------------------

void Board::initZobrist() const
{
   for (int i = 0; i < PLAYER_NUM; i++)
    for (int j = 0; j < PIECE_NUM; j++)
      for (int k = 0; k < SQUARE_NUM; k++)
        zobrist[i][j][k] = getRandomU64(); 
}

//---------------------------------------------------------------------

void Board::makeSignature()
{
  signature_ = 0;
  for (int i = 0; i < 100; i++)  
    if IS_PLAYER(board_[i])
      signature_ ^= zobrist[PLAYER_TO_INDEX(OWNER(board_[i]))][PIECE(board_[i])][i] ;
}

//---------------------------------------------------------------------

void Board::makeStep(const Step& step)
{
  
  lastStep_ = step;

  int playerIndex = PLAYER_TO_INDEX(step.player_);
  if (step.stepType_ == STEP_NULL)
    return;

  if (step.stepType_ == STEP_PASS ){
    stepCount_++; 
    return;
  }

  assert(step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL || step.stepType_ == STEP_SINGLE );

  //in STEP_PUSH victim's move must be made first ! 
  if (step.stepType_ == STEP_PUSH ){  
    assert( stepCount_ < 3 ); 
    setSquare( step.oppTo_, OPP(step.player_), step.oppPiece_);
    clearSquare(step.oppFrom_);
    stepCount_++;
    pieceArray[1 - playerIndex].del(step.oppFrom_);
    pieceArray[1 - playerIndex].add(step.oppTo_);

    checkKill(step.oppFrom_);
  }

  //single step
  setSquare( step.to_, step.player_, step.piece_);
  clearSquare(step.from_);
  stepCount_++;
  pieceArray[playerIndex].del(step.from_);
  pieceArray[playerIndex].add(step.to_);

  checkKill(step.from_);

  //pull steps
  if (step.stepType_ == STEP_PULL) {  
    assert( stepCount_ < 4 ); 
    setSquare( step.oppTo_, OPP(step.player_), step.oppPiece_);
    clearSquare(step.oppFrom_);
    stepCount_++;
    pieceArray[1 - playerIndex].del(step.oppFrom_);
    pieceArray[1 - playerIndex].add(step.oppTo_);

    checkKill(step.oppFrom_);
  }

}

//---------------------------------------------------------------------

bool Board::findRandomStep(Step& step) const
{
  
  bool found = false; //once set to true, move is generated and returned 
  
  bool tryLocal = false;
  if (cfg.localPlayout() && 
      lastStep_.player_ == toMove_ && 
      lastStep_.stepType_ != STEP_NULL && 
      IS_PLAYER(board_[lastStep_.to_])){
    tryLocal = true;
  }

  for ( int i = 0; i < cfg.randomStepTries(); i++){ 
    assert(pieceArray[toMoveIndex_].getLen() != 0);

    if (tryLocal and random01() > 0.2){
      tryLocal = false;
      step.from_ = lastStep_.to_;
    } 
    else{
      step.from_ = pieceArray[toMoveIndex_][rand() % pieceArray[toMoveIndex_].getLen()];
    }

    assert(IS_PLAYER(board_[step.from_]));
    assert(OWNER(board_[step.from_]) == toMove_);

    if (isFrozen(step.from_))
      continue;
   
    step.to_ = step.from_ + direction[rand() % 4];
    if ( board_[step.to_] == EMPTY_SQUARE ){ //single/pull
      if (PIECE(board_[step.from_]) == PIECE_RABBIT &&   //rabbits cannot backwards
         (step.to_ - step.from_ == (toMove_ == GOLD ? SOUTH : NORTH ))) {
        continue;
      }

      for (int j = 0; j < 4; j++){
        step.oppFrom_ = step.from_ + direction[j];    //todo - exclude "to" direction
        if ( OWNER(board_[step.oppFrom_]) == OPP(OWNER(board_[step.from_])) &&
             PIECE(board_[step.oppFrom_]) <  PIECE(board_[step.from_])){
          found = true; //generate the step 
          step.stepType_ = STEP_PULL;
          assert(OWNER(board_[step.oppFrom_]) == OPP(toMove_)); 
          step.oppTo_ = step.from_; 
          break;
        }
      } 
      if (! found){
        step.stepType_ = STEP_SINGLE;
        found = true;
      }
    }else{ //push

      if ( OWNER(board_[step.to_]) == OPP(OWNER(board_[step.from_])) &&
           PIECE(board_[step.to_]) <  PIECE(board_[step.from_])){
        assert(IS_PLAYER(board_[step.to_]));
        for (int j = 0; j < 4; j++){
          if (board_[step.to_ + direction[j]] == EMPTY_SQUARE){  //generate the step
            step.oppTo_ = step.to_ + direction[j];    
            step.stepType_ = STEP_PUSH;
            step.oppFrom_ = step.to_; 
            assert(OWNER(board_[step.oppFrom_]) == OPP(toMove_)); 
            found = true;
            break;
          }
        }
      }
    }

    if (found){
      step.player_ = toMove_;
      step.piece_  = PIECE(board_[step.from_]);
      assert(OWNER(board_[step.from_]) == toMove_);
      assert(step.stepType_ == STEP_SINGLE || step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL);

      if (step.stepType_ != STEP_SINGLE) {
        //push/pull not allowed 
        if (stepCount_ > 2){
          found = false;
          continue;
        }
        assert(OWNER(board_[step.oppFrom_]) == OPP(toMove_)); 
        step.oppPiece_  = PIECE(board_[step.oppFrom_]);
      }

      return true;
    }
  }
    return false;
}

//--------------------------------------------------------------------- 

Step Board::chooseStepWithKnowledge(StepArray& steps, uint stepsNum) const
{
  assert(stepsNum > 0);
  uint bestIndex = stepsNum - 1;
  float bestEval = INT_MIN; 
  float eval = 0;
  //int r = smallRandomPrime();
  //int index;

  //cerr << toString();
  if ( cfg.knowledgeTournamentSize() == 0){
    for (uint i = 0; i < stepsNum; i++){
      //index = ((i+1)*r) % stepsNum;
      if (random01() >= 0.5)
        continue;
      const Step& step = steps[i];
      eval = evaluateStep(step); 
      //cerr << i << "/" << stepsNum << "/" << endl;
      cerr << step.toString() << " " << eval << " | ";
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = i;
      }
    }
    cerr << steps[bestIndex].toString() << endl;
  }
  else {

    for (uint i = 0; i < (stepsNum/2 > cfg.knowledgeTournamentSize() ? cfg.knowledgeTournamentSize() : stepsNum/2); i++){
    //for (uint i = 0; i < cfg.knowledgeTournamentSize(); i++){
      uint r = rand() % stepsNum;
      assert(r >= 0 && r < stepsNum);
      const Step& step = steps[r];
      eval = evaluateStep(step); 
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = r;
      }
    }
  }
  assert(bestIndex >= 0 && bestIndex < stepsNum);
  return steps[bestIndex];
}

//--------------------------------------------------------------------- 

float Board::evaluateStep(const Step& step) const
{
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

  if (step.piece_ == PIECE_ELEPHANT ) {
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
}

//--------------------------------------------------------------------- 

bool Board::checkKillForward(square_t from, square_t to, KillInfo* killInfo) const
{

  if ( IS_TRAP(to) ) { //piece steps into the trap ( or is pushed/pulled in there ) 
    if ( ! hasTwoFriends(to, OWNER(board_[from])) ) { 
      if (killInfo != NULL){
        killInfo->setValues(OWNER(board_[from]), PIECE(board_[from]), to); //activates as well
      }
      return true;
    }
    else //piece was moved into the trap but hasn't died -> no kill
      return false;
  }

  int actTrapPos;
  for (int i = 0; i < 4; i++){
    actTrapPos = from + direction[i];
    if ( IS_TRAP(actTrapPos) && 
        board_[actTrapPos] != EMPTY_SQUARE &&          //it is a trap where piece is standing
        OWNER(board_[actTrapPos]) == OWNER(board_[from]) && 
        ! hasTwoFriends(actTrapPos, OWNER(board_[actTrapPos])) ){   //piece has < 2 friends
        if (killInfo != NULL){
          killInfo->setValues(OWNER(board_[actTrapPos]), PIECE(board_[actTrapPos]), actTrapPos);                   
        }
        return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------- 

bool Board::checkKill(square_t square) 
{
  int actTrapPos;
  for (int i = 0; i < 4; i++){
    actTrapPos = square + direction[i];
    if ( IS_TRAP(actTrapPos) ){    
      if ( board_[actTrapPos] != EMPTY_SQUARE && ! hasFriend(actTrapPos) ){
        performKill(actTrapPos);
        return true;
      }
      return false;
    }
  }
  return false;
}

//--------------------------------------------------------------------- 

void Board::performKill(square_t trapPos)
{
  int playerIndex = PLAYER_TO_INDEX(OWNER(board_[trapPos]));

  if (PIECE(board_[trapPos]) == PIECE_RABBIT)   //update rabbitsNum - for gameEnd check 
     rabbitsNum[playerIndex]--;
  clearSquare(trapPos);
  pieceArray[playerIndex].del(trapPos);
}

//---------------------------------------------------------------------

bool Board::makeStepTryCommitMove(const Step& step) 
{
  makeStep(step);
	if (stepCount_ >= 4 || ! step.pieceMoved()) {
    updateWinner();
    commitMove();
    return true;
  }
  return false;
}

//---------------------------------------------------------------------

void Board::makeMove(const string& moveRaw)
{
  string move = trimLeft(trimRight(moveRaw));
  if (move == "")
    return; 

  stringstream ss(move);

  while (ss.good()){
    recordAction_e recordAction;
    string token;
    player_t player; 
    piece_t  piece;
    square_t from;
    square_t to;

    ss >> token;

    recordAction = parseRecordActionToken(token, player, piece, from, to);
    Step step = Step(STEP_SINGLE, player, piece, from, to);

    switch (recordAction){
      case ACTION_PLACEMENT:
        assert(moveCount_ == 1);
        setSquare(from, player, piece);
        pieceArray[PLAYER_TO_INDEX(player)].add(from);
        if (piece == PIECE_RABBIT)
          rabbitsNum[PLAYER_TO_INDEX(player)]++; 
        break;
      case ACTION_STEP:
        assert(moveCount_ > 1);
        makeStep(step);
        break;
      case ACTION_TRAP_FALL:
        break;
    }
  }

  commitMove();
}


//---------------------------------------------------------------------

void Board::makeMove(const Move& move)
{
  makeMoveNoCommit(move);
  commitMove();
}

//---------------------------------------------------------------------

void Board::makeMoveNoCommit(const Move& move)
{
  StepList stepList;
  stepList  = move.getStepList();

  assert(stepList.size() <= STEPS_IN_MOVE);
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++)
    makeStep(*it);

}

//--------------------------------------------------------------------- 

void Board::commitMove()
{
  assert(toMove_ == GOLD || toMove_ == SILVER);
  if (toMove_ == SILVER) 
    moveCount_++;
  toMove_ = OPP(toMove_);
  toMoveIndex_ = 1 - toMoveIndex_;
  assert(toMoveIndex_ == uint(PLAYER_TO_INDEX(toMove_)));
  stepCount_ = 0;

  //preMoveSignature of the next move is the actual signature
  preMoveSignature_ = signature_;
}


//--------------------------------------------------------------------- 

void Board::updateWinner()
{
  //assert(winner_ == EMPTY);
  //winner might be set already ( when player to move has no move ) 
  for (int i = 11; i <= 19; i++)
    if (board_[i] == (SILVER | PIECE_RABBIT) )
      winner_ = SILVER;
  for (int i = 81; i <= 88; i++)
    if (board_[i] == (GOLD | PIECE_RABBIT) )
      winner_ = GOLD;

  if (rabbitsNum[toMoveIndex_] <= 0)  //if player lost his last rabbit in his move - he loses ... 
    winner_ = OPP(toMove_);
  if (rabbitsNum[1 - toMoveIndex_] <= 0)  //unless the other also lost his last rabbit 
    winner_ = toMove_;
}

//--------------------------------------------------------------------- 


bool Board::quickGoalCheck(player_t player, int stepLimit, Move* move) const
{
  assert(IS_PLAYER(player));
  assert(stepLimit <= STEPS_IN_MOVE);

  queue<int> q;
  FlagBoard flagBoard;
  int rabbitDirection[3] = {player == GOLD ? NORTH : SOUTH, EAST, WEST}; 
  int winRow[2] = {TOP_ROW, BOTTOM_ROW};
  int index = PLAYER_TO_INDEX(player);

  //bounds for flagBoard - depends on player
  //only part of flagBoard is "interesting"
  //conditions on rabbits should hold search in these bounds 
  //we get like 10% speedup in quickGoalCheck by this
  //TODO this might be still narrowed when stepLimit < 4
  const int lower = player == GOLD ? 41 : 11;
  const int upper = player == GOLD ? 89 : 59;

  //init with rabbits having chance to reach the goal (distance 4 from some trap)
  for (uint i = 0; i < pieceArray[index].getLen(); i++){
    if (PIECE(board_[pieceArray[index][i]]) != PIECE_RABBIT || 
        abs(ROW(pieceArray[index][i]) - winRow[index]) > stepLimit){
      continue;
    }

    //narrower (=>quicker) than [0, SQUARE_NUM])
    for (int k = lower; k < upper; k++ ){
      flagBoard[k] = FLAG_BOARD_EMPTY;
    } 
    
    //out of board
    uint sacrificeTrap = 0;
    //sacrifice trap issues
    for (int j = 0; j < 4; j++){
      int ngb = pieceArray[index][i] + direction[j];
      if ( IS_TRAP(ngb) && 
          OWNER(board_[ngb]) == player && 
          ! hasTwoFriends(ngb, player)){
        assert(board_[ngb] != EMPTY_SQUARE);
        sacrificeTrap = ngb;
      }
    }

    q.push(pieceArray[index][i]);
    flagBoard[pieceArray[index][i]] = 0;
   
    bool lastValid = true;
    int last = 0;
    //wave
    while(! q.empty()){
      int act = q.front();
      q.pop(); 
      if (! lastValid)
        flagBoard[last] = FLAG_BOARD_EMPTY;
      lastValid = false;
      last = act;
      //too many steps
      if (flagBoard[act] >= stepLimit) 
        continue;
      //too faw away to reach the goal/TODO - more elaborate check ? :)
      if (abs(ROW(act) - winRow[index]) > (stepLimit - flagBoard[act]))
        continue;
      //frozen or in the trap
      if (flagBoard[act] == 1  || 
          (flagBoard[act] == 2 && SQUARE_DISTANCE(act, sacrificeTrap) == 1)) {
        if (!hasTwoFriends(act, player) && 
            (hasStrongerEnemy(act, player, PIECE_RABBIT) || IS_TRAP(act)))
          continue;
      }
      else{
        if (!hasFriend(act, player) && 
            (hasStrongerEnemy(act, player, PIECE_RABBIT) || IS_TRAP(act)))
          continue;
      }

      lastValid = true;
      //add neighbours 
      for (int l = 0; l < 3; l++){
        //neighbour
        int ngb = act + rabbitDirection[l];
        if (flagBoard[ngb] != FLAG_BOARD_EMPTY ) {
          continue; 
        }
        if ( board_[ngb] == EMPTY_SQUARE ){ 
          if (ROW(ngb) == winRow[index]){
            if (move != NULL){
              flagBoard[ngb] = flagBoard[act] + 1;
              (*move) = tracebackFlagBoard(flagBoard, ngb, player);
            }
            return true;
          }
          q.push(ngb);
          flagBoard[ngb] = flagBoard[act] + 1;
        }
      }
    }
  }

  return false;
  
}

//--------------------------------------------------------------------- 

bool Board::quickGoalCheck(Move* move) const
{
  return quickGoalCheck(toMove_, STEPS_IN_MOVE - stepCount_, move );
}

//---------------------------------------------------------------------

Move Board::tracebackFlagBoard(const FlagBoard& flagBoard, 
                                int win_square, player_t player) const
{
  assert(flagBoard[win_square] <= STEPS_IN_MOVE);
  assert(flagBoard[win_square] > 0);

  Move move;
  //inverse directions ... trackback
  int act = win_square;
  bool found_nbg;

  for (int i = 0; i < flagBoard[win_square]; i++) {
    found_nbg = false;
    for (int j = 0; j < 4; j++){
      int nbg = act + direction[j];
      if ( board_[nbg] != OFF_BOARD_SQUARE &&
          flagBoard[nbg] == flagBoard[act] - 1) {
        //going backwards => prepending! 
        move.prependStep(Step(STEP_SINGLE, player, PIECE_RABBIT, nbg, act));
        act = nbg;
        found_nbg = true;
        break;
      }
    }
    assert(found_nbg);
  }

  return move;
}

//---------------------------------------------------------------------

u64 Board::calcAfterStepSignature(const Step& step) const
{
  if (step.stepType_ == STEP_PASS)
    return signature_;

  assert(step.stepType_ == STEP_PUSH || 
        step.stepType_ == STEP_PULL || 
        step.stepType_ == STEP_SINGLE );

  int checkTrap[2];         //squares in whose vicinity we check the traps
  int checkTrapNum = 0;

  u64 newSignature = signature_;

  if (step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL){
    newSignature ^= zobrist[PLAYER_TO_INDEX(OPP(toMove_))][step.oppPiece_][step.oppFrom_];  //erase previous location
    if ( (! IS_TRAP(step.oppTo_)) || hasTwoFriends(step.oppTo_, OPP(toMove_)) ){              //if not killed in the trap
      newSignature ^= zobrist[PLAYER_TO_INDEX(OPP(toMove_))][step.oppPiece_][step.oppTo_];  //add new location 
      checkTrap[checkTrapNum++] = step.oppFrom_;
    }
  }

  newSignature ^= zobrist[PLAYER_TO_INDEX(toMove_)][step.piece_][step.from_];               //erase previous location 
  if ( ! IS_TRAP(step.to_) || hasTwoFriends(step.to_, toMove_) ){                   //if not killed in the trap
    newSignature ^= zobrist[PLAYER_TO_INDEX(toMove_)][step.piece_][step.to_];               //add new location 
    checkTrap[checkTrapNum++] = step.from_;
  }
  
  //now check only for cases when moving a piece causes another (friendly) piece die in the trap because of lack of friends
  int actTrap;
  for (int j = 0; j < checkTrapNum; j++){
    for (int i = 0; i < 4; i++){
      actTrap = checkTrap[j] + direction[i];
      if ( IS_TRAP(actTrap) &&                                          //there is a trap next to the position of piece
           board_[actTrap] != EMPTY_SQUARE &&                           
           OWNER(board_[actTrap]) == OWNER(board_[checkTrap[j]]) &&    //there is a friend in the trap 
           ! hasTwoFriends(actTrap, OWNER(board_[actTrap])) )          //he will die on lack of friends
        newSignature ^= zobrist[PLAYER_TO_INDEX(OWNER(board_[actTrap]))][PIECE(board_[actTrap])][actTrap];      //erase 
    } 
  }

  return newSignature;
}

//---------------------------------------------------------------------

int Board::generateAllStepsNoPass(player_t player, StepArray& steps) const
{
  uint stepsNum = 0;
  int square;

  //go through pieces of player 
  for (uint index =0 ; 
      index < pieceArray[PLAYER_TO_INDEX(player)].getLen(); index++) { 
    square = pieceArray[PLAYER_TO_INDEX(player)][index];
    assert(OWNER(board_[square]) == player); 
    generateStepsForPiece(square, steps, stepsNum);
  } //for pieces on board

  return stepsNum;
}

//--------------------------------------------------------------------- 

int Board::generateAllSteps(player_t player, StepArray& steps) const {
  int stepsNum = generateAllStepsNoPass(player, steps);
  if (canPass()){
    steps[stepsNum++] = Step(STEP_PASS, player);
  }
  return stepsNum;
}

//--------------------------------------------------------------------- 

void Board::generateStepsForPiece(
        square_t square, StepArray& steps, uint& stepsNum) const { 

  if ( isFrozen(square))  //frozen
    return; 
  player_t player = OWNER(board_[square]);

  //generate push/pull moves
  if (stepCount_ < 3) {    
    for (int i = 0; i < 4; i++) {  
      if (OWNER(board_[square + direction[i]]) == OPP(player) 
          && PIECE(board_[square + direction[i]]) < PIECE(board_[square])){ //weaker enemy
        for (int j=0; j<4; j++)  // pull
          if (board_[square + direction[j]] == EMPTY_SQUARE) { //create move
              steps[stepsNum++].setValues( STEP_PULL, player,PIECE(board_[square]), square, 
                    square + direction[j], PIECE(board_[square + direction[i]]), square + direction[i], square);
          }
        for (int j=0; j<4; j++)  //push
          if (board_[square + direction[i] + direction[j]] == EMPTY_SQUARE) { //create move
              steps[stepsNum++].setValues( STEP_PUSH, player, PIECE(board_[square]), square, 
                    square + direction[i], PIECE(board_[square + direction[i]]),
                    square + direction[i], square + direction[i] + direction[j]);
        }
      } //if weaker enemy
    } //for
  } 

  // generate single moves
  for (int i=0; i<4; i++) // check each direction
    if (board_[square + direction[i]] == EMPTY_SQUARE)  {
      if (PIECE(board_[square]) == PIECE_RABBIT){ // rabbit cannot backwards
        if (OWNER(board_[square]) == GOLD && direction[i] == SOUTH)
          continue;
        if (OWNER(board_[square]) == SILVER && direction[i] == NORTH)
          continue;
      }
      //create move
      steps[stepsNum++].setValues(STEP_SINGLE, player, PIECE(board_[square]), 
                                      square, square + direction[i]);
    }

  
}

//---------------------------------------------------------------------

int Board::filterRepetitions(StepArray& steps, int stepsNum) const 
{

  //check virtual passes ( immediate repetetitions ) 
  //these might be checked and pruned even if the move is not over yet 
  //e.g. (Eg5n) (Eg5s) is pruned 
  
  int i = 0;
  while (i < stepsNum) 
    if (stepIsVirtualPass(steps[i]))
      steps[i] = steps[stepsNum-- - 1];
    else
      i++;  

  //check third time repetitions
  //this can be checked only for steps that finish the move
  //these are : pass, step_single for stepCount == 3, push/pull for stepCount == 2 

  if ( stepCount_ < 2 ) 
    return stepsNum;

  i = 0;
  while (i < stepsNum)
    if ((stepCount_ == 3 || steps[i].isPushPull()) &&  stepIsThirdRepetition(steps[i]))
      steps[i] = steps[stepsNum-- - 1];
    else
      i++;  
  

  return stepsNum;
}

//---------------------------------------------------------------------

bool Board::stepIsVirtualPass( Step& step ) const 
{
  u64 afterStepSignature = calcAfterStepSignature(step);
  if (afterStepSignature == preMoveSignature_) 
    return true;
  return false;
  
}

//---------------------------------------------------------------------

bool Board::stepIsThirdRepetition(const Step& step ) const 
{
  u64 afterStepSignature = calcAfterStepSignature(step);
  assert(1 - PLAYER_TO_INDEX(step.getPlayer()) == 
        PLAYER_TO_INDEX(getPlayerToMoveAfterStep(step)));
  //check whether position with opponent to move won't be a repetition
  if ( thirdRep_->isThirdRep(afterStepSignature, 1 - PLAYER_TO_INDEX(step.getPlayer()))) 
    return true;
  return false;
  
}

//---------------------------------------------------------------------

bool Board::hasFriend(square_t square) const 
{ 
  uint owner = OWNER(board_[square]);
  return hasFriend(square, owner);
}

//--------------------------------------------------------------------- 

bool Board::hasFriend(square_t square, player_t owner) const 
{ 
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == owner)
      return true;

  return false;
}

//---------------------------------------------------------------------

bool Board::hasTwoFriends(square_t square, player_t player) const 
{ 
  assert( player == GOLD || player == SILVER );
  bool hasFriend = false;
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == player) {
      if (hasFriend)
        return true;
      hasFriend = true;
    }
  return false;
}

//---------------------------------------------------------------------

bool Board::hasStrongerEnemy(square_t square) const
{
  uint owner = OWNER(board_[square]);
  return hasStrongerEnemy(square, owner, PIECE(board_[square]));
}

//--------------------------------------------------------------------- 

bool Board::hasStrongerEnemy(square_t square, player_t owner, piece_t piece) const
{
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == OPP(owner) && 
        PIECE(board_[square + direction[i]]) > piece)
      return true;

  return false;
  
}

//---------------------------------------------------------------------

bool Board::isFrozen(square_t square) const
{
  return (!hasFriend(square) && hasStrongerEnemy(square)); 
}

//---------------------------------------------------------------------

bool Board::isSetupPhase() const
{
  return moveCount_ == 1 && pieceArray[toMoveIndex_].getLen() == 0; 
}

//---------------------------------------------------------------------

uint Board::getStepCount() const
{
  return stepCount_;
}

//--------------------------------------------------------------------- 

u64 Board::getSignature() const
{
  return signature_;
}

//---------------------------------------------------------------------

u64 Board::getPreMoveSignature() const
{
  return preMoveSignature_; 
}

//---------------------------------------------------------------------

player_t Board::getPlayerToMove() const
{
  return toMove_;
}

//---------------------------------------------------------------------

player_t Board::getPlayerToMoveAfterStep(const Step& step) const
{ 
  //TODO what about resing step ? 
  assert( step.isPass() || step.isPushPull() || step.isSingleStep());
  player_t player = toMove_;

  //check whether player switches after the step
  if (step.isPass() || stepCount_ == 3 || (stepCount_ == 2 && step.isPushPull())) 
    player = OPP(player);
  return player;
}

//---------------------------------------------------------------------

player_t Board::getWinner() const
{
  return winner_;
}

//--------------------------------------------------------------------- 

player_t Board::gameOver() const
{
  return IS_PLAYER(winner_);
}

//---------------------------------------------------------------------

bool Board::canContinue(const Move& move) const
{
  return (getStepCount() + move.getStepCount() ) < STEPS_IN_MOVE;
}

//--------------------------------------------------------------------- 
  
bool Board::canPass() const
{
  return stepCount_ > 0 && ! stepIsThirdRepetition(Step(STEP_PASS, toMove_));
}

//--------------------------------------------------------------------- 

Step Board::lastStep() const 
{
  return lastStep_;
}

//--------------------------------------------------------------------- 

/*places piece at given position and updates signature*/
void Board::setSquare( square_t square, player_t player, piece_t piece)  
{
  signature_ ^=  zobrist[PLAYER_TO_INDEX(player)][piece][square]; 
  board_[square] = ( player | piece ); 
}

//---------------------------------------------------------------------

/*removes piece from given position and updates signature*/ 
void Board::clearSquare( square_t square) 
{
  signature_ ^=  zobrist[PLAYER_TO_INDEX(OWNER(board_[square]))][PIECE(board_[square])][square]; 
  board_[square] = EMPTY_SQUARE; 
} 

//---------------------------------------------------------------------

string Board::toString() const
{

  stringstream ss;

  
  ss << endl;

  //ss << pieceArray[0].toString() << pieceArray[1].toString();


  //print zobrist
  /*
  for (int i = 0; i < PLAYER_NUM; i++)
   for (int j = 0; j < PIECE_NUM; j++)
     for (int k = 0; k < SQUARE_NUM; k++)
        ss << zobrist[i][j][k] << endl;
  */


  //ss << "board at: " << this << endl; //pointer debug
  //#ifdef DEBUG_1
  ss << "Signature " << signature_ << endl;
  //#endif

  ss << "Move " << (toMove_ == GOLD ? "g" : "s") << moveCount_ ;
  #ifdef DEBUG_2
    ss << ", Step " << stepCount_ << ", ";
  #endif 
  ss << endl;
    
  assert(toMove_ == GOLD || toMove_ == SILVER );

  ss << " +-----------------+\n";

  for (int i=8; i>0; i--) {
    ss << i <<"| ";
    for (int j=1; j<9; j++) {

      switch(board_[i*10+j]){
        case (GOLD | PIECE_ELEPHANT) :    ss << "E"; break;
        case (GOLD | PIECE_CAMEL) :       ss << "M"; break;
        case (GOLD | PIECE_HORSE) :       ss << "H"; break;
        case (GOLD | PIECE_DOG) :         ss << "D"; break;
        case (GOLD | PIECE_CAT) :         ss << "C"; break;
        case (GOLD | PIECE_RABBIT) :      ss << "R"; break;
        case (SILVER | PIECE_ELEPHANT) :    ss << "e"; break;
        case (SILVER | PIECE_CAMEL) :       ss << "m"; break;
        case (SILVER | PIECE_HORSE) :       ss << "h"; break;
        case (SILVER | PIECE_DOG) :         ss << "d"; break;
        case (SILVER | PIECE_CAT) :         ss << "c"; break;
        case (SILVER | PIECE_RABBIT) :      ss << "r"; break;
        case EMPTY_SQUARE :
          if ((i==3 || i==6) && (j==3 || j==6))
              ss << "X ";
          else
              ss << ". ";
          break;
        default :
            ss << "? ";
            break;
      }
      if ( OWNER(board_[i * 10 + j]) != EMPTY){
        //assert( frozenBoard_[i * 10 + j] == isFrozen( i * 10 + j)); uncomment when ready
        if (frozenBoard_[i * 10 + j] )
          ss << "*";
        else
          ss << " ";
      }
    }
   ss << "| " << endl;
  }

  ss << " +-----------------+" << endl;
  ss << "   a b c d e f g h" << endl;

  return ss.str();
} //Board::dump

//---------------------------------------------------------------------

string Board::allStepsToString() const
{
  stringstream ss;

  for (uint playerIndex = 0; playerIndex < 2; playerIndex++) {
    if ( playerIndex ) 
      ss << endl << "Potential moves for SILVER: " << endl;
    else
      ss << endl << "Potential moves for GOLD: " << endl;

    /*for ( uint i = 0; i < stepArrayLen; i++){
      ss << stepArray[playerIndex][i].toString();
    }
    */
  }
  ss << endl;
  return ss.str();
}

//---------------------------------------------------------------------

void Board::dump() const
{
  logRaw(toString().c_str());
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
