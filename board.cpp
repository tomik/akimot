
#include "board.h"

const int direction[4]={NORTH,EAST,SOUTH,WEST};
const int trap[4]={33,36,63,66};

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


void Step::setValues( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to)
{
  stepType_ = stepType;
  player_   = player;
  piece_    = piece;
  from_     = from;
  to_       = to;
}


void Step::setValues( stepType_t stepType, player_t player, piece_t piece, square_t from, square_t to, 
            piece_t oppPiece, square_t oppFrom, square_t oppTo)
{
  stepType_ = stepType;
  player_		= player;
  piece_		= piece;
  from_			= from;
  to_				= to;
  oppPiece_ = oppPiece;
  oppFrom_	= oppFrom;
  oppTo_		= oppTo;

}


const string Step::oneSteptoString(player_t player, piece_t piece, square_t from, square_t to) 
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
 return moveCount_ == 1; //TODO ... really check whether the board is empty 
}


player_t Board::getPlayerToMove() 
{
 return toMove_;
}


void Board::makeStep(Step& step){

	int checkTrap[0];					//squares in whose vicinity we check the traps
	int checkTrapNum = 1;

    if (step.stepType_ == STEP_NO_STEP)
      return;

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

    //update board
     board_[step.to_] = ( toMove_ | step.piece_);
     board_[step.from_] = EMPTY_SQUARE;
		 checkTrap[0] = step.from_;
		 stepCount_++;

    //handle push/pull steps
    if (step.stepType_ != STEP_SINGLE) {  
      assert( stepCount_ < 4 ); 
			board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
			board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
		  checkTrap[1] = step.oppFrom_;
			checkTrapNum=2;
    }

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
		
		for (int j = 0; j < checkTrapNum; j++)
			for (int i = 0; i < 4; i++)
				if ( IS_TRAP(checkTrap[j] + direction[i]) ){		
					if ( board_[checkTrap[j] + direction[i]] != EMPTY_SQUARE && ! hasFriend(checkTrap[j] + direction[i]) ){
						//trap is not empty and piece in there has no friends around => KILL
						board_[checkTrap[j] + direction[i]] = EMPTY_SQUARE; 
						//TODO ... after killing, some moves might be added :(
					}
					break;
				}
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

  if (toMove_ == SILVER) 
    moveCount_++;
  toMove_ = OPP(toMove_);
  stepCount_ = 0;
}


Step Board::getRandomStep()
{

	generateAllSteps();
	uint len = stepsNum_[PLAYER_TO_INDEX(toMove_)];
	if ( stepCount_ > 1 ) //add pass move
		len++;

  if (len == 0){								//player to move has no step to play ( not even pass ! ) => loss
    winner_ = OPP(toMove_);
    return Step(STEP_NO_STEP);  //todo: return this as pass move ( slight speed up )
  }

  uint index = rand() % len;
  assert(index >= 0 && index < len);

	if ( stepCount_ > 1 && index-- == 0)
		return Step(STEP_PASS);

	StepNode* stepNode;

  for (int square=11; square < 89; square++) {
	  if (OWNER(board_[square]) == toMove_) {
			stepNode = stepListBoard_[STEP_LIST_FROM][square].from_next_;
			while ( stepNode != NULL ) { 
				if ( index-- == 0 ) 
					return stepNode->step_; 
				stepNode = stepNode->from_next_;
			}
								
		/*if ( index < stepBoard_[square].count_ )
			return stepBoard_[square].list[index];
			index -= stepBoard_[square].count_;*/
		}
	}
	assert(false); //it must not come here ! 
	return  Step(STEP_PASS);
}


int Board::clearStepList(StepNode head)
	/* frees the stepList which starts with head 
	 * returns how many items have been cleared == how many moves have been deleted
	 * */
{
	int i = 0;
	int index = 0;

	StepNode* stepNode;
	StepNode* destroyNode;

	if ( head.from_next_ != NULL ) {
		stepNode = head.from_next_;
		index = STEP_LIST_FROM;
		assert( head.to_next_ == NULL && head.victim_next_ == NULL );
	}
	else if ( head.to_next_ != NULL ) {
		stepNode = head.to_next_;
		index = STEP_LIST_TO;
		assert( head.from_next_ == NULL && head.victim_next_ == NULL );
	}
	else if ( head.victim_next_ != NULL ) {
		stepNode = head.victim_next_;
		index = STEP_LIST_VICTIM;
		assert( head.from_next_ == NULL && head.to_next_ == NULL );
	}else 
		return 0;   //nothing in steplist
	

	while (stepNode != NULL) {
		i++;
		destroyNode = stepNode; //this node will be destroyed

		//move to the next node
		if ( index == STEP_LIST_FROM ) 
			stepNode = stepNode->from_next_;
		else if ( index == STEP_LIST_TO ) 
			stepNode = stepNode->to_next_;
		else if ( index == STEP_LIST_VICTIM )
			stepNode = stepNode->victim_next_;

		//now "bypass the node before destroying it" - here we do 1+1 more bypasses than necessary,
		//-> for instance for index == STEP_LIST_FROM, from_next->from_previous and from_previous->from_next 
		//doesn't have to be bypassed because they will be destroyed anyway
		//but the code would get too long ... 
		assert( destroyNode->from_previous_ != NULL);
		destroyNode->from_previous_->from_next_ = destroyNode->from_next_;
		assert( destroyNode->to_previous_ != NULL);
		destroyNode->to_previous_->to_next_ = destroyNode->to_next_;

		//victim_previous_ can be NULL ( in case of move that has no victim ) - one step moves
		if ( destroyNode->victim_previous_ != NULL ) 
			destroyNode->victim_previous_->victim_next_ = destroyNode->victim_next_;

		if ( destroyNode->from_next_ != NULL )
			destroyNode->from_next_->from_previous_ = destroyNode->from_previous_;
		if ( destroyNode->to_next_ != NULL )
			destroyNode->to_next_->to_previous_ = destroyNode->to_previous_;
		if ( destroyNode->victim_next_ != NULL )
			destroyNode->victim_next_->victim_previous_ = destroyNode->victim_previous_;

		delete destroyNode; 
		//optimize: own memory management - put DestroyNode to a special list and allocate from there as well 
			
	}

	// now clean the head again
	if ( index == STEP_LIST_FROM ) 
		head.from_next_ = NULL;
	else if ( index == STEP_LIST_TO ) 
		head.to_next_ = NULL;
	else if ( index == STEP_LIST_VICTIM )
		head.victim_next_ = NULL;

	return i;
		
}

void Board::generateAllSteps()
{

		//first drop the whole - cross linked dynamic list - structure
		
		for (int i = STEP_LIST_FROM; i < STEP_LIST_VICTIM+1; i++)
			for (int square=11; square < 89; square++) 
				clearStepList(stepListBoard_[i][square]);

    
		stepsNum_[PLAYER_TO_INDEX(toMove_)] = 0;
    for (int square=11; square < 89; square++) {
        if (OWNER(board_[square]) != toMove_) // unplayable piece
					continue;
				stepBoard_[square].count_ = 0;
				assert( OWNER(board_[square]) == SILVER || OWNER(board_[square]) == GOLD );
        if ( isFrozen(square)) 	//frozen
				  continue;	

				generateStepsFromSquare(square);

				assert(PLAYER_TO_INDEX(GOLD) == 0 && PLAYER_TO_INDEX(SILVER) == 1);

    }

}

void Board::generateStepsFromSquare(square_t square)
	/*this function takes a square and generates all moves for a piece on this square
	 *the moves are generated and stored into a classical structure - cross linked dynamic list */
{
	StepNode* stepNode;
  int i, j;

	//generate push/pull moves
	if (stepCount_ < 3) {
		for (i = 0; i < 4; i++) {  
			if (OWNER(board_[square + direction[i]]) == OPP(toMove_) 
					&& PIECE(board_[square + direction[i]]) < PIECE(board_[square])){ //weaker enemy
				for (j=0; j<4; j++)  // pull
					if (board_[square + direction[j]]==EMPTY_SQUARE) { //create move
							stepNode = new StepNode;
							initStepNode(stepNode,square, square + direction[j], square + direction[i] ); //add also victim
							stepNode->step_.setValues( STEP_PULL, toMove_, PIECE(board_[square]), square, square + direction[j], 
							 															PIECE(board_[square + direction[i]]),	square + direction[i], square);
							stepsNum_[PLAYER_TO_INDEX(toMove_)]++;
																																
					}
		    for (j=0; j<4; j++)  //push
					if (board_[square+direction[i]+direction[j]]==EMPTY_SQUARE) { //create move
							stepNode = new StepNode;
							initStepNode(stepNode,square, square + direction[j], square + direction[i]); 
							stepNode->step_.setValues( STEP_PUSH, toMove_, PIECE(board_[square]), square, square + direction[i], 
															 PIECE(board_[square + direction[i]]), square + direction[i], square + direction[j]);
							stepsNum_[PLAYER_TO_INDEX(toMove_)]++;
				}
			} //if weaker enemy
		} //for
	} 

  // generate single moves
  for (i=0; i<4; i++) // check each direction
		if (board_[square+direction[i]] == EMPTY_SQUARE)  {
			if (PIECE(board_[square]) == RABBIT_PIECE){ // rabbit cannot backwards
				if (OWNER(board_[square]) == GOLD && direction[i] == SOUTH)
					continue;
        if (OWNER(board_[square]) == SILVER && direction[i] == NORTH)
					continue;
      }
			//create move
			stepNode = new StepNode;
			initStepNode(stepNode,square, square + direction[i]); 
			stepNode->step_.setValues( STEP_SINGLE, toMove_, PIECE(board_[square]), square, square + direction[i]);
			/*stepList = & stepBoard_[square];
			stepList->list[stepList->count_++].setValues( STEP_SINGLE, toMove_, PIECE(board_[square]), 
																											square, square + direction[i]);*/
			(stepsNum_[PLAYER_TO_INDEX(toMove_)])++;
		}
	
}

void Board::initStepNode(StepNode* stepNode,square_t from, square_t to, square_t victim)
{

	stepNode->from_next_ = stepListBoard_[STEP_LIST_FROM][from].from_next_;
	stepListBoard_[STEP_LIST_FROM][from].from_next_ = stepNode;
	stepNode->to_next_ = stepListBoard_[STEP_LIST_TO][to].to_next_;
	stepListBoard_[STEP_LIST_TO][to].to_next_ = stepNode;

	stepNode->from_previous_ = & stepListBoard_[STEP_LIST_FROM][from];
	if (stepNode->from_next_ != NULL) 
		stepNode->from_next_->from_previous_ = stepNode;
	stepNode->to_previous_ = & stepListBoard_[STEP_LIST_TO][to];
	if (stepNode->to_next_ != NULL) 
		stepNode->to_next_->to_previous_ = stepNode;

	if ((int) victim != -1){  //push/pull moves 
		stepNode->victim_next_ = stepListBoard_[STEP_LIST_VICTIM][victim].victim_next_;
		stepListBoard_[STEP_LIST_VICTIM][victim].victim_next_ = stepNode;
		stepNode->victim_previous_ = & stepListBoard_[STEP_LIST_VICTIM][victim];
		if (stepNode->victim_next_ != NULL) 
			stepNode->victim_next_->victim_previous_ = stepNode;

	}else{
		stepNode->victim_next_ = NULL;
		stepNode->victim_previous_ = NULL;
	}

}


bool Board::hasFriend(square_t square)
	/* checks whether piece at given square has adjacent friendly pieces*/
{
	uint owner = OWNER(board_[square]);
	assert( owner == GOLD || owner == SILVER );
	for(int i = 0; i < 4; i++)
		if (OWNER(board_[square + direction[i]]) == owner)
			return true;

	return false;
}


bool Board::hasStrongerEnemy(square_t square)
	/* checks whether piece at given square has adjacent stronger enemy pieces*/
{
	uint owner = OWNER(board_[square]);
	assert( owner == GOLD || owner == SILVER );
	for(int i = 0; i < 4; i++)
		if (OWNER(board_[square + direction[i]]) != owner && PIECE(board_[square + direction[i]]) > PIECE(board_[square]))
			return true;

	return false;
	
}

bool Board::isFrozen(square_t square)
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
	
	//initiate the stepLists
	for (int i = STEP_LIST_FROM; i < STEP_LIST_VICTIM+1; i++)
		for (int square=11; square < 89; square++) {
			stepListBoard_[i][square].from_next_ = NULL;
			stepListBoard_[i][square].to_next_ = NULL;
			stepListBoard_[i][square].victim_next_ = NULL;
		}

  for (int i = 0; i < 100; i++)  
    board_[i] = OFF_BOARD_SQUARE;

  for (int i = 1; i < 9; i++)
    for (int j = 1; j < 9; j++){
      board_[10*i+j]			 = EMPTY_SQUARE;
      frozenBoard_[10*i+j] = false;						//implicitly we consider everything not frozen
		}

  toMove_		 = GOLD;
  moveCount_ = 1;
  stepCount_ = 0;
  winner_		 = EMPTY;

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

  for (int i=1; i<9; i++) {
		ss << i / 8 + 1 <<"| ";
    for (int j=1; j<9; j++) {

      switch(board_[i*10+j]){
        case (GOLD | ELEPHANT_PIECE) :		ss << "E "; break;
        case (GOLD | CAMEL_PIECE) :		    ss << "M "; break;
        case (GOLD | HORSE_PIECE) :	      ss << "H "; break;
        case (GOLD | DOG_PIECE) :			    ss << "D "; break;
        case (GOLD | CAT_PIECE) :		      ss << "C "; break;
        case (GOLD | RABBIT_PIECE) :	    ss << "R "; break;
        case (SILVER | ELEPHANT_PIECE) :	  ss << "e "; break;
        case (SILVER | CAMEL_PIECE) :		    ss << "m "; break;
        case (SILVER | HORSE_PIECE) :	      ss << "h "; break;
        case (SILVER | DOG_PIECE) :	        ss << "d "; break;
        case (SILVER | CAT_PIECE) :		      ss << "c "; break;
        case (SILVER | RABBIT_PIECE) :			ss << "r "; break;
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
