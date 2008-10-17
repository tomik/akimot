
#include "engine.h"


string Engine::initialSetup(bool isGold)
{
	if (isGold)
		return "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2\n";
	else
		return "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7\n";
}


string Engine::doSearch(Board* board) 
{ 
  Uct* uct_ = new Uct(board);
  string result = uct_->generateMove();  
  delete uct_;
  return result;
  //return "here will be some search soon";
}


Node::Node()
{
}


Node::Node(const Step& step)
{
  firstChild_ = NULL;
  sibling_    = NULL;
  visits_     = 0;
  value_      = 0;
  
  step_ = step;
  nodeType_ = ( step.getStepPlayer() == GOLD ? NODE_MAX : NODE_MIN );
}


void Node::addChild(Node* newChild)
{
  assert(newChild->sibling_ == NULL);
  assert(newChild->firstChild_ == NULL);
  
  newChild->sibling_ = this->firstChild_;
  this->firstChild_ = newChild;
}


void Node::removeChild(Node* delChild)
{
  assert(this->firstChild_ != NULL);
  assert(delChild->firstChild_ == NULL);

  //TODO implement removal
  return;
  
}


void Node::expand(const StepArray& stepArray, uint len)
{
  //cout << "Expanding node" << endl;
  Node* newChild;
  for (uint i = 0; i < len; i++){
    newChild = new Node(stepArray[i]);
    addChild(newChild);
  }
}


float Node::ucb(float exploreCoeff)
{
  return ( nodeType_ == NODE_MAX ? value_ : - value_) + sqrt(exploreCoeff/visits_);
}


Node* Node::getUctChild()
{
  assert(this->firstChild_ != NULL);

  Node* act = this->firstChild_;
  Node* best = this->firstChild_;
  float bestUrgency = -100;       //TODO - something very large ! 
  float actUrgency = 0;

  float exploreCoeff = EXPLORE_RATE * log(visits_);

  while (act != NULL) {
    actUrgency = act->ucb(exploreCoeff);
    if ( actUrgency > bestUrgency ){
      best = act;
      bestUrgency = actUrgency;
    }
    act = act->sibling_;
  }

  return best;
}


Node* Node::getMostExploredChild()
{
  assert(this->firstChild_ != NULL);

  Node* best = this->firstChild_;
  Node* act = this->firstChild_;
  while (act != NULL) {
    if ( act->visits_ > best->visits_ )
      best = act;
    act = act->sibling_;
  }
  return best;
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
  value_ += (sample - value_)/visits_;         //TODO how this works ? 
}


bool Node::isMature() 
{
  return visits_ > MATURE_LEVEL;
}


bool Node::hasChildren() 
{
  return firstChild_ != NULL;
}


string Node::toString()
{
  stringstream ss;

  ss << step_.toString() << "(" <<  ( nodeType_  == NODE_MAX ? "NODE_MAX" : "NODE_MIN" )  << ") " << value_ << "/" << visits_ << endl;
  return ss.str();
}


string Node::recToString(int depth)
{
  stringstream ss; 
  for (int i = 0; i < depth; i++ )
    ss << "-";
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


string Tree::getBestMove()
{
  //TODO ... create move object and "undummyfy" this method 
  return root()->getMostExploredChild()->toString();
}


string Tree::toString()
{
  return root()->recToString(0);
}


Uct::Uct()
{
  assert(false);      //use only constructore with board ! 
}


Uct::Uct(Board* board) 
{
  board_ = board;
  tree_  = new Tree();
}


Uct::~Uct()
{
  delete tree_;
}


void Uct::doPlayout()
{
  //TODO ... change playBoard to an object not pointer
  Board *playBoard = new Board(*board_);
  playoutStatus_e playoutStatus;

  StepArray stepArray;    
  uint      stepArrayLen;
  
  tree_->historyReset();     //point tree's actNode to the root 

  //TODO ... what if the node in the tree is the end of the game already ? 
  do { 
    if (! tree_->actNode()->hasChildren()) { 
      if (tree_->actNode()->isMature()) {
        stepArrayLen = playBoard->generateAllSteps(playBoard->getPlayerToMove(),stepArray);
        tree_->actNode()->expand(stepArray,stepArrayLen);
        continue;
      }

      //random playout
      SimplePlayout simplePlayout(playBoard);
      playoutStatus = simplePlayout.doPlayout();
      break;
    }

    tree_->uctDescend(); 
    Step step = tree_->actNode()->getStep();
    playBoard->makeStepTryCommit(step);
    //TODO add stepCorrectness check and delete node when it is not correct ( i.e. position repetition, wrong passing, etc.)
  } while(true);

  
  tree_->updateHistory( decidePlayoutWinner(playBoard, playoutStatus));
  delete playBoard;
}


string Uct::generateMove()
{
  cout << "Starting uct" << endl;
  for ( int i = 0; i < GEN_MOVE_PLAYOUTS; i++) {
    //cout << i << " | "; 
    doPlayout();
  }
  cout << "Playout over" << endl;
  cout << tree_->toString();
  return tree_->getBestMove();
}


/*if winner is known returns 1 for Gold, -1 for Silver 
 * otherwise flips the coin according to the evaluationPercentage estimate and returns 1/-1 as above*/
int Uct::decidePlayoutWinner(Board* playBoard, playoutStatus_e playoutStatus)
{
  if IS_PLAYER(playBoard->getWinner())
    return playBoard->getWinner() == GOLD ? 1 : -1;

  float evalGold = playBoard->evaluateInPercent(GOLD);
  double r = (double)rand()/((double)(RAND_MAX) + (double)(1));
  if (r < evalGold)
    return 1;
  else
    return -1;
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
	}
	while (board_->getStepCount() < 4 && step.pieceMoved()); 
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
