/**
 * @file engine.h
 *
 * Search engine interface and time management.
 */

#pragma once

#include "utils.h"
#include "timer.h"
#include "board.h"
#include "uct.h"

using std::vector;


//how much time engine has for "clock clicking"
#define CLOCK_CLICK_RESERVE 0.1

/** Default time per move.*/
#define TC_MOVE_DEFAULT (cfg.tcMoveDefault())
//#define TC_MOVE_DEFAULT 1

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
    ~Engine();

    /**
     * Returns initial setup. 
     */
  	string initialSetup(bool isGold) const;

    /**
     * Wrapper around searching method. 
     */
  	void doSearch(const Board*);		

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
    string getBestMove() const;

    /**
     * Interface to get search statistics.
     */
    string getStats() const;

    /**
     * Interface to get additional info ( like search tree ).
     */
    string getAdditionalInfo() const;

    /**
     * Interface to winr ration getter. 
     */
    float getWinRatio() const;

    /**
     * Check whether search should be stop.
     *
     * @return True if Time is up or stop reaeust returned,
     *              otherwise false.
     */
    bool checkSearchStop() const;

  private: 

    /**
     * Takes care of mocking the results from search.
     */
    void mockupSearchResults(const Board* board, Uct* uct[], Uct* masterUct, int resultsNum);

  	static void * searchTreeWrapper(void * searchTreeInfo);		

    TimeManager* timeManager_;

    string bestMove_;
    string stats_;
    string additionalInfo_;
    float winRatio_;

    /**Ponder mode flag.*/
    bool ponder_;

    /**Flag to stop search.*/
    bool stopRequest_;
};

/**
 * Wrapper class for parallel search. 
 */
class SearchStartKit {
  public:
    SearchStartKit(const Board*, Engine*, Uct*);
  
  private:
    /**Board to search on.*/
    const Board*  board_;
    /**On which instance to start the search.*/
    Engine* engineInstance_;
    /**Uct instance to use for the search.*/
    Uct*    uct_; 
    friend class Engine;
};

