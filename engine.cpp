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
  
  Board* board_copy = new Board(*board);

  Uct * uct = new Uct();

  stopRequest_ = false;
  timeManager_->startClock();
  uct->searchTree(board_copy, this);
  timeManager_->stopClock();
  
  //get stuff from the search 
  bestMove_ = uct->getBestMoveRepr();
  stats_ = uct->getStats(timeManager_->secondsElapsed());
  additionalInfo_ = uct->getAdditionalInfo();
  winRatio_ = uct->getWinRatio();

  delete(uct);
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
//---------------------------------------------------------------------
