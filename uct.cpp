/** 
 *  @file uct.cpp
 *  @brief Uct algorithm. 
 *  @full Uct algorithm implementation.
 */

#include "uct.h"

//---------------------------------------------------------------------
//  section SimplePlayout
//---------------------------------------------------------------------

SimplePlayout::SimplePlayout()
{
  assert(false);
}

//---------------------------------------------------------------------

SimplePlayout::SimplePlayout(Board* board, uint maxPlayoutLength, uint evalAfterLength):
  board_(board), playoutLength_(0), maxPlayoutLength_(maxPlayoutLength), evalAfterLength_(evalAfterLength)
{
  ;
}

//---------------------------------------------------------------------

playoutStatus_e SimplePlayout::doPlayout()
{
  uint moves = 0;

  while (true) {  
		playOne();
		playoutLength_++;

		if ( board_->getWinner() != EMPTY ) //somebody won
			return PLAYOUT_OK;

		if ( playoutLength_ > 2 * maxPlayoutLength_ ) 
			return PLAYOUT_TOO_LONG;

    if (++moves > evalAfterLength_ && evalAfterLength_)
      return PLAYOUT_EVAL;
	}
}

//---------------------------------------------------------------------

void SimplePlayout::playOne()
{
	Step step;

	do {
		step = board_->findStepToPlay();
		//board_->makeStep(step);
	}
  //TODO IS this correct ? 
	while (! board_->makeStepTryCommitMove(step));
  //(board_->getStepCount() < 4 && step.pieceMoved()); 
	//board_->commitMove();
	return;
}

//---------------------------------------------------------------------

uint SimplePlayout::getPlayoutLength()
{
	return playoutLength_/2;
}

//---------------------------------------------------------------------
// section SearchExt
//---------------------------------------------------------------------

bool SearchExt::quickGoalCheck(const Board* board, player_t player, int stepsLimit)
{
  return board->quickGoalCheck(player, stepsLimit);
}

//---------------------------------------------------------------------
// section Node
//---------------------------------------------------------------------

Node::Node()
{
  assert(false);
}

//---------------------------------------------------------------------

Node::Node(const Step& step)
{
  firstChild_ = NULL;
  sibling_    = NULL;
  father_     = NULL;  
  bestCached_ = NULL;
  visits_     = FPU;
  value_      = FPU;

  step_ = step;
  assert(IS_PLAYER(step.getStepPlayer()));
  nodeType_ = ( step.getStepPlayer() == GOLD ? NODE_MAX : NODE_MIN );
}

//---------------------------------------------------------------------

Node* Node::findUctChild() 
{
  assert(this->firstChild_ != NULL);
  
  if (bestCached_)
    return bestCached_;

  Node* act = firstChild_;
  Node* best = firstChild_;
  float bestUrgency = -100;       //TODO - something small !  
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

  //store best one - might be invalidated in update 
  bestCached_ = best;

  return best;
}

//---------------------------------------------------------------------

Node* Node::findRandomChild() const
{
  Node* node = firstChild_;
  assert(node != NULL);
  int childNum = 0; 

  //count children
  while (node != NULL) {
    node = node->sibling_;
    childNum++; 
  }    

  //select children to be played  
  childNum = random() % childNum;

  node = firstChild_;
  for (int i=0; i < childNum; i++){
    assert(node != NULL);
    node = node->sibling_;
  }
  return node; 
}

//---------------------------------------------------------------------

Node* Node::findMostExploredChild() const
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

//---------------------------------------------------------------------

float Node::ucb(float exploreCoeff) const
{
  //nasty trick ! for first run returns inf ( infinity ) since visits_ == 0   
  return (( nodeType_ == NODE_MAX ? value_ : - value_) + sqrt((exploreCoeff/visits_)));
}

//---------------------------------------------------------------------

void Node::addChild(Node* newChild)
{
  assert(newChild->sibling_ == NULL);
  assert(newChild->firstChild_ == NULL);
  
  newChild->sibling_ = this->firstChild_;
  this->firstChild_ = newChild;
  newChild->father_ = this;
}
 
//---------------------------------------------------------------------

void Node::removeChild(Node* child)
{
  assert(this->firstChild_ != NULL);
  Node* previous = NULL;
  Node* toDelete = firstChild_;

  while(toDelete != child && toDelete != NULL){
    previous = toDelete;
    toDelete = toDelete->sibling_;
  }
  assert(toDelete == child);
  if (toDelete == NULL)
    return;

  if (previous == NULL)
    firstChild_ = toDelete->sibling_;
  else
    previous->sibling_ = toDelete->sibling_;
  
  //handle number of visits/wins num in ancestors
  //value_ = (value_ * visits_ - toDelete->value_ * toDelete->visits_)/TODO;
  visits_ -= toDelete->visits_;
  assert(visits_ >= 0);
  //wins_ -= toDelete->wins_;
  
  bestCached_ = NULL; 
  delete toDelete;
}

//---------------------------------------------------------------------

void Node::removeChildrenRec()
{
  Node* actNode = firstChild_;
  Node* sibling;
  while(actNode != NULL){
    sibling = actNode->sibling_;
    actNode->removeChildrenRec();
    delete actNode;
    actNode = sibling;
  }
}

//---------------------------------------------------------------------

void Node::update(float sample)
{
  visits_++;
  value_ += (sample - value_)/visits_;         
  if ((nodeType_ == NODE_MAX && sample == -1) || 
      (nodeType_ == NODE_MIN && sample == 1)) {
    bestCached_ = NULL;
  }

}

//---------------------------------------------------------------------

bool Node::isMature() const
{
  return visits_ > MATURE_LEVEL;
}

//---------------------------------------------------------------------

bool Node::hasChildren() const
{
  return firstChild_ != NULL;
}

//---------------------------------------------------------------------

bool Node::hasOneChild() const
{
  return firstChild_ != NULL && firstChild_->sibling_ == NULL;
}

//---------------------------------------------------------------------

Node* Node::getFather() const
{
  return father_;
}

//---------------------------------------------------------------------

Node* Node::getFirstChild() const
{
  return firstChild_;
}

//---------------------------------------------------------------------

Node* Node::getSibling() const
{
  return sibling_;
}

//---------------------------------------------------------------------

Step Node::getStep() const
{
  return step_;
}

//---------------------------------------------------------------------

int Node::getVisits() const
{
  return visits_;
}

//---------------------------------------------------------------------

float Node::getValue() const
{
  return value_;
}

//---------------------------------------------------------------------

nodeType_e Node::getNodeType() const
{
  return nodeType_;
}

//---------------------------------------------------------------------

string Node::toString() const
{
  stringstream ss;

  ss << step_.toString() << "(" <<  ( nodeType_  == NODE_MAX ? "+" : "-" )  << ") " << value_ << "/" << visits_ << endl;
  return ss.str();
}

//---------------------------------------------------------------------

string Node::recToString(int depth) const
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

//---------------------------------------------------------------------
//  section Tree
//---------------------------------------------------------------------

Tree::Tree()
{
  history[0] = NULL; 
}

//--------------------------------------------------------------------- 

Tree::Tree(player_t firstPlayer)
{
  history[0] = NULL; 
  reset(firstPlayer);
}

//--------------------------------------------------------------------- 

Tree::~Tree()
{
  history[0]->removeChildrenRec();
  delete history[0];
}

//--------------------------------------------------------------------- 

void Tree::reset(player_t firstPlayer)
{
  if (history[0] != NULL){
    history[0]->removeChildrenRec();
    delete history[0];
  }
  historyTop = 0;
  history[historyTop] = new Node(Step(STEP_NO_STEP, firstPlayer));
  nodesExpandedNum_ = 0;
  nodesNum_ = 1;
}

//---------------------------------------------------------------------

void Tree::expandNode(Node* node, const StepArray& steps, uint len)
{
  Node* newChild;
  for (uint i = 0; i < len; i++){
    newChild = new Node(steps[i]);
    node->addChild(newChild);
    nodesNum_++;
  }
  nodesExpandedNum_++;
}

//---------------------------------------------------------------------

void Tree::uctDescend()
{
  history[historyTop + 1]= actNode()->findUctChild();
  historyTop++;
  assert(actNode() != NULL);
}

//---------------------------------------------------------------------

void Tree::randomDescend()
{
  history[historyTop + 1]= actNode()->findRandomChild();
  historyTop++;
  assert(actNode() != NULL);
}

//---------------------------------------------------------------------

string Tree::findBestMoveRepr(Node* bestMoveNode,const Board* boardGiven)
{

  Board *board = new Board (*boardGiven);

  Node* act = bestMoveNode; 

  //steps in best Move
  StepWithKills  steps[4];
  uint stepNum = 0;

  if (! act) {     //TODO what to do ? he hasn't reached 4th level in  
    assert(false);
  }

  while (act != root()) {
    assert(stepNum < 4);
    steps[stepNum++] = StepWithKills(act->getStep());
    act = act->getFather(); 
  } 

  assert(stepNum > 0);

  stringstream ss; 
  ss.str("");

  //print the steps 
  bool commited = false; 
  for (int i = stepNum - 1; i >= 0; i--){
    steps[i].addKills(board);
    assert(! commited);
    commited = board->makeStepTryCommitMove(steps[i]);
    ss << trimRight(steps[i].toString()) << " ";   //resultPrint of move
  } 
  assert(commited);
  
  //add signature of final position ! -> for future thirdRepetitionCheck
  //move is not commited - therefore it is 1 - index_of_player_to_move
  thirdRep.update(board->getSignature(), PLAYER_TO_INDEX(board->getPlayerToMove()) );

  delete(board);
  return ss.str(); 
} 

//---------------------------------------------------------------------

void Tree::updateHistory(float sample)
{
  for(int i = historyTop; i >= 0; i--)
    history[i]->update(sample);
  historyReset();
} 

//---------------------------------------------------------------------

void Tree::historyReset()
{
  historyTop = 0;
}

//---------------------------------------------------------------------

Node* Tree::root() 
{
  return history[0];
}

//---------------------------------------------------------------------

Node* Tree::actNode() 
{
  return history[historyTop];
}

//--------------------------------------------------------------------- 

int Tree::removeNodeCascade(Node* node)
{
  assert(node != NULL);
  assert(! node->hasChildren());

  Node* ancestor = node->getFather();
  Node* toRemove = node;

  int removedNodes = 0;

  while (ancestor != NULL && ancestor->hasOneChild()){
    toRemove = ancestor;
    ancestor = ancestor->getFather();
    removedNodes++;
  } 

  assert(ancestor != NULL); //removing root
  //should never happen !!!
  if (ancestor == NULL)
    return 0;
    
  toRemove->removeChildrenRec();
  ancestor->removeChild(toRemove);
  nodesNum_ -= removedNodes;

  //updates actual node
  historyReset();
  return removedNodes;
}

//---------------------------------------------------------------------

int Tree::getNodeDepth(Node* node) 
{
  assert(node != NULL);
  int depth = 0;

  while (node != root()){
    node = node->getFather();
    depth++;
  }
  return depth;
  
}

//--------------------------------------------------------------------- 

int Tree::getNodesNum() 
{
    return nodesNum_;
}

//--------------------------------------------------------------------- 

int Tree::getNodesExpandedNum() 
{
    return nodesExpandedNum_;
}

//--------------------------------------------------------------------- 

string Tree::toString() {
  return root()->recToString(0);
}

//---------------------------------------------------------------------

string Tree::pathToActToString(bool onlyLastMove )
{
  Node* act = actNode();
  string s;
  while (act != root() && ( ! onlyLastMove || act->getNodeType() == actNode()->getNodeType())) {
    s = act->getStep().toString() + s; 
    act = act->getFather(); 
  } 

  return s;
}

//---------------------------------------------------------------------
//  section Uct
//---------------------------------------------------------------------

Uct::Uct(): Engine()
{
  tree_  = new Tree();
  eval_  = new Eval();
  tt_    = NULL;

  searchExt_ = new SearchExt();

}

//---------------------------------------------------------------------

Uct::~Uct()
{
  delete eval_;
  delete tree_;
  delete tt_;
}

void Uct::reset(const Board* board)
{
  if (tt_){
    delete tt_;
  }
    tt_ = new TT();

  bestMoveNode_ = NULL;
  bestMoveRepr_ = "";
  nodesPruned_ = 0;
  playouts_ = 0;

  tree_->reset(board->getPlayerToMove());

}

//---------------------------------------------------------------------

void Uct::searchTree(const Board* board)
{
  reset(board);
  while (! checkSearchStop())
    doPlayout(board);

  assert(bestMoveNode_ != NULL);
  bestMoveRepr_ = tree_->findBestMoveRepr(bestMoveNode_, board);
}

//---------------------------------------------------------------------

void Uct::doPlayout(const Board* board)
{
  Board *playBoard = new Board(*board);
  playoutStatus_e playoutStatus;
  Node* MoveNode = NULL;
  int descendNum = 0;

  playouts_++;

  StepArray steps;    
  uint      stepsNum;
  
  tree_->historyReset();     //point tree's actNode to the root 

  do { 
    if (! tree_->actNode()->hasChildren()) { 
      if (tree_->actNode()->isMature()) {
        stepsNum = playBoard->generateAllSteps(playBoard->getPlayerToMove(),steps);
        stepsNum = playBoard->filterRepetitions(steps, stepsNum);

          stepsNum = filterTT(steps, stepsNum, playBoard); 
        if (stepsNum > 0) {
          tree_->expandNode(tree_->actNode(), steps, stepsNum);
          updateTT(tree_->actNode()->getFirstChild(), playBoard); 
        }else{
          tree_->removeNodeCascade(tree_->actNode());
          break;
        }

        continue;
      }

      //"random" playout
      SimplePlayout simplePlayout(playBoard, MAX_PLAYOUT_LENGTH, EVAL_AFTER_LENGTH);
      playoutStatus = simplePlayout.doPlayout();
      //playoutStatus = PLAYOUT_EVAL;
      tree_->updateHistory( decidePlayoutWinner(playBoard, playoutStatus));
      break;
    }

    tree_->uctDescend(); 
    descendNum++;

    Step step = tree_->actNode()->getStep();
    player_t playerAfterStep = playBoard->getPlayerToMoveAfterStep(step);
    //determine which node represents (most visited) end of the first move - for output
    //if at the end of first move ... 
    if (MoveNode == NULL && 
        (tree_->root()->getNodeType() != PLAYER_TO_NODE_TYPE(playerAfterStep))){
      MoveNode = tree_->actNode();
      //check actual node is better than so far hold one
      if ((! bestMoveNode_) || (bestMoveNode_->getVisits() < MoveNode->getVisits() + 1 ))
        bestMoveNode_ = MoveNode;
    }

    //perform the step and try commit
    if ( playBoard->makeStepTryCommitMove(step) )   {
      //commit was successful - check whether winning criteria are not reached already
      if ( playBoard->getWinner() != EMPTY ) {
        tree_->updateHistory(playBoard->getWinner() == GOLD ? 1 : -1); 
        break;  //winner is known already in the uct tree -> no need to go deeper
      }
    }

  } while(true);
  
  delete playBoard;
}

//--------------------------------------------------------------------- 

string Uct::getStats() const
{
  //TODO this is only place where timemanager is used - make private in engine! 
  float seconds = timeManager_->secondsElapsed();
  assert(seconds > 0);
  stringstream ss;

    ss  << "UCT: " << endl
        << "  " << playouts_ << " playouts" << endl 
        << "  " << seconds << " seconds" << endl
        << "  " << int(playouts_ / seconds) << " playouts per second" << endl
        << "  " << tree_->getNodesNum() << " nodes in the tree" << endl 
        << "  " << tree_->getNodesExpandedNum() << " nodes expanded" << endl 
        //<< "  " << tree_->getLongestVariant() << " nodes in longest path" << endl
        << "  " << nodesPruned_ << " nodes pruned" << endl 
      ;

  return ss.str();
}

//---------------------------------------------------------------------

string Uct::getBestMoveRepr() const
{
  return bestMoveRepr_;
}

//---------------------------------------------------------------------

float Uct::getBestMoveValue() const
{
  assert(bestMoveNode_);
  return bestMoveNode_->getValue();
}

//--------------------------------------------------------------------- 

float Uct::getWinRatio() const
{
  return (bestMoveNode_) ? (bestMoveNode_->getValue() + 1 )/2 : 0.5;
}

//--------------------------------------------------------------------- 

int Uct::decidePlayoutWinner(const Board* playBoard, playoutStatus_e playoutStatus) const
{
  if IS_PLAYER(playBoard->getWinner())
    return playBoard->getWinner() == GOLD ? 1 : -1;

  //TODO test this
  float evalGold = eval_->evaluateInPercent(playBoard);
  double r = (double)rand()/((double)(RAND_MAX) + (double)(1));
  if (r < evalGold)
    return 1;
  else
    return -1;
}

//--------------------------------------------------------------------- 

int Uct::filterTT(StepArray& steps, uint stepsNum, const Board* board)
{
  uint i = 0;
  u64 afterStepSignature;

  while (i < stepsNum){
    afterStepSignature = board->calcAfterStepSignature(steps[i]);
    if (tt_->hasItem(afterStepSignature, 
                     PLAYER_TO_INDEX(board->getPlayerToMoveAfterStep(steps[i])))){
      //TODO change this policy     
      //if node with same position already exists in the tree new node is not added 
      steps[i] = steps[stepsNum-- - 1];
      nodesPruned_++;
      continue;
    }
    i++; 
  }

  return stepsNum;
}  

//--------------------------------------------------------------------- 

void Uct::updateTT(Node* nodeList, const Board* board)
{
  assert(nodeList != NULL); 
  Node* node = nodeList; 
  u64 afterStepSignature;

  while (node != NULL){
    afterStepSignature = board->calcAfterStepSignature(node->getStep());
    tt_->insertItem(afterStepSignature,
                    PLAYER_TO_INDEX(board->getPlayerToMoveAfterStep(node->getStep())), 
                    node);
    node = node->getSibling();
  }
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

