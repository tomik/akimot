
#include "timer.h"

//--------------------------------------------------------------------- 

Timer::Timer()
{
  elapsed_ = 0;
}

//--------------------------------------------------------------------- 

Timer::Timer(double swTime): swTime_(swTime)
{
  elapsed_ = 0;
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

void Timer::stop()
{
  timeval t;
  gettimeofday(&t, NULL); 

  elapsed_ = toSeconds(t) - start_; 
}

//--------------------------------------------------------------------- 

double Timer::elapsed()
{
  #ifdef NDEBUG
    timeval t;
    gettimeofday(&t, NULL);
    assert( toSeconds(t) - start_ >= elapsed_); 
  #endif

  return elapsed_;
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
