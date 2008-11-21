#include "aei.h"

void * aeiSearchInThread(void* instance)
{
  Aei* aei= (Aei*) instance;
  aei->searchInThread();
  return NULL;
}

//--------------------------------------------------------------------- 
// section AeiRecord
//--------------------------------------------------------------------- 

AeiRecord::AeiRecord(string command, aeiState_e state, aeiState_e nextState, aeiAction_e action):
  state_(state), command_(command), nextState_(nextState), action_(action)
{
}

//--------------------------------------------------------------------- 
// section Aei
//--------------------------------------------------------------------- 

Aei::Aei()
{
  records_.clear();
  records_.push_back(AeiRecord(STR_AEI, AS_OPEN, AS_MAIN, AA_OPEN));
  records_.push_back(AeiRecord(STR_READY, AS_ALL, AS_SAME, AA_READY));
  records_.push_back(AeiRecord(STR_QUIT, AS_ALL, AS_SAME, AA_QUIT));
  records_.push_back(AeiRecord(STR_NEW_GAME, AS_MAIN, AS_GAME, AA_NEW_GAME));
  records_.push_back(AeiRecord(STR_SET_POSITION, AS_GAME, AS_SAME, AA_SET_POSITION));
  records_.push_back(AeiRecord(STR_SET_POSITION_FILE, AS_GAME, AS_SAME, AA_SET_POSITION_FILE));
  records_.push_back(AeiRecord(STR_SET_OPTION, AS_MAIN, AS_SAME, AA_SET_OPTION));
  records_.push_back(AeiRecord(STR_GO, AS_GAME, AS_SEARCH, AA_GO));
  records_.push_back(AeiRecord(STR_STOP, AS_SEARCH, AS_GAME, AA_STOP));

  state_ = AS_OPEN;
  board_ = new Board();
  engine_ = new Engine();
}

//--------------------------------------------------------------------- 

Aei::~Aei()
{
  delete board_;
  delete engine_;
}

//--------------------------------------------------------------------- 

void Aei::runLoop() 
{
  string line;
  while (true) {
    if (cin.good()){
      getline(cin, line);
      if (line != "")
        handleInput(line);
    }
  }
}

//--------------------------------------------------------------------- 

bool Aei::checkSearchInterrupt()
{
  assert(state_ == AS_SEARCH);
  bool interrupt = false;
  string line;

  if (cin.good()){
    getline(cin, line);
    if (line != ""){
      handleInput(line);
      interrupt = true;
    }
  }
  
  return interrupt;
}

//--------------------------------------------------------------------- 

void Aei::handleInput(const string& line)
{
  stringstream ssLine;
  stringstream rest;
  ssLine.str("");
  ssLine.str(line);
  string command = "";
  string s = "";
  ssLine >> command; 
  response_ = "";
  
  AeiRecord* record = NULL;
  AeiRecordList::iterator it; 

  /*
  cout << line << endl;
  cout << command << endl;
  cout << getStreamRest(ssLine) << endl;
  command = "";
  ssLine >> command;
  cout << command << endl;
  return;
  */

  for (it = records_.begin(); it != records_.end(); it++)
    if ((it->state_ == state_ || it->state_ == AS_ALL) && it->command_ == command){
      record = &(*it); 
      break;
    }
  if (! record) {
    response_ = STR_INVALID_COMMAND;
    quit();
  }

  if (record->nextState_ != AS_SAME)
    state_ = record->nextState_;

  aeiAction_e action = record->action_;
  switch (action) {
    case AA_OPEN:
                  sendId();
                  response_ = STR_AEI_OK;
                  break;
    case AA_READY: 
                  response_ = STR_READY_OK;
                  break;
    case AA_NEW_GAME: 
                  break;
    case AA_SET_OPTION: 
                  break;
    case AA_SET_POSITION_FILE: 
                  board_->initFromPosition(getStreamRest(ssLine).c_str());
                  break;
    case AA_SET_POSITION: 
                  rest.str(getStreamRest(ssLine));
                  board_->initFromPositionStream(rest);
                  break;
    case AA_GO:    
                  rest.str(getStreamRest(ssLine));
                  if (rest.str() == STR_PONDER || rest.str() == STR_INFINITE)
                    engine_->timeManager()->setTimeOption(TO_INFINITE, true);
                  pthread_create(&engineThread, NULL, aeiSearchInThread, this);
                  break;
    case AA_STOP: 
                  break;
    case AA_QUIT: response_ = STR_BYE;
                  quit();
                  break;
    default: assert(false);
             break;
  }

  send(response_);
}

//--------------------------------------------------------------------- 

void Aei::searchInThread()
{
  engine_->doSearch(board_);
  response_ = STR_BEST_MOVE + engine_->getBestMove();
  send(response_);
}

//--------------------------------------------------------------------- 

void Aei::quit() const
{
  send(response_);
  exit(1);
}

//--------------------------------------------------------------------- 

void Aei::sendId() const
{
  stringstream ss; 
  ss.clear();

  ss << STR_NAME << " " << ID_NAME;
  send(ss.str());
  ss.str("");

  ss << STR_AUTHOR << " " << ID_AUTHOR;
  send(ss.str());
  ss.str("");

  ss << STR_VERSION << " " << ID_VERSION;
  send(ss.str());
  ss.str("");
}

//--------------------------------------------------------------------- //

void Aei::send(const string& s) const
{
  if (s.length())
  cout << s << endl;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

Aei* aei = new Aei();
