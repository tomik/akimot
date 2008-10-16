
#include "board.h"

/*! \file board.cpp
 *  \brief Most important ! 
 */

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int trap[4]={33,36,63,66};



PieceArray::PieceArray()
{
  len = 0;
}

void PieceArray::add(square_t elem)
{
  elems[len++] = elem;
  assert(MAX_PIECES >= len);
}

void PieceArray::del(square_t elem)
{

  for (uint i = 0; i < len;i++)
    if (elems[i] == elem){
      elems[i] = elems[--len];
      return;
    }
  assert(false);
}

 
uint PieceArray::getLen() const
{
  return len;
}

 
square_t PieceArray::operator[](uint index) const
{
  assert( index >= 0 && index < len );
  return elems[index];
}


Step::Step( stepType_t stepType )
  /*this constructor is mainly used for 
   *step_no_step or step_pass which don't use other values than stepType*/
{
  stepType_ = stepType;
}


bool Step::pieceMoved() 
  /* returns true if any piece was moved
   * i.e. returns false if pass or no_step */
{
  return (! (stepType_ == STEP_PASS || stepType_ == STEP_NO_STEP));
}

Step::Step( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to){
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}

void Step::setValues( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}

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


const string Step::oneSteptoString(player_t player, piece_t piece, square_t from, square_t to) const
  /**prints step string for given values */
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


void Step::dump()
{
  log_() << toString();
}


const string Step::toString() const
{
  stringstream ss;
  ss.clear(); 

  ss << "(";
  switch (stepType_) {
    case STEP_PASS: 
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
  ss << ") ";

  return ss.str();
}


bool Step::operator== ( const Step& other)
  /* optimize: just return equality of all values - but MAKE SURE that unused values ( like oppTo_ are set to 0  )*/
{
  if  ( stepType_ == other.stepType_ ){ //necessary condition ! 
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


void Board::dumpAllSteps()
{
  log_() << allStepsToString(); 
}


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

bool Board::isEmpty() 
  /* this check is used in the beginning for pieces positioning*/
{
 return moveCount_ == 1; //TODO ... really check whether the board is empty 
}

player_t Board::getGenerateAllCount() 
{
 return generateAllCount;
}

player_t Board::getPlayerToMove() 
{
 return toMove_;
}



/*calls makeStep and then tries to commit move*/
void Board::makeStepTryCommit(Step& step) {
  makeStep(step);
	if (stepCount_ >= 4 || ! step.pieceMoved()) 
    commitMove();
}

void Board::makeStep(Step& step){

  #ifdef DEBUG_3
    log_() << endl <<  "=== PLAYING STEP: " << step.toString() << endl;
  #endif 

  int checkTrap[2];         //squares in whose vicinity we check the traps
  int trapped[2] = {0,0} ;
  int checkTrapNum = 1;

    if (step.stepType_ == STEP_NO_STEP)
      return;

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

    //in STEP_PUSH victim's move must be made first ! 
    if (step.stepType_ == STEP_PUSH ){  
      assert( stepCount_ < 3 ); 
      board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
      board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
      checkTrap[1] = step.oppFrom_;
      checkTrapNum=2;
      //updateAfterStep(step.oppFrom_,step.oppTo_);  
    }

    //update board after single step
     board_[step.to_] = ( toMove_ | step.piece_);
     board_[step.from_] = EMPTY_SQUARE;
     checkTrap[0] = step.from_;
     stepCount_++;
    //updateAfterStep(step.from_,step.to_);
   
     pieceArray[toMoveIndex_].del(step.from_);
     pieceArray[toMoveIndex_].add(step.to_);


    //pull steps
    if (step.stepType_ == STEP_PULL) {  
      assert( stepCount_ < 4 ); 
      board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
      board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
      checkTrap[1] = step.oppFrom_;
      checkTrapNum=2;
     // updateAfterStep(step.oppFrom_,step.oppTo_);  
    }

    if (step.stepType_ != STEP_SINGLE) {  
     pieceArray[1 - toMoveIndex_].del(step.oppFrom_);
     pieceArray[1 - toMoveIndex_].add(step.oppTo_);
    }

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
    for (int j = 0; j < checkTrapNum; j++)
      for (int i = 0; i < 4; i++)
        if ( IS_TRAP(checkTrap[j] + direction[i]) ){    
          if ( board_[checkTrap[j] + direction[i]] != EMPTY_SQUARE && ! hasFriend(checkTrap[j] + direction[i]) ){
            //trap is not empty and piece in there has no friends around => KILL
            trapped[j] = checkTrap[j] + direction[i];
            if (PIECE(board_[trapped[j]]) == PIECE_RABBIT)              //update rabbitsNum - for gameEnd check 
              rabbitsNum[PLAYER_TO_INDEX(OWNER(board_[trapped[j]]))]--;
            board_[trapped[j]] = EMPTY_SQUARE; 
            //updateAfterKill(checkTrap[j] + direction[i]);
           if ( j == 0 ) 
             pieceArray[toMoveIndex_].del(trapped[j]);
           else
             pieceArray[1 - toMoveIndex_].del(trapped[j]);
          }
          break;
        }
            
}

void Board::updateAfterStep(square_t from, square_t to)
  /* update step structure after performance of step 
   * from -> to ( push/pulls are considered as two separate steps )*/
{
  #ifdef DEBUG_3
    log_() << "=== BEGIN Board::updateAfterStep" << endl;
  #endif

  #ifdef DEBUG_3
    log_() << "=== END Board::updateAfterStep" << endl;
  #endif
}

void Board::updateAfterKill(square_t square)
  /*update of step structure after a piece was removed in the trap at square */
{

  #ifdef DEBUG_3
    log_() << "=== BEGIN Board::updateAfterKill" << endl;
  #endif
  #ifdef DEBUG_3
    log_() << "=== END Board::updateAfterKill" << endl;
  #endif
}

void Board::commitMove()
  /*called after player ends his move, switches the sides and checks the winner*/
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
}

bool Board::createRandomStep(Step& step)
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


Step Board::getRandomStep()
{
  uint playerIndex = PLAYER_TO_INDEX(toMove_);
  Step step;

  assert( rabbitsNum[toMoveIndex_] > 0 || stepCount_ > 0); //it's not possible to have 0 rabbits in the beginning of move

  if (pieceArray[toMoveIndex_].getLen() == 0) //no piece for player to move 
    return Step(STEP_PASS);                   //step_pass since the player with no pieces still might win 
                                              //if he managed to kill opponent's last rabbit before he lost his last piece
  //if (createRandomStep(step))
  //  return step;

  generateAllCount++;   
  stepArrayLen[playerIndex] = generateAllSteps(toMove_,stepArray[playerIndex]);
  uint len = stepArrayLen[playerIndex];

  assert(stepArrayLen[playerIndex] < MAX_STEPS);

  if (len == 0 && stepCount_ == 0){ //player to move has no step to play and cannot play pass => he lost
    winner_ = OPP(toMove_);
    return Step(STEP_NO_STEP); 
  }

  uint index = 0;

  if ( len == 0 && stepCount_ > 0 ) 
    return Step(STEP_PASS);
  /*
  if ( stepCount_ != 0) {       //check pass move
    index = rand() % (len + 1);
    if ( index == 0 )
      return Step(STEP_PASS);
   // index--;  //not nece
  }
  */

  assert(len > 0);
  index = rand() % len;
  assert( index >= 0 && index < len );

  /*step verification - is it reasonable ?
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
    }
  

    if ( (step.piece_ == PIECE_RABBIT  &&  rand() % 99 > 70 ) )
          continue;

    if ( (step.piece_ == PIECE_CAT  &&  rand() % 99 > 50 ) )
          continue;

    if ( (step.piece_ == PIECE_DOG  &&  rand() % 99 > 30 ) )
          continue;
    //;   if ( step

    //we prefer push/pulls
    if ( step.stepType_ == STEP_SINGLE && rand() % 99 > 20) 
      continue;
      //rabbits move only forward
    if ( step.piece_ == PIECE_RABBIT && (step.to_ - step.from_ == WEST || step.to_ - step.from_ == EAST ))
      continue;
    break;
  }

  */

  return( stepArray[playerIndex][index]);

}

bool Board::hasFriend(square_t square) const
  /* checks whether piece at given square has adjacent friendly pieces*/
{
  uint owner = OWNER(board_[square]);
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == owner)
      return true;

  return false;
}

bool Board::hasStrongerEnemy(square_t square) const
  /* checks whether piece at given square has adjacent stronger enemy pieces*/
{
  uint owner = OWNER(board_[square]);
  assert( owner == GOLD || owner == SILVER );
  for(int i = 0; i < 4; i++)
    if (OWNER(board_[square + direction[i]]) == OPP(owner) && 
        PIECE(board_[square + direction[i]]) > PIECE(board_[square]))
      return true;

  return false;
  
}

bool Board::isFrozen(square_t square) const
  /*checks whether piece at given square is frozen == !combination of hasFriend and hasStrongerEnemy
   * optimize: actually code the function, not use the hasFriend,hasStrongerEnemy */
{
  return (!hasFriend(square) && hasStrongerEnemy(square)); 
}


bool Board::init(const char* fn)
 /**
 * Inits position from a method in file.
 *
 * returns true if initialization went right 
 * otherwise returns false
 */
{

  assert(OPP(GOLD) == SILVER);
  assert(OPP(SILVER) == GOLD);

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
  generateAllCount = 0;

 // BOARD_Calculate_Hashkey(bp);
 

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


void Board::dump()
{
  log_() << toString();
}


string Board::toString() 
{

  stringstream ss;

  
  ss << endl;
  //ss << "board at: " << this << endl; //pointer debug
  ss << "Move " << moveCount_  << ", Step " << stepCount_ << ", ";

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

  //ss << "Hashkey: %#.8X%.8X\n",(unsigned long int)(bp->hashkey>>32),(unsigned long int)(bp->hashkey&0xFFFFFFFFULL);
  
  return ss.str();
} //Board::dump

uint Board::getStepCount() 
{
  return stepCount_;
}

player_t Board::getWinner()
{
  return winner_;
}

uint Board::getAllStepsNum(player_t player)
  /*returns number of all steps for given player - not index but player ! GOLD/SILVER */
{
  return stepArrayLen[PLAYER_TO_INDEX(player)];
}


/*wrapper around generateAllSteps - fills inner array stepArray, lenStepArray
void Board::fillStepArrayToMove()
{
}
*/


int Board::generateAllSteps(player_t player, StepArray oldStepArray) const
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
                oldStepArray[stepsNum++].setValues( STEP_PULL, player,PIECE(board_[square]), square, 
                      square + direction[j], PIECE(board_[square + direction[i]]), square + direction[i], square);
            }
          for (j=0; j<4; j++)  //push
            if (board_[square + direction[i] + direction[j]] == EMPTY_SQUARE) { //create move
                oldStepArray[stepsNum++].setValues( STEP_PUSH, player, PIECE(board_[square]), square, 
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
        oldStepArray[stepsNum++].setValues( STEP_SINGLE, player, PIECE(board_[square]),square, square + direction[i]);
      }
  } //for pieces on board
  return stepsNum;
}

int Board::evaluate(player_t player){
  static const int piece_value[7]={0,RABBIT_VALUE,CAT_VALUE,DOG_VALUE,HORSE_VALUE,CAMEL_VALUE,ELEPHANT_VALUE};
  int eval[2] = {0,0};

  for ( uint i = 0; i < 2; i++){
    for ( uint j = 0; j < pieceArray[i].getLen(); j++){
      assert(PIECE(board_[pieceArray[i][j]]) > 0 && PIECE(board_[pieceArray[i][j]]) < 7);
      eval[i] += piece_value[PIECE(board_[pieceArray[i][j]])];
  //    cerr << i << "+ " << piece_value[PIECE(board_[pieceArray[i][j]])] << endl;
    }
   // cerr <<endl;
  }
  if (player == GOLD)
    return eval[0] - eval[1];
  else
    return eval[1] - eval[0];
}

double Board::evaluateInPercent(player_t player) 
{
  //TODO programm the function - call evaluate and express in percentage
  return 0.5;
}
