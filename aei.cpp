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
#define STR_GO_NO_THREAD "gonothread"
#define STR_PONDER "ponder"
#define STR_INFINITE "infinite"
#define STR_STOP "stop"
#define STR_MAKE_MOVE "makemove"
#define STR_MAKE_MOVE_REC "makemoverec"
#define STR_BEST_MOVE "bestmove"
#define STR_QUIT "quit"
#define STR_BYE "bye"
#define STR_BOARD_DUMP "boarddump"
#define STR_TREE_DUMP "treedump"
#define STR_GOAL_CHECK "goalcheck"
#define STR_TRAP_CHECK "trapcheck"
#define STR_EVAL "eval"


#define STR_INFO "info"  
#define STR_INFO_TIME "time" 
#define STR_INFO_WIN_RATIO "winratio"
//TODO 
#define STR_INFO_DEPTH "depth"
#define STR_INFO_STATS "stat"
#define STR_INFO_NODES "nodes"
#define STR_INFO_GOAL_CHECK "goalcheck"
#define STR_INFO_TRAP_CHECK "trapcheck"


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

#define STR_LOG_ERROR "log Error: "
#define STR_LOG_WARNING "log Warning: "
#define STR_LOG_DEBUG "log Debug: "
#define STR_LOG_INFO "log Info: "

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
  commandSet_ = AC_STD;
}

//--------------------------------------------------------------------- 

AeiRecord::AeiRecord(string command, aeiState_e state,
                        aeiState_e nextState, aeiAction_e action, aeiCommandSet_e commandSet):
  state_(state), command_(command), nextState_(nextState), action_(action), commandSet_(commandSet)
{
}

//--------------------------------------------------------------------- 
// section Aei
//--------------------------------------------------------------------- 

Aei::Aei()
{ 
  init();

}

//--------------------------------------------------------------------- 

Aei::Aei(aeiCommandSet_e commandSet)
{
  init();
  commandSet_ = commandSet;
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

void Aei::initFromFile(string fn)
{
  fstream f;
  string line;
  stringstream ss;

  try { 
    f.open(fn.c_str(), fstream::in);
    if (! f.good()){
      f.close();
      return; 
    }

    while (! f.eof()){
      getline(f, line);
      if (! line.length())
        continue;
      if (line[0] != '#'){
        logRaw("received: %s", line.c_str());
        handleInput(line);
      }
    }

  }
  catch(int e) {
    return; 
  }

}

//--------------------------------------------------------------------- 

void Aei::init()
{

  records_.clear();
  records_.push_back(AeiRecord(STR_AEI, AS_OPEN, AS_MAIN, AA_OPEN));
  records_.push_back(AeiRecord(STR_READY, AS_ALL, AS_SAME, AA_READY));
  records_.push_back(AeiRecord(STR_QUIT, AS_ALL, AS_SAME, AA_QUIT));
  records_.push_back(AeiRecord(STR_NEW_GAME, AS_MAIN, AS_SAME, AA_NEW_GAME));
  records_.push_back(AeiRecord(STR_SET_POSITION, AS_MAIN, AS_SAME, AA_SET_POSITION));
  records_.push_back(AeiRecord(STR_SET_POSITION_FILE, AS_MAIN, AS_SAME, AA_SET_POSITION_FILE));
  records_.push_back(AeiRecord(STR_SET_OPTION, AS_ALL, AS_SAME, AA_SET_OPTION));
  records_.push_back(AeiRecord(STR_GO, AS_MAIN, AS_SEARCH, AA_GO));
  records_.push_back(AeiRecord(STR_GO_NO_THREAD, AS_MAIN, AS_SAME, AA_GO_NO_THREAD, AC_EXT));
  records_.push_back(AeiRecord(STR_STOP, AS_SEARCH, AS_MAIN, AA_STOP));
  records_.push_back(AeiRecord(STR_STOP, AS_PONDER, AS_MAIN, AA_STOP));
  records_.push_back(AeiRecord(STR_MAKE_MOVE, AS_MAIN, AS_SAME, AA_MAKE_MOVE));
  records_.push_back(AeiRecord(STR_MAKE_MOVE, AS_PONDER, AS_MAIN, AA_MAKE_MOVE));
  //makes move recommended from last search
  records_.push_back(AeiRecord(STR_MAKE_MOVE_REC, AS_MAIN, AS_SAME, AA_MAKE_MOVE_REC, AC_EXT));
  //board dump
  records_.push_back(AeiRecord(STR_BOARD_DUMP, AS_MAIN, AS_SAME, AA_BOARD_DUMP, AC_EXT));
  //tree dump
  records_.push_back(AeiRecord(STR_TREE_DUMP, AS_MAIN, AS_SAME, AA_TREE_DUMP, AC_EXT));
  //goal check
  records_.push_back(AeiRecord(STR_GOAL_CHECK, AS_MAIN, AS_MAIN, AA_GOAL_CHECK));
  //trap check
  records_.push_back(AeiRecord(STR_TRAP_CHECK, AS_MAIN, AS_MAIN, AA_TRAP_CHECK));
  //evaluation
  records_.push_back(AeiRecord(STR_EVAL, AS_MAIN, AS_MAIN, AA_EVAL));

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
  commandSet_ = AC_STD;
  board_ = new Board();
  engine_ = new Engine();
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
  aeiState_e oldState; 
  string lineRest;
  
  AeiRecord* record = NULL;
  AeiRecordList::iterator it; 

  //comments 
  if (line == "" || line[0] == '#')
    return;

  for (it = records_.begin(); it != records_.end(); it++)
    if ((it->state_ == state_ || it->state_ == AS_ALL) 
        && it->command_ == command && commandSet_ >= it->commandSet_){
      record = &(*it); 
      break;
    }

  if (! record) {
    aeiLog(string(STR_INVALID_COMMAND) + string(" ") + command, AL_ERROR);
    return;
  }

  oldState = state_;
  if (record->nextState_ != AS_SAME)
    state_ = record->nextState_;

  lineRest = getStreamRest(ssLine);

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
                  handleOption(lineRest);
                  break;
    case AA_SET_POSITION_FILE: 
                  if (! board_->initFromPosition(lineRest.c_str())){
                    aeiLog(STR_LOAD_FAIL, AL_ERROR);
                    quit();
                  }
                  break;
    case AA_SET_POSITION: 
                  board_->initNewGame();
                  if (! board_->initFromPositionCompactString(lineRest)){
                    aeiLog(STR_LOAD_FAIL, AL_ERROR);
                    quit();
                  }
                  break;
    case AA_GO:    
                  if (lineRest == STR_PONDER){
                    state_ = AS_PONDER;
                  }
                  startSearch(lineRest);
                  break;
    case AA_GOAL_CHECK:    
                  goalCheck(); 
                  break;
    case AA_TRAP_CHECK:    
                  trapCheck(); 
                  break;
    case AA_EVAL:    
                  evalActPos();
                  break;
    case AA_GO_NO_THREAD: 
                  engine_->timeManager()->resetSettings();
                  engine_->doSearch(board_);
                  sendSearchInfo();
                  break;
    case AA_STOP: 
                  stopSearch();
                  break;
    case AA_MAKE_MOVE:
                  if (oldState == AS_PONDER){
                    stopSearch();
                  }
                  board_->makeMove(lineRest);
                  aeiLog("Making move: " + lineRest, AL_DEBUG);
                  break;
    case AA_MAKE_MOVE_REC:
                  board_->makeMove(engine_->getBestMove());
                  aeiLog("Making move: " + engine_->getBestMove(), AL_DEBUG);
                  break;
    case AA_BOARD_DUMP:
                 aeiLog(board_->toString(), AL_DEBUG);
                 break;
    case AA_TREE_DUMP:
                 aeiLog(engine_->getAdditionalInfo(), AL_DEBUG);
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
      engine_->timeManager()->setTimeControl(it->second, str2float(value));
      return;
    }

  aeiLog(STR_UNKNOWN_OPTION + option, AL_WARNING);
}

//--------------------------------------------------------------------- 

void Aei::startSearch(const string& arg)
{
  engine_->setPonder(arg == STR_PONDER);
  engine_->timeManager()->resetSettings();
  if (arg == STR_PONDER || arg == STR_INFINITE)
    engine_->timeManager()->setNoTimeLimit();
  int rc;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  //no mutex is needed - this is done only when no engineThread runs
  rc = pthread_create(&engineThread_, &attr, Aei::SearchInThreadWrapper, this);

  if (rc){ //allocating thread failed
    stringstream ss;
    ss << "Fatal thread error no. " << rc << " when creating.";
    aeiLog(ss.str(), AL_ERROR);
    quit();
  }
}

//--------------------------------------------------------------------- 

void Aei::goalCheck()
{
  Board * bb = new Board(*board_);
  Move goaldMove;
  Move silverMove;
  bool goldGoal = bb->goalCheck(GOLD, 4, &goaldMove);
  bool silverGoal = bb->goalCheck(SILVER, 4, &silverMove);

  if (goldGoal){
    sendInfo(STR_INFO_GOAL_CHECK, "gold " + goaldMove.toString());
  }
  if (silverGoal){
    sendInfo(STR_INFO_GOAL_CHECK, "silver " + silverMove.toString());
  }
  delete bb; 
  if (! goldGoal && ! silverGoal ){
    sendInfo(STR_INFO_GOAL_CHECK, "none");
  }
}

//--------------------------------------------------------------------- 

void Aei::trapCheck()
{
  Board * bb = new Board(*board_);
  //MoveList goaldMove;
  //MoveList silverMove;
  bool goldGoal = bb->trapCheck(GOLD, NULL);
  bool silverGoal = bb->trapCheck(SILVER, NULL);

  if (goldGoal){
    sendInfo(STR_INFO_TRAP_CHECK, "gold can be trapped"); 
  }
  if (silverGoal){
    sendInfo(STR_INFO_TRAP_CHECK, "silver can be trapped"); 
  }
  delete bb; 
  if (! goldGoal && ! silverGoal ){
    sendInfo(STR_INFO_TRAP_CHECK, "none");
  }
}

//--------------------------------------------------------------------- 

void Aei::evalActPos()
{
  Eval* eval = new Eval();
  stringstream ss;
  ss << eval->evaluateInPercent(board_);
  aeiLog("Estimated winning probability for gold:" + ss.str(), AL_INFO);
  delete eval;
}

void Aei::searchInThread()
{
  engine_->doSearch(board_);
  stopSearch(true);
}

//--------------------------------------------------------------------- 

void * Aei::SearchInThreadWrapper(void* instance)
{
  ((Aei*) instance)->searchInThread();
  return NULL;
}

//--------------------------------------------------------------------- 

void Aei::stopSearch(bool fromThread)
{
  void* status;
  int rc;

  if (fromThread){
    //detach itself
    pthread_detach(engineThread_);
    //TODO refactor - not to alter state manually
    state_ = AS_MAIN;
  }
  else{
    //wait for search thread to finish
    engine_->requestSearchStop();
    rc=pthread_join(engineThread_, &status);
    if (rc){
      stringstream ss;
      ss << "Fatal thread error no. " << rc << " when joining.";
      aeiLog(ss.str(), AL_ERROR);
      pthread_detach(engineThread_);
    }
  }

  sendSearchInfo();
}

//--------------------------------------------------------------------- 

void Aei::sendSearchInfo()
{
  aeiLog("Search finished. Suggested move: " + engine_->getBestMove(), AL_DEBUG);
  stringstream ss;
  //send statistics
  ss.str(engine_->getStats());
  string s;
  while (getline(ss, s)){
    sendInfo(STR_INFO_STATS, s);
  }
  //send time info/winratio
  ss.clear();
  ss.str("");
  ss << engine_->timeManager()->secondsElapsed(); 
  sendInfo(STR_INFO_TIME, ss.str());
  ss.str(""); 
  ss << engine_->getWinRatio();
  sendInfo(STR_INFO_WIN_RATIO, ss.str()); 
   //send the answer when not pondering
  if (! engine_->getPonder()){
    send(string(STR_BEST_MOVE) + " " + engine_->getBestMove());
  }   
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

void Aei::sendInfo(const string& type, const string& value) const
{
  stringstream ss;
  ss << STR_INFO << " " << type << " " <<  value;
  send(ss.str());
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
