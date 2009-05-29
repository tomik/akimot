/** 
 *  @file engine.cpp
 *  @brief Searching engine. 
 *  @full Implementation of UCT and supporting components.
 */

#include "engine.h"

//---------------------------------------------------------------------
//  section TimeManager
//---------------------------------------------------------------------

TimeManager::TimeManager()
{
  for (int i = 0; i < TIME_CONTROLS_NUM; i++)
    timeControls_[i] = 0;
  noTimeLimit_ = false;
  timeControls_[TC_MOVE] = TC_MOVE_DEFAULT;
}

//--------------------------------------------------------------------- 

void TimeManager::startClock()
{
  timer.setTimer(timeControls_[TC_MOVE] - CLOCK_CLICK_RESERVE);
  timer.start();
}

//--------------------------------------------------------------------- 

void TimeManager::stopClock()
{
  timer.stop();
}

//---------------------------------------------------------------------

bool TimeManager::timeUp()
{
  if ( noTimeLimit_ || ! timer.timeUp())
    return false;
  return true;
}

//---------------------------------------------------------------------

double TimeManager::secondsElapsed()
{
  return timer.elapsed();
}
  
//---------------------------------------------------------------------

void TimeManager::setTimeControl(timeControl_e tc, float value)
{
  assert(tc >= 0 && tc < TIME_CONTROLS_NUM);

  if ( tc >= 0 && tc < TIME_CONTROLS_NUM)
    timeControls_[tc] = value;
}

//--------------------------------------------------------------------- 

float TimeManager::getTimeControl(timeControl_e tc)
{
  assert(tc >= 0 && tc < TIME_CONTROLS_NUM);

  if ( tc >= 0 && tc < TIME_CONTROLS_NUM)
    return timeControls_[tc];
  return -1;
}

//--------------------------------------------------------------------- 

void TimeManager::setNoTimeLimit()
{
  noTimeLimit_ = true; 
}

//--------------------------------------------------------------------- 

void TimeManager::resetSettings()
{
  noTimeLimit_ = false;
}

//---------------------------------------------------------------------
//  section Engine
//---------------------------------------------------------------------

Engine::Engine()
{
  timeManager_ = new TimeManager();
  ponder_ = false;
  bestMove_ = "";
  additionalInfo_ = "";
  stats_ = "";
  winRatio_ = 0.5;
}

//--------------------------------------------------------------------- 

Engine::~Engine()
{
  delete timeManager_;
}

//--------------------------------------------------------------------- 

string Engine::initialSetup(bool isGold) const
{
	if (isGold)
		return "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2";
	else
		return "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7";
}


//---------------------------------------------------------------------

void Engine::doSearch(const Board* board) 
{ 
  if (board->isSetupPhase()){
    bestMove_ = initialSetup(board->getPlayerToMove() == GOLD);
    return;
  }

  int threadsNum = cfg.searchThreadsNum();

  if (threadsNum < 0){
    logWarning("Too little threads falling back to 1 thread.");
    threadsNum = 1;
  }
  if (threadsNum > MAX_THREADS){
    logWarning("Too many threads falling back to %d", MAX_THREADS);
    threadsNum = MAX_THREADS;
  }

  glob.init();

  pthread_t threads[MAX_THREADS];
  Uct* ucts[MAX_THREADS];
  pthread_attr_t attr;
  int rc, t;
  void *status;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  stopRequest_ = false;
  timeManager_->startClock();

  Uct* masterUct  = cfg.searchThreadsNum() > 1 ? new Uct(board) : NULL;

  for(t=0; t<threadsNum; t++){
    ucts[t] = new Uct(board, masterUct);
    SearchStartKit * s = new SearchStartKit(board, this, ucts[t]);
    rc = pthread_create(&threads[t], &attr, Engine::searchTreeWrapper,(void*) s); 
    if (rc) {
      stringstream ss;
      ss << "Fatal thread error no. " << rc << " when creating.";
      logError(ss.str().c_str());
      exit(1);
    } 
  } 

  for(t=0; t<threadsNum; t++){
    rc = pthread_join(threads[t], &status); 
    if (rc) {
      stringstream ss;
      ss << "Fatal thread error no. " << rc << " when joining.";
      logWarning(ss.str().c_str());
      pthread_detach(threads[t]);
    }
  }
  timeManager_->stopClock();
  mockupSearchResults(board, ucts, masterUct, threadsNum); 

  for(t=0; t<threadsNum; t++){
    delete ucts[t];
  }
}

//---------------------------------------------------------------------

void Engine::requestSearchStop()
{
  stopRequest_ = true;
}

//--------------------------------------------------------------------- 

TimeManager* Engine::timeManager()
{
  return timeManager_;
}

//--------------------------------------------------------------------- 

void Engine::setPonder(bool value)
{
  ponder_ = value;
}

//--------------------------------------------------------------------- 

bool Engine::getPonder() const
{
  return ponder_;
}

//--------------------------------------------------------------------- 

string Engine::getBestMove() const
{
  return bestMove_;
}

//--------------------------------------------------------------------- 

string Engine::getStats() const
{
  return stats_;
}

//--------------------------------------------------------------------- 

string Engine::getAdditionalInfo() const
{
  return additionalInfo_;
}

//--------------------------------------------------------------------- 

float Engine::getWinRatio() const
{
  return winRatio_;
}

//--------------------------------------------------------------------- 

bool Engine::checkSearchStop() const
{
  return timeManager_->timeUp() || stopRequest_;
}

//--------------------------------------------------------------------- 

void Engine::mockupSearchResults(const Board* board, Uct* ucts[], Uct* masterUct, int resultsNum)
{
  assert(resultsNum);

  Uct * uct = ucts[0]; 
  if (masterUct) {
    uct = masterUct; 
    uct->updateStatistics(ucts, resultsNum);
  }
  uct->refineResults(board);
  //Uct * uct = new Uct(board, ucts, resultsNum);
  //get stuff from the search 
  bestMove_ = uct->getBestMoveRepr();
  stats_ = uct->getStats(timeManager_->secondsElapsed());
  additionalInfo_ = uct->getAdditionalInfo();
  winRatio_ = uct->getWinRatio();

}

//--------------------------------------------------------------------- 

void * Engine::searchTreeWrapper(void * searchStartKit) 
{
  SearchStartKit * s = (SearchStartKit *) searchStartKit;
  s->uct_->searchTree(s->board_, s->engineInstance_);
  delete s;
  return NULL;
}

//---------------------------------------------------------------------
//  section TimeManager
//---------------------------------------------------------------------

SearchStartKit::SearchStartKit(const Board* board, Engine * engine, Uct* uct)
{
  board_ = board;  
  engineInstance_ = engine;
  uct_ = uct;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
