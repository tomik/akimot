/** 
 *  @file uct.cpp
 *  @brief Uct algorithm. 
 *  @full Uct algorithm implementation.
 */

#include "engine.h"
#include "uct.h"
#include "eval.h"

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
    if (hasWinner())
	    return PLAYOUT_OK;

		if (playoutLength_ > 2 * maxPlayoutLength_) 
			return PLAYOUT_TOO_LONG;

    if (++moves >= evalAfterLength_ && evalAfterLength_)
      return PLAYOUT_EVAL;
	}
}

//--------------------------------------------------------------------- 

bool SimplePlayout::hasWinner(){
	if ( board_->getWinner() != NO_PLAYER ) {
    return true;  
  }
  return false;
}

//---------------------------------------------------------------------

void SimplePlayout::playOne()
{
  if (cfg.playoutByMoves()) {
    board_->findMCmoveAndMake();
  }
  else{
    Step step;

    do {
      step = board_->findMCstep();
      logDDebug(step.toString().c_str());
    }
    while (! board_->makeStepTryCommit(step));
  }
}

//---------------------------------------------------------------------

uint SimplePlayout::getPlayoutLength()
{
	return playoutLength_/2;
}

//---------------------------------------------------------------------
// section AdvisorPlayout
//---------------------------------------------------------------------

AdvisorPlayout::AdvisorPlayout(Board* board, uint maxPlayoutLength, uint evalAfterLength, MoveAdvisor* advisor):
  SimplePlayout(board, maxPlayoutLength, evalAfterLength), advisor_(advisor)
{
  ;
}

//---------------------------------------------------------------------

void AdvisorPlayout::playOne()
{
  Move move;
  if (cfg.moveAdvisor() && glob.grand()->get01() < cfg.moveAdvisor() && 
      advisor_->getMove(board_->getPlayerToMove(), board_->getBitboard(), 
      board_->getStepCountLeft(), &move)){
    //cerr << "APPLYING MOVE IN PLAYOUT " << endl;
    //cerr << board_->toString();
    //cerr << board_->moveToStringWithKills(move) << endl;
    board_->makeMove(move);
  }  
  else{
    SimplePlayout::playOne();
  }
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
  TWsteps::iterator it = find(step);
  if (it != end()){
    return it->second;
  }

  pair< TWsteps::iterator, bool> p;
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

Node::Node(TWstep* twStep, int level, float heur)
{
  assert(IS_PLAYER(twStep->step.getPlayer()));
  nodeType_ = ( twStep->step.getPlayer() == GOLD ? NODE_MAX : NODE_MIN );
  firstChild_ = NULL;
  sibling_    = NULL;
  father_     = NULL;  
  visits_     = 0;
  value_      = 0; 
  squareSum_  = 0;
  heur_       = heur; 
  twStep_     = twStep;
  level_      = level;
  ttRep_      = NULL;
  childrenNum_ = 0;
  for (int i = 0; i < CHILDREN_CACHE_SIZE; i++){ 
    cCache_[i] = NULL;
  }
  cCacheLastUpdate_ = 0;
}

//---------------------------------------------------------------------

Node* Node::findUctChild(Node* realFather) 
{
  assert(this->firstChild_ != NULL);
  assert(realFather != NULL);
  
  Node* act = firstChild_;
  Node* best = firstChild_;
  float bestUrgency = INT_MIN;   
  
  //dynamic exploreRate tuning ? 
  //float exploreRate = cfg.ucbTuned() ? 1 : min(0.2, max(0.01, 1.15/(log(realFather->visits_))));
  float exploreRate = cfg.ucbTuned() ? 1 : cfg.exploreRate();
  float exploreCoeff = exploreRate * log(realFather->visits_);

  if (cfg.childrenCache() && visits_ > 3 * childrenNum_){
    //using children cache
    cCacheUpdate(exploreCoeff); 
    for (int i = 0; i < CHILDREN_CACHE_SIZE && cCache_[i]; i++){
      act = cCache_[i];   
      uctOneChild(act, best, bestUrgency, exploreCoeff);
    }
  }else{
    while (act != NULL) {
      uctOneChild(act, best, bestUrgency, exploreCoeff);
      act = act->sibling_;
    }
  }

  return best;
}

//--------------------------------------------------------------------- 

void Node::latePruning()
{

  //late pruning
  float minPruneVisits = 128;
  if (visits_ < minPruneVisits) {
    return;
  }
  float allowed = 17 - max(0,(int(log(visits_)) - int(log(minPruneVisits))));
  if (allowed >= 3 && allowed < childrenNum_ ){
    //cerr << int(log(visits_)) << " " << int(log(minPruneVisits)) << endl;
    for (int i = 0; i < childrenNum_ - allowed; i++){
      Node** pp = &firstChild_;
      Node** worst = &firstChild_;
      float worstVal = INT_MAX;   

      while (*pp != NULL) {
        if ( (*pp)->visits_ < worstVal ){
          worst = pp;
          worstVal = (*pp)->visits_;
        }
        pp = &((*pp)->sibling_);
      }
      //Node* pom = *worst;
      //memory cleanup !
      *worst = (*worst)->sibling_; 
      childrenNum_--;
    }
  }
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

void Node::cCacheUpdate(float exploreCoeff)
{
  if (floor(sqrt(visits_)) <= cCacheLastUpdate_)
    return;

  cCacheLastUpdate_ = int(floor(sqrt(visits_)));
  
  //nullify cache
  for (int i = 0; i < CHILDREN_CACHE_SIZE; i++){ 
    cCache_[i] = NULL;
  }

  Node * act = firstChild_;
  float actUrgency = 0;

  float urgencies[CHILDREN_CACHE_SIZE];

  //fill cache
  while (act != NULL) {
    actUrgency = act->exploreFormula(exploreCoeff);
    for (int i = 0; i < CHILDREN_CACHE_SIZE; i++){ 
      if (! cCache_[i] || 
          urgencies[i] < actUrgency ){
        //bubbling
        for (int j = CHILDREN_CACHE_SIZE - 1; j > i ;j--){ 
          cCache_[j] = cCache_[j - 1]; 
          urgencies[j] = urgencies[j - 1];
        }
        cCache_[i] = act;
        urgencies[i] = actUrgency;
        break;
      }
    }
    act = act->sibling_;
  }
}

//---------------------------------------------------------------------

void Node::uctOneChild(Node* act, Node* & best, float & bestUrgency, float exploreCoeff) const
{
  assert(act);
  float actUrgency = act->exploreFormula(exploreCoeff);

  if ( actUrgency > bestUrgency ){
    best = act;
    bestUrgency = actUrgency;
  }
}

//--------------------------------------------------------------------- 

float Node::exploreFormula(float exploreCoeff) const
{
 if (visits_ == 0)
   return cfg.fpu();

 return (cfg.ucbTuned() ? ucbTuned(exploreCoeff) : ucb(exploreCoeff))
        + heur_/visits_
        + (cfg.historyHeuristic() ? ((nodeType_ == NODE_MAX ? twStep_->value : - twStep_->value)/sqrt(visits_)) : 0)
        ;
}

//---------------------------------------------------------------------

float Node::ucb(float exploreCoeff) const
{
  return (nodeType_ == NODE_MAX ? value_ : - value_) 
         + sqrt(exploreCoeff/visits_) ;
}

//---------------------------------------------------------------------

float Node::ucbTuned(float exploreCoeff) const
{
  double v = max(float(0.03), min(float(0.2), float((squareSum_/visits_) + sqrt(0.2 * exploreCoeff/visits_))));
  //double val = (value_ + 1) / 2;
  //double v = max(0.01, val * (1- val));
  //cerr << v << endl;

  return (nodeType_ == NODE_MAX ? value_ : - value_) 
         + sqrt(v * exploreCoeff/visits_);
}

//---------------------------------------------------------------------

void Node::addChild(Node* newChild)
{
  assert(newChild->sibling_ == NULL);
  assert(newChild->firstChild_ == NULL);
  
  newChild->sibling_ = this->firstChild_;
  this->firstChild_ = newChild;
  newChild->father_ = this;
  childrenNum_++;
}
 
//---------------------------------------------------------------------

void Node::delChildrenRec()
{
  //somebody else will take care
  if (ttRep_ && ttRep_->front() != this) {
    return;
  }

  Node* actNode = firstChild_;
  Node* sibling;
  while(actNode != NULL){
    sibling = actNode->sibling_;
    actNode->delChildrenRec();
    delete actNode;
    actNode = sibling;
  }
}

//---------------------------------------------------------------------

void Node::update(float sample)
{
  float old_value = value_;
  if (cfg.uctRelativeUpdate() && isMature()){
    //int diff = abs(int((sample - value_ ) * sqrt(visits_)));
    //int weight = min(max(diff, 1), 100);
    int weight = min(max(int(sqrt(visits_)), 1), 10);
    float added = ((sample - value_) * weight)/(visits_ + weight);
    visits_ += 1;
    //cerr << sample - value_ << "/" << visits_ << "/" << weight << " ";
    //cerr << value_ << "/" << visits_ << "/" << diff << "/" << added << "/" << (sample - value_)/visits_ << " ";
    value_ += added;
  }else{
    value_ += (sample - value_)/++visits_;         
  }
  squareSum_ += (sample - old_value) * (sample - value_);
  //value update in ttNodes
  if (ttRep_) {
    for (NodeList::iterator it = ttRep_->begin(); it != ttRep_->end(); it++){
    (*it)->value_ = value_;
   } 
  }
}

//--------------------------------------------------------------------- 

void Node::updateTWstep(float sample)
{
  twStep_->value += (sample - twStep_->value) / ++(twStep_->visits);
}

//---------------------------------------------------------------------

bool Node::isMature() const
{
  return visits_ > cfg.matureLevel();
}

//--------------------------------------------------------------------- 

bool Node::isJustMature() const
{
  return visits_ == cfg.matureLevel();
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

NodeList* Node::getTTrep() const
{
  return ttRep_;
}

//---------------------------------------------------------------------

void Node::setTTrep(NodeList* nodelist) 
{ 
  ttRep_ = nodelist; 
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

int Node::getDepth() const
{
  int depth = 0;
  Node* node = this->father_;

  while (node != NULL){
    node = node->getFather();
    depth++;
  }
  return depth;
  
}

//--------------------------------------------------------------------- 

int Node::getLocalDepth() const
{
  int depth = 0;
  const Node* node = this;

  while (node->father_ != NULL && node->getNodeType() == nodeType_){
    depth += max(1, node->getStep().count());
    node = node->getFather();
  }
  return depth;
}

//--------------------------------------------------------------------- 


int Node::getLevel() const
{
  return level_;
}

//--------------------------------------------------------------------- 

int Node::getDepthIdentifier() const
{
  return STEPS_IN_MOVE * getLevel() + getLocalDepth();
}

//--------------------------------------------------------------------- 

string Node::toString() const
{
  stringstream ss;

  ss << getStep().toString() << "(" << getDepthIdentifier() << " " <<  ( nodeType_  == NODE_MAX ? "+" : "-" )  << ") " << value_ << "/" << visits_ << " twstep " << twStep_->value << "/" << twStep_->visits << endl;
  return ss.str();
}

//---------------------------------------------------------------------

string Node::recToString(int depth) const
{
  const float print_visit_threshold_base    = 300;
  const float print_visit_threshold_parent  = 0.05;
  const int   print_max_brothers            = 3;

  float minVisitCount = print_visit_threshold_base + 
                        visits_ * print_visit_threshold_parent; 
  //minVisitCount = 0;
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
  
  int printed = 0;
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
    if (++printed >= print_max_brothers) {
      break;
    }
  }
  return ss.str();
}

//---------------------------------------------------------------------
//  section Tree
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
    nodesPruned_ += trees[i]->nodesPruned_;  
  }
  //TODO nodesPruned_ simple average now ...
  nodesPruned_ /= treesNum;

  //tree from merged trees
  if (treesNum == 1){
    history[0] = trees[0]->root();
    nodesNum_ = trees[0]->nodesNum_;
    nodesExpandedNum_ = trees[0]->nodesExpandedNum_;
  }else{
    history[0] = Tree::mergeTrees(head, NULL, nodesNum_, nodesExpandedNum_);
  }

}

//--------------------------------------------------------------------- 

Tree::Tree(player_t firstPlayer)
{
  init();
  history[historyTop] = new Node(&(twSteps_[Step(STEP_NULL, firstPlayer)]), 0);
  nodesNum_ = 1;
}

//--------------------------------------------------------------------- 

Tree::~Tree()
{
  history[0]->delChildrenRec();
  delete history[0];
  delete tt_;
}

//--------------------------------------------------------------------- 

void Tree::expandNode(Node* node, const StepArray& steps, uint len, const HeurArray* heurs)
{
  Node* newChild;
  assert(len);
  assert(node);
  int level = len ? calcNodeLevel(node, steps[0]) : 0;
  assert(steps[0].getPlayer() == steps[len-1].getPlayer());
  for (uint i = 0; i < len; i++){
    newChild = new Node(&(twSteps_[steps[i]]), level,  
                          heurs ? (*heurs)[i] : 0);  
    node->addChild(newChild);
    nodesNum_++;
  }
  nodesExpandedNum_++;
  
  //expander different from representant
  if (node->getTTrep()) {
    for (NodeList::iterator it = node->getTTrep()->begin(); it != node->getTTrep()->end(); it++){
      (*it)->setFirstChild(node->getFirstChild());
    }
  }
}

//--------------------------------------------------------------------- 

void Tree::expandNodeLimited(Node* node, const Move& move)
{
  
  Node* newChild;
  StepList stepList = move.getStepList();
  assert(! stepList.empty());
  assert(node);
  int level = ! stepList.empty() ? calcNodeLevel(node, *stepList.begin()) : 0;
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    newChild = new Node(&twSteps_[*it], level);
    node->addChild(newChild);
    node = newChild;
    nodesNum_++;
    nodesExpandedNum_++;
  }
}

//---------------------------------------------------------------------

void Tree::uctDescend()
{
  assert(actNode()->hasChildren());
  if (cfg.latePruning()){
    actNode()->latePruning();
  }
  history[historyTop + 1]= actNode()->findUctChild(history[historyTop]);
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

  //return act;

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
  int level = father ? calcNodeLevel(father, eqNode->node->getTWstep()->step) : 0;
  Node * node = new Node(eqNode->node->getTWstep(), level);
                         
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

int Tree::getNodesNum() 
{
    return nodesNum_;
}

//--------------------------------------------------------------------- 

int Tree::getNodesPruned() 
{
    return nodesPruned_;
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

void Tree::updateTT(Node* father, const Board* board)
{
  assert(father != NULL); 
  Node* node = father->getFirstChild(); 
  NodeList* rep = NULL; 
  Node* repNode = NULL; 
  u64 afterStepSignature;
  while (node != NULL){
    if (node->getStep().isPass()){
      node = node->getSibling();
      continue;
    }
    afterStepSignature = board->calcAfterStepSignature(node->getStep());
    //check whether position was encountered already
    if (tt_->loadItem(afterStepSignature, 
                     board->getPlayerToMove(), 
                     rep,
                     node->getDepthIdentifier())){
      assert(rep != NULL);
      repNode = rep->front(); 
      nodesPruned_++;
      
      node->setFirstChild(repNode->getFirstChild());
      node->setValue(repNode->getValue());
      node->setTTrep(rep);
      rep->push_back(node);
      //cerr << node->toString() << " --- " << ttNode->toString() << endl;

    }else{
      //position is not in tt yet -> store it 
      rep = new NodeList();
      rep->push_back(node);
      node->setTTrep(rep);
      tt_->insertItem(afterStepSignature,
                    board->getPlayerToMove(), 
                    rep, 
                    node->getDepthIdentifier());
    }
    node = node->getSibling();
  }
}

//--------------------------------------------------------------------- 

Tree::Tree()
{
  assert(false);
}

//--------------------------------------------------------------------- 

void Tree::init()
{
  tt_ = new TT();

  //root is NOT saved 
  /*tt_->insertItem(board->getSignature(),
                  board->getPlayerToMove(), 
                  tree_->root());
  */

  history[0] = NULL; 
  twSteps_.clear();
  historyTop = 0;
  nodesExpandedNum_ = 0;
  nodesNum_ = 0;
  nodesPruned_ = 0;
}

//--------------------------------------------------------------------- 

int Tree::calcNodeLevel(Node* father, const Step& step) 
{
  return father->getLevel() + (father->getPlayer() == step.getPlayer() ? 0 : 1);  
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
    uctDescends_ += ucts[i]->uctDescends_;
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
  eval_  = new Eval(board);
  tree_  = new Tree(board->getPlayerToMove());
  advisor_ = new MoveAdvisor();

  bestMoveNode_ = NULL;
  bestMoveRepr_ = "";
  playouts_ = 0;
  uctDescends_ = 0; 


}

//---------------------------------------------------------------------

Uct::~Uct()
{
  delete eval_;
  delete tree_;
  delete advisor_;
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
  bestMoveRepr_ = board->moveToStringWithKills(bestMove);

  //add signature of final position ! -> for future thirdRepetitionCheck
  //TODO IS THIS NECCESSARY - use new position loading - AEI ? 
  Board* playBoard = new Board(*board);
  playBoard->makeMove(bestMove);
}

//---------------------------------------------------------------------

void Uct::doPlayout(const Board* board)
{
  Board *playBoard = new Board(*board);
  playoutStatus_e playoutStatus;

  StepArray steps;    
  uint      stepsNum;

  tree_->historyReset();     //point tree's actNode to the root 

  logDDebug(board->toString().c_str());
  logDDebug("===== Playout :===== ");
  do { 
   logDDebug(tree_->actNode()->toString().c_str());
  
    if (! tree_->actNode()->hasChildren()) { 
      if (tree_->actNode()->getDepth() < UCT_MAX_DEPTH - 1) {
        if (tree_->actNode()->isJustMature()) {
          //goalCheck 
          
          Move move;
          if (playBoard->getStepCount() == 0 && playBoard->goalCheck(&move)){
            float value = WINNER_TO_VALUE(playBoard->getPlayerToMove());
            tree_->expandNodeLimited(tree_->actNode(), move);
            //descend to the expanded area
            while (tree_->actNode()->hasChildren()){
              tree_->firstChildDescend();
            }
            tree_->updateHistory(value);
            break;
          }
          
          //tactics in playouts extension
         
          if (cfg.moveAdvisor()){
            fill_advisor(playBoard); 
          } 

          stepsNum = playBoard->genStepsNoPass(playBoard->getPlayerToMove(), steps);
          stepsNum = playBoard->filterRepetitions(steps, stepsNum);

          if (playBoard->canPass()) { //add pass if possible
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
            if (cfg.uct_tt()){
              tree_->updateTT(tree_->actNode(), playBoard); 
            }
          }
          //imobilization
          //little hack - raise visits_ artificially to break the infinite for loop
          else{
            tree_->actNode()->setVisits(tree_->actNode()->getVisits() + 1);
          }

          continue;
        }
        //check win by immobilization 
        if (tree_->actNode()->isMature()){
          tree_->updateHistory(OPP(playBoard->getPlayerToMove()));
          break;
        }
      } //< UCT_MAX_DEPTH check


      int base = 1 + (glob.grand()->getOne() % cfg.playoutLen());
      int playoutLen = base + (tree_->actNode()->getNodeType() == tree_->root()->getNodeType() ? 1 : 0);
           
      AdvisorPlayout playoutManager(playBoard, MAX_PLAYOUT_LENGTH, playoutLen, advisor_);

      playoutStatus = playoutManager.doPlayout();
      float sample = decidePlayoutWinner(playBoard);
      tree_->updateHistory(sample);
      if (cfg.moveAdvisor()){
        advisor_->update(sample);
      }
      break;
    }

    tree_->uctDescend(); 
    uctDescends_++;

    Step step = tree_->actNode()->getStep();

    //perform the step and try commit
    if (playBoard->makeStepTryCommit(step) )   {
      //commit was successful - check whether winning criteria are reached already
      if (playBoard->getWinner() != NO_PLAYER ) {
        tree_->updateHistory(WINNER_TO_VALUE(playBoard->getWinner()));
        break;  //winner is known already in the uct tree -> no need to go deeper
      }
    }

  } while(true);

  playouts_++; 
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
        << "  " << tree_->getNodesPruned() << " nodes pruned" << endl 
        << "  " << uctDescends_/float(playouts_) << " average descends in playout" << endl 
        << "  " << "best move: " << getBestMoveRepr() << endl 
        << "  " << "best move visits: " << getBestMoveVisits() << endl 
        << "  " << "win condidence: " << getWinRatio() << endl 
      ;

  /*
  for (int player = 0; player < 2; player++){
    for (int i = 0; i < TRAPS_NUM; i++){
       ss << glob.losesValue[player][i]/glob.losesCount[player][i] << "/" << glob.losesCount[player][i] << " ";
    }
    ss << endl;
  }
  */

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

double Uct::decidePlayoutWinner(const Board* playBoard) const
{
  if IS_PLAYER(playBoard->getWinner()){
    return WINNER_TO_VALUE(playBoard->getWinner()); 
  }

  double evalGold = eval_->evaluateInPercent(playBoard);
  if (cfg.exactPlayoutValue()){
    return 2 * (evalGold - 0.5);
  }
  double r = (double)glob.grand()->getOne()/((double)(RAND_MAX) + (double)(1));
  if (r < evalGold)
    return 1;
  else
    return -1;
}

//--------------------------------------------------------------------- 

void Uct::fill_advisor(const Board * playBoard) {

  //opponent goal check
  Move move = Move();
  player_t opp = OPP(playBoard->getPlayerToMove());
  if (playBoard->goalCheck(opp, STEPS_IN_MOVE, &move)){
    if (move.getStepCount()) {
      advisor_->addMove(move, playBoard->getBitboard());
                      //playBoard->getStepCountLeft()); 
    }
  }

/*  
  //opponent trapCheck
  MoveList moves;
  moves.clear();
  if (playBoard->trapCheck(playBoard->getPlayerToMove(), &moves)){ 
    for (MoveList::const_iterator it = moves.begin(); it != moves.end(); it++){ 
      advisor_->addMove((*it), playBoard->getBitboard());
      //if (advisor_->addMove((*it), playBoard->getBitboard())){
      //  cerr << playBoard->toString();
      //  cerr << it->toString() << endl;
      //}
    }
  }

  //trapCheck 
  moves.clear();
  if (playBoard->getStepCount() == 0 && 
      playBoard->trapCheck(OPP(playBoard->getPlayerToMove()), &moves)){ 
    for (MoveList::const_iterator it = moves.begin(); 
                                  it != moves.end(); it++){ 
      advisor_->addMove((*it), playBoard->getBitboard());
    }
  }
  */
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

