/** 
 *  @file uct.cpp
 *  @brief Uct algorithm implemenation.
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
  p = insert(make_pair(step, TWstep(step, 0, 1)));
  assert(p.second);
  return p.first->second;
}

//---------------------------------------------------------------------
// section TTitem
//---------------------------------------------------------------------

TTitem::TTitem() 
{ 
  assert(false);
}

//--------------------------------------------------------------------- 

TTitem::TTitem(NodeList* nodes) 
{
  visits_ = 0; 
  value_ = 0; 
  nodes_ = nodes;
}

//--------------------------------------------------------------------- 

NodeList* TTitem::getNodes() const
{
  return nodes_;
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
  pthread_mutex_init(&mutex, NULL);
  assert(IS_PLAYER(twStep->step.getPlayer()));
  firstChild_ = NULL;
  sibling_    = NULL;
  father_     = NULL;  
  visits_     = (cfg.fpu() == 0) ? 5 : 0;
  value_      = 0; 
  squareSum_  = 0;
  heur_       = heur; 
  twStep_     = twStep;
  ttItem_      = NULL;
  master_     = NULL;
  //full cCache_ initialization in node::expand
  cCache_     = NULL;
  
  masterValue_  = value_;
  masterVisits_ = visits_;
}

//---------------------------------------------------------------------

Node* Node::findUctChild(Node* realFather) 
{
  assert(this->firstChild_ != NULL);
  assert(realFather != NULL);
  
  Node* act = firstChild_;
  Node* best = firstChild_;
  float bestUrgency = INT_MIN;   
  
  //dynamic exploreRate tuning 
  float exploreRate = cfg.ucbTuned() ? 1 : 
                                  (cfg.dynamicExploration() ? 
                                  max(0.01, min(0.25, 3.5 * (double(squareSum_)/visits_)))
                                  : cfg.exploreRate());
  float exploreCoeff = exploreRate * log(realFather->visits_);

  if (cCache_ &&  visits_ > CCACHE_START_THRESHOLD){
    //using children cache
    assert(cCache_);
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

void Node::cCacheInit(){
  cCache_ = new Node*[CHILDREN_CACHE_SIZE];
  assert(cCache_);
  for (int i = 0; i < CHILDREN_CACHE_SIZE; i++){ 
    cCache_[i] = NULL;
  }
  cCacheLastUpdate_ = 0;
}

//--------------------------------------------------------------------- 

void Node::cCacheUpdate(float exploreCoeff)
{
  if (floor(sqrt(visits_)) <= cCacheLastUpdate_)
    return;

  cCacheLastUpdate_ = int(floor(sqrt(visits_)));
  
  //empty cache
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
        + (cfg.historyHeuristic() ? ((getNodeType() == NODE_MAX ? twStep_->value : - twStep_->value) * 1.1 /mysqrt(visits_)) : 0)
        ;
}

//---------------------------------------------------------------------

float Node::ucb(float exploreCoeff) const
{
  return (getNodeType() == NODE_MAX ? value_ : - value_) 
         + sqrt(exploreCoeff)/mysqrt(visits_);
}

//---------------------------------------------------------------------

float Node::ucbTuned(float exploreCoeff) const
{
  double v = max(0.01, min(0.25, 2 * double(squareSum_)/visits_ + 0.2 * sqrt(exploreCoeff/visits_)));

  return (getNodeType() == NODE_MAX ? value_ : - value_) 
         + sqrt(v * exploreCoeff)/mysqrt(visits_);
}

//---------------------------------------------------------------------

void Node::addChild(Node* newChild)
{
  assert(newChild->sibling_ == NULL);
  assert(newChild->firstChild_ == NULL);
  
  newChild->sibling_ = this->firstChild_;
  this->firstChild_ = newChild;
  newChild->father_ = this;
  //obsolete, now using connectChildrenToMaster
  //newChild->connectToMaster();
}

//--------------------------------------------------------------------- 
 
void Node::reverseChildren()
{
  Node* act = firstChild_;
  Node* head = NULL;
  while (act != NULL){
    firstChild_ = act->sibling_;
    act->sibling_ = head;
    head = act;
    act = firstChild_;
  }
  firstChild_ = head;
}

//---------------------------------------------------------------------

void Node::delChildrenRec()
{
  //somebody else will take care
  if (getTTitem() && getTTitem()->getNodes()->front() != this) {
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

void Node::connectToMaster(const bool lock)
{
  if ((! getFather()) || (! getFather()->getMaster())){
    return;
  }

  Node* masterFather = getFather()->getMaster(); 
  if (lock){
    masterFather->lock();
  }
  Node* child = masterFather->getFirstChild();

  while (child != NULL){
    if (child->getStep() == getStep()){
      //add link only
      setMaster(child);
      if (lock){
        masterFather->unlock();
      }
      return;
    }
    child = child->getSibling();
  }

  //if not found - create node and add to the master father
  child = new Node(twStep_, 0);  
  masterFather->addChild(child);
  setMaster(child);
  if (lock){
    masterFather->unlock();
  }
}

//--------------------------------------------------------------------- 

void Node::connectChildrenToMaster()
{
  if (! master_){
    return;
  }
  
  master_->lock();
  Node* mChild = master_->getFirstChild();
  Node* child = firstChild_;

  bool addMode = mChild ? false : true;

  while (child != NULL){
    if (addMode){
      //add node 
      //TODO remove the bind to local twStep
      Node* mChildNew = new Node(child->getTWstep(), 0);  
      master_->addChild(mChildNew);
      child->setMaster(mChildNew);
    }else{
      //connect only
      if (mChild && mChild->getStep() == child->getStep()){
        child->setMaster(mChild);
        mChild = mChild->getSibling();
      }else{
        //this shouldn't happen but it MIGHT - for instance because of tt discrepancies
        //use lockless connection to master(father already locked)
        child->connectToMaster(false);
      }
    }
    child = child->getSibling();
  }
  
  if (addMode){
    //small hack to have the same ordering in the master and slave trees
    master_->reverseChildren();
  }

  master_->unlock();
}

//--------------------------------------------------------------------- 

void Node::syncMaster()
{
  assert(master_);
  master_->lock();
  float mCombined  = master_->getValue() * master_->getVisits();
  float mOldCombined  = masterValue_ * masterVisits_; 
  float combined = value_ * visits_;
  int newVisits = visits_ + master_->getVisits() - masterVisits_;
  float newValue = (combined + mCombined - mOldCombined) / newVisits;
  master_->setVisits(newVisits);
  master_->setValue(newValue);
  master_->unlock();

  assert(newVisits >= visits_);

  /*
  float twStepCombined = twStep_->visits * twStep_->value;
  twStep_->visits = twStep_->visits + newVisits - visits_;
  twStep_->value = 
    (twStepCombined + newVisits * newValue - combined)/twStep_->visits;
  */

  value_ = newValue;
  visits_= newVisits;
  masterValue_ = newValue;
  masterVisits_= newVisits;
}

//--------------------------------------------------------------------- 

void Node::recSyncMaster()
{
  if (! master_){
    return;
  }
    
  syncMaster();

  Node* child = firstChild_;
  while(child != NULL){
    child->recSyncMaster();
    child = child->getSibling();
  }
}

//--------------------------------------------------------------------- 

void Node::updateTTbrothers(float sample, int size)
{
  //update in ttNodes
  if (getTTitem()) {
      ttItem_->value_ = 
        (ttItem_->value_ * ttItem_->visits_ + sample)/(ttItem_->visits_ + size);
      ttItem_->visits_ += size;
      //cerr << ttItem_->visits_ << " " << ttItem_->value_ << endl;
      
    NodeList * nl = getTTitem()->getNodes();
    for (NodeList::iterator it = nl->begin(); it != nl->end(); it++){
      (*it)->value_ = ttItem_->value_;
    //(*it)->visits_ = visits_;

    //this makes sense only in parallel mode
    //(*it)->masterValue_ = masterValue_;
    //(*it)->masterVisits_ = masterVisits_;
   } 
  }
}

//---------------------------------------------------------------------

void Node::update(float sample)
{
  float old_value = value_;
  if (cfg.uctRelativeUpdate() && isMature()){
    float weight = min(max(sqrt(visits_), 1.0), 10.0);
    float added = ((sample - value_) * weight)/(visits_ + weight);
    visits_ += 1;
    value_ += added;
  }else{
    value_ += (sample - value_)/++visits_;         
  }
  squareSum_ += (sample - old_value) * (sample - value_);

  if (getMaster() && glob.grand()->get01() < 0.2){
    syncMaster();
  }
  //updating brothers comes after potential sync! 
  updateTTbrothers(sample, 1);
}

//--------------------------------------------------------------------- 

void Node::updateTWstep(float sample)
{
  twStep_->value += (sample - twStep_->value) / ++(twStep_->visits);
}

//---------------------------------------------------------------------

bool Node::isMature() const
{
  return visits_ >= cfg.matureLevel() + getDepth();
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

TTitem* Node::getTTitem() const
{
  return ttItem_;
}

//---------------------------------------------------------------------

void Node::setTTitem(TTitem* item) 
{ 
  ttItem_ = item;
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

void Node::setMaster(Node* master)
{
  master_ = master;
}

//--------------------------------------------------------------------- 

Node* Node::getMaster()
{
  return master_;
}

//--------------------------------------------------------------------- 

void Node::lock()
{
  pthread_mutex_lock(&mutex);
}

//--------------------------------------------------------------------- 

void Node::unlock()
{
  pthread_mutex_unlock(&mutex);
}

//--------------------------------------------------------------------- 

nodeType_e Node::getNodeType() const
{
  return ( twStep_->step.getPlayer() == GOLD ? NODE_MAX : NODE_MIN );
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

  while (node->father_ != NULL && node->getNodeType() == getNodeType()){
    depth += max(1, node->getStep().count());
    node = node->getFather();
  }
  return depth;
}

//--------------------------------------------------------------------- 


int Node::getLevel() const
{
  Node * act = father_;
  nodeType_e lastNodeType = getNodeType();
  int level = 0;

  while (act != NULL){
    if (act->getNodeType() != lastNodeType) {
      level++;
      lastNodeType = act->getNodeType();
    }
    act = act->father_;
  }
  return level;
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

  ss << getStep().toString() << "(" << getDepthIdentifier() << " " <<  ( getNodeType()  == NODE_MAX ? "+" : "-" )  << ") " << 
        value_ << "/" << visits_ << " twstep " << twStep_->value << "/" << twStep_->visits << " " << endl;
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

void Tree::syncMaster(){
  root()->recSyncMaster();
}

//--------------------------------------------------------------------- 

void Tree::expandNode(Node* node, const StepArray& steps, uint len, const HeurArray* heurs)
{
  Node* newChild;
  assert(len);
  assert(node);
  assert(steps[0].getPlayer() == steps[len-1].getPlayer());
  for (uint i = 0; i < len; i++){
    newChild = new Node(&(twSteps_[steps[i]]), heurs ? (*heurs)[i] : 0);  
    node->addChild(newChild);
    nodesNum_++;
  }
  nodesExpandedNum_++;
  
  //expander different from representant
  if (node->getTTitem()) {
    NodeList * nl = node->getTTitem()->getNodes();
    for (NodeList::iterator it = nl->begin(); it != nl->end(); it++){
      (*it)->setFirstChild(node->getFirstChild());
    }
  }

  //ccache init
  if (cfg.childrenCache()){
    node->cCacheInit();
  }

  node->connectChildrenToMaster();
}

//--------------------------------------------------------------------- 

void Tree::expandNodeLimited(Node* node, const Move& move)
{
  Node* newChild;
  StepList stepList = move.getStepList();
  assert(! stepList.empty());
  assert(node);
  for (StepListIter it = stepList.begin(); it != stepList.end(); it++){
    newChild = new Node(&twSteps_[*it]);
    node->addChild(newChild);
    //for parallel mode(otherwise the nodes would not be in the master tree)
    newChild->connectToMaster();
    node = newChild;
    nodesNum_++;
    nodesExpandedNum_++;
  }
}

//---------------------------------------------------------------------

void Tree::uctDescend()
{
  assert(actNode()->hasChildren());
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
    //pass is not handled in the TT
    if (node->getStep().isPass() || node->getStep().isNull()){
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

      //TODO there are issues in children sharing in connection with virtual passes 
      //what is a virtual pass in one node doesn't have to be a virtual pass in another
      node->setFirstChild(repNode->getFirstChild());
      node->setTTitem(repNode->getTTitem());
      node->setValue(repNode->getValue());
      rep->push_back(node);
      //node->setVisits(repNode->getVisits());

    }else{
      //position is not in tt yet -> store it 
      rep = new NodeList();
      rep->push_back(node);
      node->setTTitem(new TTitem(rep));
      tt_->insertItem(afterStepSignature,
                    board->getPlayerToMove(), 
                    rep, 
                    node->getDepthIdentifier());
      //initial update
      node->updateTTbrothers(node->getValue() * node->getVisits(), node->getVisits());
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
  //root is NOT saved in tt

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

Uct::Uct(const Board* board)
{
  init(board);
}

//--------------------------------------------------------------------- 

Uct::Uct(const Board* board, const Uct* masterUct)
{
  init(board);
  if (masterUct){
    tree_->root()->setMaster(masterUct->tree_->root());
  }
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

void Uct::updateStatistics(Uct* ucts[], int uctsNum)
{
  int pl = 0;
  int ud = 0;
  for (int i = 0; i < uctsNum; i++){
    pl += ucts[i]->getPlayoutsNum();
    ud += ucts[i]->uctDescends_;
  }
  playouts_ = pl;
  uctDescends_ = ud;
}

//---------------------------------------------------------------------

void Uct::searchTree(const Board* refBoard, const Engine* engine)
{
  Board* board = new Board(*refBoard);
  while (! engine->checkSearchStop()){
    doPlayout(board);
  }
  delete(board);

  //this slows down a lot (final tree might be big) 
  //and is virtually useless since syncing is continuous
  //tree_->syncMaster();
}

//--------------------------------------------------------------------- 

void Uct::refineResults(const Board* board) 
{
  bestMoveNode_ = tree_->findBestMoveNode(tree_->root());
  Move bestMove = tree_->findBestMove(bestMoveNode_);
  bestMoveRepr_ = board->moveToStringWithKills(bestMove);

  //TODO future thirdRepetitionCheck - add signature of final position to thirdRep ?
}

//---------------------------------------------------------------------

void Uct::doPlayout(const Board* board)
{
  Board *playBoard = new Board(*board);
  playoutStatus_e playoutStatus;

  //point tree's actNode to the root 
  tree_->historyReset();    

  logDDebug(board->toString().c_str());
  logDDebug("===== Playout :===== ");
  do { 
   logDDebug(tree_->actNode()->toString().c_str());
  
    if (! tree_->actNode()->hasChildren()) { 
      if (tree_->actNode()->getDepth() < UCT_MAX_DEPTH - 1) {
        if (tree_->actNode()->isMature()) {
          Move move;

          //goalCheck 
          //
          if (playBoard->getStepCount() == 0 && playBoard->goalCheck(&move)){
            assert(playBoard->getWinner() == NO_PLAYER );
            float value = WINNER_TO_VALUE(playBoard->getPlayerToMove());
            tree_->expandNodeLimited(tree_->actNode(), move);
            //descend to the expanded area
            while (tree_->actNode() && tree_->actNode()->hasChildren()){
              tree_->firstChildDescend();
            }
            tree_->updateHistory(value);
            break;
          }
          
          //tactics in playouts extension
          if (cfg.moveAdvisor()){
            fill_advisor(playBoard); 
          } 

          StepArray steps;    
          uint      stepsNum;

          stepsNum = playBoard->genStepsNoPass(playBoard->getPlayerToMove(), steps);
          stepsNum = playBoard->filterRepetitions(steps, stepsNum);

          //add pass if possible
          if (playBoard->canPass()) { 
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
          //imobilization, expand with null step
          else{
            //null step means a loss for player to play it
            steps[stepsNum++] = Step(STEP_NULL, playBoard->getPlayerToMove());
            tree_->expandNode(tree_->actNode(), steps, stepsNum);
          }

          continue;
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
    } //no children

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
        << "  " << tree_->getNodesPruned() << " nodes pruned" << endl 
        << "  " << uctDescends_/float(playouts_) << " average descends in playout" << endl 
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

  //opponent trapCheck
  MoveList moves;
  moves.clear();
  if (playBoard->trapCheck(playBoard->getPlayerToMove(), &moves)){ 
    for (MoveList::const_iterator it = moves.begin(); it != moves.end(); it++){ 
      advisor_->addMove((*it), playBoard->getBitboard());
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
  
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

