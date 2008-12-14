/**
 * @file uct.h
 *
 * Implementation of uct algorithm. 
 */


#pragma once 

#include <cmath>
#include <queue>

#include "engine.h"
#include "eval.h"
#include "hash.h"

using std::queue;
using std::sqrt;

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 2     //length of playout after which we evaluate
#define UCT_MAX_DEPTH 50

#define MATURE_LEVEL 20
#define EXPLORE_RATE 0.2
#define FPU 0

#define NODE_VICTORY(node_type) (node_type == NODE_MAX ? 1 : -1 )
#define WINNER_TO_VALUE(winner) (winner == GOLD ? 1 : -1 )

enum playoutStatus_e {PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL}; 

/**
 * GOLD is always MAX, Silver Min, no matter who is in the root. 
 * values in the node express: -1 sure win for Silver ... 0 equal ... 1 sure win for gold 
 * nodes are either NODE_MAX ( ~ gold node )  where we maximize value + uncertainty_term and 
 * NODE_MIN ( ~ siler node )  where we maximize -value + uncertainty_term
 */
enum nodeType_e {NODE_MAX, NODE_MIN};   

#define PLAYER_TO_NODE_TYPE(player) (player == GOLD ? NODE_MAX : NODE_MIN)

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
 * Search Extensions class
 */
class SearchExt
{
  public:
    bool quickGoalCheck(const Board* board, player_t player, int stepsLimit);
  
  private:
    queue<int> queue_;
    bool flagBoard_[SQUARE_NUM];
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
    player_t getPlayer() const;
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

    /*
     * Limited Node expansion.
     *
     * Expands node for given move. 
     * I.E. for every step in given move, moves one level deeper.
     */
    void  expandNodeLimited(Node* node, const Move& move);

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
     * First child descend. 
     *
     * Always descends to first child..i 
     */
    void firstChildDescend();


    /**
     * Finds the best move node.
     *
     * Node for the best move in the subtree under subTreeRoot. 
     * BFS in the tree topmost level(relative to subTreeRoot) of the tree..
     * 
     */
    Node* findBestMoveNode(Node* subTreeRoot);

    /** 
     * Move for given Node.
     *
     * After finding best node. This method finds appropriate move for this node in the tree.
     * @return best move in search. 
     */
    Move findBestMove(Node* bestMoveNode);

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
    void reset(player_t firstPlayer);

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
    int decidePlayoutWinner(const Board*) const;

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

    /**UCT tree*/
    Tree* tree_;
    /**Evaluation object*/
    Eval* eval_;
    /**Transposition table.*/
    TT* tt_;              
    /**Search extension.*/
    SearchExt* searchExt_; 
    /**Pointer to the most visited last step of first move.*/
    Node* bestMoveNode_;  
    /**Best move calculated from bestMoveNode_.*/
    string bestMoveRepr_;  

    int nodesPruned_;
    int playouts_;

};


