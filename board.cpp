
/** 
 *  @file board.cpp
 *  @brief Board implementation. 
 *  @full Integer board, able to evaluate itself, compute it's hash, etc. 
 */

#include "board.h"

// zobrist base table for signature creating 
u64  Board::zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     

//thirdRepetition class
ThirdRep*  Board::thirdRep_;

// switch to know when to init static variables in class Board
bool Board::classInit = false;

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int trap[4]={33,36,63,66};

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
//  section Step
//---------------------------------------------------------------------

/* this constructor is mainly used for 
 * step_no_step or step_pass which don't use other values than stepType */
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

/* returns true if any piece was moved
 * i.e. returns false if pass or no_step */
bool Step::pieceMoved() const 
{
  return (! (stepType_ == STEP_PASS || stepType_ == STEP_NO_STEP));
}

//---------------------------------------------------------------------

bool Step::operator== ( const Step& other) const
{
  if  ( stepType_ == other.stepType_ ){ // necessary condition ! 
    if ( stepType_ == STEP_PASS || stepType_ == STEP_NO_STEP ) 
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

string Step::toString() const
{
  stringstream ss;
  ss.str(""); 

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
    case STEP_NO_STEP:
      ss << "NO_STEP";  
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

  s << pieceRefStr[piece + 6 * PLAYER_TO_INDEX(player)] << columnRefStr[from % 10 - 1] << from / 10; 
  switch (to - from)
  {
    case NORTH : s << "n"; break;
    case WEST :  s << "w"; break;
    case EAST :  s << "e"; break;
    case SOUTH : s << "s"; break;
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

Step Board::findStepToPlay() 
{
  uint playerIndex = PLAYER_TO_INDEX(toMove_);
  Step step;

  //it's not possible to have 0 rabbits in the beginning of move
  assert( rabbitsNum[toMoveIndex_] > 0 || stepCount_ > 0); 

  //no piece for player to move 
  if (pieceArray[toMoveIndex_].getLen() == 0) {
    //step_pass since the player with no pieces still might win 
    //if he managed to kill opponent's last rabbit before he lost his last piece
    return Step(STEP_PASS, toMove_);          
  }
  //if (findRandomStep(step))
  //  return step;

  // STEP_PASS is always last move generated in generateAllSteps 
  // it can be simply removed to get "no-pass" move 
  stepArrayLen[playerIndex] = generateAllSteps(toMove_, stepArray[playerIndex]);
  uint len = stepArrayLen[playerIndex];

  assert(stepArrayLen[playerIndex] < MAX_STEPS);

  if (len == 0 ){ //player to move has no step to play - not even pass
    winner_ = OPP(toMove_);
    return Step(STEP_NO_STEP,toMove_); 
  }

  assert(len > 0);
  uint index = rand() % len;
  assert( index >= 0 && index < len );

  return( stepArray[playerIndex][index]); 
} 

//--------------------------------------------------------------------- 

void Board::init(bool newGame)
{

  stepArrayLen[0] = 0;
  stepArrayLen[1] = 0;

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
  winner_    = EMPTY;
  //moveCount_ = 0;
  //stepCount_ = 0;

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

        if (c == ' ' || c=='X' || c=='x')
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

void Board::makeStep(Step& step)
{

  int playerIndex = PLAYER_TO_INDEX(step.player_);
  if (step.stepType_ == STEP_NO_STEP)
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

  for ( int i = 0; i < 10; i++){ 
    assert(pieceArray[toMoveIndex_].getLen() != 0);
    step.from_ = pieceArray[toMoveIndex_][rand() % pieceArray[toMoveIndex_].getLen()];

    assert(IS_PLAYER(board_[step.from_]));

    if ( ! isFrozen(step.from_)){
      step.to_ = step.from_ + direction[rand() % 4];
      if (stepCount_ < 3 && PIECE(board_[step.from_]) != PIECE_RABBIT){
        step.stepType_ = (rand() % 3) + 1;
      }
      else 
        step.stepType_ = STEP_SINGLE;

      assert(step.stepType_ == STEP_SINGLE || step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL);

      switch (step.stepType_){
        case STEP_SINGLE:
           if ( board_[step.to_] == EMPTY_SQUARE && 
                (PIECE(board_[step.from_]) != PIECE_RABBIT ||   //rabbits cannot backwards
                ((toMove_ == GOLD && step.to_ - step.from_ != SOUTH) || (toMove_ == SILVER && step.to_ - step.from_ != NORTH))))  {
             found = true; //generate the step 
           }
          break;
        case STEP_PUSH: 
          if ( OWNER(board_[step.to_]) == OPP(OWNER(board_[step.from_])) &&
               PIECE(board_[step.to_]) <  PIECE(board_[step.from_])){
            assert(IS_PLAYER(board_[step.to_]));
            step.oppTo_ = step.to_ + direction[rand() % 4];    //todo - exclude "self" direction
            if (board_[step.oppTo_] == EMPTY_SQUARE){  //generate the step
              step.oppFrom_ = step.to_; 
              found = true;
            }
          }
          break; 
        case STEP_PULL: 
          if (board_[step.to_] == EMPTY_SQUARE){  
            step.oppFrom_ = step.from_ + direction[rand() % 4];    //todo - exclude "to" direction
            if ( OWNER(board_[step.oppFrom_]) == OPP(OWNER(board_[step.from_])) &&
                 PIECE(board_[step.oppFrom_]) <  PIECE(board_[step.from_])){
              step.oppTo_ = step.from_; 
              found = true;
            }
          }
        break;
      }

      if (found){
        step.player_ = toMove_;
        step.piece_  = PIECE(board_[step.from_]);
        assert(OWNER(board_[step.from_]) == toMove_);

        if (step.stepType_ != STEP_SINGLE) {
          assert(OWNER(board_[step.oppFrom_]) == OPP(toMove_)); 
          step.oppPiece_  = PIECE(board_[step.oppFrom_]);
        }


        return true;
      }
    }
  }
    return false;
}

//--------------------------------------------------------------------- 

bool Board::checkKillForward(square_t from, square_t to, KillInfo* killInfo) const
{

  if ( IS_TRAP(to) ) { //piece steps into the trap ( or is pushed/pulled in there ) 
    if ( ! hasTwoFriends(to, OWNER(board_[from])) ) { 
      killInfo->setValues(OWNER(board_[from]), PIECE(board_[from]), to); //activates as well
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
        killInfo->setValues(OWNER(board_[actTrapPos]), PIECE(board_[actTrapPos]), actTrapPos);                   
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

bool Board::makeStepTryCommitMove(Step& step) 
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
  //logDebug("Commiting move %d from player %d", moveCount_, toMove_);
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
    if ( ! IS_TRAP(step.oppTo_) || hasTwoFriends(step.oppTo_, OPP(toMove_)) ){              //if not killed in the trap
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
  for (int j = 0; j < checkTrapNum; j++)
    for (int i = 0; i < 4; i++){
      actTrap = checkTrap[j] + direction[i];
      if ( IS_TRAP(actTrap) && board_[actTrap] != EMPTY_SQUARE && ! hasTwoFriends(actTrap, OWNER(board_[actTrap])) )
        newSignature ^= zobrist[PLAYER_TO_INDEX(OWNER(board_[actTrap]))][PIECE(board_[actTrap])][actTrap];      //erase 
    } 

  return newSignature;
}

//---------------------------------------------------------------------

int Board::generateAllSteps(player_t player, StepArray& stepArray) const
{
  int stepsNum;
  int i,j;
  int square;
  
  stepsNum = 0;
  for (uint index =0 ; index < pieceArray[PLAYER_TO_INDEX(player)].getLen(); index++) { //go through pieces of player 
    square = pieceArray[PLAYER_TO_INDEX(player)][index];
    assert(OWNER(board_[square]) == player); 

    if ( isFrozen(square))  //frozen
      continue; 

    //generate push/pull moves
    if (stepCount_ < 3) {    
      for (i = 0; i < 4; i++) {  
        if (OWNER(board_[square + direction[i]]) == OPP(player) 
            && PIECE(board_[square + direction[i]]) < PIECE(board_[square])){ //weaker enemy
          for (j=0; j<4; j++)  // pull
            if (board_[square + direction[j]] == EMPTY_SQUARE) { //create move
                stepArray[stepsNum++].setValues( STEP_PULL, player,PIECE(board_[square]), square, 
                      square + direction[j], PIECE(board_[square + direction[i]]), square + direction[i], square);
            }
          for (j=0; j<4; j++)  //push
            if (board_[square + direction[i] + direction[j]] == EMPTY_SQUARE) { //create move
                stepArray[stepsNum++].setValues( STEP_PUSH, player, PIECE(board_[square]), square, 
                      square + direction[i], PIECE(board_[square + direction[i]]),
                      square + direction[i], square + direction[i] + direction[j]);
          }
        } //if weaker enemy
      } //for
    } 

    // generate single moves
    for (i=0; i<4; i++) // check each direction
      if (board_[square + direction[i]] == EMPTY_SQUARE)  {
        if (PIECE(board_[square]) == PIECE_RABBIT){ // rabbit cannot backwards
          if (OWNER(board_[square]) == GOLD && direction[i] == SOUTH)
            continue;
          if (OWNER(board_[square]) == SILVER && direction[i] == NORTH)
            continue;
        }
        //create move
        stepArray[stepsNum++].setValues(STEP_SINGLE, player, PIECE(board_[square]), 
                                        square, square + direction[i]);
      }
  } //for pieces on board

  //add step pass, if it's legal
  //other methods (filterRepetitions) are relying on fact, 
  //that stepPass is listed as a last one ! 
  if (stepCount_ > 0 )
    stepArray[stepsNum++] = Step(STEP_PASS, player);
  
  return stepsNum;
}

//---------------------------------------------------------------------

int Board::filterRepetitions(StepArray& stepArray, int stepsNum) const 
{
  // Careful - presumes that if there is a pass move 
  // then it is not pass at stepCount == 0 and this pass move is in the very end ! 

  //third time repetition check for pass move 
  //it is presumed, that pass move is in the very end ! TODO - cancel the presumption ? 
  if ( stepsNum > 0 && stepArray[stepsNum - 1].isPass() && 
      stepIsThirdRepetition(stepArray[stepsNum - 1])) 
    stepsNum--;

  //check virtual passes ( immediate repetetitions ) 
  //these might be checked and pruned even if the move is not over yet 
  //e.g. (Eg5n) (Eg5s) is pruned 
  
  int i = 0;
  while (i < stepsNum) 
    if (stepIsVirtualPass(stepArray[i]))
      stepArray[i] = stepArray[stepsNum-- - 1];
    else
      i++;  

  //check third time repetitions
  //this can be checked only for steps that finish the move
  //these are : pass, step_single for stepCount == 3, push/pull for stepCount == 2 

  if ( stepCount_ < 2 ) 
    return stepsNum;

  i = 0;
  while (i < stepsNum)
    if ((stepCount_ == 3 || stepArray[i].isPushPull()) &&  stepIsThirdRepetition(stepArray[i]))
      stepArray[i] = stepArray[stepsNum-- - 1];
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

bool Board::stepIsThirdRepetition( Step& step ) const 
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

/*returns number of all steps for given player - not index but player ! GOLD/SILVER */
uint Board::getAllStepsNum(player_t player) const
{
  return stepArrayLen[PLAYER_TO_INDEX(player)];
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
      ss << endl << "Potential moves for SILVER: " << getAllStepsNum(SILVER);
    else
      ss << endl << "Potential moves for GOLD: " << getAllStepsNum(GOLD);

    for ( uint i = 0; i < stepArrayLen[playerIndex]; i++){
      //assert(ss.str().find(stepArray[playerIndex].getElem(i).toString()) == string::npos);  
      ss << stepArray[playerIndex][i].toString();
    }
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

void Board::dumpAllSteps() const
{
  logRaw(allStepsToString().c_str());
}

//---------------------------------------------------------------------

void Board::testPieceArray()
{
  for (uint playerIndex = 0; playerIndex < 2; playerIndex++){
    cerr << endl << "player " << playerIndex << "("<< pieceArray[playerIndex].getLen()<<"):";
    for(uint i = 0; i < pieceArray[playerIndex].getLen(); i++){
      assert( OWNER(board_[pieceArray[playerIndex][i]] ) == INDEX_TO_PLAYER(playerIndex)); 
      cerr << pieceArray[playerIndex][i] << " ";
    }
  }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
