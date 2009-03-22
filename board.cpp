
/** 
 *  @file board.cpp
 *  @brief Board implementation. 
 *  @full Integer board, able to evaluate itself, compute it's hash, etc. 
 */

#include "board.h"
#include "eval.h"  //for evaluateStep

// zobrist base table for signature creating 
//u64  Board::zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     

ThirdRep*  Board::thirdRep_;
Eval*  Board::eval_;

Glob glob;

int threadsNum = 0;
int threadIds[MAX_THREADS];

//#define DEBUG_TRAPCHECK_ON

#ifdef DEBUG_TRAPCHECK_ON
  #define DEBUG_TRAPCHECK(x) x
#else
  #define DEBUG_TRAPCHECK(x) ((void) 0) 
#endif

// switch to know when to init static variables in class Board
bool Board::classInit = false;

//---------------------------------------------------------------------
//  section Global
//---------------------------------------------------------------------

//--------------------------------------------------------------------- 

Glob::Glob() 
{
  for (int i = 0; i < MAX_THREADS; i++){
    bpool_[i] = NULL;
    thirdRep_[i] = NULL;
  }
  init();
}

//--------------------------------------------------------------------- 

void Glob::init()
{
  assert(threadsNum < MAX_THREADS);
  for (int i = 0; i < MAX_THREADS; i++){
    if (bpool_[i] != NULL){
      delete bpool_[i];
      bpool_[i] = NULL;
    }
    if (thirdRep_[i] != NULL){
      delete thirdRep_[i];
      thirdRep_[i] = NULL;
    }
  }
  threadsNum_ = 0;
}

//--------------------------------------------------------------------- 

int Glob::tti() 
{
  int index = pthread_self();
  for (int i = 0; i < threadsNum_; i++){
    if (threadIds_[i] == index){
      return i;
    }  
  }

  return add_thread();
}

//--------------------------------------------------------------------- 

int Glob::add_thread() {

  pthread_mutex_lock(&lock);
  threadIds_[threadsNum_] = pthread_self();
  bpool_[threadsNum_] = new Bpool();
  thirdRep_[threadsNum_] = new ThirdRep();
  int ret = threadsNum_;
  threadsNum_++;
  pthread_mutex_unlock(&lock);
  return ret;
}

//--------------------------------------------------------------------- 

void randomStructuresInit()
{

  srand((unsigned) time(NULL));
  //srand(0);
  grand.seed(rand());
  bits::initZobrist();
}

//--------------------------------------------------------------------- 

bool parsePieceChar(char pieceChar, player_t &player, piece_t& piece) 
{
  player = GOLD;
  if (islower(pieceChar)){
    player = SILVER;
  }
  pieceChar = tolower(pieceChar); 

  switch(pieceChar) {
    case 'e' : piece = ELEPHANT; break;
    case 'm' : piece = CAMEL;    break;
    case 'h' : piece = HORSE;    break;
    case 'd' : piece = DOG;      break;
    case 'c' : piece = CAT;      break;
    case 'r' : piece = RABBIT;   break;
    default:
      logError("Incorrect piece Character encountered.");
      return false;
      break;
  }
  return true;
}

//--------------------------------------------------------------------- 

string coordToStr(coord_t coord)
{
  stringstream ss;
  string columnRefStr("abcdefgh");
  ss << columnRefStr[coord % 8] << coord / 8 + 1; 
  return ss.str();
}

//--------------------------------------------------------------------- 


//---------------------------------------------------------------------
//  section Soldier
//---------------------------------------------------------------------


Soldier::Soldier(player_t player, piece_t piece, coord_t coord)
: player_(player), piece_(piece), coord_(coord)
{
}

//--------------------------------------------------------------------- 

string Soldier::toString()
{
  stringstream ss;
  string pieceRefStr(" RCDHMErcdhme");

  ss << pieceRefStr[piece_ + 6 * player_] << coordToStr(coord_);
  return ss.str();
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

Step::Step( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to){
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
  oppPiece_ = NO_PIECE;
  oppFrom_  = NO_SQUARE;
  oppTo_    = NO_SQUARE;
}

//---------------------------------------------------------------------

Step::Step( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to, 
            piece_t oppPiece, coord_t oppFrom, coord_t oppTo)
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

void Step::setValues( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}

//---------------------------------------------------------------------

void Step::setValues( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to, 
            piece_t oppPiece, coord_t oppFrom, coord_t oppTo)
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

Step Step::toNew() const
{
  //already new
  if (pieceMoved() && (player_ == GOLD || player_ == SILVER)){
    assert(false);
    return *this;
  }

  Step s = (*this);
  s.from_ = SQUARE_TO_INDEX_64(s.from_);
  s.to_ = SQUARE_TO_INDEX_64(s.to_);
  s.oppTo_ = SQUARE_TO_INDEX_64(s.oppTo_);
  s.oppFrom_ = SQUARE_TO_INDEX_64(s.oppFrom_);
  s.player_ = OLD_PLAYER_TO_NEW(s.player_);
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
  return (stepType_ == STEP_SINGLE || stepType_ == STEP_PUSH || 
          stepType_ == STEP_PULL);
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
  //TODO why is this causing problems ??? 
  if (*this == other){ 
    return false;
  }

  const int itemsNum = 8;
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
  if (pieceMoved() && (player_ != GOLD && player_ != SILVER)){
    return toNew().toString();
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

const string Step::oneSteptoString(player_t player, piece_t piece, coord_t from, coord_t to) const
{
  stringstream s;
  s.str("");
  string pieceRefStr(" RCDHMErcdhme");
  string columnRefStr("abcdefgh");

  s << pieceRefStr[piece + 6 * player] << columnRefStr[from % 8] << from / 8 + 1; 
    //s << pieceRefStr[piece + 6 * PLAYER_TO_INDEX(player)] << columnRefStr[from % 10 - 1] << from / 10; 
  switch (to - from)
  {
    case NORTH : s << "n"; break;
    case WEST :  s << "w"; break;
    case EAST :  s << "e"; break;
    case SOUTH : s << "s"; break;

    default :
      //assert(false);
      break;
  }
  s << " ";

  return s.str();
}

//---------------------------------------------------------------------
//  section KillInfo
//--------------------------------------------------------------------- 

KillInfo::KillInfo()
{
  active_ = false;
}

//--------------------------------------------------------------------- 

KillInfo::KillInfo( player_t player, piece_t piece, coord_t coord):
  player_ (player), piece_ (piece), coord_(coord)
{
  active_ = false;
}

//--------------------------------------------------------------------- 

void KillInfo::setValues( player_t player, piece_t piece, coord_t coord)
{
  active_ = true;
  player_ = player;
  piece_  = piece;
  coord_ = coord;
}

//--------------------------------------------------------------------- 

const string KillInfo::toString() const
{
  if (! active_ ) 
    return "";

  stringstream s;
  string pieceRefStr(" RCDHMErcdhme");
  string columnRefStr("abcdefgh");

  s << pieceRefStr[piece_ + 6 * player_] << columnRefStr[coord_  % 8] 
    << coord_ / 8 + 1; 
  //s << pieceRefStr[piece_ + 6 * PLAYER_TO_INDEX(player_)] << columnRefStr[square_ % 10 - 1] << square_ / 10; 
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

  if (firstPart != ""){
   firstPart += " "; 
  }

  if (secondPart != ""){
   secondPart  += " "; 
  }

  ssOut << firstPart  << kills[0].toString() 
        << secondPart  << kills[1].toString();
  return ssOut.str();
}

//---------------------------------------------------------------------
//  section Move
//---------------------------------------------------------------------

Move::Move() 
{
  opening_ = false;
  stepCount_ = 0;
}

//--------------------------------------------------------------------- 

Move::Move(string moveStr)
{
  opening_ = false;
  assert(moveStr != "");
  stringstream ss(moveStr);

  while (ss.good()){
    recordAction_e recordAction;
    string token;
    player_t player; 
    piece_t  piece;
    coord_t from;
    coord_t to;

    ss >> token;

    recordAction = parseRecordActionToken(token, player, piece, from, to);
    //TODO this way even push/pulls are represented as single steps ... weird ! 
    Step step = Step(STEP_SINGLE, player, piece, from, to);

    switch (recordAction){
      case ACTION_PLACEMENT:
        opening_ = true;
        appendStep(step);
        break;
      case ACTION_STEP:
        assert(opening_ == false);
        appendStep(step);
        updateStepCount(step);
        break;
      case ACTION_TRAP_FALL:
      case ACTION_ERROR:
        break;
    }
  }

}

//--------------------------------------------------------------------- 

void Move::appendStep(Step step)
{
  stepList_.push_back(step);
  updateStepCount(step);
}

//--------------------------------------------------------------------- 

void Move::prependStep(Step step)
{
  stepList_.push_front(step);
  updateStepCount(step);
}

//--------------------------------------------------------------------- 

void Move::appendStepList(StepList stepList)
{
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    stepList_.push_back(*it);
    updateStepCount(*it);
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
  return stepCount_;
}

//--------------------------------------------------------------------- 

bool Move::isOpening() const 
{
  return opening_;
}

//--------------------------------------------------------------------- 

player_t Move::getPlayer() const
{
  if (stepList_.empty()) {
    return NO_PLAYER;
  }
  return stepList_.begin()->getPlayer();
}
    
//--------------------------------------------------------------------- 


bool Move::operator==(const Move& other) const
{
  if (stepCount_ != other.stepCount_ ) {
    return false;
  }
  StepList::const_iterator it = stepList_.begin();
  StepList::const_iterator jt = other.stepList_.begin(); 
  for (; it != stepList_.end() && jt != other.stepList_.end(); it++, jt++){
    if (! (*it == *jt)) {
      return false;
    }
  }
  return true;
}


//--------------------------------------------------------------------- 

string Move::toString() const
{
  string s;
  for (StepList::const_iterator it = stepList_.begin(); it != stepList_.end(); it++){
    s = s + (*it).toString();
  }
  return s;
}

//--------------------------------------------------------------------- 

recordAction_e  Move::parseRecordActionToken(const string& token, player_t& player, piece_t& piece, coord_t& from, coord_t& to)
{
  assert(token.length() == 3 || token.length() == 4);
  recordAction_e recordAction = ACTION_STEP;

  if (token.length() == 3) 
    recordAction = ACTION_PLACEMENT;
  else if (token[3] == 'x') //kill in trap
    recordAction = ACTION_TRAP_FALL; 

  //parse player/piece
  if (! parsePieceChar(token[0], player, piece)) {
    logError("Invalid piece char");
    return ACTION_ERROR;
  }

  //parse from
  string columnRefStr("abcdefgh");
  string::size_type col = columnRefStr.find(token[1], 0);
  assert(col != string::npos);
  uint row = token[2] - '1';
  assert(row >= 0 && row < 8);
  from = (row) * 8 + col;

  if (recordAction == ACTION_STEP){
    uint direction = 0; 
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

void Move::updateStepCount(const Step& step)
{
  switch (step.stepType_) {
    case STEP_NULL: 
    case STEP_PASS :   break;
    case STEP_PUSH:
    case STEP_PULL:  stepCount_++;
    case STEP_SINGLE : stepCount_++;
                  break; 
  }
}


//---------------------------------------------------------------------
//  section ContextMove
//---------------------------------------------------------------------

ContextMove::ContextMove(Move move, const Bitboard& bitboard):
  move_(move)
{
  StepList steps = move_.getStepList();
  mask_ = 0ULL;
  //TODO
  for (StepListIter it = steps.begin(); it != steps.end(); it++){
    assert(it->pieceMoved());
    //mask_ |= BIT_ON(it->from_) | (it->piece_ == ELEPHANT ? 0ULL : bits::neighborsOne(it->from_));
    mask_ |= BIT_ON(it->to_) | bits::neighborsOne(it->to_);
    mask_ |= BIT_ON(it->from_) | bits::neighborsOne(it->from_);
    if (it->isPushPull()){
      mask_ |= BIT_ON(it->oppFrom_) | bits::neighborsOne(it->oppFrom_);
      mask_ |= BIT_ON(it->oppTo_) | bits::neighborsOne(it->oppTo_);
    }
  }

  for (int i = 0; i < 2; i++){
    for (int j = 0; j < PIECE_NUM + 1; j++){
      context_[i][j] = mask_ & bitboard[i][j];
    }
  }

  visits_ = 0;
  value_ = 0;

  /*cerr << "=== Context Move ===" << endl 
    << Board::bitboardToString(bitboard) 
    << " + " << endl << move.toString() << endl;
    bits::print(cerr, mask_);
    cerr <<  " = " << endl 
    << Board::bitboardToString(context_);
  */
}

//--------------------------------------------------------------------- 

bool ContextMove::applicable(const Bitboard& bitboard, int stepsLeft) const
{
  if (stepsLeft < move_.getStepCount()) {
    return false;
  }
  for (int j = 0; j < PIECE_NUM + 1; j++){
    for (int i = 0; i < 2; i++){
      if ((mask_ & bitboard[i][j]) != context_[i][j]){
        return false;
      }
    }
  }
  return true;
}


//--------------------------------------------------------------------- 

Move ContextMove::getMove() const 
{
  return move_;
}

//--------------------------------------------------------------------- 

float ContextMove::getValue() const 
{ 
  return move_.getPlayer() == GOLD ? value_ : -1 * value_;
}

//---------------------------------------------------------------------

void ContextMove::update(float sample) {
  value_ += (sample - value_)/++visits_;  
}

//---------------------------------------------------------------------
//  section MoveAdviser
//---------------------------------------------------------------------

bool MoveAdvisor::getMove(player_t player, const Bitboard& bitboard, int stepsLeft, Move* move)
{
  return getMoveRand(player, bitboard, stepsLeft, move);

  for (uint i = 0; i < contextMoves[player].size(); i++) {
    if (contextMoves[player][i].applicable(bitboard, stepsLeft)){
      *move = contextMoves[player][i].getMove();
      playedCMs[player].push_back(i);
      update_ = true;
      return true;
    }
  }  
  return false;
}

//--------------------------------------------------------------------- 

bool MoveAdvisor::getMoveRand(player_t player, const Bitboard& bitboard, int stepsLeft, Move* move)
{
  int len = contextMoves[player].size();
  if (! len) {
    return false;
  }
  int sample = len;
  float bestValue = INT_MIN;
  int bestIndex = -1;
  for (int i = 0; i < sample; i++) {
    int index = i; //grand() % len; 
    if (contextMoves[player][index].getValue() > bestValue
        && contextMoves[player][index].applicable(bitboard, stepsLeft))
        {
      bestIndex = index;
      bestValue = contextMoves[player][index].getValue();
    }
  }  
  if (bestIndex != -1){
      *move = contextMoves[player][bestIndex].getMove();
      playedCMs[player].push_back(bestIndex);
      update_ = true;
      //cerr << move->toString() << "/" << contextMoves[player][bestIndex].getValue() << endl;
      return true;
  }
  return false;
}

//--------------------------------------------------------------------- 

void MoveAdvisor::addMove(const Move & move, const Bitboard& bitboard)
{
  if (! hasMove(move, bitboard)) {
    //cerr << move.toString() << " " << move.getStepCount() << endl;
    contextMoves[move.getPlayer()].push_back(ContextMove(move, bitboard));
  }
}

//--------------------------------------------------------------------- 

void MoveAdvisor::update(float sample) {
  if (! update_) {
    return;
  }
  for (player_t player = 0; player < 2; player++){
    for (list<int>::const_iterator it = playedCMs[player].begin(); 
          it != playedCMs[player].end(); it++){
      contextMoves[player][*it].update(sample);
    }
    playedCMs[player].clear();
  }
  update_ = false;
}

//--------------------------------------------------------------------- 

bool MoveAdvisor::hasMove(const Move & move, const Bitboard& bitboard){
  player_t pl = move.getPlayer();
  assert(IS_PLAYER(pl));
  for (ContextMoves::const_iterator it = contextMoves[pl].begin(); 
                                       it != contextMoves[pl].end(); it++){
    if (it->applicable(bitboard, move.getStepCount()) && it->getMove() == move){
      return true;
    }
  }
  return false;
}


//---------------------------------------------------------------------
//  section Board
//---------------------------------------------------------------------

string Board::moveToStringWithKills(const Move& move) const
{
  Board * playBoard = new Board(*this);
  string s;
  StepList stepList = move.getStepList();
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    s = s + StepWithKills((*it), playBoard).toString();
    playBoard->makeStepTryCommit(*it);
  }
  delete playBoard;
  return trim(s);
}

//--------------------------------------------------------------------- 

const int bdirection[4]={NORTH, EAST, SOUTH, WEST};
u64   bits::zobrist[2][7][64];     

u64   bits::stepOffset_[2][7][64]; 
u64   bits::winRank[2] = { 0xff00000000000000ULL ,0x00000000000000ffULL};

void bits::initZobrist() 
{
   for (int i = 0; i < 2; i++)
    for (int j = 0; j < 7; j++)
      for (int k = 0; k < 64; k++){
        bits::zobrist[i][j][k] = getRandomU64(); 
      }
}

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

int bits::bitCount(u64 b)
{
  int c = 0;

  while (b) {
    b -= (b & (-b));
    c++;
  }
  return(c);
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

u64 bits::neighborsOne(coord_t coord){
  return bits::stepOffset_[0][6][coord]; 
}

//---------------------------------------------------------------------

int bits::neighborsOneNum(coord_t coord, u64 mask){
  int num = 0;
  for (int i = 0; i < 4; i++) {
    if (mask & BIT_ON(coord + bdirection[i])){
      num++;
    }
  }
  return num; 
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

bool bits::isTrap(coord_t coord){
  return BIT_ON(coord) & TRAPS;
}
  
//--------------------------------------------------------------------- 


void bits::buildStepOffsets()
{
  player_t  player;
  piece_t  piece;
  coord_t  coord_;

  // do rabbits as a special case
  for (player = 0; player < 2; player++)
    for (coord_ = 0; coord_ < BIT_LEN; coord_++) {
      u64  ts = 0ULL;
      
      ts |= ((BIT_ON(coord_)) & NOT_H_FILE) << 1;  // east
      ts |= ((BIT_ON(coord_)) & NOT_A_FILE) >> 1;  // west
      if (player == 0)    
        ts |= ((BIT_ON(coord_)) & NOT_1_RANK) << 8;  //north
      else
        ts |= ((BIT_ON(coord_)) & NOT_8_RANK) >> 8;  //south

      stepOffset_[player][RABBIT][coord_] = ts;    
    }

  // Now do the rest
  for (player = 0; player < 2; player++)
    for (piece = 2; piece < 7; piece++)
      for (coord_ = 0; coord_ < BIT_LEN; coord_++) {
        u64  ts = bits::neighbors(BIT_ON(coord_));
        stepOffset_[player][piece][coord_] = ts;
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

Step Board::findMCstep() 
{
  Step step;
  StepArray steps;

  //it's not possible to have 0 rabbits in the beginning of move
  assert( bitboard_[toMove_][1] || stepCount_ > 0); 

  //no piece for player to move 
  if (! bitboard_[toMove_][0]) {
    //step_pass since the player with no pieces still might win 
    //if he managed to kill opponent's last rabbit before he lost his last piece
    return Step(STEP_PASS, toMove_);          
  }
  int len = genSteps(toMove_, steps);
  assert(len < MAX_STEPS);

  //player to move has no step to play - not even pass
  if (len == 0 ){ 
    winner_ = OPP(toMove_);
    return Step(STEP_NULL,toMove_); 
  }

  if (cfg.knowledgeInPlayout()){
    return chooseStepWithKnowledge(steps, len);
  }

  assert(len > 0);
  int index = grand() % len;
  assert( index >= 0 && index < len );

  return(steps[index]); 
} 

//--------------------------------------------------------------------- 

bool Board::makeStepTryCommit(const Step& step)
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

void Board::commit()
{
  assert(toMove_ == GOLD || toMove_ == SILVER);
  if (toMove_ == SILVER) {
    moveCount_++;
  }
  
  toMove_ = OPP(toMove_);
  stepCount_ = 0;

  preMoveSignature_ = signature_;
}

//--------------------------------------------------------------------- 

void Board::makeStep(const Step& step){

    lastStep_ = step;

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
      delSquare(step.oppFrom_, OPP(step.player_), step.oppPiece_);
      setSquare(step.oppTo_, OPP(step.player_), step.oppPiece_);
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
        if ( trap != BIT_EMPTY){
          delSquare(trap, player);
          //no more than one dead per player
          assert(bits::lix(dieHard) == BIT_EMPTY); 
        }
      }
    } 

}

//--------------------------------------------------------------------- 

void Board::updateWinner()
{
  //check goal
  if (bitboard_[toMove_][1] & bits::winRank[toMove_]) {
    winner_ = toMove_;
    return;
  }
  //check self goal
  if (bitboard_[OPP(toMove_)][1] & bits::winRank[OPP(toMove_)]){
    winner_ = OPP(toMove_);
    return;
  }
  //opp has no rabbit 
  if (! bitboard_[OPP(toMove_)][1]){
    winner_ = toMove_;
    return;
  }
  //player has no rabbit 
  if (! bitboard_[toMove_][1]){
    winner_ = OPP(toMove_);
    return;
  }
}

//--------------------------------------------------------------------- 

int Board::genStepsNoPass(player_t player, StepArray& steps) const
{
  int stepsNum = 0;
  u64 movable = calcMovable(player);

  //small opt ... prefill victims for piece types
  u64 victims[7];
  calcWeaker(player, victims);

  coord_t coord = BIT_LEN;
  while ( (coord = bits::lix(movable)) != -1) { 
    assert(getPlayer(coord) == player); 
    piece_t piece = getPiece(coord, player);
    genStepsOneTuned(coord, player, piece, steps, stepsNum, victims[piece]);
  }

  return stepsNum;
}

//--------------------------------------------------------------------- 

int Board::genSteps(player_t player, StepArray& steps) const
{
  int stepsNum = genStepsNoPass(player, steps);
  if ( stepCount_ >= 1 ){
    steps[stepsNum++] = Step(STEP_PASS, player);
  }
  return stepsNum;
}
   
//--------------------------------------------------------------------- 

bool Board::goalCheck(player_t player, int stepLimit, Move * move) const
{
  
  u64 rabbits = bitboard_[player][RABBIT];
  int from;
  while ( (from = bits::lix(rabbits)) != -1){
    
    u64 goals = bits::winRank[player] & bits::sphere(from, stepLimit);
    int to;
    while ( (to = bits::lix(goals)) != -1){
      if (reachability(from, to, player, stepLimit, 0, move) != -1){
        return true;
      }
    } 
    
  }
  return false;
}

//--------------------------------------------------------------------- 

bool Board::goalCheck(Move * move) const
{
  return goalCheck(toMove_, STEPS_IN_MOVE - stepCount_, move);
}

//---------------------------------------------------------------------

bool Board::trapCheck(player_t player, coord_t trap, int limit, MoveList* moves, SoldierList* soldiers) const
{
  assert(bits::isTrap(trap));

  u64 victims = bitboard_[player][0] & 
                bits::sphere(trap, limit/2); 
  int pos;
  bool found = false;
  for (int i = 2; i < limit + 1; i++){
    u64 victimsAct = victims & bits::sphere(trap, i/2); 
    while ((pos = bits::lix(victimsAct)) != BIT_EMPTY){
      //must be here !!! inside the cycle
      Move move;

      DEBUG_TRAPCHECK(cerr << "trapCheck " << 
       Soldier(player, getPiece(pos, player), pos).toString() << 
      " -> " << coordToStr(trap) << endl;
      cerr << "=================" << endl;);

      if (trapCheck(pos, getPiece(pos, player), player, trap, i, 0, &move)){
        found = true;
        DEBUG_TRAPCHECK(cerr << "FOUND KILL : " << endl << moveToStringWithKills(move) << endl;);
        victims ^= BIT_ON(pos);
        if (moves){
          moves->push_back(move);
        }
        if (soldiers) { 
          soldiers->push_back(Soldier(player, getPiece(pos, player), pos));
        }
      }
    }
  }
  
  return found; 
  
}

//--------------------------------------------------------------------- 

bool Board::trapCheck(player_t player, MoveList* moves, SoldierList* soldiers) const
{
  DEBUG_TRAPCHECK(cerr << "TRAP CHECK FOR PLAYER " << player << endl;);
  u64 t = TRAPS;
  coord_t trap;
  bool found = false;
  while ((trap = bits::lix(t)) != BIT_EMPTY){
    found = trapCheck(player, trap, STEPS_IN_MOVE, moves, soldiers) || found;
  }
  return found;
}

//--------------------------------------------------------------------- 

bool Board::trapCheck(coord_t vpos, piece_t piece, player_t player, 
                      coord_t trap, int limit, int used, Move* move) const
{

  //winner check 
  if (! bits::getBit(bitboard_[player][0], vpos)){
    if (vpos == trap){
      return true;
    } 
    //he might have been pushed to wrong trap
    return false;
  }
  assert(bits::isTrap(trap));
  //cutoff check 
  //guards - vpos not included 
  u64 guards = bits::neighborsOne(trap) & (bitboard_[player][0] ^ BIT_ON(vpos));
  int guardsNum = bits::neighborsOneNum(trap, guards);
  int d = SQUARE_DISTANCE(vpos, trap);
  int reserve = (limit - used) - 2 * (d + guardsNum);

  DEBUG_TRAPCHECK( cerr << "--------------------- " << endl;
  cerr.width(2 * used);
  cerr << "+" << "checking " 
      << Soldier(player, getPiece(vpos, player), vpos).toString() << " -> " 
        << coordToStr(trap) << " guadsnum " << guardsNum << " reserve " << reserve << endl;);
    
    if (reserve < 0){
      return false;
    }
    
    u64 stronger = strongerWithinDistance(player, piece, vpos, reserve + 1);
    if (vpos != trap && ! stronger) { 
      return false;
    }

    //optimize ... ? 
    StepArray steps; 

    //optimize reserve + 2 is too much waste 
    int distLimit = reserve + 2;

      
    u64 movable = calcMovable(OPP(player));
    u64 victims[7];
    calcWeaker(OPP(player), victims);

    u64 active = movable;

    if (vpos == trap) {
    //guard elimination 
    //this is dependant max on 4 steps right now !  
    assert(guardsNum == 1 || guardsNum == 2);
    if (guardsNum == 1) {
      //TODO optimize
      u64 g = guards;
      int guard = bits::lix(g);
      assert(bits::lix(g) == BIT_EMPTY && guard != BIT_EMPTY);
     
      active &= bits::sphere(guard, distLimit); 
      DEBUG_TRAPCHECK( cerr << "1 guard situation" << endl; 
        bits::print(cerr, active));
    }else {
      active &= bits::neighbors(guards);
      DEBUG_TRAPCHECK( cerr << "2 guards situation" << endl; 
        bits::print(cerr, active));
    }
  }else { 
   active &= bits::sphere(vpos, distLimit);
  }

  //active &= stronger; 

  int bit;
  while ( (bit = bits::lix(active)) != BIT_EMPTY) {

    int len = 0;
    //must act
    piece_t p = getPiece(bit, OPP(player));
    genStepsOneTuned(bit, OPP(player), p, steps, len, victims[p]);

    for (int j = 0; j < len; j++){
      if (! reserve && ! steps[j].isPushPull()) {
        continue;
      }
      Board* bb = new Board(*this);
      DEBUG_TRAPCHECK(cerr << "making step " << steps[j].toString() << endl);
      bb->makeStep(steps[j]);

      int newvpos = vpos;
      if (steps[j].oppFrom_ == vpos){
        newvpos = steps[j].oppTo_;
      }

      if (bb->trapCheck(newvpos, piece, player, trap, limit, 
                       used + steps[j].count(), move)){
        move->prependStep(steps[j]);
        delete(bb);
        return true;
      }
      delete(bb);
    }
  }
  return false; 
}

//--------------------------------------------------------------------- 

int Board::reachability(int from, int to, player_t player, int limit, int used, Move * move) const
{
  u64 movable = calcMovable(player);
  u64 victims[7];
  calcWeaker(player, victims);

  int reserve = limit - used - SQUARE_DISTANCE(from, to);
  //distance limit for pieces lookup
  //TODO optimize ! to + 1 and "referer from"
  int distLimit = reserve + 3;

  if (from == to){
    return used;
  }

  int pseudoReserve = reserve;
  //someone is blocking the to field 
  if (getPlayer(to) != NO_PLAYER){
    //might be friend
    pseudoReserve -= 1;
    //or opponent
    if (getPlayer(to) == OPP(player)){
      //generalize not only for rabbits
      pseudoReserve -= 1;
    }
  }

  if (pseudoReserve < 0 || getPlayer(from) == NO_PLAYER){
    //cerr << "PSEUDORESERVE/DEAD CUTOFF" << endl;
    return -1;
  }

  //cerr.width(2 * (used+1));
  //cerr << ' ';
  //cerr << "used " << used << " reserve " << reserve << endl;
  StepArray steps;
  //u64 passive = bits::BIT_ON(from);

  for (int i = 0; i <= distLimit; i ++){
    u64 iFarAway = bitboard_[player][0] & bits::circle(from, i);
                      
    u64 active = iFarAway & movable;
  /*
    u64 passive = passive & ~movable;
    for (int j = 0; j < i + 1; j ++){
      passive |= neighbors(passive);
    }
    */
    int bit;
    while ( (bit = bits::lix(active)) != -1){
      //cerr << "piece at " << bit << " is " << i << " far away" << endl;
      assert(IS_PLAYER(getPlayer(bit)));
      assert(player == GOLD || player == SILVER);
      int len = 0; 
      //genStepsOne(bit, player, steps, len);
      piece_t piece = getPiece(bit, player);
      genStepsOneTuned(bit, player, piece, steps, len, victims[piece]);

      for (int j = 0; j < len; j++){
       // cerr.width(2 * (used+1));
       // cerr << ' ';
       // cerr << "trying " << steps[j].toString() << endl;

        int newfrom = from;
        if (steps[j].from_ == from){
          newfrom = steps[j].to_;
        }
        //moving another piece => must have reserve
        if (from == newfrom && ! reserve){
           // cerr << "RESERVE CUTOFF" << endl;
            continue;
        }
        //new board - makestep - recurse 
        Board* bb = new Board(*this);
        bb->makeStep(steps[j]);
        int r = bb->reachability(newfrom, to, player, limit, 
                                  used + steps[j].count(), move);
        delete(bb);
        if (r != -1){
          //assert(steps[j].player_ == 0 || steps[j].player_ == 1);
          if (move != NULL){
            move->prependStep(steps[j]);
          }
          return r;
        }
      }
    }
  }
  return -1;

}

//--------------------------------------------------------------------- 

void Board::genStepsOne(coord_t coord, player_t player, 
                            StepArray& steps, int& stepsNum) const { 
  if (bits::getBit(calcMovable(player), coord)){
    u64 victims[7];
    calcWeaker(player, victims);

    assert(getPlayer(coord) == player); 
    piece_t piece = getPiece(coord, player);
    //TODO calling two functions here instead of one might make it (much) slower 
    genStepsOneTuned(coord, player, piece, steps, stepsNum, victims[piece]);
  }
}

//--------------------------------------------------------------------- 


//--------------------------------------------------------------------- 

void Board::genStepsOneTuned(coord_t from, player_t player, piece_t piece,
                                StepArray& steps, int & stepsNum, u64 victims) const
{
  assert(getPlayer(from) == player);
  assert(getPiece(from, player)  == piece);

  u64 empty = ~(bitboard_[0][0] | bitboard_[1][0]);
  u64 whereStep = empty & bits::stepOffset_[player][piece][from];
    
  //single steps
  for (int i = 0; i < 4; i++) {
    coord_t to = from + bdirection[i];
    if (whereStep & BIT_ON(to)) {
      steps[stepsNum++].setValues(STEP_SINGLE, player, piece, from, to);
    }
  }

  if ( piece == RABBIT || stepCount_ >= 3 || 
       ! (bits::stepOffset_[player][piece][from] & bitboard_[OPP(player)][0])) { 
    return;
  }

  victims &= bits::stepOffset_[player][piece][from];
   
  for (int i = 0; i < 4; i++) {
    coord_t victimFrom = from + bdirection[i];
    if (! (victims & BIT_ON(victimFrom))) {
      continue;
    }

    //pull
    u64 wherePull = empty & bits::stepOffset_[player][piece][from];          
    for (int j = 0; j < 4; j++) {
      coord_t pullerTo = from + bdirection[j];
      if (! (wherePull & BIT_ON(pullerTo))) {
        continue;
      }

      steps[stepsNum++].setValues(STEP_PULL, player, piece, from, pullerTo, 
                            getPiece(victimFrom, OPP(player)), victimFrom, from); 
    }

    //push
    u64 wherePush = empty & bits::stepOffset_[player][piece][victimFrom]; 
    for (int k = 0; k < 4; k++) {
      coord_t victimTo = victimFrom + bdirection[k];
      if (! (wherePush & BIT_ON(victimTo))) {
        continue;
      }

      steps[stepsNum++].setValues(STEP_PUSH, player, piece, from, victimFrom,
                        getPiece(victimFrom, OPP(player)), victimFrom, victimTo);
    }
  } 
};

string Board::bitboardToString(const Bitboard & bitb)
{
  stringstream ss;

  ss << " +-----------------+\n";

  string refStr(".123456rcdhmeRCDHME");
  for (int i = SIDE_SIZE - 1; i >= 0; i--) {
    ss << i + 1 <<"| ";
    for (int j = 0; j <  SIDE_SIZE; j++) {
      coord_t coord = i * SIDE_SIZE + j;
      player_t player = NO_PLAYER;
      piece_t piece = NO_PIECE;
      for (int k = 0; k <  2; k++) 
        for (int l = 1; l <  PIECE_NUM + 1; l++) {
			    if (bits::getBit(bitb[k][l], coord)){
            player = k;
            piece = l;
            break;
          }
        }
      assert(player == NO_PLAYER || 
                       piece + ((2 - player) * 6) <= int(refStr.length()));
			if (player == NO_PLAYER ){
			  if (bits::getBit(TRAPS, coord)){
				  ss << "X ";
        }
        else{
				  ss << ". ";
        }
      }
			else{
				ss << refStr[piece + ((2 - player) * 6)] << " ";
      }
    }
    ss << "| " << endl;
  }

  ss << " +-----------------+" << endl;
  ss << "   a b c d e f g h" << endl;

  return ss.str();
}
  

//--------------------------------------------------------------------- 

string Board::toString() const
{

	stringstream ss;

  ss << endl;
  ss << "Signature " << signature_ << endl;
  //ss << "Pre Signature " << preMoveScgnature_ << endl;
  ss << "Move ";

  if (toMove_ == GOLD) 
    ss << "g" ;
  else
    ss << "s" ;
  ss << moveCount_ << endl;
  ss << "Step " << stepCount_ << endl;

  assert(toMove_ == GOLD || toMove_ == SILVER );

  ss << bitboardToString(bitboard_);

	return ss.str();
} 

//--------------------------------------------------------------------- 

u64 Board::getSignature() const 
{
  return signature_;
}

//--------------------------------------------------------------------- 

player_t Board::getWinner() const 
{
  return winner_;
}

//--------------------------------------------------------------------- 

player_t Board::gameOver() const 
{
  return winner_ != NO_PLAYER;
}

//--------------------------------------------------------------------- 

player_t Board::getPlayerToMove() const
{
  return toMove_;
}

//--------------------------------------------------------------------- 

const Bitboard& Board::getBitboard() const
{
  return bitboard_;
}

//--------------------------------------------------------------------- 

u64 Board::calcMovable(player_t player) const
{
  u64 ngb[2]; 
  ngb[0] = bits::neighbors(bitboard_[0][0]);
  ngb[1] = bits::neighbors(bitboard_[1][0]);
  u64 stronger = bitboard_[OPP(player)][0];
  u64 movable = 0ULL; 
  
  for (int piece = 1; piece < 7; piece++) {
    movable |= bitboard_[player][piece];                           
    stronger ^= bitboard_[OPP(player)][piece];                              
    movable &= ngb[player] | (~bits::neighbors(stronger)); 
  }
  return movable;
}

//--------------------------------------------------------------------- 

void Board::calcWeaker(player_t player, u64 (&weaker)[7]) const
{
  weaker[1] = 0ULL;
  for (int i = 2; i < 7; i++){
    weaker[i] = weaker[i-1] | bitboard_[OPP(player)][i-1];
  }
}

//--------------------------------------------------------------------- 

void Board::setSquare(coord_t coord, player_t player, piece_t piece) 
{
  assert(player == GOLD || player == SILVER);
  assert(piece >= RABBIT && piece <= ELEPHANT );
  assert(coord >= 0 && coord < BIT_LEN );

  bitboard_[player][piece] |= BIT_ON(coord);
  bitboard_[player][0] |= BIT_ON(coord);

  signature_ ^= bits::zobrist[player][piece][coord]; 
}

//--------------------------------------------------------------------- 

void Board::delSquare(coord_t coord, player_t player)
{
  assert(bits::getBit(bitboard_[player][0], coord));
  bitboard_[player][0] ^= BIT_ON(coord);
  for (int i = 1; i < 7; i++ ){
    if (bits::getBit(bitboard_[player][i], coord)){
      bitboard_[player][i] ^= BIT_ON(coord);
      signature_ ^=  bits::zobrist[player][i][coord]; 
      return;
    }
  }
}

//--------------------------------------------------------------------- 

void Board::delSquare(coord_t coord, player_t player, piece_t piece) 
{
  assert(bits::getBit(bitboard_[player][0],coord));
  assert(bits::getBit(bitboard_[player][piece],coord));
  bitboard_[player][0] ^= BIT_ON(coord);
  bitboard_[player][piece] ^= BIT_ON(coord);
  signature_ ^=  bits::zobrist[player][piece][coord]; 
}

//--------------------------------------------------------------------- 

piece_t Board::getPiece(coord_t coord, player_t player) const
{
  assert(bits::getBit(bitboard_[player][0], coord));
  for (int i = 1; i < 7; i++ ){
    if (bits::getBit(bitboard_[player][i],coord)){
      return i;
    }
  } 
  assert(false);

  return NO_PIECE;
}

//--------------------------------------------------------------------- 

player_t Board::getPlayer(coord_t coord) const
{
  if (bits::getBit(bitboard_[GOLD][0], coord))
    return GOLD;

  if (bits::getBit(bitboard_[SILVER][0], coord))
    return SILVER;

  return NO_PLAYER;
}

//--------------------------------------------------------------------- 

int Board::strongerDistance(player_t player, piece_t piece, coord_t coord) const
{
  u64 stronger = 0ULL;
  for (int i = 1; i + piece < PIECE_NUM + 1; i++){
    stronger |= bitboard_[OPP(player)][piece + i];
  }
  if (! stronger){ 
    return BIT_EMPTY;
  }
  int radius = 1;
  while (! (stronger & bits::circle(coord, radius))){
    radius++;
  }
  return radius; 
     
}

//--------------------------------------------------------------------- 

u64 Board::strongerWithinDistance(player_t player, piece_t piece, 
                                  coord_t coord, int distance) const
{
  u64 stronger = 0ULL;
  for (int i = 1; i + piece < PIECE_NUM + 1; i++){
    stronger |= bitboard_[OPP(player)][piece + i];
  }
  return stronger & bits::sphere(coord, distance);
}

//--------------------------------------------------------------------- 

int Board::equalDistance(player_t player, piece_t piece, coord_t coord) const
{
  u64 stronger = bitboard_[OPP(player)][piece];
  if (! stronger){ 
    return BIT_EMPTY;
  }

  int radius = 1;
  while (! (stronger & bits::circle(coord, radius))){
    radius++;
  }
  return radius; 
}

//--------------------------------------------------------------------- 

player_t Board::strongestPieceOwner(u64 area) const
{
  for (int i = PIECE_NUM; i>= 0; i--){
    u64 g =  area & bitboard_[GOLD][i];
    u64 s =  area & bitboard_[SILVER][i];
    if (! (g | s)){
      continue;
    }
    return g ? (s ? NO_PLAYER : GOLD) : SILVER;
  }
  return NO_PLAYER;
  
}

//--------------------------------------------------------------------- 

piece_t Board::strongestPiece(player_t player, u64 area) const
{
  for (int i = PIECE_NUM; i>= 0; i--){
    if (bitboard_[player][i]){
      return i;
    }
  }
  return NO_PIECE;
  
}

//--------------------------------------------------------------------- 

u64 Board::weaker(player_t player, piece_t piece) const
{
  u64 res = 0ULL;
  for (int i = piece - 1; i > 0; i--){
    res |= bitboard_[player][i];  
  }
  return res;
}

//--------------------------------------------------------------------- 

bool Board::isMoveBeginning() const 
{
  return stepCount_ == 0;
}

//--------------------------------------------------------------------- 
// FROM OLD
//--------------------------------------------------------------------- 

void Board::initNewGame()
{
  init(true);
}

//--------------------------------------------------------------------- 

bool Board::initFromRecord(const char* fn, bool init_repetitions)
{

  init();
  
  string line;
  stringstream ss;
  string token;
  uint moveCount;

  FileRead* f = new FileRead(string(fn));
  if (! f->good()){
    return false;
  }

  while (f->getLine(line)){
    ss.clear();
    ss.str(line);

    ss >> token; 
    moveCount = str2int(token.substr(0, token.length() - 1));
    assert(moveCount == moveCount_);
    makeMove(getStreamRest(ss));
    if (init_repetitions) {
      thirdRep_->update( signature_, toMove_);
    }
  }

  return true;
} 

//--------------------------------------------------------------------- 

bool Board::operator== ( const Board& board) const
{
  return (signature_ == board.signature_ && moveCount_ == board.moveCount_ ) ;
}

//---------------------------------------------------------------------

void* Board::operator new(size_t size) {
  if (! glob.bpool()->empty()) {
    Board* b = glob.bpool()->top();
    glob.bpool()->pop();
    return b;
  } 
  return malloc(size);
}

//--------------------------------------------------------------------- 

void Board::operator delete(void* p) {
  glob.bpool()->push(static_cast<Board*> (p));
  //Board* b = static_cast<Board*>(p);
  //free(b);
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

  char c;
  string pom;
  string s = "";
  int moveCount;
  f >> moveCount;
  //side 
  f >> pom; s += pom;
  s += " [";
  
  f.ignore(1024,'\n'); //ignores the rest of initial line 
  f.ignore(1024,'\n'); //ignores the top of the border till EOF 

  for (int i = 8; i > 0; i--) { // do this for each of the 8 lines of the board
    f.ignore(2); //ignore trailing characters

    for (int j = 0; j < 8; j++) {
      f.ignore(1); //ignore a white space 
      c = f.get();
      s += c;
    } 
    f.ignore(1024,'\n'); //finish the line
  } 

  s += "]";
  f.close();
  bool res = initFromPositionCompactString(s);
  moveCount_ = moveCount;
  return res;
}

//---------------------------------------------------------------------

bool Board::initFromPositionCompactString(const string& s)
{
  stringstream ss;
  ss.str(s);
  init();
  char side;
//ss >> moveCount_; 
  ss >> side;
  toMove_ = sideCharToPlayer(side);
  if (! IS_PLAYER(toMove_)){
    return false;
  }
  string boardStr = getStreamRest(ss);
  
  //check format '[' .. 64 characters for position ... ']'
  if (boardStr[0] != '[' || boardStr[65] != ']') {
    return false;
  }
  uint k = 1;
  for (int i = 7; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      assert(k < boardStr.length());
      char c = boardStr[k++];
      if (c == ' ' || c=='X' || c=='x' || c=='.')
        continue;
      else {
        player_t player;
        piece_t piece = 0;
        if (! parsePieceChar(c, player, piece)){
          logError("Unknown character %c encountered while reading board at [%d, %d]\n", c, i, j);
          return false;
        }
        setSquare(8*i + j, player, piece);
        //return false;
      }
    }
  }

  afterPositionLoad();
  return true;
}

//--------------------------------------------------------------------- 

void Board::findMCmoveAndMake()
{

  if (bitboard_[toMove_][0] == 0){
    //TODO handle case when stepCount_ == 0 and player MUST move ? 
    makeStepTryCommit(Step(STEP_PASS, toMove_));
    return;
  }

  StepArray steps;

  Step step;
  intList p; 
  
  /*
  //area selection
  #define RADIUS 4
  u64 area = 0ULL;
  for (int i = 0; i < 2; i++){
    int start = grand() % BIT_LEN; 
    area |= bits::sphere(start, RADIUS);
  }
  //bits::print(cerr, area);
  area &= bitboard_[toMove_][0] & getRandomU64();
  //bits::print(cerr, area);
  
  for (int i = 0; i < 2; i++){
    int pos = bits::lix(area);
    if (pos == -1){
      break;
    }
    p.push_back(pos);
  }
  */
  
  for (int i = 0; i < BIT_LEN/2; i++){
    int pos = grand() % BIT_LEN;
    if (bits::getBit(bitboard_[toMove_][0], pos)){
      p.push_back(pos);
      if (p.size() >= 3) {
        break;
      }
    }
  }

  do { 
    //int len = 1;
    //steps[0] = Step(STEP_PASS, toMove_);
    int len = 0;

    for (intList::iterator it = p.begin(); it != p.end(); it++) { 
      if (getPlayer((*it)) != toMove_){ //might have fallen into trap
        continue;
      }
      assert(getPlayer((*it)) == toMove_);
      genStepsOne((*it), toMove_, steps, len);
    }

    if (len == 0){
      steps[len++] = Step(STEP_PASS, toMove_);
    }

    if (cfg.knowledgeInPlayout()){
      step = chooseStepWithKnowledge(steps, len);
    }
    else{
      step = steps[grand() % len];
    }
    if (! step.isPass()){
      p.remove(step.from_);
      p.push_back(step.to_);
    }
    //cerr << toString();
    //cerr << step.toString() << endl;
    //
  } while ( ! makeStepTryCommit(step));

}

//--------------------------------------------------------------------- 

void Board::getHeuristics(const StepArray& steps, uint stepsNum, HeurArray& heurs) const
{
  for (uint i = 0; i < stepsNum; i++){
    heurs[i] = eval_->evaluateStep(this, steps[i]); 
  }
}

//--------------------------------------------------------------------- 

void Board::init(bool newGame)
{

  assert(OPP(GOLD) == SILVER);
  assert(OPP(SILVER) == GOLD);

  //game initialization - done in the first run or when newgame is specified
  if (! classInit || newGame) {
    classInit = true;
    //bits::initZobrist();  
    bits::buildStepOffsets();
    thirdRep.clear();
    thirdRep_ = &thirdRep;
    eval_ = new Eval();
  }

  //clear bitboards
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 8; j++){
      bitboard_[i][j] = 0ULL;
    }

  toMove_    = GOLD;
  stepCount_ = 0;
  moveCount_ = 1;
  lastStep_  = Step();
  winner_    = NO_PLAYER;

  signature_ = 0;
  preMoveSignature_ = 0; 
  //signature is generated after the position is loaded
}

//---------------------------------------------------------------------

player_t Board::sideCharToPlayer(char side) const
{
  player_t player;
  if (! (side == 'w' || side == 'b' || side == 'g' || side =='s'))
    return NO_PLAYER;

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
}

//--------------------------------------------------------------------- 

void Board::makeSignature()
{
  signature_ = 0;
  player_t player;
  for (int i = 0; i < BIT_LEN; i++) {
    if ((player = getPlayer(i)) != NO_PLAYER){
      signature_ ^= bits::zobrist[player][getPiece(i, player)][i] ;
    }
  }
}

//---------------------------------------------------------------------

bool Board::checkKillForward(coord_t from, coord_t to, KillInfo* killInfo) const
{
  //optimize => put as a parameter
  player_t player = getPlayer(from);

  u64 trapped = BIT_ON(to) & TRAPS;
  
  //1) piece steps into the trap ( or is pushed/pulled in there ) 
  //check more than one neighbor (one is actual piece)
  if (trapped){
    if (! (bits::neighborsOne(to) & (BIT_ON(from) ^ bitboard_[player][0]))){
      if (killInfo != NULL){
        killInfo->setValues(player, getPiece(from, player), to); 
      }
      return true;
    }
    else { //piece was moved into the trap but hasn't died -> no kill
      return false;
    }
  }

  //2) piece is standing in the trap and his last supporter goes away
  trapped = bits::neighborsOne(from) & TRAPS  & bitboard_[player][0];
  if (trapped ){
    int trappedCoord = bits::lix(trapped);
    assert(trappedCoord != -1 && bits::lix(trapped) == -1);
    if (! (bits::neighborsOne(trappedCoord) & (BIT_ON(from) ^ bitboard_[player][0]))){
      if (killInfo != NULL){
        killInfo->setValues(player, getPiece(trappedCoord, player), trappedCoord);
      }
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------- 

void Board::makeMove(const string& moveRaw)
{
  string moveStr = trimLeft(trimRight(moveRaw));
  if (moveStr == ""){
    return; 
  }
  //cerr << "move str: " << moveStr << endl;
  makeMove(Move(moveStr));
}


//---------------------------------------------------------------------

void Board::makeMove(const Move& move)
{
  StepList stepList;
  stepList  = move.getStepList();
  //cerr << "making move " << move.toString() << endl;

  assert(stepList.size() <= STEPS_IN_MOVE || 
        (move.isOpening() && stepList.size() <= MAX_PIECES));
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    if (move.isOpening()){
      assert(moveCount_ == 1);
      setSquare(it->from_, it->player_, it->piece_);
    }else{
      assert(moveCount_ > 1);
      makeStep(*it);
    }
  
  }
  commit();
}

//--------------------------------------------------------------------- 

u64 Board::calcAfterStepSignature(const Step& step) const
{
  //if (step.stepType_ == STEP_PASS)
  //  return signature_;

  Board* bb = new Board(*this);
  bb->makeStep(step);
  u64 sig = bb->getSignature();
  delete bb;
  assert(! step.pieceMoved() || sig != signature_);
  return sig;
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
      steps[i] = steps[--stepsNum];
    else
      i++;  

  //check third time repetitions
  //this can be checked only for steps that finish the move
  //these are : pass, step_single for stepCount == 3, push/pull for stepCount == 2 
  
  i = 0;
  while (i < stepsNum){
    if (steps[i].stepType_ == STEP_PASS && stepIsThirdRepetition(steps[i])){
      steps[i] = steps[--stepsNum];
      break;
    }
    i++;
  }

  if ( stepCount_ < 2 ) 
    return stepsNum;

  i = 0;
  while (i < stepsNum)
    if ((stepCount_ == 3 || steps[i].isPushPull()) &&  
         stepIsThirdRepetition(steps[i])){
      steps[i] = steps[--stepsNum];
    } 
    else {
      i++;  
    }
  

  return stepsNum;
}

//---------------------------------------------------------------------

bool Board::stepIsVirtualPass(Step& step) const 
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
  assert(1 - step.getPlayer() == 
        getPlayerToMoveAfterStep(step));
  //check whether position with opponent to move won't be a repetition
  if ( thirdRep_->isThirdRep(afterStepSignature, 1 - step.getPlayer())) {
    return true;
  }
  return false;
  
}

//---------------------------------------------------------------------

bool Board::isSetupPhase() const
{
  return moveCount_ == 1 && ! bitboard_[toMove_][0];
}

//---------------------------------------------------------------------

uint Board::getStepCount() const
{
  return stepCount_;
}

//--------------------------------------------------------------------- 

uint Board::getStepCountLeft() const
{
  return STEPS_IN_MOVE - stepCount_;
}

//--------------------------------------------------------------------- 

u64 Board::getPreMoveSignature() const
{
  return preMoveSignature_; 
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

Step Board::chooseStepWithKnowledge(StepArray& steps, uint stepsNum) const
{
  assert(stepsNum > 0);
  uint bestIndex = stepsNum - 1;
  float bestEval = INT_MIN; 
  float eval = 0;
  //int r = smallRandomPrime();
  //int index;

  if ( cfg.knowledgeTournamentSize() == 0){
    for (uint i = 0; i < stepsNum; i++){
      //take only half of steps into account 
      //TODO This random01 call slows down a lot -> use smallRandomPrime tricks 
      //like: index = ((i+1)*r) % stepsNum;
      //to get pseudo random numbers
      if (random01() <= 0.5)
        continue;
      const Step& step = steps[i];
      eval = eval_->evaluateStep(this, step); 
      logDebug("%d/%d/", i, stepsNum);
      logDebug("%s %d | ", step.toString().c_str(), eval);
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = i;
      }
    }
    logDebug(steps[bestIndex].toString().c_str());
  }
  else {

    for (uint i = 0; i < (stepsNum/2 > cfg.knowledgeTournamentSize() ? cfg.knowledgeTournamentSize() : stepsNum/2); i++){
      uint r = grand() % stepsNum;
      const Step& step = steps[r];
      eval = eval_->evaluateStep(this, step); 
      //cerr << " eval: " << step.toString() << " " << eval;
      if (eval > bestEval){
        bestEval = eval;
        bestIndex = r;
      } 
      else
      //TODO improve ! - roulette ? 
      if ( eval == bestEval && random01() > 0.5) { 
        bestIndex = r;
      }

    }
  }
  assert(bestIndex >= 0 && bestIndex < stepsNum);
  return steps[bestIndex];
}


//--------------------------------------------------------------------- 
//---------------------------------------------------------------------
