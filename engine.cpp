
#include "engine.h"

/** 
 *  @file engine.cpp
 *  @brief Searching engine. 
 *  @full Implementation of UCT and supporting components.
 */

//---------------------------------------------------------------------
/* section Node*/
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
  visits_     = 0;
  value_      = 0;

  step_ = step;
  assert(IS_PLAYER(step.getStepPlayer()));
  nodeType_ = ( step.getStepPlayer() == GOLD ? NODE_MAX : NODE_MIN );
}

//---------------------------------------------------------------------

void Node::expand(const StepArray& steps, uint len)
{
  //cout << "Expanding node" << endl;
  Node* newChild;
  for (uint i = 0; i < len; i++){
    newChild = new Node(steps[i]);
    addChild(newChild);
  }
}

//---------------------------------------------------------------------

Node* Node::findUctChild() const
{
  assert(this->firstChild_ != NULL);

  Node* act = firstChild_;
  Node* best = firstChild_;
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
  Node* node = firstChild_;

  while(node != child && node != NULL){
    previous = node;
    node = node->sibling_;
  }
  assert(node == child);

  if (previous == NULL)
    firstChild_ = node->sibling_;
  else
    previous->sibling_ = node->sibling_;


  //handle number of visits/value in father
  father_->visits_ -= visits_;
  father_->value_  -= value_;
  assert(father_->visits_ >= 0);
  
  delete node;
  if (firstChild_ == NULL){
    remove();
  }
}

//---------------------------------------------------------------------

void Node::remove()
{
  assert(firstChild_ == NULL);
  assert(father_ != NULL);

  //TODO: father==NULL -> root should be removed ? 
  if (father_ != NULL) 
    father_->removeChild(this);
  return;
  
}

//---------------------------------------------------------------------

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

//---------------------------------------------------------------------

void Node::update(float sample)
{
  visits_++;
  value_ += (sample - value_)/visits_;         //TODO how this works ? 
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
  assert(false);
}

//---------------------------------------------------------------------

Tree::Tree(player_t firstPlayer)
{
  historyTop = 0;
  history[historyTop] = new Node(Step(STEP_NO_STEP, firstPlayer));
}

//---------------------------------------------------------------------

Tree::~Tree()
{
  history[0]->freeChildren();
  delete history[0];
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

string Tree::findBestMove(Node* bestMoveNode,const Board* boardGiven)
{

  Board *board = new Board (*boardGiven);

  Node* act = bestMoveNode; 

  //steps in best Move
  StepWithKills  steps[4];
  uint stepNum = 0;

  if (! act) {     //TODO what to do ? he hasn't reached 4th level in  
    assert(false);
  }

  //log_() << "Best move visited :" << bestMoveNode->getVisits() << endl;

  while (act != root()) {
    assert(stepNum < 4);
    steps[stepNum++] = StepWithKills(act->getStep());
    act = act->getFather(); 
  } 

  assert(stepNum > 0);

  stringstream ss; 
  ss.str("");
  //print the steps 
  for (int i = stepNum - 1; i >= 0; i--){
    steps[i].addKills(board);
    board->makeStep(steps[i]);
    ss << trimRight(steps[i].toString()) << " ";   //resultPrint of move
  } 

  //add signature of final position ! -> for future thirdRepetitionCheck
  //move is not commited - therefore it is 1 - index_of_player_to_move
  thirdRep.update(board->getSignature(), 1 - PLAYER_TO_INDEX(board->getPlayerToMove()) );

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
/* section SimplePlayout*/
//---------------------------------------------------------------------

SimplePlayout::SimplePlayout()
{
  assert(false);
}

//---------------------------------------------------------------------

SimplePlayout::SimplePlayout(Board* board)
{
	board_ = board;
	playoutLength_ = 0;
}

//---------------------------------------------------------------------

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

//---------------------------------------------------------------------

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
		step = board_->findStepToPlay();
		board_->makeStep(step);
	}
	while (board_->getStepCount() < 4 && step.pieceMoved()); 
	board_->commitMove();
	return;
}

//---------------------------------------------------------------------

uint SimplePlayout::getPlayoutLength()
{
	return playoutLength_/2;
}

//---------------------------------------------------------------------
/* section Uct*/
//---------------------------------------------------------------------

Uct::Uct()
{
  assert(false);      //use only constructor with board ! 
}

//---------------------------------------------------------------------

Uct::Uct(Board* board) 
{
  board_ = board;
  tree_  = new Tree(board->getPlayerToMove());
  eval_  = new Eval();
  tt_    = new TT();
  bestMoveNode_ = NULL;
  nodesPruned_ = 0;
  nodesExpanded_ = 0;
  nodesInTheTree_ = 1;
  playouts_ = 0;
}

//---------------------------------------------------------------------

Uct::~Uct()
{
  delete eval_;
  delete tree_;
  delete tt_;
}

//---------------------------------------------------------------------

void Uct::doPlayout()
{
  //TODO ... change playBoard to an object not pointer
  Board *playBoard = new Board(*board_);
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
          tree_->actNode()->expand(steps,stepsNum);
          updateTT(tree_->actNode()->getFirstChild(), playBoard); 
          nodesExpanded_++;
          nodesInTheTree_ += stepsNum;
        }else{
          tree_->actNode()->remove();
          //TODO remove might cause a cascade
          nodesInTheTree_ -= 1; 
          break;
        }

        continue;
      }

      //"random" playout
      SimplePlayout simplePlayout(playBoard);
      playoutStatus = simplePlayout.doPlayout();
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

int Uct::decidePlayoutWinner(Board* playBoard, playoutStatus_e playoutStatus)
{
  if IS_PLAYER(playBoard->getWinner())
    return playBoard->getWinner() == GOLD ? 1 : -1;

  float evalGold = eval_->evaluateInPercent(playBoard);
  double r = (double)rand()/((double)(RAND_MAX) + (double)(1));
  if (r < evalGold)
    return 1;
  else
    return -1;
}

//--------------------------------------------------------------------- 

int Uct::filterTT(StepArray& steps, uint stepsNum, Board* board)
{
  uint i = 0;
  u64 afterStepSignature;
  //player_t newNodePlayer;
  //Node* node

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

void Uct::updateTT(Node* nodeList, Board* board)
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

string Uct::statisticsToString(float seconds)
{
  assert(seconds > 0);
  stringstream ss;

    ss  << "UCT: " << endl
        << "  " << playouts_ << " playouts" << endl 
        << "  " << seconds << " seconds" << endl
        << "  " << int(playouts_ / seconds) << " playouts per second" << endl
        << "  " << nodesInTheTree_ << " nodes in the tree" << endl 
        << "  " << nodesExpanded_ << " nodes expanded" << endl 
        << "  " << nodesPruned_ << " nodes pruned" << endl 
      ;

  return ss.str();
}

//---------------------------------------------------------------------

string Uct::getBestMove()
{
  assert(bestMoveNode_ != NULL);
  return tree_->findBestMove(bestMoveNode_, board_);
}

//---------------------------------------------------------------------

float Uct::getBestMoveValue()
{
  assert(bestMoveNode_);
  return bestMoveNode_->getValue();
}

//--------------------------------------------------------------------- 

Tree* Uct::getTree() const
{
  return tree_;
}

//---------------------------------------------------------------------
//  section TimeManager
//---------------------------------------------------------------------

TimeManager::TimeManager()
{
  noTimeLimit_ = false;
  secondsPerMove_ = 3;  //TODO change 
}

//--------------------------------------------------------------------- 

void TimeManager::startClock()
{
  clockBegin_ = clock();
}

//---------------------------------------------------------------------

bool TimeManager::checkClock()
{
  if (secondsElapsed() < secondsPerMove_ || noTimeLimit_)
    return true;
  return false;
}

//---------------------------------------------------------------------

float TimeManager::secondsElapsed()
{
  assert(clock() >= clockBegin_);

  return float(clock() - clockBegin_)/CLOCKS_PER_SEC;
}
  
//---------------------------------------------------------------------

void TimeManager::setTimeOption(timeOption_e option, bool value)
{
  switch (option){
    case TO_INFINITE: 
            noTimeLimit_ = value;
            break;
    default: assert(false);
            break;
  }
}

//---------------------------------------------------------------------
//  section Engine
//---------------------------------------------------------------------

Engine::Engine()
{
}

//--------------------------------------------------------------------- 

Engine::~Engine()
{
}

//--------------------------------------------------------------------- 

string Engine::initialSetup(bool isGold)
{
	if (isGold)
		return "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2\n";
	else
		return "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7\n";
}

//---------------------------------------------------------------------

void Engine::doSearch(Board* board) 
{ 
  uct_ = new Uct(board);
  timeManager_ = new TimeManager();
  stopRequest_ = false;

  timeManager_->startClock();
  while (timeManager_->checkClock() && ! stopRequest_)
    uct_->doPlayout();

  //log_() << board->toString() << endl;
  //log_() << uct_->statisticsToString(timeManager_->secondsElapsed());
  bestMove_ = uct_->getBestMove(); 
  delete uct_;
}

//---------------------------------------------------------------------

void Engine::requestSearchStop()
{
  stopRequest_ = true;
}

//---------------------------------------------------------------------

string Engine::getBestMove()
{
  return bestMove_; 
}

//---------------------------------------------------------------------

TimeManager* Engine::timeManager()
{
  return timeManager_;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
