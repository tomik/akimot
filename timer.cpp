
#include "timer.h"

//--------------------------------------------------------------------- 

Timer::Timer()
{
}

//--------------------------------------------------------------------- 

Timer::Timer(double swTime): swTime_(swTime)
{
}

//--------------------------------------------------------------------- 

void Timer::setTimer(double swTime)
{
  swTime_ = swTime;
}

//--------------------------------------------------------------------- 

double Timer::now()
{
  timeval t;
  gettimeofday(&t, NULL); 
  return toSeconds(t);
}

//--------------------------------------------------------------------- 

void Timer::start()
{
  timeval t;
  gettimeofday(&t, NULL); 
  start_ = toSeconds(t);
}

//--------------------------------------------------------------------- 

double Timer::elapsed()
{
  timeval t;
  gettimeofday(&t, NULL); 
  return toSeconds(t) - start_; 
}

//--------------------------------------------------------------------- 

bool Timer::timeUp()
{
  timeval t;
  gettimeofday(&t, NULL); 
  if (toSeconds(t) - start_ >= swTime_) 
    return true;
  return false;  
}

//--------------------------------------------------------------------- 

double Timer::toSeconds(timeval t) 
{
  return t.tv_sec + t.tv_usec / double(MICRO_IN_SEC);
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 
