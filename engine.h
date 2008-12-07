/**
 * @file engine.h
 *
 * Search engine interface and time management.
 */

#pragma once

#include "utils.h"
#include "timer.h"
#include "board.h"


//how much time engine has for "clock clicking"
#define CLOCK_CLICK_RESERVE 0.1

/** Default time per move.*/
#define TC_MOVE_DEFAULT 1
#define TC_RESERVE_DEFAULT 60
#define TC_MAX_DEFAULT 20

//this defines size of following enum !!!
#define TIME_CONTROLS_NUM 13

enum timeControl_e {TC_MOVE, TC_RESERVE, TC_PERCENT, TC_MAX, TC_TOTAL, TC_TURNS,
                    TC_TURN_TIME, TC_W_RESERVE, TC_B_RESERVE, TC_W_USED, TC_B_USED, 
                    TC_MOVE_USED, TC_LAST_MOVE_USED};


/**
 * Time management.
 *
 * Accepts time settings and takes care of time management durint the game.
 */
class TimeManager
{
  public:
    TimeManager();
    
    /**
     * Starts clock for given move. 
     */
    void  startClock();

    /**
     * Stops the clock. 
     */
    void stopClock();

    /**
     * Checks the clock.
     *
     * @return True if time is up, false otherwise.
     * TODO: Searcher might provide importance of time addition.
     */
    bool timeUp();

    /**
     *  Seconds elapsed since last startClock().
     */
    double secondsElapsed(); 

    /**
     * Setting time controls from aei.
     *
     * @param tc Time control identifier - used as index into timeControls array.
     * @param value Value for option in seconds (all controal are aligned to secs). 
     */
    void setTimeControl(timeControl_e tc, float value);

    /**
     * Time Control getter.
     */
    float getTimeControl(timeControl_e tc);

    /**
     * Sets time unlimited search. 
     */
    void setNoTimeLimit();
    
    /**
     * Resets temporary settings like noTimeLimit.
     */
    void resetSettings();

  private:
    Timer timer;
    float timeControls_[TIME_CONTROLS_NUM];
    bool noTimeLimit_;
  
};


/**
 * Interface to whole search.
 *
 * Abstract class implementing interface memthods for search
 * and retrieving after-search information.
 */
class Engine
{
  public:
    Engine();
    virtual ~Engine();

    /**
     * Returns initial setup. 
     */
  	string initialSetup(bool isGold) const;

    /**
     * Wrapper around searching method. 
     */
  	void doSearch(const Board*);		

    /**
     * Interface to crucial method implementing search.
     */
    virtual void searchTree(const Board*)=0;

    /**
     * Requests search stop. 
     *
     * Search will stop after current playout is finished.
     */
    void requestSearchStop();

    /**
     * TimeManager getter. 
     */
    TimeManager* timeManager();

    /**
     * Ponder mode setter.
     */
    void setPonder(bool value);

    /**
     * Ponder mode getter. 
     */
    bool getPonder() const;

    /**
     * Move to be performed.
     *
     * @return initialMove_ if it is set. 
     *         best Move from search otherwise. 
     */
    virtual string getBestMove() const;

    /**
     * Interface to best move getter.
     */
    virtual string getBestMoveRepr() const =0;

    /**
     * Interface to get search statistics.
     */
    virtual string getStatistics() const =0;

    /**
     * Interface to winr ration getter. 
     */
    virtual float getWinRatio() const =0;

  protected:

    /**
     * Check whether search should be stop.
     *
     * @return True if Time is up or stop reaeust returned,
     *              otherwise false.
     */
    bool checkSearchStop() const;

    TimeManager* timeManager_;
    /**Flag to stop search.*/
    bool stopRequest_;

  private: 
    string initialMove_;
    /**Ponder mode flag.*/
    bool ponder_;
};

