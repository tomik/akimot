
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
		case STEP_HEAD:
			ss << "HEAD";	
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

void Board::dumpAllSteps()
{
	log_() << allStepsToString(); 
}

string Board::allStepsToString()
{
	stringstream ss;
	StepNode* stepNode;

	for (uint player = 0; player < 2; player++) {
		uint realStepsNum = 0;
		ss << endl << "Potential moves for GOLD: " << stepsNum_[player];

		for (int square=11; square < 89; square++) {
			if (PLAYER_TO_INDEX(OWNER(board_[square])) == 1 - player)
				continue;
			//ss << "[sqr " << square << "] ";
			stepNode = stepListBoard_[STEP_LIST_FROM][square].from_next_;
			while ( stepNode != NULL ) { 

				//assert - move not already there
				assert(ss.str().find(stepNode->step_.toString()) == string::npos);	

				ss << stepNode->step_.toString();
				stepNode = stepNode->from_next_;
				realStepsNum++;
			}
		}
		assert( realStepsNum == stepsNum_[player]);
	}

	ss << endl;
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

	#ifdef DEBUG_3
		cerr << "=== BEGIN Board::makeStep" << endl;
	#endif 
	
	cerr << "playing step: " << step.toString() << endl;

	int checkTrap[0];					//squares in whose vicinity we check the traps
	int checkTrapNum = 1;

    if (step.stepType_ == STEP_NO_STEP)
      return;

    if (step.stepType_ == STEP_PASS ){
      stepCount_++; 
      return;
    }

		//in STEP_PUSH victim's move must be made first ! 
    if (step.stepType_ == STEP_PUSH ){  
			assert(false);
      assert( stepCount_ < 3 ); 
			board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
			board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
		  checkTrap[1] = step.oppFrom_;
			checkTrapNum=2;
			//update the step structure
			updateAfterStep(step);  //true is set to show that this is a victim move
		}

    //update board after single step
     board_[step.to_] = ( toMove_ | step.piece_);
     board_[step.from_] = EMPTY_SQUARE;
		 checkTrap[0] = step.from_;
		 stepCount_++;
		 //update the step structure 
		 updateAfterStep(step);


    //handle push/pull steps
    if (step.stepType_ == STEP_PULL) {  
			assert(false);
      assert( stepCount_ < 4 ); 
			board_[step.oppTo_] = ( OPP(toMove_) | step.oppPiece_);
			board_[step.oppFrom_] = EMPTY_SQUARE;
      stepCount_++;
		  checkTrap[1] = step.oppFrom_;
			checkTrapNum=2;
			//update the step structure
			updateAfterStep(step);  //true is set to show that this is a victim move
    }

    //check traps (o) at most 2 traps "kill" after a push/pull step, otherwise just one
		
		for (int j = 0; j < checkTrapNum; j++)
			for (int i = 0; i < 4; i++)
				if ( IS_TRAP(checkTrap[j] + direction[i]) ){		
					if ( board_[checkTrap[j] + direction[i]] != EMPTY_SQUARE && ! hasFriend(checkTrap[j] + direction[i]) ){
						//trap is not empty and piece in there has no friends around => KILL
						board_[checkTrap[j] + direction[i]] = EMPTY_SQUARE; 
						//after killing there comes a move update
						updateAfterKill(checkTrap[j] + direction[i]);
						
					}
					break;
				}
}

void Board::updateAfterStep(const Step & step)
	/* update step structure after performance of step 
	 * step is from -> to ( push/pulls are considered as two separate steps )*/
	/* optimize: give just from/to, not the whole step */
{
	#ifdef DEBUG_3
		cerr << "=== BEGIN Board::updateAfterStep" << endl;
	#endif

	 //after step update ... first clear moves from original field and to destination field
	clearStepList(& stepListBoard_[STEP_LIST_FROM][step.from_]);
	clearStepList(& stepListBoard_[STEP_LIST_TO][step.to_]);
	//if the guy at the step.from_ was somebody's victim - he must be cleared too 
	//clearStepList(stepListBoard_[STEP_LIST_VICTIM][step.from_]);
	// add also a newPosition of the piece - step.to_, so the moves for it might be generated explicitly
	updateStepsForNeighbours(step.from_, step.to_);
	updateStepsForNeighbours(step.to_);
		//generate moves to just emptied field
	//generatePushPullsToSquare(step.from_);
	frozenBoard_[step.from_] = false;							//status of empty field is implicitly unfrozen 

	#ifdef DEBUG_3
		cerr << "=== END Board::updateAfterStep" << endl;
	#endif
}

void Board::updateAfterKill(square_t square)
	/*update of step structure after a piece was removed in the trap at square */
{

	#ifdef DEBUG_3
		cerr << "=== BEGIN Board::updateAfterKill" << endl;
	#endif
	clearStepList(& stepListBoard_[STEP_LIST_FROM][square]);  //todo add victim
	//update neighbours ( some of them might get (un)frozen )
	updateStepsForNeighbours(square);
	generatePushPullsToSquare(square);		
	//generateSingleStepsToSquare(square);
	frozenBoard_[square] = false;								//status of empty field is implicitly unfrozen 
	#ifdef DEBUG_3
		cerr << "=== END Board::updateAfterKill" << endl;
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

	cerr << "COMITTING MOVE" <<endl;
	cerr << "TO MOVE: " << toMove_ << endl;
	cerr << "move count: " << moveCount_;
  if (toMove_ == SILVER) 
    moveCount_++;
	else 
		assert(toMove_ == GOLD);
  toMove_ = OPP(toMove_);
	
  stepCount_ = 0;
}


Step Board::getRandomStep()
{

	//generateAllSteps();
	uint len = stepsNum_[PLAYER_TO_INDEX(toMove_)];
	if ( stepCount_ > 1 ) //add pass move
		len++;

  if (len == 0){								//player to move has no step to play ( not even pass ! ) => loss
    winner_ = OPP(toMove_);
    return Step(STEP_NO_STEP);  //todo: return this as pass move ( slight speed up )
  }

	assert(len != 0);
  uint index = rand() % len;
  assert(index >= 0 && index < len);

	if ( stepCount_ > 1 && index-- == 0)
		return Step(STEP_PASS);

	StepNode* stepNode;

  for (int square=11; square < 89; square++) {
	  if (OWNER(board_[square]) == toMove_) {
			stepNode = stepListBoard_[STEP_LIST_FROM][square].from_next_;
			while ( stepNode != NULL ) { 
				// we decrease the index and check the move only if it is played by player toMove 
				// ( moves by both players are mixed in the structure ) 
				if ( stepNode->step_.player_ == toMove_ && index-- == 0 ) {
					return stepNode->step_; 
				}
				stepNode = stepNode->from_next_;
			}
								
		}
	}
	cerr << index << endl;
	assert(false); //it must not come here !
	return  Step(STEP_PASS);
}


int Board::clearStepList(StepNode* head)
	/* frees the stepList which starts with head 
	 * returns how many items have been cleared == how many moves have been deleted
	 * */
{
	int i = 0;
	int index = 0;

	StepNode* stepNode;
	StepNode* destroyNode;

	//cerr << "Clearing steplist " << endl;   //todo remove

	if ( head->from_next_ != NULL ) {
		stepNode = head->from_next_;
		index = STEP_LIST_FROM;
		assert( head->to_next_ == NULL && head->victim_next_ == NULL );
		//cerr << "Clearing from steplist " << endl;  //todo remove
	}
	else if ( head->to_next_ != NULL ) {
		stepNode = head->to_next_;
		index = STEP_LIST_TO;
		assert( head->from_next_ == NULL && head->victim_next_ == NULL );
		//cerr << "Clearing to steplist " << endl;//todo remove
	}
	else if ( head->victim_next_ != NULL ) {
		stepNode = head->victim_next_;
		index = STEP_LIST_VICTIM;
		assert( head->from_next_ == NULL && head->to_next_ == NULL );
	}else 
		return 0;   //nothing in steplist


	while (stepNode != NULL) {
		//cerr << "xxx" << endl;
		i++;
		destroyNode = stepNode; //this node will be destroyed

		//move to the next node
		if ( index == STEP_LIST_FROM ) 
			stepNode = stepNode->from_next_;
		else if ( index == STEP_LIST_TO ) 
			stepNode = stepNode->to_next_;
		else if ( index == STEP_LIST_VICTIM )
			stepNode = stepNode->victim_next_;
		else 
			assert(false);

		//now "bypass the node before destroying it" - here we do 1+1 more bypasses than necessary,
		//-> for instance for index == STEP_LIST_FROM, from_next->from_previous and from_previous->from_next 
		//doesn't have to be bypassed because they will be destroyed anyway
		//but the code would get too long ... 
		if ( index != STEP_LIST_FROM ){
			assert( destroyNode->from_previous_ != NULL);
			destroyNode->from_previous_->from_next_ = destroyNode->from_next_;
			if ( destroyNode->from_next_ != NULL )
				destroyNode->from_next_->from_previous_ = destroyNode->from_previous_;
		}

		if ( index != STEP_LIST_TO ){
			assert( destroyNode->to_previous_ != NULL);
			destroyNode->to_previous_->to_next_ = destroyNode->to_next_;

			//assert( destroyNode->to_previous_->to_next_ == NULL);
			//assert( &(stepListBoard_[STEP_LIST_TO][destroyNode->step_.to_]) == (destroyNode->to_previous_));

			if ( destroyNode->to_next_ != NULL )
				destroyNode->to_next_->to_previous_ = destroyNode->to_previous_;
		}

	/*
		if ( index != STEP_LIST_VICTIM ){
		//victim_previous_ can be NULL ( in case of move that has no victim ) - one step moves
			if ( destroyNode->victim_previous_ != NULL ) 
				destroyNode->victim_previous_->victim_next_ = destroyNode->victim_next_;

			if ( destroyNode->victim_next_ != NULL )
				destroyNode->victim_next_->victim_previous_ = destroyNode->victim_previous_;
		}
*/

		cerr << "-";
		destroyNode->step_.dump();
		cerr << endl;

		stepsNum_[PLAYER_TO_INDEX(destroyNode->step_.player_)]--; //TODO put this out of the function
		delete destroyNode; 
		//optimize: own memory management - put DestroyNode to a special list and allocate from there as well 
			
	}

	// now clean the head again
	if ( index == STEP_LIST_FROM ) 
		head->from_next_ = NULL;
	else if ( index == STEP_LIST_TO ) 
		head->to_next_ = NULL;
	else if ( index == STEP_LIST_VICTIM )
		head->victim_next_ = NULL;
	else 
		assert(false);

	return i;
		
}

void Board::generateAllSteps(player_t player)
{
		//first drop the whole - cross linked dynamic list - structure
		
    
		stepsNum_[PLAYER_TO_INDEX(player)] = 0;
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

void Board::generatePushPullsFromSquare(square_t square)
	/*this function takes a square and generates push pull moves for a piece on this square
	 *the moves are generated and stored into a classical structure - cross linked dynamic list */
{
	return; //TODO DEBUGGING 

	if ( isFrozen(square)) 	//frozen cannot make any move
	 return;	

	player_t squareOwner = OWNER(board_[square]);
	StepNode* stepNode;
  int i, j;

	//generate push/pull moves
		for (i = 0; i < 4; i++) {  
			if (OWNER(board_[square + direction[i]]) == OPP(squareOwner) 
					&& PIECE(board_[square + direction[i]]) < PIECE(board_[square])){ //weaker enemy
				for (j=0; j<4; j++)  // pull
					if (board_[square + direction[j]]==EMPTY_SQUARE) { //create move
							stepNode = new StepNode;
							initStepNode(stepNode,square, square + direction[j], square + direction[i] ); //add also victim
							stepNode->step_.setValues( STEP_PULL, squareOwner, PIECE(board_[square]), square, square + direction[j], 
							 															PIECE(board_[square + direction[i]]),	square + direction[i], square);
							stepsNum_[PLAYER_TO_INDEX(squareOwner)]++;
																																
					}
		    for (j=0; j<4; j++)  //push
					if (board_[square+direction[i]+direction[j]]==EMPTY_SQUARE) { //create move
							stepNode = new StepNode;
							initStepNode(stepNode,square, square + direction[j], square + direction[i]); 
							stepNode->step_.setValues( STEP_PUSH, squareOwner, PIECE(board_[square]), square, square + direction[i], 
															 PIECE(board_[square + direction[i]]), square + direction[i], square + direction[j]);
							stepsNum_[PLAYER_TO_INDEX(squareOwner)]++;
				}
			} //if weaker enemy
		} //for
}

void Board::generateSingleStepsFromSquare(square_t square)
	/*this function takes a square and generates single step moves for a piece on this square
	 *the moves are generated and stored into a classical structure - cross linked dynamic list */
{
	if ( isFrozen(square)) 	//frozen cannot make any move
		return;	

	player_t squareOwner = OWNER(board_[square]);
	StepNode* stepNode;
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
			//create move
			stepNode = new StepNode;
			initStepNode(stepNode,square, square + direction[i]); 
			stepNode->step_.setValues( STEP_SINGLE, squareOwner, PIECE(board_[square]), square, square + direction[i]);
			(stepsNum_[PLAYER_TO_INDEX(squareOwner)])++;

			cerr << "+" << stepNode->step_.toString() << endl;
		}
	
}

void Board::generatePushPullsToSquare(square_t square)
	/*this function takes a square and generates push pull moves for all pieces ( of both players ) 
	 * to this square 
	 *the moves are generated and stored into a classical structure - cross linked dynamic list 
	 * 
	 * single step moves are handled in separate function
	 */
{
	return;//TODO DEBUGGING 

	//optimize: don't use the macros OWNER,PIECE all the time - store the value to the 
	//variable once and then use the variable ! 

	assert( board_[square] == EMPTY_SQUARE );
	square_t from, to, victimFrom;

	StepNode* stepNode;
  int i, j;

	//generate push/pull moves
	for (i = 0; i < 4; i++) {  
		from = square + direction[i];
		//the guy who is pulling should be GOLD or SILVER and should not be frozen
		//optimize:following condintion can be simplified: if ( OWNER(....) ) both GOLD,SILVER are non-zero values
		if ((((OWNER(board_[from])) == GOLD) || ((OWNER(board_[from]) == SILVER))) && ! isFrozen(from)){
			for (j=0; j<4; j++) { // the piece closer is pulling

				to = square;
				victimFrom = square + direction[i] + direction[j];
				//check whether there is a capable puller on the adjacent square     : square + direction[i]
				//	and a feasible pullee even further ( also checks "is on board" ) : square + direction[i] + direction[j]
				if ( OWNER(board_[from]) == OPP(OWNER(board_[victimFrom]))
					&& PIECE(board_[from]) > PIECE(board_[victimFrom])) 
						stepNode = new StepNode;
						//add the node to the cross linked dynamic list, arguments:
						//										from ( pulling! )					to				victim position
						initStepNode(stepNode, from, to, victimFrom); 
						stepNode->step_.setValues( STEP_PULL, OWNER(board_[from]), PIECE(board_[from]), from, to, 
																																			 PIECE(board_[victimFrom]),	victimFrom, from);
						stepsNum_[PLAYER_TO_INDEX(OWNER(board_[from]))]++;
																															
			}
	    for (j=0; j<4; j++){  // the piece far away is pushing
				from = square + direction[i] + direction[j];
				to = square;
				victimFrom = square + direction[i];
				if ( OWNER(board_[victimFrom]) == OPP(OWNER(board_[from]))
					&& PIECE(board_[victimFrom]) < PIECE(board_[from])) {
						stepNode = new StepNode;
						//add the node to the cross linked dynamic list, arguments:
						//										from (pushing ! )												to				victim position
						initStepNode(stepNode,from, to, victimFrom); 
						stepNode->step_.setValues( STEP_PUSH, OWNER(board_[from]), PIECE(board_[from]), from , to, 
																																			 PIECE(board_[victimFrom]), victimFrom, to);
						stepsNum_[PLAYER_TO_INDEX(OWNER(board_[from]))]++;
				}
			}
		} //if weaker enemy
	} //for

} //function 

void Board::generateSingleStepsToSquare(square_t square)
	/* this is analogy of function generatePushPullStepsToSquare for single step moves */
{

	//return; //TODO just for debugging

	assert( board_[square] == EMPTY_SQUARE );
	square_t from, to;

	StepNode* stepNode;
  int i;

  // generate single moves
  for (i=0; i<4; i++){ // check each direction
		//check if there is a piece to do a move
		from = square + direction[i];
		to = square;
		if (((OWNER(board_[from])) == GOLD || (OWNER(board_[from]) == SILVER) ) && ! isFrozen(from) )	{
			if (PIECE(board_[from]) == RABBIT_PIECE){ // rabbit cannot backwards
				if (OWNER(board_[from]) == GOLD && direction[i] == NORTH) //here is north cause GOLD RABBITS cannot go SOUTH ! 
					continue;
        if (OWNER(board_[from]) == SILVER && direction[i] == SOUTH)
					continue;
      }
			//create move
			stepNode = new StepNode;
			initStepNode(stepNode, from, to); 
			stepNode->step_.setValues( STEP_SINGLE, OWNER(board_[from]), PIECE(board_[from]), from, to);
			(stepsNum_[PLAYER_TO_INDEX(OWNER(board_[from]))])++;

			cerr << "+" << stepNode->step_.toString() << endl;
		}
	}
	
}


void Board::updateStepsForNeighbours(square_t square, square_t newPosition) 
	/*goes through neighbours and updates move for them
	 * i.e. frozen->active => generates moves
	 *			active->frozen => deletes moves    
	 *      if new position is set then this is a new position of a piece from square - for this moves must 
	 *				be generated explicitly ( frozenBoard_[newPosition] = false ) 
	 * before this method information in frozenBoard field must be "old" not updated to perform correctly*/
{
	bool nowFrozen;
	int neighbour;

	for (int i = 0; i < 4; i++){ 
		neighbour = square + direction[i];
		if ( (OWNER(board_[neighbour])) == GOLD || (OWNER(board_[neighbour])) == SILVER ) {
			nowFrozen = isFrozen(neighbour);
			//if we are considering a newPosition of the piece - generate the moves to make explicitly ! 
			if ( newPosition != -1 && newPosition == neighbour ) {
				if ( ! nowFrozen ) {
					generateSingleStepsFromSquare(neighbour);
					generatePushPullsFromSquare(neighbour);
				}
				frozenBoard_[neighbour] = nowFrozen;
				continue;	
			}
			if ( nowFrozen && ! frozenBoard_[neighbour] ){ //it got frozen by the last move ( delete moves from it )
				clearStepList(&stepListBoard_[STEP_LIST_FROM][neighbour]);
				frozenBoard_[neighbour] = true;							 // update frozen status "freeze"
			}else if ( ! nowFrozen && frozenBoard_[neighbour] ){ //it got "unfrozen" by the last move ( generate moves for it )
				frozenBoard_[neighbour] = false;						 // update frozen status "unfreeze"
				generateSingleStepsFromSquare(neighbour);
				generatePushPullsFromSquare(neighbour);
			} else if ( !nowFrozen && ! frozenBoard_[neighbour] && (OWNER(board_[square]) == EMPTY) ){
				//we must manuall add moves from unfrozen pieces to the newly emptied square - TODO move elsewhere ? 
				StepNode* stepNode = new StepNode;
				initStepNode(stepNode,neighbour, square); 
				stepNode->step_.setValues( STEP_SINGLE, OWNER(board_[neighbour]), PIECE(board_[neighbour]), neighbour, square );
				(stepsNum_[PLAYER_TO_INDEX(OWNER(board_[neighbour]))])++;
				cerr << "+" << stepNode->step_.toString() << endl;
			}
		} // if OWNER(board_[neighbour]) ... 
	}  //for neighbours
} //function
	

void Board::initStepNode(StepNode* stepNode,square_t from, square_t to, square_t victim)
{

	//todo is this necessary ? move to constructor
	stepNode->from_next_ = NULL;
	stepNode->from_previous_ = NULL;
	stepNode->to_previous_ = NULL;
	stepNode->to_next_ = NULL;
	stepNode->victim_next_ = NULL;
	stepNode->victim_previous_ = NULL;

	stepNode->from_next_ = stepListBoard_[STEP_LIST_FROM][from].from_next_;
	stepListBoard_[STEP_LIST_FROM][from].from_next_ = stepNode;

	//assert( stepListBoard_[STEP_LIST_TO][to].to_next_ == NULL);   //todo remove

	stepNode->to_next_ = stepListBoard_[STEP_LIST_TO][to].to_next_;
	stepListBoard_[STEP_LIST_TO][to].to_next_ = stepNode;

	//assert( stepListBoard_[STEP_LIST_TO][to].to_next_->to_next_ == NULL); //todo remove

	stepNode->from_previous_ = & stepListBoard_[STEP_LIST_FROM][from];
	if (stepNode->from_next_ != NULL) 
		stepNode->from_next_->from_previous_ = stepNode;

	stepNode->to_previous_ = & (stepListBoard_[STEP_LIST_TO][to]);
	if (stepNode->to_next_ != NULL) 
		stepNode->to_next_->to_previous_ = stepNode;

	//assert( &(stepListBoard_[STEP_LIST_TO][to]) == (stepNode->to_previous_)); //todo remove
	//assert( stepListBoard_[STEP_LIST_TO][to].to_next_  == stepNode); //todo remove

	assert( stepNode->from_previous_->from_next_ == stepNode );
	assert( stepNode->to_previous_->to_next_ == stepNode );

	/*
	if (victim != -1){  //push/pull moves 
		stepNode->victim_next_ = stepListBoard_[STEP_LIST_VICTIM][victim].victim_next_;
		stepListBoard_[STEP_LIST_VICTIM][victim].victim_next_ = stepNode;
		stepNode->victim_previous_ = & (stepListBoard_[STEP_LIST_VICTIM][victim]);
		if (stepNode->victim_next_ != NULL) 
			stepNode->victim_next_->victim_previous_ = stepNode;

	}else{
		stepNode->victim_next_ = NULL;
		stepNode->victim_previous_ = NULL;
	}
	*/

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
		if (OWNER(board_[square + direction[i]]) == OPP(owner) && 
				PIECE(board_[square + direction[i]]) > PIECE(board_[square]))
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
	for (int i = STEP_LIST_FROM; i < STEP_LIST_VICTIM + 1; i++)
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

	//drop the dynamic stucture TODO - make nicer ! 
	//for (int i = STEP_LIST_FROM; i < STEP_LIST_VICTIM+1; i++)
	//	for (int square=11; square < 89; square++) 
	//		clearStepList(stepListBoard_[i][square]);
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
        case (GOLD | ELEPHANT_PIECE) :		ss << "E"; break;
        case (GOLD | CAMEL_PIECE) :		    ss << "M"; break;
        case (GOLD | HORSE_PIECE) :	      ss << "H"; break;
        case (GOLD | DOG_PIECE) :			    ss << "D"; break;
        case (GOLD | CAT_PIECE) :		      ss << "C"; break;
        case (GOLD | RABBIT_PIECE) :	    ss << "R"; break;
        case (SILVER | ELEPHANT_PIECE) :	  ss << "e"; break;
        case (SILVER | CAMEL_PIECE) :		    ss << "m"; break;
        case (SILVER | HORSE_PIECE) :	      ss << "h"; break;
        case (SILVER | DOG_PIECE) :	        ss << "d"; break;
        case (SILVER | CAT_PIECE) :		      ss << "c"; break;
        case (SILVER | RABBIT_PIECE) :			ss << "r"; break;
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
