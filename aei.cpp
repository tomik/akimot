#include "aei.h"
//--------------------------------------------------------------------- 
// section Strings used in communication
//--------------------------------------------------------------------- 

#define STR_AEI "aei"
#define STR_AEI_OK "aeiok"
#define STR_READY "isready"
#define STR_READY_OK "readyok"
#define STR_NEW_GAME "newgame"
#define STR_SET_POSITION "setposition"
#define STR_SET_POSITION_FILE "setpositionfile"
#define STR_SET_OPTION "setoption"
#define STR_OPTION_NAME "name"
#define STR_OPTION_VALUE "value"
#define STR_UNKNOWN_OPTION "unknown option "
#define STR_GO "go"
#define STR_PONDER "ponder"
#define STR_INFINITE "infinite"
#define STR_STOP "stop"
#define STR_MAKE_MOVE "makemove"
#define STR_BEST_MOVE "bestmove"
#define STR_QUIT "quit"
#define STR_BYE "bye"

#define STR_TC_MOVE "tcmove"
#define STR_TC_RESERVE "tcreserve"
#define STR_TC_PERCENT "tcpercent"
#define STR_TC_MAX "tcmax"
#define STR_TC_TOTAL "tctotal"
#define STR_TC_TURNS "tcturns"
#define STR_TC_TURN_TIME "tcturntime"
#define STR_TC_W_RESERVE "wreserve"
#define STR_TC_B_RESERVE "breserve"
#define STR_TC_W_USED "wused"
#define STR_TC_B_USED "bused"
#define STR_TC_LAST_MOVE_USED "lastmoveused"
#define STR_TC_MOVE_USED "tcmoveused"

#define STR_LOG_ERROR "log Error:"
#define STR_LOG_WARNING "log Warning:"
#define STR_LOG_DEBUG "log Debug:"
#define STR_LOG_INFO "log "

#define STR_LOAD_FAIL "Fatal error occured while loading position."

#define STR_INVALID_COMMAND "Invalid command"

#define STR_ID "id"
#define STR_ID_NAME "name"
#define STR_ID_AUTHOR "author"
#define STR_ID_VERSION "version"


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
  records_.push_back(AeiRecord(STR_NEW_GAME, AS_MAIN, AS_SAME, AA_NEW_GAME));
  records_.push_back(AeiRecord(STR_SET_POSITION, AS_MAIN, AS_SAME, AA_SET_POSITION));
  records_.push_back(AeiRecord(STR_SET_POSITION_FILE, AS_MAIN, AS_SAME, AA_SET_POSITION_FILE));
  records_.push_back(AeiRecord(STR_SET_OPTION, AS_MAIN, AS_SAME, AA_SET_OPTION));
  records_.push_back(AeiRecord(STR_GO, AS_MAIN, AS_SEARCH, AA_GO));
  records_.push_back(AeiRecord(STR_STOP, AS_SEARCH, AS_MAIN, AA_STOP));
  //TODO MAKE_MOVE might come also during search ( pondering ) 
  //should end pondering and make move, no output !
  records_.push_back(AeiRecord(STR_MAKE_MOVE, AS_MAIN, AS_SAME, AA_MAKE_MOVE));

  timeControls_.push_back(timeControlPair(STR_TC_MOVE, TC_MOVE));
  timeControls_.push_back(timeControlPair(STR_TC_RESERVE, TC_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_PERCENT, TC_PERCENT));
  timeControls_.push_back(timeControlPair(STR_TC_MAX, TC_MAX));
  timeControls_.push_back(timeControlPair(STR_TC_TOTAL, TC_TOTAL));
  timeControls_.push_back(timeControlPair(STR_TC_TURNS, TC_TURNS));
  timeControls_.push_back(timeControlPair(STR_TC_TURN_TIME, TC_TURN_TIME));
  timeControls_.push_back(timeControlPair(STR_TC_W_RESERVE, TC_W_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_B_RESERVE, TC_B_RESERVE));
  timeControls_.push_back(timeControlPair(STR_TC_W_USED, TC_W_USED));
  timeControls_.push_back(timeControlPair(STR_TC_B_USED, TC_B_USED));
  timeControls_.push_back(timeControlPair(STR_TC_LAST_MOVE_USED, TC_LAST_MOVE_USED));
  timeControls_.push_back(timeControlPair(STR_TC_MOVE_USED, TC_MOVE_USED));

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

void Aei::implicitSessionStart()
{
  handleInput(STR_AEI);
  handleInput(STR_NEW_GAME);
  //handleInput((string) STR_SET_POSITION_FILE + " " + config.fnInput());
  handleInput("setposition w [rrr r rrrdd  e                   ED     RhMH  C   mC   RRRR c RR]");
  handleInput("setoption name tcmove value 5");
  //handleInput(STR_GO);
}

//--------------------------------------------------------------------- 

void Aei::handleInput(const string& line)
{
  stringstream ssLine;
  stringstream rest;
  ssLine.clear();
  rest.clear();
  ssLine.str(line);
  string command;
  ssLine >> command; 
  response_ = "";
  
  AeiRecord* record = NULL;
  AeiRecordList::iterator it; 

  //log("received: " + line, LL_INFO);

  for (it = records_.begin(); it != records_.end(); it++)
    if ((it->state_ == state_ || it->state_ == AS_ALL) && it->command_ == command){
      record = &(*it); 
      break;
    }

  if (! record) {
    aeiLog(STR_INVALID_COMMAND, AL_ERROR);
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
                  board_->initNewGame(); 
                  break;
    case AA_SET_OPTION:
                  handleOption(getStreamRest(ssLine));
                  break;
    case AA_SET_POSITION_FILE: 
                  if (! board_->initFromPosition(getStreamRest(ssLine).c_str())){
                    aeiLog(STR_LOAD_FAIL, AL_ERROR);
                    quit();
                  }
                  break;
    case AA_SET_POSITION: 
                  if (! board_->initFromPositionCompactString(getStreamRest(ssLine))){
                    aeiLog(STR_LOAD_FAIL, AL_ERROR);
                    quit();
                  }
                  break;
    case AA_GO:    
                  startSearch(getStreamRest(ssLine));
                  break;
    case AA_STOP: 
                  engine_->requestSearchStop();
                  break;
    case AA_MAKE_MOVE:
                  board_->makeMove(getStreamRest(ssLine));
                  break;
    case AA_QUIT: 
                  quit();
                  break;
    default: 
             assert(false);
             break;
  }

  send(response_);
}

//--------------------------------------------------------------------- 

void Aei::handleOption(const string& commandRest)
{
  string option;
  string value;
  string s;
  stringstream ss ;
  ss.str(commandRest);

  bool correct = true;;

  ss >> s;
  if ( s != STR_OPTION_NAME)
    correct = false;
  ss >> option;
  ss >> s;
  if ( s != STR_OPTION_VALUE)
    correct = false;
  ss >> value;

  if (! correct) {
    aeiLog(STR_UNKNOWN_OPTION + option, AL_WARNING);
    return;
  }

  TimeControlList::iterator it;
  for (it = timeControls_.begin(); it != timeControls_.end(); it++)
    if (it->first == option){
      engine_->timeManager()->setTimeControl(it->second, str2int(value));
      return;
    }

  aeiLog(STR_UNKNOWN_OPTION + option, AL_WARNING);
}

//--------------------------------------------------------------------- 

void Aei::startSearch(const string& arg)
{
  int rc;

  if (arg == STR_PONDER || arg == STR_INFINITE)
    engine_->timeManager()->setNoTimeLimit();
  //no mutex is needed - this is done only when no engineThread runs
  rc = pthread_create(&engineThread, NULL, Aei::SearchInThreadWrapper, this);

  if (rc){ //allocating thread failed
    aeiLog("Fatal thread error occured.", AL_ERROR);
    quit();
  }
}

//--------------------------------------------------------------------- 

void Aei::searchInThread()
{
  engine_->doSearch(board_);
  state_ = AS_MAIN; //after search switch back to GAME mood - TODO mutex this?   
  send(string(STR_BEST_MOVE) + " " + engine_->getBestMove());
}

//--------------------------------------------------------------------- 

void * Aei::SearchInThreadWrapper(void* instance)
{
  Aei* aei= (Aei*) instance;
  aei->searchInThread();
  return NULL;
}

//--------------------------------------------------------------------- 

void Aei::aeiLog(const string& msg, const aeiLogLevel_e logLevel) const 
{
  string completeMsg = msg;

  switch(logLevel){
    case AL_ERROR:
                  completeMsg = STR_LOG_ERROR + completeMsg;
                  break;
    case AL_WARNING:
                  completeMsg = STR_LOG_WARNING + completeMsg;
                  break;
    case AL_DEBUG:
                  completeMsg = STR_LOG_DEBUG + completeMsg;
                  break;
    case AL_INFO:
                  completeMsg = STR_LOG_INFO + completeMsg;
                  break;
    default:
                  assert(false);
                  break;
  }
  send(completeMsg);
}

//--------------------------------------------------------------------- 

void Aei::quit() const
{
  aeiLog(STR_BYE, AL_INFO);
  exit(0);
}

//--------------------------------------------------------------------- 

void Aei::sendId() const
{
  stringstream ss; 
  ss.clear();

  ss << STR_ID << " " << STR_ID_NAME << " " << ID_NAME;
  send(ss.str());
  ss.str("");

  ss << STR_ID << " " << STR_ID_AUTHOR << " " << ID_AUTHOR;
  send(ss.str());
  ss.str("");

  ss << STR_ID << " " << STR_ID_VERSION << " " << ID_VERSION;
  send(ss.str());
  ss.str("");
}

//---------------------------------------------------------------------

void Aei::send(const string& s) const
{
  if (s.length() > 0 || s == "\n"){
    cout << s << endl << flush;
  }
  
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 
