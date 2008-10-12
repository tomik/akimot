
#include "engine.h"


string Engine::initialSetup(bool isGold)
{
	if (isGold)
		return "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2\n";
	else
		return "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7\n";
}


string Engine::doSearch(Board&) 
{ 
	return "here will be some search soon";
}


Node::Node()
{
}


Node::Node(Step& step)
{
  step_ = step;
}


void Node::addChild(Node* newChild)
{
  assert(newChild->sibling_ == NULL);
  assert(newChild->firstChild_ == NULL);
  
  newChild->sibling_ = this->firstChild_;
  this->firstChild_ = newChild->sibling_;
  
}


void Node::removeChild(Node* delChild)
{
  assert(this->firstChild_ == NULL);

  //TODO implement removal
  return;
  
}


Node* Node::getUctChild()
{
  assert(this->firstChild_ != NULL);

  //TODO implement UCB1
  return this->firstChild_;

}


Node* Node::getMostExploredChild()
{
  //TODO implement get most explored
  return this->firstChild_;
}

string Node::toString()
{
  stringstream ss;

  ss << step_.toString() << " " << visits_ << " " << value_ << endl;
  return ss.str();

}

SimplePlayout::SimplePlayout(Board* board)
{
	board_ = board;
	playoutLength_ = 0;
}


void SimplePlayout::playOne()
{
	Step step;

	do {
		#ifdef DEBUG_3
			board_->dump();
  		board_->dumpAllSteps();
		#endif
		#ifdef DEBUG_2
      board_->testPieceArray();  
		#endif
		step = board_->getRandomStep();
		board_->makeStep(step);
 // step.dump();
	//board_->dump();

	}
	while ( board_->getStepCount() < 4 && step.pieceMoved()); 
	board_->commitMove();
	return;
}


playoutStatus_e SimplePlayout::doPlayout()
{
  int moves = 0;

  while (true) {  
		playOne();
		playoutLength_++;

		if ( board_->getWinner() != EMPTY ) //somebody won
			return PLAYOUT_OK;

		if ( playoutLength_ > 2 * MAX_PLAYOUT_LENGTH ) 
			return PLAYOUT_TOO_LONG;

    if (++moves > EVAL_AFTER_LENGTH  )
      return PLAYOUT_EVAL;
	}
}

uint SimplePlayout::getPlayoutLength()
{
	return playoutLength_/2;
}
 //pointer debug
