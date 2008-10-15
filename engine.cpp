
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
  assert(this->firstChild_ != NULL);
  assert(delChild->firstChild_ == NULL);

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


Step Node::getStep()
{
  return step_;
}

/*
 * Recursive method for freeing nodes children
 */
void Node::freeChildren()
{
  Node* actNode = firstChild_;
  Node* sibling;
  while(actNode != NULL){
    sibling = actNode->sibling_;
    actNode->freeChildren();
    delete actNode;
    actNode = sibling;
  }
}

void Node::update(float sample)
{
  visits_++;
  value_ += sample;
}


bool Node::isMature() 
{
  //TODO use constant
  return visits_ > 100;
}

bool Node::hasChildren() 
{
  return firstChild_ != NULL;
}

string Node::toString()
{
  stringstream ss;

  ss << step_.toString() << " " << visits_ << " " << value_ << endl;
  return ss.str();
}

string Node::recToString(int depth)
{
  stringstream ss; 
  for (int i = 0; i < depth; i++ )
    ss << " ";
  ss << toString();
  Node* actNode = firstChild_;
  while(actNode != NULL){
    ss << actNode->recToString(depth + 1);
    actNode = actNode->sibling_;
  }
  return ss.str();
}


Tree::Tree()
{
  historyTop = 0;
  history[historyTop] = new Node();
}

Tree::~Tree()
{
  history[0]->freeChildren();
  delete history[0];
}

Node* Tree::root() 
{
  return history[0];
}

Node* Tree::actNode() 
{
  return history[historyTop];
}

void Tree::historyReset()
{
  historyTop = 0;
}

void Tree::uctDescend()
{
  history[historyTop + 1]= actNode()->getUctChild();
  historyTop++;
  assert(actNode() != NULL);
}

void Tree::updateHistory(float sample)
{
  for(int i = historyTop; i >= 0; i--)
    history[i]->update(sample);
  historyReset();
} 


string Tree::toString()
{
  return root()->recToString(0);
}


Uct::Uct()
{
}

Uct::Uct(Board* board) 
{
  board_ = board;
}

void Uct::doPlayout()
{
  //TODO ... change playBoard to an object not pointer
  Board *playBoard = new Board(*board_);
  playoutStatus_e playoutStatus;
  
  tree_.historyReset();     //point tree's actNode to the root 

  //TODO ... what if the node in the tree is the end of the game already ? 

  do { 

    if (! tree_.actNode()->hasChildren()) { 
      if (tree_.actNode()->isMature()) {
        //TODO node expansion
        continue;
      }

      //random playout
      SimplePlayout simplePlayout(playBoard);
      playoutStatus = simplePlayout.doPlayout();
      break;
    }

    tree_.uctDescend(); 
    Step step = tree_.actNode()->getStep();
    playBoard->makeStep(step);
    //TODO makeStep needs wrapper to watch the whole moves, maybe hash them as well
    //maybe add stepCorrectness check and delete node when it is not correct ( i.e. position repetition, wrong passing, etc.)
  } while(true);

  
  tree_.updateHistory( decidePlayoutWinner(playBoard, playoutStatus));
}


int Uct::decidePlayoutWinner(Board* playBoard, playoutStatus_e playoutStatus)
{
  return -1;  
  //TODO change function so it returns 1/-1
  if IS_PLAYER(playBoard->getWinner())
    return playBoard->getWinner();

  float evalGold = playBoard->evaluateInPercent(GOLD);
  double r = ((double)rand()/(double)(RAND_MAX) + (double)(1));
  if (r < evalGold)
    return GOLD;
  else
    return SILVER;
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
