
#include "board.h"

using namespace bitStuff;

namespace bitStuff{

  bit64 getNeighbours(bit64 pieces)
    /*returns neighbors bitset for a bitset of pieces*/
  {
    bit64   result;

    result =  (pieces & notHfile) << 1;
    result |= (pieces & notAfile) >> 1;
    result |= (pieces & not1rank) << 8;
    result |= (pieces & not8rank) >> 8;

    return(result);
  }


  bit64         stepOffset_[2][7][64]; //extern definition in board.h

  void buildStepOffsets()
     /*returns a bitmap of legal squares for a given piece and a given player on a given square.
       TODO: this might be simplified by having only RABBITS/OTHER "piece types" */
  {
    int  player;
    int  piece;
    int  square;

    // do rabbits as a special case
    // --
    for (player = 0; player < 2; player++)
      for (square = 0; square < BIT_LEN; square++) {
        bit64  ts;
        assert( ts.none() );

        
        ts |= ((one << square) & notHfile) << 1;  // east
        ts |= ((one << square) & notAfile) >> 1;  // west
        if (player == 0)    
          ts |= ((one << square) & not8rank) >> 8;  // north
        else
          ts |= ((one << square) & not1rank) << 8;  // south

        stepOffset_[player][RABBIT][square] = ts;    
      }

    // Now do the rest
    for (player = 0; player < 2; player++)
      for (piece = 2; piece < 7; piece++)
        for (square = 0; square < BIT_LEN; square++) {
            bit64  ts;
            assert( ts.none());

            ts |= ((one << square) & notHfile) << 1;  // east
            ts |= ((one << square) & notAfile) >> 1;  // west
            ts |= ((one << square) & not1rank) << 8;  // south
            ts |= ((one << square) & not8rank) >> 8;  // north

            stepOffset_[player][piece][square] = ts;
        }

    }

#ifdef DEBUG
  string stepOffsettoString()
  {
    stringstream ss;

    for (int player = 0; player < 2; player++)
      for (int piece = 0; piece < 7; piece++){
        ss << "piece: " << piece << endl;
        for ( int j = 0; j < BIT_LEN; j ++ ) {
          ss << "position: " << j << endl;
          for ( int i = 0; i < BIT_LEN; i ++ ){
            ss << stepOffset_[player][piece][j][i] ;
            if ( i% 8 == 7 )
              ss << endl;
          }
        }
      }
    return ss.str();
  }
#endif 

}


Step::Step( stepType_t stepType )
  /*this constructor is mainly used for 
   *step_no_step or step_pass which don't use other values than stepType*/
{
  stepType_ = stepType;
}


void Step::setPass() 
{
  stepType_ = STEP_PASS;
}


bool Step::pieceMoved() 
  /* returns true if any piece was moved
   * i.e. returns false if pass or no_step */
{
  return (! (stepType_ == STEP_PASS || stepType_ == STEP_NO_STEP));
}


void Step::setValues( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}


void Step::setValues( stepType_t stepType, player_t player, piece_t piece, coord_t from, coord_t to, 
            piece_t oppPiece, coord_t oppFrom, coord_t oppTo)
{
  stepType_ = stepType;
  player_ = player;
  piece_ = piece;
  from_ = from;
  to_ = to;
  oppPiece_ = oppPiece;
  oppFrom_ = oppFrom;
  oppTo_ = oppTo;

}


const string Step::oneSteptoString(player_t player, piece_t piece, coord_t from, coord_t to) 
  /**prints step string for given values */
{
  stringstream s;
  string pieceRefStr(" RCDHMErcdhme");
  string columnRefStr("abcdefgh");

  s << pieceRefStr[piece + 6 * player] << columnRefStr[from % 8] << from / 8 + 1; 
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


const string Step::toString()
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
    default:
      assert(false);

  }
  ss << ") ";

  return ss.str();
}


bool Board::isEmpty() 
  /* this check is used in the beginning for pieces positioning*/
{
 return (( bitBoard_[GOLD][0] | bitBoard_[SILVER][0] ).none());
}


player_t Board::getPlayerToMove() 
{
 return toMove_;
}


void Board::makeStep(Step& step){

    if (step.stepType_ == STEP_NO_STEP)
      return;

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

    //update board
    bitBoard_[step.player_][step.piece_].reset(step.from_);
    bitBoard_[step.player_][step.piece_].set(step.to_);
    bitBoard_[step.player_][0].reset(step.from_);
    bitBoard_[step.player_][0].set(step.to_);
    stepCount_++;

    //handle push/pull steps
    if (step.stepType_ != STEP_SINGLE) {  
      assert( stepCount_ < 4 ); 
      bitBoard_[step.player_][step.oppPiece_].reset(step.oppFrom_);
      bitBoard_[step.player_][step.oppPiece_].set(step.oppTo_);
      bitBoard_[step.player_][0].reset(step.oppFrom_);
      bitBoard_[step.player_][0].set(step.oppTo_);
      stepCount_++;
    }

    // update zobrist hash key 
    //nxt->signature ^= zobrist[ s.col ][ s.typ ][ s.fsq ];
    //nxt->signature ^= zobrist[ s.col ][ s.typ ][ s.tsq ];

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
    bit64 fullTraps;
    int trap;
    
    for (int player = 0; player < 2; player++){
      //a little explanation of following two lines:
      //fullTraps ... traps ( bitstuff::traps ) that have (&) someone on them bitBoard[player][0] of particular player
      //biStuff::trapsNeighbours & bitBoard_[player][0] ... these are trapGuards
      //getNeighbours(trapGuards) & bitStuff:traps  ... these are guardedTraps
      //so guardedTraps ^ fullTraps gives us full unguarded traps 
      //I have a bad feeling of utter inefficiency anyway :( 
      //optimize: do it the "stupid way" checking trap by trap and 
      
      fullTraps = bitStuff::traps & bitBoard_[player][0];
      fullTraps = (getNeighbours((bitStuff::trapsNeighbours & bitBoard_[player][0])) & bitStuff::traps)^fullTraps;

      //now go through fulltraps and discard the pieces 
      trap = fullTraps._Find_first();             // consider unguarded full trap
      while (trap != BIT_LEN) {                   // Find_first fails => returns bitset length
        delSquare(trap);
        //nxt->signature ^= zobrist[c][typ][t];   // repair hash
        trap = fullTraps._Find_next(trap);        // consider trap
      }
    }
 
    return;
}


void Board::commitMove()
  /*called after player ends his move, switches the sides and checks the winner*/
{

  //winner might be set already ( when player to move has no move ) 
  if ( (bitBoard_[toMove_][1] & winRank[toMove_]).any() )  //rabits are in the finish
    winner_ = toMove_;
  if ( (bitBoard_[OPP(toMove_)][1] & winRank[OPP(toMove_)]).any() )  //pushing opponents rabbit to the goal - wtf ?
    winner_ = OPP(toMove_);

  if (toMove_ == SILVER) 
    moveCount_++;
  toMove_ = OPP(toMove_);
  stepCount_ = 0;
}


Step Board::generateRandomStep()
  /* optimize: this must be a very quick method !
   * generate all and then select one is VERY slow 
   */
{
  int len = generateSteps(stepList_);

  if (len == 0){ //player to move has no step to play ( not even pass ! ) => loss
    winner_ = 1 - toMove_; 
    return Step(STEP_NO_STEP);  //todo: return this as pass move ( slight speed up )
  }

  int index = rand() % len;
  assert(index >= 0 && index < len);
  return stepList_[index];
}

int Board::generateSteps(StepList& stepList)
  /*Crucial function generating steps from position for player to Step.
   *
   * returns number of generated steps saved in stepList array from index 0,
   * fixed array is used as a datastructure for performance reasons ( instead of 
   * for instance dynamic list )*/
{
  int tm = toMove_;                                       //player to step
  int listCount = 0;
  bit64   empty(~(bitBoard_[0][0] | bitBoard_[1][0]));    // mask of unoccupied squares    
  bit64   notFrozenSquares;                               // mask of friendly pieces which are not frozen
  bit64   movablePieces;                                  // mask of friendly pieces on notFrozenSquares
  bit64   stronger (bitBoard_[OPP(tm)][0]);                  // mask of stronger enemy pcs for movablePieces
  bit64   weaker;                                      // weaker enemy pieces all over the board 
  bit64   victims;                                     // weaker enemy pieces in adjacent squares
  bit64   wherePush;                                   // where to push
  bit64   wherePull;                                   // where to pull
  bit64   whereStep;                                   // where to step    

  //step posibilities
  //single: (piece) fromSquare-> toSquare 
  //push: (piece) fromSquare->victimFromSquare, (victim) victimFromSquare->victimToSquare
  //pull: (piece) fromSquare->pullertoSquare, (victim) victimFromSquare->fromSquare
  int fromSquare;
  int toSquare;
  int victimFromSquare;
  int victimToSquare;                            
  int pullerToSquare;

  for (int piece = 1; piece < 7; piece++) {
    movablePieces = bitBoard_[tm][piece];                           // get all pieces of this type
    stronger ^= bitBoard_[OPP(tm)][piece];                              // remove from consideration
    notFrozenSquares = 
      getNeighbours(bitBoard_[tm][0]) | (~getNeighbours(stronger)); //around is friendly piece or no stroger enemy piece 
    movablePieces &= notFrozenSquares;                              // pieces from slice on notFrozenSquares

    weaker |= bitBoard_[OPP(tm)][piece-1];                            // all pieces we can we can push/pull
    
    fromSquare = movablePieces._Find_first();             // consider steps from this square next 
    while (fromSquare != BIT_LEN) {

      whereStep = empty & stepOffset_[tm][piece][fromSquare];
      victims = weaker & stepOffset_[tm][piece][fromSquare];

      //generate single steps
      toSquare = whereStep._Find_first();            // get next whereStep step  
      while (toSquare != BIT_LEN) {

        stepList[listCount++].setValues(STEP_SINGLE, tm, piece, fromSquare, toSquare);

        toSquare = whereStep._Find_next(toSquare);             // get next whereStep step  
      }

      if ( piece > RABBIT && stepCount_ < 3 ) { //push/pull only possible for strong pieces and not in last step
        //generate push/pull steps
        victimFromSquare = victims._Find_first();                       // get next victim 
        while (victimFromSquare != BIT_LEN) {
          assert(piece != RABBIT );                                     // rabbits can't have victims :)
          wherePush = empty & stepOffset_[0][piece][victimFromSquare]; 
          wherePull = empty & stepOffset_[0][piece][fromSquare];          // optimize: out of the while cycle

          pullerToSquare = wherePull._Find_first();                     // where our piece (puller) can retrieve
          while (pullerToSquare != BIT_LEN) {

            stepList[listCount++].setValues(STEP_PULL, tm, piece, fromSquare, pullerToSquare, 
                                      getSquarePiece(victimFromSquare), victimFromSquare, fromSquare ); 
            pullerToSquare = wherePull._Find_next(pullerToSquare);       // where our piece (puller) can retrieve
          }

          victimToSquare = wherePush._Find_first();                       // where victim can be pushed to
          while (victimToSquare!=BIT_LEN) {

            stepList[listCount++].setValues(STEP_PUSH, tm, piece, fromSquare, victimFromSquare,
                                      getSquarePiece(victimFromSquare), victimFromSquare, victimToSquare  );
            victimToSquare = wherePush._Find_next(victimToSquare);                        // where victim can be pushed to
          }
        victimFromSquare = victims._Find_next(victimFromSquare);          // get next victim 
        } //while victims
      }//if piece > rabbit and stepcount < 3 
    fromSquare = movablePieces._Find_next(fromSquare);              // consider steps from this square next 
    } //while movablePieces any
  } // for piece

    
  if ( stepCount_ > 1 ) //pass is added for steps 2,3,4
    stepList[listCount++].setPass();

  return listCount;
} //Board::generateSteps


bool Board::init(const char* fn)
 /**
 * Inits position from a method in file.
 *
 * returns true if initialization went right 
 * otherwise returns false
 */
{

  /* clear board */
  for (int i=0; i<7; i++) {
    bitBoard_[0][i].reset();
    bitBoard_[1][i].reset();
  }

  moveCount_ = 1;
  stepCount_ = 0;
  winner_ = NO_PLAYER;

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

    for (int i = 0; i < 8; i++) { // do this for each of the 8 lines of the board
      f.ignore(2); //ignore trailing characters

      for (int j = 0; j < 8; j++) {
        f.ignore(1); //ignore a white space 
         c=f.get();

         switch(c) {
           case 'E' : setSquare(i*8+j, GOLD, ELEPHANT);  break;
           case 'M' : setSquare(i*8+j, GOLD, CAMEL);   break;
           case 'H' : setSquare(i*8+j, GOLD, HORSE);   break;
           case 'D' : setSquare(i*8+j, GOLD, DOG);    break;
           case 'C' : setSquare(i*8+j, GOLD, CAT);    break;
           case 'R' : setSquare(i*8+j, GOLD, RABBIT);   break;
           case 'e' : setSquare(i*8+j, SILVER, ELEPHANT); break;
           case 'm' : setSquare(i*8+j, SILVER, CAMEL);  break;
           case 'h' : setSquare(i*8+j, SILVER, HORSE);  break;
           case 'd' : setSquare(i*8+j, SILVER, DOG);   break;
           case 'c' : setSquare(i*8+j, SILVER, CAT);   break;
           case 'r' : setSquare(i*8+j, SILVER, RABBIT);  break;
           case ' ' : case 'X' : case 'x': break; //setSquare(i*10+j, NO_PLAYER, EMPTY); break;
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

  buildStepOffsets();

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
  ss << "Move " << moveCount_ / 2 + 1 << ", Step " << stepCount_ << ", ";

  if (toMove_ == GOLD) 
    ss << "Gold to move." << endl;
  else
    ss << "Silver to move." << endl;
  assert(toMove_ == GOLD || toMove_ == SILVER );

  ss << " +-----------------+\n";

    
  string refStr(".123456rcdhmeRCDHME");
  for (int i = 0; i < BIT_LEN; i++) {
      if ( i % 8 == 0 ) 
        ss << i / 8 + 1 <<"| ";
      assert(getSquarePiece(i) + ((2-getSquarePlayer(i)) * 6) <= refStr.length() );
			if (traps[i] && getSquarePlayer(i) == NO_PLAYER )
				ss << "X ";
			else
				ss << refStr[getSquarePiece(i) + ((2-getSquarePlayer(i)) * 6)] << " ";
      if ( i % 8 == 7 ) 
        ss << "| " << endl;
  }


  ss << " +-----------------+" << endl;
  ss << "   a b c d e f g h" << endl;

	//debuging
  //for (int player = 0; player < 2; player++)
  //  for (int piece = 0; piece < 7; piece++) 
  //    ss << bitBoard_[player][piece] << endl;
  //ss << "Hashkey: %#.8X%.8X\n",(unsigned long int)(bp->hashkey>>32),(unsigned long int)(bp->hashkey&0xFFFFFFFFULL);
  
	return ss.str();
} //Board::dump


void Board::setSquare(coord_t coord, player_t player, piece_t piece) 
{
 //bitBoard is [2][7] array of bit64
 assert(player == 1 || player == 0);
 assert(piece >= 1 && piece < 7 );
 assert(coord >= 0 && coord < BIT_LEN );
 
 bitBoard_[player][piece].set(coord);
 bitBoard_[player][0].set(coord);
}


//optimize: methods delSquare, getSquarePiece, etc. are VERY slow - optimize using one board for all pieces
void Board::delSquare(coord_t coord) 
{
  if (bitBoard_[GOLD][0][coord]){
    bitBoard_[GOLD][0].reset(coord);
    for (int i = 1; i < 7; i++ )
      if (bitBoard_[GOLD][i][coord])
        bitBoard_[GOLD][i].reset(coord);
  }

  if (bitBoard_[SILVER][0][coord]){
    bitBoard_[SILVER][0].reset(coord);
    for (int i = 1; i < 7; i++ )
      if (bitBoard_[SILVER][i][coord])
        bitBoard_[SILVER][i].reset(coord);
  }
}


piece_t Board::getSquarePiece(coord_t coord) 
{
  if (bitBoard_[GOLD][0][coord])
    for (int i = 1; i < 7; i++ )
      if (bitBoard_[GOLD][i][coord])
        return i;

  if (bitBoard_[SILVER][0][coord])
    for (int i = 1; i < 7; i++ )
      if (bitBoard_[SILVER][i][coord])
        return i;

  return EMPTY;
}


player_t Board::getSquarePlayer(coord_t coord) 
{
  if (bitBoard_[GOLD][0][coord])
    return GOLD;

  if (bitBoard_[SILVER][0][coord])
    return SILVER;

  return NO_PLAYER;
}


uint Board::getStepCount() 
{
  return stepCount_;
}


player_t Board::getWinner()
{
  return winner_;
}
