/**
 * @file engine.h
 *
 * Search engine and time management.
 */

#pragma once

#include "utils.h"
#include "config.h"
#include "board.h"
#include "eval.h"
#include "hash.h"

#include <cmath>

using std::sqrt;

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 2    //length of playout after which we evaluate
#define UCT_MAX_DEPTH 50

#define MATURE_LEVEL  20
#define EXPLORE_RATE 0.2

//how much time engine has for "clock clicking"
#define CLOCK_CLICK_RESERVE 0.3

/** Default time per move.*/
#define TC_MOVE_DEFAULT 3
#define TC_RESERVE_DEFAULT 60
#define TC_MAX_DEFAULT 20

#define TC_DEFAULT_ 5 

//this defines size of following enum !!!
#define TIME_CONTROLS_NUM 13

enum timeControl_e {TC_MOVE, TC_RESERVE, TC_PERCENT, TC_MAX, TC_TOTAL, TC_TURNS,
                    TC_TURN_TIME, TC_W_RESERVE, TC_B_RESERVE, TC_W_USED, TC_B_USED, 
                    TC_MOVE_USED, TC_LAST_MOVE_USED};

enum playoutStatus_e {PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL}; 
enum nodeType_e {NODE_MAX, NODE_MIN};   

#define PLAYER_TO_NODE_TYPE(player) (player == GOLD ? NODE_MAX : NODE_MIN)

// values in the node express: -1 sure win for Silver ... 0 equal ... 1 sure win for gold 
// nodes are either NODE_MAX ( ~ gold node )  where we maximize value + uncertainty_term and 
// NODE_MIN ( ~ siler node )  where we maximize -value + uncertainty_term

/**
 * Node in uct tree. 
 */
class Node
{
  public:
    Node();
    Node(const Step&);

    /**
     * Node expansion.
     *
     * Creates children for every given step. 
     * Steps are expected to be already filtered through 
     * repetition tests, virtual pass test, transposition tables.
     */
    void  expand(const StepArray& stepArray, uint len);

    /**
     * Finds child with highest UCB1 value.
     */
    Node* findUctChild() const;

    /**
     * Finds random child.
     *
     * Mostly for testing.
     */
    Node* findRandomChild() const;

    /**
     * Finds child with most visits. 
     */
    Node* findMostExploredChild() const;

    /**
     * The UCB1 formula.
     *
     * Core of the UCT algorithm. 
     */
    float ucb(float) const;

    void  addChild(Node*);
    void  removeChild(Node*);

    /**
     * Removes itself - TODO still might have issues ! 
     */
    void  remove();

    /**
     * Delete children. 
     */
    void  freeChildren();

    /**
     * Update after playout. 
     *
     * Updates value/visits.
     */
    void  update(float);

    /**
     * Maturity test.
     *
     * Checks whether number of descends through node passed 
     * some given threshold (around number of legal steps from average position) 
     */
    bool  isMature() const;

    bool  hasChildren() const;
    Node* getFather() const;
    Node* getFirstChild() const;
    Node* getSibling() const;
    Step  getStep() const;
    int   getVisits() const;
    float getValue() const;
    nodeType_e getNodeType() const;

    string toString() const; 

    /**
     * Recursively (children as well) to string. 
     */
    string recToString(int) const;

  private:
    float       value_; 
    int         visits_;
    Step        step_;

    Node*       sibling_;
    Node*       firstChild_;  
    Node*       father_;
    nodeType_e  nodeType_;
};


/**
 * Uct tree. 
 */
class Tree
{

  public:
    /**
     * Constructor with player initialization.
     *
     * Root node is created(bottom of history stack) with given player.
     */
    Tree(player_t firstPlayer);   

    /**
     * Destructor.
     *
     * Recursively deletes the tree.
     */
    ~Tree();

    /**
     * UCT-based descend to next level.
     *
     * Crucial method implementing core idea of UCT algorithm - 
     * the descend by UCB1 formula. History is updated - simulating the descend.
     */
    void uctDescend();

    /**
     * Random descend.
     *
     * Mostly for testing purposes.
     */
    void randomDescend();

    /** 
     * Finding the resulting move.
     *
     * In the end of the search, finds the best move (sequence of up to 4 steps)
     * for player who owns the root and returns it's string representation 
     * to be outputted (also trap falls are outputed ... e,g. RC1n RC2n RC3x RB1n)
     */
    string findBestMove(Node* bestFirstNode = NULL, const Board* boardGiven = NULL);

    /**
     * Backpropagation of playout sample.
     */
    void updateHistory(float);

    /**
     * History reset.
     *
     * Performed after updateHistory. Points actual node to the root.
     */
    void historyReset();

    /**
     * Gets tree root(bottom of history stack). 
     */
    Node* root();

    /**
     * Gets the actual node (top of history stack). 
     */
    Node* actNode();

    /**
     * Gets ply in which node lies.
     */
    int getNodeDepth(Node* node);

    /**
     * String representation of the tree.
     */
    string toString();

    /**
     * Path to actNode() to string.  
     *
     * Returns string with steps leading to actNode().
     * @param onlyLastMove if true then only steps from last move are returned.
     */
    string pathToActToString(bool onlyLastMove = false);

  private:
    Node*      history[UCT_MAX_DEPTH];
    uint       historyTop;
    
    Logger     log_;

    Tree();
};


/**
 * Simple random playout.
 *
 * Performs random playout from position given in constructor.
 * Playout returns playout status.
 */
class SimplePlayout
{
	public:
    /**
     * Constructor with board initialization.
     */
		SimplePlayout(Board*, uint, uint);

    /**
     * Performs whole playout. 
     *
     * Consists of repetitive calls to playOne().
     *
     * @return Final playout status.
     */
		playoutStatus_e doPlayout();	

    /**
     * Returns playout length in moves.  
     */
		uint getPlayoutLength();  

  private:
    /**
     * Performs one move of one player.
     *
     * Implements random step play to get the move.
     */
    void playOne();	

    Board*				board_;
    uint					playoutLength_;
    uint          maxPlayoutLength_;
    uint          evalAfterLength_;

		SimplePlayout();
};


/**
 * Uct search. 
 */
class Uct
{
  public:
    Uct(Board*);
    ~Uct();

    /**
     * Does one uct-monte carlo playout. 
     *
     * Crucial method of search. Performs UCT descend as deep as possible and 
     * then starts the "monte carlo" playout through object SimplePlayout.
     */
    void doPlayout();

    /**
     * Get uct search statistics.
     *
     * @param seconds Length of run of the search in seconds.
     */
    string statisticsToString(float seconds);

    /**
     * Finds the best move. 
    void calculateBestMove();
     */

    /**
     * String representation of best move.
     */
    string getBestMove();

    /**
     * Value of best move.
     */
    float getBestMoveValue();

  private:
    /**
     * Decide winner of the game. 
     * 
     * Always returns 1 ( GOLD wins ) or -1 (SILVER wins). If winner is not known ( no winning criteria reached )
     * position is evaluated and biased coin si flipped to estimate the winner 
     */
    int decidePlayoutWinner(Board*, playoutStatus_e);

    /**
     * Filtering steps through Transposition tables.
     *
     * Signature of every step in steps is counted and checked 
     * against transposition table. If found 
     * step is deleted (won't be added to the tree).
     * 
     * @return Number of steps in steps array after update.
     */
    int filterTT(StepArray& steps, uint stepsNum, Board* board);

    /**
     * Updates Transposition tables after nodes were added. 
     *
     * Signature of every node in nodeList is added to the TT. 
     *
     * @param nodes Dynamic List of children (retrieved by getBrother())
     * @return Number of steps in steps array after update.
     */
    void updateTT(Node* nodeList, Board* board);

    Board* board_;
    Tree* tree_;
    Eval* eval_;
    TT* tt_;              //!< Transposition table.
    Logger log_;
    Node* bestMoveNode_;  //!< Pointer to the most visited last step of first move.
    int nodesPruned_;
    int nodesExpanded_;
    int nodesInTheTree_;
    int playouts_;

    Uct();
    
};


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
     * Checks the clock.
     *
     * @return true if there is time, false otherwise.
     * TODO: Searcher might provide importance of time addition.
     */
    bool checkClock();

    /**
     *  Seconds elapsed since last startClock().
     */
    float secondsElapsed(); 

    /**
     * Setting time controls from aei.
     *
     * @param tc Time control identifier - used as index into timeControls array.
     * @param value Value for option in seconds (all controal are aligned to secs). 
     */
    void setTimeControl(timeControl_e tc, int value);

    /**
     * Time Control getter.
     */
    int getTimeControl(timeControl_e tc);

    /**
     * Sets time unlimited search. 
     */
    void setNoTimeLimit();

  private:
    clock_t clockBegin_;
    int timeControls_[TIME_CONTROLS_NUM];
    bool noTimeLimit_;
  
};


/**
 * Interface to whole search. 
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
     * Search loop.  *
     * Crucial method running the search.
     */
  	void doSearch(Board*);		

    /**
     * Requests search stop. 
     *
     * Search will stop after current playout is finished.
     */
    void requestSearchStop();

    /**
     * After search pick the best move.
     */
  	string getBestMove();		
    
    /**
     * TimeManager getter. 
     */
    TimeManager* timeManager();

  private:
    Uct* uct_;
    TimeManager* timeManager_;
    string bestMove_;
    bool stopRequest_;

	  Logger log_;

};
