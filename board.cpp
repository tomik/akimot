
/** 
 *  @file board.cpp
 *  @brief Board implementation. 
 *  @full Integer board, able to evaluate itself, compute it's hash, etc. 
 */

#include "board.h"

// zobrist base table for signature creating 
u64  Board::zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     
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

player_t Step::getStepPlayer() const 
{
  return player_;
}

//---------------------------------------------------------------------

bool Step::isPass() const
{
  return stepType_ == STEP_PASS;
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

const string Step::oneSteptoString(player_t player, piece_t piece, square_t from, square_t to) const
{
  stringstream s;
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
  }
  s << " ";
  return s.str();
}

//---------------------------------------------------------------------

const string Step::toString(bool resultPrint) const
{
  stringstream ss;
  ss.clear(); 

  if ( ! resultPrint)
    ss << "(";

  switch (stepType_) {
    case STEP_PASS: 
      if ( ! resultPrint ) 
        ss << "pass";     
      break;
    case STEP_SINGLE: 
      ss << oneSteptoString(player_, piece_, from_, to_) ; 
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

  }
  if ( ! resultPrint ) 
    ss << ") ";

  return ss.str();
}

//---------------------------------------------------------------------

void Step::dump()
{
  log_() << toString();
}

//---------------------------------------------------------------------
//  section Board
//---------------------------------------------------------------------

Board::Board()
{
}

//---------------------------------------------------------------------

bool Board::init(const char* fn)
{

  stepArrayLen[0] = 0;
  stepArrayLen[1] = 0;

  assert(OPP(GOLD) == SILVER);
  assert(OPP(SILVER) == GOLD);

  //this is the only place in program to call initZobrist ... 
  //this if is fullfilled only once - when static member classInit is false ( initialized this way ) 
  if (! classInit ) {
    initZobrist();  
    classInit = true;
  }

  for (int i = 0; i < 100; i++)  
    board_[i] = OFF_BOARD_SQUARE;

  for (int i = 1; i < 9; i++)
    for (int j = 1; j < 9; j++){
      board_[10*i+j]       = EMPTY_SQUARE;
      frozenBoard_[10*i+j] = false;           //implicitly we consider everything not frozen
    }

  toMove_    = GOLD;
  stepCount_ = 0;
  moveCount_ = 1;
  winner_    = EMPTY;


  fstream f;
  char side;
  char c;
  
  try { 
    f.open(fn,fstream::in);

    if (! f.good()){
      f.close();
       return false;
     }

    f >> moveCount_; 
    f >> side;
    f.ignore(1024,'\n'); //ignores the rest of initial line 

    if (side == 'w')
      toMove_ = GOLD;
    else {
      toMove_ = SILVER;
    }
    toMoveIndex_ = PLAYER_TO_INDEX(toMove_);

    f.ignore(1024,'\n'); //ignores the top of the border till EOF 

    for (int i = 1; i < 9; i++) { // do this for each of the 8 lines of the board
      f.ignore(2); //ignore trailing characters

      for (int j = 1; j < 9; j++) {
        f.ignore(1); //ignore a white space 
         c=f.get();

         switch(c) {
           case 'E' : board_[i*10+j] = (GOLD | PIECE_ELEPHANT);  break;
           case 'M' : board_[i*10+j] = (GOLD | PIECE_CAMEL);   break;
           case 'H' : board_[i*10+j] = (GOLD | PIECE_HORSE);   break;
           case 'D' : board_[i*10+j] = (GOLD | PIECE_DOG);    break;
           case 'C' : board_[i*10+j] = (GOLD | PIECE_CAT);    break;
           case 'R' : board_[i*10+j] = (GOLD | PIECE_RABBIT);   break;
           case 'e' : board_[i*10+j] = (SILVER | PIECE_ELEPHANT); break;
           case 'm' : board_[i*10+j] = (SILVER | PIECE_CAMEL);  break;
           case 'h' : board_[i*10+j] = (SILVER | PIECE_HORSE);  break;
           case 'd' : board_[i*10+j] = (SILVER | PIECE_DOG);   break;
           case 'c' : board_[i*10+j] = (SILVER | PIECE_CAT);   break;
           case 'r' : board_[i*10+j] = (SILVER | PIECE_RABBIT);  break;
           case ' ' : case 'X' : case 'x': board_[i*10+j] = (EMPTY_SQUARE); break;
           default :
           log_() << "Unknown character " << c << " encountered while reading board at [" << i << "," << j << "].\n";
           f.close();
           return false;
           break;
         }
      } //for
      f.ignore(1024,'\n'); //finish the line
    } //for 
      f.close();
  } //try
  catch(int e) {
    return false; //initialization from file failed
  }

  makeSignature();    //(unique) position identification
  preMoveSignature = signature;

  //init pieceArray and rabbitsNum
  rabbitsNum[0] = 0;
  rabbitsNum[1] = 0;
  for (int square = 11; square < 89; square++){
    if (IS_PLAYER(board_[square]))
      pieceArray[PLAYER_TO_INDEX(OWNER(board_[square]))].add(square);
    if (PIECE(board_[square]) == PIECE_RABBIT)
      rabbitsNum[PLAYER_TO_INDEX(OWNER(board_[square]))]++;

  }

  assert(PLAYER_TO_INDEX(GOLD) == 0 && PLAYER_TO_INDEX(SILVER) == 1);

  return true;
}

//---------------------------------------------------------------------

void Board::initZobrist() const
{
   for (int i = 0; i < PLAYER_NUM; i++)
    for (int j = 0; j < PIECE_NUM; j++)
      for (int k = 0; k < SQUARE_NUM; k++)
        zobrist[i][j][k] = (((u64) random()) << 40) ^ 
                           (((u64) random()) << 20) ^ 
                           ((u64) random() );
}

//---------------------------------------------------------------------

void Board::makeSignature()
{
  signature = 0;
  for (int i = 0; i < 100; i++)  
    if IS_PLAYER(board_[i])
      signature ^= zobrist[PLAYER_TO_INDEX(OWNER(board_[i]))][PIECE(board_[i])][i] ;
}

//---------------------------------------------------------------------

Step Board::findStepToPlay() 
{
  uint playerIndex = PLAYER_TO_INDEX(toMove_);
  Step step;

  assert( rabbitsNum[toMoveIndex_] > 0 || stepCount_ > 0); //it's not possible to have 0 rabbits in the beginning of move

  if (pieceArray[toMoveIndex_].getLen() == 0) //no piece for player to move 
    return Step(STEP_PASS, toMove_);          //step_pass since the player with no pieces still might win 
                                              //if he managed to kill opponent's last rabbit before he lost his last piece
  //if (findRandomStep(step))
  //  return step;

  /* STEP_PASS is always last move generated in generateAllSteps 
   * it can be simply removed to get "no-pass" move */
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

  /*check step reasonability
  for (int i = 0; i < 20; i++){
    index = rand() % len;
    step = stepArray[playerIndex][index]; 
    
    //avoid suicide
    if (  IS_TRAP(step.to_) ){
      bool suicide = true;
      for(int i = 0; i < 4; i++)
        if (OWNER(board_[step.to_ + direction[i]]) == step.player_ && step.to_ + direction[i] != step.from_ )
          suicide = false;

      if ( suicide )
          continue;
    //TODO - rabbits move forward, prefer push/pulls, ...
    }
  
  }

  */

  return( stepArray[playerIndex][index]); 
} 

//---------------------------------------------------------------------

bool Board::findRandomStep(Step& step) const
{
  
  bool found = false; //once set to true, move is generated and returned 

  for ( int i = 0; i < 30; i++){ 
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

void Board::makeStep(Step& step)
{

  int checkTrap[2];         //squares in whose vicinity we check the traps
  int checkTrapNum = 1;
  int actTrapPos;

    if (step.stepType_ == STEP_NO_STEP)
      return;

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

    //in STEP_PUSH victim's move must be made first ! 
    if (step.stepType_ == STEP_PUSH ){  
      assert( stepCount_ < 3 ); 
      setSquare( step.oppTo_, OPP(toMove_), step.oppPiece_);
      clearSquare(step.oppFrom_);
      stepCount_++;
      checkTrap[1] = step.oppFrom_;
      checkTrapNum=2;
    }

    //update board after single step
     setSquare( step.to_, toMove_, step.piece_);
     clearSquare(step.from_);
     checkTrap[0] = step.from_;
     stepCount_++;
   
     pieceArray[toMoveIndex_].del(step.from_);
     pieceArray[toMoveIndex_].add(step.to_);


    //pull steps
    if (step.stepType_ == STEP_PULL) {  
      assert( stepCount_ < 4 ); 
      setSquare( step.oppTo_, OPP(toMove_), step.oppPiece_);
      clearSquare(step.oppFrom_);
      stepCount_++;
      checkTrap[1] = step.oppFrom_;
      checkTrapNum=2;
    }

    if (step.stepType_ != STEP_SINGLE) {  
     pieceArray[1 - toMoveIndex_].del(step.oppFrom_);
     pieceArray[1 - toMoveIndex_].add(step.oppTo_);
    }

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
    for (int j = 0; j < checkTrapNum; j++)
      for (int i = 0; i < 4; i++){
        actTrapPos = checkTrap[j] + direction[i];
        if ( IS_TRAP(actTrapPos) ){    
          if ( board_[actTrapPos] != EMPTY_SQUARE && ! hasFriend(actTrapPos) ){
            //trap is not empty and piece in there has no friends around => KILL
            if (PIECE(board_[actTrapPos]) == PIECE_RABBIT)              //update rabbitsNum - for gameEnd check 
              rabbitsNum[PLAYER_TO_INDEX(OWNER(board_[actTrapPos]))]--;
            board_[actTrapPos] = EMPTY_SQUARE; 
           if ( j == 0 ) 
             pieceArray[toMoveIndex_].del(actTrapPos);
           else
             pieceArray[1 - toMoveIndex_].del(actTrapPos);
          }
          break;
        }
      }
}

//---------------------------------------------------------------------

void Board::makeStepTryCommitMove(Step& step) {
  makeStep(step);
	if (stepCount_ >= 4 || ! step.pieceMoved()) 
    commitMove();
}

//---------------------------------------------------------------------

void Board::commitMove()
{
  //winner might be set already ( when player to move has no move ) 
  for (int i = 11; i <= 19; i++)
    if (board_[i] == (GOLD | PIECE_RABBIT) )
      winner_ = GOLD;
  for (int i = 81; i <= 88; i++)
    if (board_[i] == (SILVER | PIECE_RABBIT) )
      winner_ = SILVER;

  if (rabbitsNum[toMoveIndex_] == 0)  //if player lost his last rabbit in his move - he loses ... 
    winner_ = OPP(toMove_);
  if (rabbitsNum[1 - toMoveIndex_] == 0)  //unless the other also lost his last rabbit 
    winner_ = toMove_;

  #ifdef DEBUG_3
    log_() << "COMITTING MOVE" <<endl;
    log_() << "TO MOVE: " << toMove_ << endl;
    log_() << "move count: " << moveCount_ << endl;
  #endif

  if (toMove_ == SILVER) 
    moveCount_++;
  else 
    assert(toMove_ == GOLD);

  toMove_ = OPP(toMove_);
  toMoveIndex_ = 1 - toMoveIndex_;
  assert(toMoveIndex_ == PLAYER_TO_INDEX(toMove_));
  stepCount_ = 0;

  //preMoveSignature of the next move is the actual signature
  preMoveSignature = signature;
}

//---------------------------------------------------------------------

u64 Board::calcAfterStepSignature(Step& step) const
{
  if (step.stepType_ == STEP_PASS)
    return signature;

  assert(step.stepType_ == STEP_PUSH || step.stepType_ == STEP_PULL || step.stepType_ == STEP_SINGLE );

  int checkTrap[2];         //squares in whose vicinity we check the traps
  int checkTrapNum = 0;

  u64 newSignature = signature;

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
        stepArray[stepsNum++].setValues(STEP_SINGLE, player, PIECE(board_[square]),square, square + direction[i]);
      }
  } //for pieces on board

  //add step pass, if it's legal
  //other methods ( filterRepetitions ) are relying on fact, that stepPass is listed as a last one ! 
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
  if ( stepsNum > 0 && stepArray[stepsNum - 1].isPass() && stepIsThirdRepetition(stepArray[stepsNum - 1])) 
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
  if (afterStepSignature == preMoveSignature) 
    return true;
  return false;
  
}

//---------------------------------------------------------------------

bool Board::stepIsThirdRepetition( Step& step ) const 
{
  //TODO check afterStepSignature is in 3rd repetition table ! 
  //u64 afterStepSignature = calcAfterStepSignature(step);
  //if (afterStepSignature == preMoveSignature) 
  //  return false;
  return false;
  
}

//---------------------------------------------------------------------

/* checks whether piece at given square has adjacent friendly pieces*/ 
bool Board::hasFriend(square_t square) const 
{ 
  uint owner = OWNER(board_[square]); 
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == owner)
      return true;

  return false;
}

//---------------------------------------------------------------------

/* checks whether piece at given square has at least 2 adjacent friendly pieces
 * this function is good for determination of position signature after one step - see calcAfterStepSignature */ 
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

/* checks whether piece at given square has adjacent stronger enemy pieces*/
bool Board::hasStrongerEnemy(square_t square) const
{
  uint owner = OWNER(board_[square]);
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == OPP(owner) && 
        PIECE(board_[square + direction[i]]) > PIECE(board_[square]))
      return true;

  return false;
  
}

//---------------------------------------------------------------------

/*checks whether piece at given square is frozen == !combination of hasFriend and hasStrongerEnemy
 * optimize: actually code the function, not use the hasFriend,hasStrongerEnemy */
bool Board::isFrozen(square_t square) const
{
  return (!hasFriend(square) && hasStrongerEnemy(square)); 
}

//---------------------------------------------------------------------

bool Board::isEmpty() 
  /* this check is used in the beginning for pieces positioning*/
{
 return moveCount_ == 1; //TODO ... really check whether the board is empty 
}

//---------------------------------------------------------------------

/*returns number of all steps for given player - not index but player ! GOLD/SILVER */
uint Board::getAllStepsNum(player_t player)
{
  return stepArrayLen[PLAYER_TO_INDEX(player)];
}

//---------------------------------------------------------------------

uint Board::getStepCount() 
{
  return stepCount_;
}

//---------------------------------------------------------------------

int Board::getPreMoveSignature()
{
  return preMoveSignature; 
}

//---------------------------------------------------------------------

player_t Board::getPlayerToMove() 
{
 return toMove_;
}

//---------------------------------------------------------------------

player_t Board::getWinner()
{
  return winner_;
}

//---------------------------------------------------------------------

/*places piece at given position and updates signature*/
void Board::setSquare( square_t square, player_t player, piece_t piece)  
{
  signature ^=  zobrist[PLAYER_TO_INDEX(player)][piece][square]; 
  board_[square] = ( player | piece ); 
}

//---------------------------------------------------------------------

/*removes piece from given position and updates signature*/ 
void Board::clearSquare( square_t square) 
{
  signature ^=  zobrist[PLAYER_TO_INDEX(OWNER(board_[square]))][PIECE(board_[square])][square]; 
  board_[square] = EMPTY_SQUARE; 
} 

//---------------------------------------------------------------------

string Board::toString() 
{

  stringstream ss;

  
  ss << endl;


  //print zobrist
  /*
  for (int i = 0; i < PLAYER_NUM; i++)
   for (int j = 0; j < PIECE_NUM; j++)
     for (int k = 0; k < SQUARE_NUM; k++)
        ss << zobrist[i][j][k] << endl;
  */


  //ss << "board at: " << this << endl; //pointer debug
  ss << "Move " << moveCount_  << ", Step " << stepCount_ << ", " << endl;
  ss << "Signature " << signature << endl;

  if (toMove_ == GOLD) 
    ss << "Gold to move." << endl;
  else
    ss << "Silver to move." << endl;
  assert(toMove_ == GOLD || toMove_ == SILVER );

  ss << " +-----------------+\n";

  for (int i=1; i<9; i++) {
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

string Board::allStepsToString()
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

void Board::dump()
{
  log_() << toString();
}

//---------------------------------------------------------------------

void Board::dumpAllSteps()
{
  log_() << allStepsToString(); 
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
