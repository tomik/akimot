/**
 * @file timer.h
 *
 * Timer class declaration. 
 */

#pragma once

#include <sys/time.h>
#include <iostream>
#include <ostream>
#include <cassert>

#define MICRO_IN_SEC 1000000

using std::cout;
using std::endl;

/**
 * Time management.
 *
 * Class for getting actual time, 
 * starting/stopping timer. 
 */
class Timer
{
  public  :
    Timer();

    Timer(double swTime);

    /**
     * Time in seconds.
     */
    double now();

    /**
     * Set timer interval.
     */
    void setTimer(double swTime);

    /**
     * Starts the timer. 
     */
    void start();
    
    /**
     * Stops the timer. 
     */
    void stop();

    /**
     * Time elapsed since start. 
     */
    double elapsed(); 

    /**
     * Check timer.
     *
     * @return True if over, false otherwise.
     */
    bool timeUp(); 

  private: 
    /**
     * Timeval -> seconds conversion.
     */
    double toSeconds(timeval t);

    /** Start in seconds. */
    double start_;

    /** Stop watch time. */
    double swTime_;

    /** Elapsed time.*/
    double elapsed_;
};
