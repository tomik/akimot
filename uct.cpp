/** 
 *  @file uct.cpp
 *  @brief Uct algorithm. 
 *  @full Uct algorithm implementation.
 */

#include "engine.h"
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
  /*if (options.localMode()){
    cerr << "X                 PLAYOUT START                X " << endl;
  }
  */

  while (true) {  
	  //playOne();
    board_->findMCmoveAndMake();
		playoutLength_++;


		if ( board_->getWinner() != EMPTY ) //somebody won
			return PLAYOUT_OK;

		if ( playoutLength_ > 2 * maxPlayoutLength_ ) 
			return PLAYOUT_TOO_LONG;

    if (++moves >= evalAfterLength_ && evalAfterLength_)
      return PLAYOUT_EVAL;
	}
}

//---------------------------------------------------------------------

void SimplePlayout::playOne()
{
	Step step;

	do {
		step = board_->findMCstep();
	}
	while (! board_->makeStepTryCommitMove(step));
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
// section TWstep
//---------------------------------------------------------------------


TWstep::TWstep(Step s, float val, int vis) :
step(s), value(val), visits(vis)
{
}

//---------------------------------------------------------------------
// section TWsteps 
//---------------------------------------------------------------------

TWstep& TWsteps::operator[](const Step& step)
{ 
  TWstepsMap::iterator it = TWstepsMap::find(step);
  if (it != TWstepsMap::end()){
    return it->second;
  }

  pair< TWstepsMap::iterator, bool> p;
  p = insert(make_pair(step, TWstep(step, 0, 0)));
  assert(p.second);
  return p.first->second;
}

//---------------------------------------------------------------------
// section Node
//---------------------------------------------------------------------

Node::Node()
{
  assert(false);
}

//---------------------------------------------------------------------

Node::Node(TWstep* twStep, float heur)
{
  assert(IS_PLAYER(twStep->step.getPlayer()));
  nodeType_ = ( twStep->step.getPlayer() == GOLD ? NODE_MAX : NODE_MIN );
  firstChild_ = NULL;
  sibling_    = NULL;
  father_     = NULL;  
  bestCached_ = NULL;
  visits_     = 0;
  value_      = 0; 
  heur_ = heur;
  twStep_ = twStep;
}

//---------------------------------------------------------------------

Node* Node::findUctChild() 
{
  assert(this->firstChild_ != NULL);
  
  //if (bestCached_)
  //  return bestCached_;

  Node* act = firstChild_;
  Node* best = firstChild_;
  float bestUrgency = INT_MIN;   
  float actUrgency = 0;

  float exploreCoeff = EXPLORE_RATE * log(visits_);

  while (act != NULL) {
    actUrgency = (act->visits_ == 0 ? cfg.fpu() : act->ucb(exploreCoeff));

    if ( actUrgency > bestUrgency ){
      best = act;
      bestUrgency = actUrgency;
    }
    act = act->sibling_;
  }

  //store best one - might be invalidated in update 
  //bestCached_ = best;

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
  return (nodeType_ == NODE_MAX ? value_ : - value_) 
         + sqrt((exploreCoeff/visits_)) 
         + heur_/visits_ 
         + (nodeType_ == NODE_MAX ? twStep_->value : - twStep_->value)/sqrt(visits_);
  
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
  
  //bestCached_ = NULL; 
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
  value_ += (sample - value_)/++visits_;         
  //invalidate cache if neccessary
 /* if ((nodeType_ == NODE_MAX && sample == -1) || 
      (nodeType_ == NODE_MIN && sample == 1)) {
    bestCached_ = NULL;
  }
  */

}

//--------------------------------------------------------------------- 

void Node::updateTWstep(float sample)
{
  twStep_->value += (sample - twStep_->value) / ++(twStep_->visits);
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

void Node::setFather(Node* father)
{
  father_ = father;
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

void Node::setSibling(Node* node) 
{ 
  sibling_ = node; 
}

//---------------------------------------------------------------------

void Node::setFirstChild(Node* node) 
{ 
  firstChild_ = node; 
}

//---------------------------------------------------------------------

Step Node::getStep() const
{
  assert(twStep_);
  return twStep_->step;
}

//---------------------------------------------------------------------

TWstep* Node::getTWstep() const 
{
  return twStep_; 
}

//---------------------------------------------------------------------

player_t Node::getPlayer() const
{
  return getStep().getPlayer();
}

//---------------------------------------------------------------------

int Node::getVisits() const
{
  return visits_;
}

//---------------------------------------------------------------------

void Node::setVisits(int visits) 
{
  visits_ = visits;
}

//---------------------------------------------------------------------

float Node::getValue() const
{
  return value_;
}

//---------------------------------------------------------------------

void Node::setValue(float value) 
{
  value_ = value;
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

  ss << getStep().toString() << "(" <<  ( nodeType_  == NODE_MAX ? "+" : "-" )  << ") " << value_ << "/" << visits_ << " -> " 
    << (father_ != NULL ? ucb(EXPLORE_RATE * log(father_->visits_)) : 1 )
    << endl;
  return ss.str();
}

//---------------------------------------------------------------------

string Node::recToString(int depth) const
{
  const float print_visit_threshold_base    = 500;
  const float print_visit_threshold_parent  = 0.05;

  float minVisitCount = print_visit_threshold_base + 
                        visits_ * print_visit_threshold_parent; 

  stringstream ss; 
  for (int i = 0; i < depth; i++ )
    ss << "   ";
  ss << toString();

  Node* best = NULL; 

  typedef set<Node* > nodeTab;
  nodeTab tab;
  
  Node* actNode = firstChild_;
  while(actNode != NULL){
    if ((actNode->visits_) >= minVisitCount){
      tab.insert(actNode); 
    }
    actNode = actNode->sibling_;
  }
  
  while (true){
    for (nodeTab::iterator it = tab.begin(); it != tab.end(); it++){
      if(! best || ((*it)->visits_ > best->visits_)){
          best = (*it);
      }
    }
    if (best){
        ss << best->recToString(depth + 1);
        tab.erase(best);
        best = NULL;
    }else{
      break;
    }
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

Tree::Tree(Node* root)
{
  assert(root->getFather() == NULL);
  init();
  history[0] = root; 
}

//--------------------------------------------------------------------- 

Tree::Tree(Tree* trees[], int treesNum) 
{
  init();
  EqNode * head = NULL;
  EqNode * en; 

  for (int i = 0; i < treesNum; i++){
    en = new EqNode();
    en->node = trees[i]->root();
    en->next = head;
    head = en;
  }

  //tree from merged trees
  history[0] = Tree::mergeTrees(head, NULL, nodesNum_, nodesExpandedNum_);

}

//--------------------------------------------------------------------- 

Tree::Tree(player_t firstPlayer)
{
  init();
  history[historyTop] = new Node(&(twSteps_[Step(STEP_NULL, firstPlayer)]));
  nodesNum_ = 1;
}

//--------------------------------------------------------------------- 

Tree::~Tree()
{
  history[0]->removeChildrenRec();
  delete history[0];
}

//--------------------------------------------------------------------- 

void Tree::init()
{
  history[0] = NULL; 
  twSteps_.clear();
  historyTop = 0;
  nodesExpandedNum_ = 0;
  nodesNum_ = 0;
}

//---------------------------------------------------------------------

void Tree::expandNode(Node* node, const StepArray& steps, uint len, const HeurArray* heurs)
{
  Node* newChild;
  for (uint i = 0; i < len; i++){
    newChild = new Node(&(twSteps_[steps[i]]), heurs ? (*heurs)[i] : 0 );  
    node->addChild(newChild);
    nodesNum_++;
  }
  nodesExpandedNum_++;
}

//--------------------------------------------------------------------- 

void Tree::expandNodeLimited(Node* node, const Move& move)
{
  
  Node* newChild;
  StepList stepList = move.getStepList();
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    newChild = new Node(&twSteps_[*it]);
    node->addChild(newChild);
    node = newChild;
   // nodesNum_++;
   // nodesExpandedNum_++;
  }
}

//---------------------------------------------------------------------

void Tree::uctDescend()
{
  assert(actNode()->hasChildren());
  history[historyTop + 1]= actNode()->findUctChild();
  historyTop++;
  assert(actNode() != NULL);
}

//---------------------------------------------------------------------

void Tree::randomDescend()
{
  assert(actNode()->hasChildren());
  history[historyTop + 1]= actNode()->findRandomChild();
  historyTop++;
  assert(actNode() != NULL);
}

//---------------------------------------------------------------------

void Tree::firstChildDescend()
{
  assert(actNode()->hasChildren());
  history[historyTop + 1]= actNode()->getFirstChild();
  historyTop++;
  assert(actNode() != NULL);
}

//---------------------------------------------------------------------

Node* Tree::findBestMoveNode(Node* subTreeRoot)
{
  Node* act = subTreeRoot; 

  while (true){
    if (! act->hasChildren() || 
          (act->getFirstChild()->getNodeType() != subTreeRoot->getNodeType())){
      break;
    }
    act = act->findMostExploredChild();
  }

  //Now we have some good solution - DFS in the 
  //first layer of the tree follows
   
  Node * best = act;
  list<Node *> stack;
  stack.clear();
  stack.push_back(subTreeRoot);
  while (! stack.empty()){
    act = stack.back();
    stack.pop_back(); 
    assert(act != NULL);
    if (act->getVisits() > best->getVisits()){
      //"leaf" node action
      if (! act->hasChildren() || 
            (act->getFirstChild()->getNodeType() != subTreeRoot->getNodeType())){
        best = act;
      }
      else {
        Node * child = act->getFirstChild();
        while (child != NULL){
          stack.push_back(child);
          child = child->getSibling();
        }
      }
    }
  }
  //assert(false);
  return best;
}

//--------------------------------------------------------------------- 

Move Tree::findBestMove(Node* bestMoveNode)
{
  assert(bestMoveNode != NULL && bestMoveNode != root());

  Move bestMove;
  Node* act = bestMoveNode; 

  while (act!= NULL && act != root() && 
         act->getNodeType() == bestMoveNode->getNodeType()) {
    bestMove.prependStep(act->getStep());
    act = act->getFather(); 
  } 
  
  return bestMove;
} 

//---------------------------------------------------------------------

Node* Tree::mergeTrees(EqNode* eqNode, Node* father,
                       int& nodesNum, int& nodesExpandedNum)
{
  assert(eqNode != NULL);
  if (eqNode == NULL){
    return NULL;
  }

  //for going through the eqnode
  EqNode * en = eqNode;
  //blocks for children 
  EqNodeBlock * eqNodeBlocks = NULL;
  int visits = 0;
  float value = 0;

  while (en != NULL ){
    assert(en->node != NULL);
    visits += en->node->getVisits();
    value += en->node->getValue() * en->node->getVisits();
    Node* child = en->node->getFirstChild();
    while (child != NULL){
      EqNodeBlock* enb = eqNodeBlocks;
      while (enb != NULL){
        assert(enb->eqNode != NULL);
        if (enb->eqNode->node->getStep() == child->getStep()){
          break;
        }
        enb = enb->next;
      }
      if (enb != NULL){
        //some nodes with such a step already exist in eqNodeBlock
        EqNode * e = new EqNode();     
        e->next = enb->eqNode;
        enb->eqNode = e;
        e->node = child;
      }
      else {
        enb = new EqNodeBlock();
        enb->next = eqNodeBlocks;
        eqNodeBlocks = enb;
        enb->eqNode = new EqNode();
        enb->eqNode->next = NULL;
        enb->eqNode->node = child;
      }
      child = child->getSibling();
    }
    en = en->next;
  }

  //TODO bindings to foreign TWsteps are made - tree should create 
  //it's own twsteps and make bindings to them.
  Node * node = new Node(eqNode->node->getTWstep());
  nodesNum++;
  node->setFather(father);
  node->setVisits(visits);
  node->setValue(visits == 0 ? 0 : value/visits);

  //go through nodes on given level and recurse
  EqNodeBlock* enb = eqNodeBlocks;
  Node * child = NULL;
  Node * firstChild = NULL;

  while (enb != NULL) {
    //create new node
    EqNode * en = enb->eqNode;    
    if (en == NULL) {
      assert(false);
      break;
    }
    assert(en != NULL);
    //recurse
    child = mergeTrees(en, node, nodesNum, nodesExpandedNum);
    child->setSibling(firstChild);
    firstChild = child;
    enb = enb->next;
  }


  node->setFirstChild(firstChild);
  if (firstChild){
    nodesExpandedNum++;
  }
  return node;
  
}

//--------------------------------------------------------------------- 

void Tree::updateHistory(float sample)
{
  for(int i = historyTop; i > 0; i--){
    //node update
    history[i]->update(sample);
    //tree wide step update
    history[i]->updateTWstep(sample);
  }
  //root update
  history[0]->update(sample);
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

Uct::Uct() 
{
  assert(false);
}

//--------------------------------------------------------------------- 

Uct::Uct(const Board* board, Uct* ucts[], int uctsNum)
{
  init(board);
  Tree** trees = new Tree*[uctsNum];
  
  for (int i = 0; i < uctsNum; i++){
    trees[i] = ucts[i]->getTree();
    playouts_ += ucts[i]->getPlayoutsNum();
    nodesPruned_ += ucts[i]->getNodesTTpruned();
  }

  //delete tree initialized in init
  delete tree_;
  //tree from merged trees
  tree_ = new Tree(trees, uctsNum);

  refineResults(board);
}

//--------------------------------------------------------------------- 

Uct::Uct(const Board* board)
{
  init(board);
}

//--------------------------------------------------------------------- 

void Uct::init(const Board* board)
{
  eval_  = new Eval();
  searchExt_ = new SearchExt();
  tree_  = new Tree(board->getPlayerToMove());
  tt_ = new TT();

  bestMoveNode_ = NULL;
  bestMoveRepr_ = "";
  nodesPruned_ = 0;
  playouts_ = 0;

  //save root in the tt
  tt_->insertItem(board->getSignature(),
                  PLAYER_TO_INDEX(board->getPlayerToMove()), 
                  tree_->root());

}

//---------------------------------------------------------------------

Uct::~Uct()
{
  delete eval_;
  delete tree_;
  delete tt_;
  delete searchExt_;
}


//---------------------------------------------------------------------

void Uct::searchTree(const Board* refBoard, const Engine* engine)
{
  Board* board = new Board(*refBoard);
  while (! engine->checkSearchStop()){
    doPlayout(board);
  }
  delete(board);
}

//--------------------------------------------------------------------- 

void Uct::refineResults(const Board* board) 
{
  
  bestMoveNode_ = tree_->findBestMoveNode(tree_->root());
  Move bestMove = tree_->findBestMove(bestMoveNode_);
  bestMoveRepr_ = bestMove.toStringWithKills(board);

  //add signature of final position ! -> for future thirdRepetitionCheck
  //TODO this should be done when the move is actually MADE ! 
  Board* playBoard = new Board(*board);
  playBoard->makeMove(bestMove);
  thirdRep.update(playBoard->getSignature(), PLAYER_TO_INDEX(playBoard->getPlayerToMove()) );
}

//---------------------------------------------------------------------

void Uct::doPlayout(const Board* board)
{
  Board *playBoard = new Board(*board);
  playoutStatus_e playoutStatus;
  int descendNum = 0;

  playouts_++;

  StepArray steps;    
  uint      stepsNum;
  
  tree_->historyReset();     //point tree's actNode to the root 

  //cerr << "Playout : " << endl; 
  do { 
   /*cerr << "   " << tree_->actNode()->toString();
    Node* n=tree_->actNode()->getFirstChild();
    while (n!= NULL){
      cerr << "           " << n->toString();
      n = n->getSibling();
    }
  */
  
    if (! tree_->actNode()->hasChildren()) { 
      if (tree_->actNode()->isMature()) {

        //goalCheck => value fixation
        Move move;
        if (playBoard->quickGoalCheck(&move)){
          float value = WINNER_TO_VALUE(tree_->actNode()->getPlayer());
          //if not complete step - add pass 
          //small workaround preventing for instance rabbits crouching along the victory line
          if (playBoard->canContinue(move)){
            move.appendStep(Step(STEP_PASS, playBoard->getPlayerToMove()));
          }
          tree_->expandNodeLimited(tree_->actNode(), move);
          //descend to the expanded area
          while (tree_->actNode()->hasChildren())
            tree_->firstChildDescend();
          tree_->updateHistory(value);
          break;
        }

        stepsNum = playBoard->generateAllStepsNoPass(playBoard->getPlayerToMove(), steps);
        stepsNum = playBoard->filterRepetitions(steps, stepsNum);
        stepsNum = filterTT(steps, stepsNum, playBoard); 
        if (playBoard->canPass()){ //add pass if possible
          steps[stepsNum++] = Step(STEP_PASS, playBoard->getPlayerToMove());
        }

        if (stepsNum > 0) {
          if (cfg.knowledgeInTree()){
            HeurArray heurs;
            playBoard->getHeuristics(steps, stepsNum, heurs);
            tree_->expandNode(tree_->actNode(), steps, stepsNum, &heurs);
          }
          else{
            tree_->expandNode(tree_->actNode(), steps, stepsNum);
          }
          updateTT(tree_->actNode()->getFirstChild(), playBoard); 
        }else{
          tree_->removeNodeCascade(tree_->actNode());
          break;
        }

        continue;
      }

      //"random" playout TODO ... check
      SimplePlayout simplePlayout(playBoard, MAX_PLAYOUT_LENGTH, 
          tree_->actNode()->getNodeType() == tree_->root()->getNodeType() ?
          EVAL_AFTER_LENGTH + 1:
          EVAL_AFTER_LENGTH 
          );
      playoutStatus = simplePlayout.doPlayout();
      //playoutStatus = PLAYOUT_EVAL;
      tree_->updateHistory( decidePlayoutWinner(playBoard));
      break;
    }

    tree_->uctDescend(); 
    descendNum++;

    Step step = tree_->actNode()->getStep();

    //perform the step and try commit
    if ( playBoard->makeStepTryCommitMove(step) )   {
      //commit was successful - check whether winning criteria are reached already
      if ( playBoard->getWinner() != EMPTY ) {
        tree_->updateHistory(playBoard->getWinner() == GOLD ? 1 : -1); 
        break;  //winner is known already in the uct tree -> no need to go deeper
      }
    }

  } while(true);
  
  delete playBoard;
}

//--------------------------------------------------------------------- 

string Uct::getStats(float seconds) const
{
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
        << "  " << "best move: " << getBestMoveRepr() << endl 
        << "  " << "best move visits: " << getBestMoveVisits() << endl 
        << "  " << "win condidence: " << getWinRatio() << endl 
      ;

  return ss.str();
}

//---------------------------------------------------------------------

string Uct::getAdditionalInfo() const
{
  return tree_ ? tree_->toString() : "No additional info.";
}

//---------------------------------------------------------------------

string Uct::getBestMoveRepr() const
{
  return bestMoveRepr_;
}

//---------------------------------------------------------------------

int Uct::getBestMoveVisits() const
{
  return (bestMoveNode_) ? bestMoveNode_->getVisits() : 0;
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
  float ratio = (bestMoveNode_) ? (bestMoveNode_->getValue() + 1 )/2 : 0.5;
  assert(tree_->root());
  return (tree_->root()->getNodeType() == NODE_MAX) ? ratio : 1 - ratio;
}

//--------------------------------------------------------------------- 

Tree* Uct::getTree() const 
{
  return tree_;
}

//--------------------------------------------------------------------- 

int Uct::getPlayoutsNum() const 
{
  return playouts_;
}

//--------------------------------------------------------------------- 

int Uct::getNodesTTpruned() const 
{
  return nodesPruned_;
}

//--------------------------------------------------------------------- 

double Uct::decidePlayoutWinner(const Board* playBoard) const
{
  if IS_PLAYER(playBoard->getWinner()){
    return WINNER_TO_VALUE(playBoard->getWinner()); 
  }

  double evalGold = eval_->evaluateInPercent(playBoard);
  if (cfg.exactPlayoutValue()){
    return 2 * (evalGold - 0.5);
  }
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
                     PLAYER_TO_INDEX(board->getPlayerToMove()))){
                     //PLAYER_TO_INDEX(board->getPlayerToMoveAfterStep(steps[i])))){
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
                    PLAYER_TO_INDEX(board->getPlayerToMove()), 
                    //PLAYER_TO_INDEX(board->getPlayerToMoveAfterStep(node->getStep())), 
                    node);
    node = node->getSibling();
  }
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

