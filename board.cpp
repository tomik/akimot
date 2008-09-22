
#include "board.h"

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int trap[4]={33,36,63,66};

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


void Board::testStepsStructure()
  /*testing function to check whether the actually kept set of moves is correct - 
   * for this all moves from position are generated and stepStructure is compared against this 
   * 1) all moves in structure are unique
   * //////// 2) every move in the structure is also among generated all moves
   * 3) every move in the generated moves is also in the structure */
{

  SimpleStepArray simpleStepArray;
  uint stepsNum;

  player_t player;

  for ( uint playerIndex = 0; playerIndex < 2; playerIndex++) {
    player = INDEX_TO_PLAYER(playerIndex);
    stepsNum = generateAllStepsOld(player,simpleStepArray,true);  //false == nopushpulls
    
    if (player == GOLD)
      log_() << "VERIFIED: GOLD (" <<  stepsNum << ")" ;
    else
      log_() << "VERIFIED: SILVER(" <<  stepsNum << ")" ;
        
    for ( uint i = 0; i < stepsNum; i++ )
      log_() << simpleStepArray[i].toString();
    log_() << endl;
    

    for ( uint j = 0; j < stepArrayLen[playerIndex]; j++)
      for ( uint i = 0; i < stepsNum; i++ )                     //check whether is in ref array
        if (stepArray[playerIndex][j] == simpleStepArray[i]) 
          simpleStepArray[i] = Step(STEP_NO_STEP);    //to check in the end that all moves were found

      
       
    //all moves generated by verified method were found 
    for ( uint i = 0; i < stepsNum; i++ ) {
      //cerr << simpleStepArray[i].toString();
      assert( simpleStepArray[i].stepType_ == STEP_NO_STEP );
    }
  }

}


bool Board::isEmpty() 
  /* this check is used in the beginning for pieces positioning*/
{
 return moveCount_ == 1; //TODO ... really check whether the board is empty 
}


player_t Board::getPlayerToMove() 
{
 return toMove_;
}


void Board::makeStep(Step& step){

  #ifdef DEBUG_3
    log_() << endl <<  "=== PLAYING STEP: " << step.toString() << endl;
  #endif 

  int checkTrap[2];         //squares in whose vicinity we check the traps
  //int trap[2] = {0,0} ;
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
      //update the step structure
      updateAfterStep(step.oppFrom_,step.oppTo_);  
    }

    //update board after single step
     board_[step.to_] = ( toMove_ | step.piece_);
     board_[step.from_] = EMPTY_SQUARE;
     checkTrap[0] = step.from_;
     stepCount_++;
     //update the step structure 
    updateAfterStep(step.from_,step.to_);


    //pull steps
    if (step.stepType_ == STEP_PULL) {  
      assert( stepCount_ < 4 ); 
      board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
      board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
      checkTrap[1] = step.oppFrom_;
      checkTrapNum=2;
      //update the step structure
      updateAfterStep(step.oppFrom_,step.oppTo_);  
    }

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
    
    for (int j = 0; j < checkTrapNum; j++)
      for (int i = 0; i < 4; i++)
        if ( IS_TRAP(checkTrap[j] + direction[i]) ){    
          if ( board_[checkTrap[j] + direction[i]] != EMPTY_SQUARE && ! hasFriend(checkTrap[j] + direction[i]) ){
            //trap is not empty and piece in there has no friends around => KILL
            board_[checkTrap[j] + direction[i]] = EMPTY_SQUARE; 
           // trap[j] = checkTrap[j] + direction[i];
            updateAfterKill(checkTrap[j] + direction[i]);
          }
          break;
        }
            
  /*

    if ( step.stepType_ == STEP_PUSH ){
      updateAfterStep(step.oppFrom_,step.oppTo_);  
      if (trap[1])
        updateAfterKill(trap[1]);
    }

    updateAfterStep(step.from_,step.to_);
    if (trap[0] )
      updateAfterKill(trap[0]);

    if ( step.stepType_ == STEP_PULL ){
      updateAfterStep(step.oppFrom_,step.oppTo_);  
      if (trap[1]) 
        updateAfterKill(trap[1]);
    }
  */
}

void Board::updateAfterStep(square_t from, square_t to)
  /* update step structure after performance of step 
   * from -> to ( push/pulls are considered as two separate steps )*/
{
  #ifdef DEBUG_3
    log_() << "=== BEGIN Board::updateAfterStep" << endl;
  #endif

  updateStepsForNeighbours(from, to);
  updateStepsForNeighbours(to, from);
    //generate moves to just emptied field
  generatePushesToSquare(from);
  frozenBoard_[from] = false;             //status of empty field is implicitly unfrozen 

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
  //update neighbours ( some of them might get (un)frozen )
  updateStepsForNeighbours(square);
  generatePushesToSquare(square);    
  frozenBoard_[square] = false;               //status of empty field is implicitly unfrozen 
  #ifdef DEBUG_3
    log_() << "=== END Board::updateAfterKill" << endl;
  #endif
}

void Board::commitMove()
  /*called after player ends his move, switches the sides and checks the winner*/
{

  //winner might be set already ( when player to move has no move ) 
  for (int i = 11; i <= 19; i++)
    if (board_[i] == (GOLD | RABBIT_PIECE) )
      winner_ = GOLD;
  for (int i = 81; i <= 88; i++)
    if (board_[i] == (SILVER | RABBIT_PIECE) )
      winner_ = SILVER;

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
  
  stepCount_ = 0;
}


Step Board::getRandomStep()
{
  uint playerIndex = PLAYER_TO_INDEX(toMove_);
  uint len = stepArrayLen[playerIndex];

  if (len >= 60 && stepCount_ == 0 ){
    clearStepArray(toMove_);
    len = stepArrayLen[playerIndex];
  }


  if (len == 0 && stepCount_ > 0){ //player to move has no step to play and cannot play pass => he lost
    winner_ = OPP(toMove_);
    return Step(STEP_NO_STEP); 
  }

  uint index = 0;
  if ( stepCount_ == 0) {
    index = rand() % (len + 1);
    if ( index-- == 0 )
      return Step(STEP_PASS);
  }

  assert(len > 0);
  index = rand() % len;


  //selection combined with move removal
  Step step;
  uint i = 0;
  assert( index >= 0 && index < len );

  do {
    if (index >= len)
      index -= len;

    assert(index >= 0 && index % len < stepArrayLen[playerIndex]);
    step = stepArray[playerIndex][index];

    if (! ( step.stepType_ != STEP_SINGLE && stepCount_ >= 3 ))
      if (checkStepValidity(step)) {
        assert(step.stepType_ != STEP_PASS);
        return step; 
      }
        //removeStepFromStepHash(step); 
    i++;
    index++;


  } while ( i < len );  //no valid move in the array
  
  //assert(false);
  //it must not come here !
  winner_ = OPP(toMove_);
  return  Step(STEP_NO_STEP);
}

void Board::removeStepFromStepHash(const Step& step)
{
  //setStepHash(step, false);
  #ifdef DEBUG_3
    log_() << "-" << step.toString() << endl;
  #endif
}

void Board::clearStepArray(player_t player)
{
  #ifdef DEBUG_3
    log_() << "=== BEGIN Board::clearStepArray" << endl;
  #endif

  //false == nopushpulls
  uint playerIndex = PLAYER_TO_INDEX(player);
  stepArrayLen[playerIndex] = generateAllStepsOld(player,stepArray[playerIndex],true);  
  
  #ifdef DEBUG_3
    log_() << "=== END Board::clearStepArray" << endl;
  #endif

}

bool Board::checkStepValidity(const Step& step)
{
  switch (step.stepType_ ) {
    case STEP_SINGLE:
      if ( ! frozenBoard_[step.from_] &&
           board_[step.to_] == EMPTY_SQUARE && board_[step.from_] == ( step.player_ | step.piece_ ) )
       return true;
      return false;
    case STEP_PUSH: 
      if ( ! frozenBoard_[step.from_] && board_[step.oppTo_] == EMPTY_SQUARE 
          && board_[step.from_] == ( step.player_ | step.piece_ ) 
          && board_[step.oppFrom_] == ( OPP(step.player_) | step.oppPiece_) ) 
        return true;
      return false;
    case STEP_PULL: 
      if ( ! frozenBoard_[step.from_] && board_[step.to_] == EMPTY_SQUARE 
          && board_[step.from_] == ( step.player_ | step.piece_ ) 
          && board_[step.oppFrom_] == ( OPP(step.player_) | step.oppPiece_) )
        return true;
      return false;
  }
  assert(false);
  return false;
}


void Board::generateAllSteps(player_t player)
{
    //first drop the whole - cross linked dynamic list - structure
   
    for (int square=11; square < 89; square++) {
        if (OWNER(board_[square]) != player) // unplayable piece
          continue;

        frozenBoard_[square] = isFrozen(square);
        if (frozenBoard_[square])
          continue;

        assert( OWNER(board_[square]) == SILVER || OWNER(board_[square]) == GOLD );

        //we now know that a piece on the square belongs to player on the move
        generateSingleStepsFromSquare(square);
        if (stepCount_ < 3) 
          generatePushPullsFromSquare(square);

        assert(PLAYER_TO_INDEX(GOLD) == 0 && PLAYER_TO_INDEX(SILVER) == 1);

    }
}

void Board::generatePushPullsFromSquare(square_t square, square_t excludePushTo)
  /*this function takes a square and generates push pull moves for a piece on this square
   *the moves are generated and stored into a classical structure - cross linked dynamic list 
   * DIRTY:( exclude is a field to exclude from pushing to ... workaround of a special situation
   *         this is because step structure doesn't allow to add moves already there ... 
   */
{
  assert(IS_PLAYER(board_[square]));
  assert( ! isFrozen(square));  //frozen cannot make any move

  player_t squareOwner = OWNER(board_[square]);
  square_t from, victimFrom;
  int i;

  from = square;
  //generate push/pull moves
    for (i = 0; i < 4; i++) {  
      victimFrom = square + direction[i]; 
      if ( IS_PLAYER(board_[victimFrom])
           && OWNER(board_[victimFrom]) == OPP(squareOwner) 
           && PIECE(board_[victimFrom]) < PIECE(board_[square])){ //weaker enemy
        generatePullsFromSquareVictim(from, victimFrom); 
        generatePushesFromSquareThrough(from, victimFrom, excludePushTo);
      } //if weaker enemy
    } //for
}

void Board::generatePullsFromSquareVictim(square_t from, square_t victimFrom, square_t exclude )
  /*for dry principles - 
   * this function is called from generatePushesFromSquare ( naturally ) 
   *                and also from updateStepsForNeighbours ( pulls with new position of the piece as victim)
   */
{
  square_t to;
  for (int j = 0; j < 4; j++) { // pull
    to = from + direction[j];   
    if (board_[to] == EMPTY_SQUARE)  //create move
      generatePull(from, to, victimFrom); 
  }

}
void Board::generatePushesFromSquareThrough(square_t from, square_t victimFrom, square_t excludePushTo)
  /*for dry principles - 
   * this function is called from generatePushesFromSquare ( naturally ) 
   *                and also from updateStepsForNeighbours ( pushes through new position of the piece )
   */
{
  square_t to;

  for (int j=0; j<4; j++) { 
    to = victimFrom + direction[j];
    if ( board_[to] == EMPTY_SQUARE && 
         (excludePushTo == -1 || to != excludePushTo) ) //DIRTY workaround of special situation :(
      generatePush(from, to, victimFrom);  //create move
  }
}

void Board::generateSingleStepsFromSquare(square_t square)
  /*this function takes a square and generates single step moves for a piece on this square*/

{
  assert( ! isFrozen(square));  //frozen cannot make any move

  player_t squareOwner = OWNER(board_[square]);
  int i;

  // generate single moves
  for (i=0; i<4; i++) // check each direction
    if (board_[square+direction[i]] == EMPTY_SQUARE)  {
      if (PIECE(board_[square]) == RABBIT_PIECE){ // rabbit cannot backwards
        if (squareOwner == GOLD && direction[i] == SOUTH)
          continue;
        if (squareOwner == SILVER && direction[i] == NORTH)
          continue;
      }
      generateSingleStep( square, square + direction[i]);  
    }
  
}


void Board::generatePushesToSquare(square_t square)
  /*this function takes a square and generates pushes to it for all pieces ( of both players ) */
{

  if ( board_[square] != EMPTY_SQUARE )
    return;

  square_t from, to, victimFrom;
  int i, j;
  to = square;
  assert( board_[to] == EMPTY_SQUARE );

  //generate push/pull moves

  for (i = 0; i < 4; i++) {  
    victimFrom = to + direction[i];
    if (IS_PLAYER(board_[victimFrom])){  //victim is a player 
      for (j = 0; j < 4; j++){   
        from = victimFrom + direction[j]; //pusher is behind the victim
        if ( OWNER(board_[victimFrom]) == OPP(OWNER(board_[from])) // this also covers IS_PLAYER(board_[from]) :)
            && ! isFrozen(from)                                    // not frozen
            && PIECE(board_[victimFrom]) < PIECE(board_[from])) {
          assert(IS_PLAYER(board_[from]));
          generatePush(from, to, victimFrom);
        }
      }
    } 
  } //for

} //function 


void Board::generatePullsToSquareFrom(square_t from,square_t to )
  /* another refinement which is also separetely used in updateStepsForNeighbours 
   * assumptions: to is EMPTY_SQUARE, 
   *              from is not frozen and there is a player at from 
   * */
{
  assert( board_[to] == EMPTY_SQUARE );
  assert( IS_PLAYER(board_[from]));
  assert( ! isFrozen(from));

  square_t victimFrom;
  for (int j = 0; j < 4; j++) { 
    victimFrom = from + direction[j];
    if (    OWNER(board_[from]) == OPP(OWNER(board_[victimFrom]))
         && PIECE(board_[from]) > PIECE(board_[victimFrom])) {
      assert(IS_PLAYER(board_[victimFrom]));
      generatePull(from, to, victimFrom);
    }
  }
  
}

void Board::generatePush(square_t from, square_t to, square_t victimFrom)
  /* assumptions: from,to,victimFrom - everything is consistent !!!   
   * i.e. to is EMPTY_SQUARE, 
   *      from,victimFrom are players of opposing colors 
   *      from is not frozen and stronger than victimFrom
   *      adjacency
   * */
{

  player_t player = OWNER(board_[from]); 
  Step newStep = Step(STEP_PUSH, player, PIECE(board_[from]), from, victimFrom, 
                      PIECE(board_[victimFrom]), victimFrom, to);

//  if ( checkStepHash(STEP_PUSH, from, to, victimFrom)) //already in 
  //  return; 


  stepArray[PLAYER_TO_INDEX(player)][(stepArrayLen[PLAYER_TO_INDEX(player)])++] = newStep;

  #ifdef DEBUG_3
    log_() << "+" <<  newStep.toString() << endl;
  #endif
  

  setStepHash(newStep, true);
}
  
void Board::generatePull(square_t from, square_t to, square_t victimFrom )
  /* assumptions: from,to,victimFrom - everything is consistent !!! ( see board::generatePush)*/
{
  //add the node to the cross linked dynamic list, arguments:

  player_t player = OWNER(board_[from]); 
  Step newStep = Step(STEP_PULL, player, PIECE(board_[from]), from, to, 
                      PIECE(board_[victimFrom]), victimFrom, from);

  //if (checkStepHash(STEP_PULL, from, to, victimFrom)) //already in 
   // return;

  stepArray[PLAYER_TO_INDEX(player)][(stepArrayLen[PLAYER_TO_INDEX(player)])++] = newStep;

  #ifdef DEBUG_3
    log_() << "+" << newStep.toString() << endl;
  #endif

  setStepHash(newStep, true);
}

void Board::generateSingleStep(square_t from, square_t to )
  /* assumptions: from,to consistent
   * i.e from is occupied by player
   *     to is empty 
   *     adjacency of to, fom */ 
{
  //if ( checkStepHash(STEP_SINGLE, from, to)) //already in 
   // return; 

  player_t player = OWNER(board_[from]); 
  Step newStep = Step(STEP_SINGLE, player, PIECE(board_[from]), from, to);
  stepArray[PLAYER_TO_INDEX(player)][(stepArrayLen[PLAYER_TO_INDEX(player)])++] = newStep;

  #ifdef DEBUG_3
    log_() << "+" << newStep.toString() << endl;
  #endif

  setStepHash(newStep, true);
}


void Board::updateStepsForNeighbours(square_t square, square_t exclude) 
  /*goes through neighbours and updates move for them
   * i.e. frozen->active => generates moves
   *      active->frozen => deletes moves    
   *      if new position is set then this is a new position of a piece from square - for this moves must 
   *        be generated explicitly ( frozenBoard_[newPosition] = false ) 
   * before this method information in frozenBoard field must be "old" not updated to perform correctly*/
{
  bool nowFrozen;
  int neighbour;

  bool isEmpty = board_[square] == EMPTY_SQUARE ;
    
  if ( ! isEmpty ){ //generate moves for itself 
    frozenBoard_[square] = isFrozen(square);
    if ( ! frozenBoard_[square] ) {
      generateSingleStepsFromSquare(square);
      generatePushPullsFromSquare(square);
    }
  } 

  for (int i = 0; i < 4; i++){ 
    neighbour = square + direction[i];
    if ( isEmpty && neighbour == exclude) //don't generate moves for neighbours from the move
      continue; 
    if ( ! IS_PLAYER(board_[neighbour])) 
      continue;

    nowFrozen = isFrozen(neighbour);

    if ( nowFrozen && ! frozenBoard_[neighbour] ){ //it got frozen by the last move ( delete moves from it )
      frozenBoard_[neighbour] = true;              // update frozen status "freeze"
    }else if ( ! nowFrozen && frozenBoard_[neighbour] ){ //it got "unfrozen" by the last move =>generate moves for it
      //first update frozen status - other functions might be using it 
      frozenBoard_[neighbour] = false;                  
      generateSingleStepsFromSquare(neighbour);
      //don't generate push moves for empty field - it will take care of it itself
      generatePushPullsFromSquare(neighbour, exclude);  
     
    }else  //add moves from active pieces to the newly emptied square ( single step && pulls && pushes_through ) 
       if ( ! nowFrozen && ! frozenBoard_[neighbour]){
         if ( isEmpty){    //single steps && pulls
           //single steps - still neccessary to check for rabbits moving in wrong direction :(
           if ( ! ( PIECE(board_[neighbour]) == RABBIT_PIECE && 
                  (( square - neighbour == SOUTH && OWNER(board_[neighbour]) == GOLD ) || 
                  ( square - neighbour == NORTH && OWNER(board_[neighbour]) == SILVER ) ))){
              generateSingleStep(neighbour, square);
            }
           generatePullsToSquareFrom(neighbour, square);   //pulls to empty square
           
         }else {            // pushes through
           if ( OWNER(board_[neighbour]) == OPP(OWNER(board_[square])) &&
                PIECE(board_[neighbour]) > PIECE(board_[square])){ 
             assert( IS_PLAYER(board_[neighbour]));
             generatePushesFromSquareThrough(neighbour, square, exclude);
             generatePullsFromSquareVictim(neighbour, square);  //todo add exclude ! when handling 2step move as ont
           }
         }
       }
    
  }  //for neighbours
} //function
  

bool Board::checkStepHash(const Step& step) 
  /* the move must be within the bounds ! victimFrom is implicitly -1 */
  //optimize: no switch ! index directly by stepType, speedup directionToIndex
{
  switch(step.stepType_){
    case STEP_SINGLE  :
      return stepHashSingle[step.from_ - 11][directionToIndex(step.to_ - step.from_)];        
    case STEP_PUSH    :  
      return stepHashPush[step.from_ - 11][directionToIndex(step.oppFrom_ - step.from_)]
                                          [directionToIndex(step.to_ - step.oppFrom_)];
    case STEP_PULL    :
      return stepHashPull[step.from_ - 11][directionToIndex(step.to_ -step.from_)]
                                          [directionToIndex(step.oppFrom_ - step.from_)];
  }

  assert( false); //must not come here
  return false;
}

void Board::setStepHash(const Step& step, bool value)
  /* the move must be within the bounds ! victimFrom is implicitly -1 */
{
  switch(step.stepType_){
    case STEP_SINGLE  :
      stepHashSingle[step.from_ - 11][directionToIndex(step.to_ - step.from_)] = value; 
      return;
    case STEP_PUSH    :   // in case of step_push to is a "total" to i.e from -> victimFrom -> to
      stepHashPush[step.from_ - 11][directionToIndex(step.oppFrom_ - step.from_)]
                                  [directionToIndex(step.to_ - step.oppTo_)] = value;
      return;
    case STEP_PULL    :
      stepHashPull[step.from_ - 11][directionToIndex(step.to_ - step.from_)]
                                   [directionToIndex(step.oppFrom_ - step.from_)] = value;
      return;
  }

  assert(false); //must not come here
}

uint Board::directionToIndex(uint direction)
{
  switch(direction){
    case NORTH: return 0; 
    case EAST : return 1; 
    case WEST : return 2; 
    case SOUTH: return 3; 
  }
  assert(false);
  return 0;
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

  for (int i = 0; i < HASH_ITEMS; i++)
    for (int j = 0; j < 4; j++){
      stepHashSingle[i][j] = false;			
      for (int k = 0; k < 4; k++){
        stepHashPush[i][j][k] = false;			
        stepHashPull[i][j][k] = false;			
      }
    }

  
  for (int i = 0; i < 100; i++)  
    board_[i] = OFF_BOARD_SQUARE;

  for (int i = 1; i < 9; i++)
    for (int j = 1; j < 9; j++){
      board_[10*i+j]       = EMPTY_SQUARE;
      frozenBoard_[10*i+j] = false;           //implicitly we consider everything not frozen
    }

  toMove_    = GOLD;
  moveCount_ = 1;
  stepCount_ = 0;
  winner_    = EMPTY;

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
      moveCount_++; //TODO ? 
    }

    f.ignore(1024,'\n'); //ignores the top of the border till EOF 

    for (int i = 1; i < 9; i++) { // do this for each of the 8 lines of the board
      f.ignore(2); //ignore trailing characters

      for (int j = 1; j < 9; j++) {
        f.ignore(1); //ignore a white space 
         c=f.get();

         switch(c) {
           case 'E' : board_[i*10+j] = (GOLD | ELEPHANT_PIECE);  break;
           case 'M' : board_[i*10+j] = (GOLD | CAMEL_PIECE);   break;
           case 'H' : board_[i*10+j] = (GOLD | HORSE_PIECE);   break;
           case 'D' : board_[i*10+j] = (GOLD | DOG_PIECE);    break;
           case 'C' : board_[i*10+j] = (GOLD | CAT_PIECE);    break;
           case 'R' : board_[i*10+j] = (GOLD | RABBIT_PIECE);   break;
           case 'e' : board_[i*10+j] = (SILVER | ELEPHANT_PIECE); break;
           case 'm' : board_[i*10+j] = (SILVER | CAMEL_PIECE);  break;
           case 'h' : board_[i*10+j] = (SILVER | HORSE_PIECE);  break;
           case 'd' : board_[i*10+j] = (SILVER | DOG_PIECE);   break;
           case 'c' : board_[i*10+j] = (SILVER | CAT_PIECE);   break;
           case 'r' : board_[i*10+j] = (SILVER | RABBIT_PIECE);  break;
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

  generateAllSteps(GOLD);
  generateAllSteps(SILVER);

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
  ss << "board at: " << this << endl;
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
        case (GOLD | ELEPHANT_PIECE) :    ss << "E"; break;
        case (GOLD | CAMEL_PIECE) :       ss << "M"; break;
        case (GOLD | HORSE_PIECE) :       ss << "H"; break;
        case (GOLD | DOG_PIECE) :         ss << "D"; break;
        case (GOLD | CAT_PIECE) :         ss << "C"; break;
        case (GOLD | RABBIT_PIECE) :      ss << "R"; break;
        case (SILVER | ELEPHANT_PIECE) :    ss << "e"; break;
        case (SILVER | CAMEL_PIECE) :       ss << "m"; break;
        case (SILVER | HORSE_PIECE) :       ss << "h"; break;
        case (SILVER | DOG_PIECE) :         ss << "d"; break;
        case (SILVER | CAT_PIECE) :         ss << "c"; break;
        case (SILVER | RABBIT_PIECE) :      ss << "r"; break;
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
        assert( frozenBoard_[i * 10 + j] == isFrozen( i * 10 + j));
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


int Board::generateAllStepsOld(player_t player, SimpleStepArray simpleStepArray, bool pushPulls) const
{
    int stepsNum;
    int i,j;
    
    stepsNum = 0;
    for (int square=11; square < 89; square++) {
        if (OWNER(board_[square]) != player) // unplayable piece
          continue;
        assert( IS_PLAYER(board_[square]));
        if ( isFrozen(square))  //frozen
          continue; 

        //generate push/pull moves
        if (pushPulls /*&& stepCount_ < 3*/) {      //no stepcount limit ! this is solved elsewhere
          for (i = 0; i < 4; i++) {  
            if (OWNER(board_[square + direction[i]]) == OPP(player) 
                && PIECE(board_[square + direction[i]]) < PIECE(board_[square])){ //weaker enemy
              for (j=0; j<4; j++)  // pull
                if (board_[square + direction[j]] == EMPTY_SQUARE) { //create move
                    simpleStepArray[stepsNum++].setValues( STEP_PULL, player,PIECE(board_[square]), square, 
                          square + direction[j], PIECE(board_[square + direction[i]]), square + direction[i], square);
                }
              for (j=0; j<4; j++)  //push
                if (board_[square + direction[i] + direction[j]] == EMPTY_SQUARE) { //create move
                    simpleStepArray[stepsNum++].setValues( STEP_PUSH, player, PIECE(board_[square]), square, 
                          square + direction[i], PIECE(board_[square + direction[i]]),
                          square + direction[i], square + direction[i] + direction[j]);
              }
            } //if weaker enemy
          } //for
        } 

        // generate single moves
        for (i=0; i<4; i++) // check each direction
          if (board_[square + direction[i]] == EMPTY_SQUARE)  {
            if (PIECE(board_[square]) == RABBIT_PIECE){ // rabbit cannot backwards
              if (OWNER(board_[square]) == GOLD && direction[i] == SOUTH)
                continue;
              if (OWNER(board_[square]) == SILVER && direction[i] == NORTH)
                continue;
            }
            //create move
            simpleStepArray[stepsNum++].setValues( STEP_SINGLE, player, PIECE(board_[square]),square, square + direction[i]);
          }
    } //for pieces on board
  return stepsNum;
}
