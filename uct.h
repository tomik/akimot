/**
 * @file uct.h
 *
 * Implementation of uct algorithm. 
 */


#pragma once 

#include <cmath>

#include "engine.h"
#include "eval.h"
#include "hash.h"

using std::sqrt;

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 4     //length of playout after which we evaluate
#define UCT_MAX_DEPTH 50

#define MATURE_LEVEL 20
#define EXPLORE_RATE 0.2
#define FPU 0

enum playoutStatus_e {PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL}; 
enum nodeType_e {NODE_MAX, NODE_MIN};   

#define PLAYER_TO_NODE_TYPE(player) (player == GOLD ? NODE_MAX : NODE_MIN)

// values in the node express: -1 sure win for Silver ... 0 equal ... 1 sure win for gold 
// nodes are either NODE_MAX ( ~ gold node )  where we maximize value + uncertainty_term and 
// NODE_MIN ( ~ siler node )  where we maximize -value + uncertainty_term


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
 * Node in uct tree. 
 */
class Node
{
  public:
    Node();
    Node(const Step&);

    /**
     * Finds child with highest UCB1 value.
     */
    Node* findUctChild();

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

    /**
     * Removes one child. 
     */
    void removeChild(Node* child);

    /**
     * Delete children. 
     */
    void  removeChildrenRec();

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
    bool  hasOneChild() const;
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

    /** Best son cached to be used immediately for UCB descend*/
    Node*       bestCached_; 
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
     * Simple constructor.
     *
     * Sets the root to NULL. 
     */
    Tree();   

    /**
     * Constructor which creates root node. 
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
     * Tree reset for reuse. 
     *
     * Old tree is dropped and new root created.
     */
    void reset(player_t firstPlayer);

    /**
     * Node expansion.
     *
     * Creates children for every given step. 
     * Steps are expected to be already filtered through 
     * repetition tests, virtual pass test, transposition tables.
     */
    void  expandNode(Node* node, const StepArray& stepArray, uint len);

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
    string findBestMoveRepr(Node* bestFirstNode, const Board* boardGiven);

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
     * Cascade remove.
     *
     * Removes and goes up the tree and keeps 
     * removing till it is possible. 
     *
     * @return Number of moves removed.
     */
    int  removeNodeCascade(Node* node);

    /**
     * Gets ply in which node lies.
     */
    int getNodeDepth(Node* node);

    /**
     * Nodes num getter. 
     */
    int getNodesNum();

    /**
     * Nodes expanded getter.
     */
    int getNodesExpandedNum();

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
    int   nodesExpandedNum_;
    int   nodesNum_;
    
};

/**
 * Uct search. 
 */
class Uct:public Engine
{
  public:
    Uct();
    ~Uct();

    /**
     * Update before every search.
     */
    void reset(const Board*);

    /**
     * Crucial method implementing search.
     *
     * Runs the doPlayout loop.
     */
    void searchTree(const Board*);

    /**
     * Does one uct-monte carlo playout. 
     *
     * Crucial method of search. Performs UCT descend as deep as possible and 
     * then starts the "monte carlo" playout through object SimplePlayout.
     */
    void doPlayout(const Board* board);

    /**
     * Get uct search statistics.
     */
    string getStats() const;

    /**
     * String representation of best move.
     */
    string getBestMoveRepr() const;

    /**
     * Value of best move.
     */
    float getBestMoveValue() const;

    /**
     * Estimated win ratio.
     *
     * Win ratio(percentage) ~ chance of winning for player to move.
     */
    float getWinRatio() const;

  private:
    /**
     * Decide winner of the game. 
     * @return Returns 1 ( GOLD wins ) or -1 (SILVER wins). 
     *         If winner is not known ( no winning criteria reached ), position
     *         is evaluated and biased coin si flipped to estimate the winner
     */
    int decidePlayoutWinner(const Board*, playoutStatus_e) const;

    /**
     * Filtering steps through Transposition tables.
     *
     * Signature of every step in steps is counted and checked 
     * against transposition table. If found 
     * step is deleted (won't be added to the tree).
     * 
     * @return Number of steps in steps array after update.
     */
    int filterTT(StepArray& steps, uint stepsNum, const Board* board);

    /**
     * Updates Transposition tables after nodes were added. 
     *
     * Signature of every node in nodeList is added to the TT. 
     *
     * @param nodes Dynamic List of children (retrieved by getBrother())
     * @return Number of steps in steps array after update.
     */
    void updateTT(Node* nodeList, const Board* board);

    //Board* board_;
    Tree* tree_;
    Eval* eval_;
    /**Transposition table.*/
    TT* tt_;              
    /**Pointer to the most visited last step of first move.*/
    Node* bestMoveNode_;  
    /**Best move string representation.*/
    string bestMoveRepr_;  

    int nodesPruned_;
    int playouts_;

};


