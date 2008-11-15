#ifndef ENGINE_H
#define ENGINE_H

#include "utils.h"
#include "config.h"
#include "board.h"
#include "eval.h"
#include "hash.h"

#include <cmath>

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 8    //length of playout after which we evaluate
#define UCT_MAX_DEPTH 50

#define MATURE_LEVEL  10
#define EXPLORE_RATE 0.2

enum playoutStatus_e {PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL}; 
enum nodeType_e {NODE_MAX, NODE_MIN};   

// values in the node express: -1 sure win for Silver ... 0 equal ... 1 sure win for gold 
// nodes are either NODE_MAX ( ~ gold node )  where we maximize value + uncertainty_term and 
// NODE_MIN ( ~ siler node )  where we maximize -value + uncertainty_term

class Node
{
  private:
    float       value_; 
    int         visits_;
    Step        step_;

    Node*       sibling_;
    Node*       firstChild_;  
    Node*       father_;
    nodeType_e  nodeType_;

  public:
    Node();
    Node(const Step&);

    void  expand(const StepArray& stepArray, uint len);
    Node* findUctChild() const;
    Node* findMostExploredChild() const;

    float ucb(float) const;

    void  addChild(Node*);
    void  removeChild(Node*);
    void  remove();
    void  freeChildren();
    void  update(float);

    bool  isMature() const;
    bool  hasChildren() const;
    Node* getFather() const;
    Node* getFirstChild() const;
    Node* getSibling() const;
    Step  getStep() const;
    int   getVisits() const;
    nodeType_e getNodeType() const;

    string toString() const; 
    string recToString(int) const;
};


class Tree
{
  private:
    Node*      history[UCT_MAX_DEPTH];
    uint       historyTop;
    
    Logger     log_;

  public:
    Tree();
    Tree(player_t);   
    ~Tree();

    /**
     * UCT-based descend to next level.
     *
     * Crucial method implementing core idea of UCT algorithm - 
     * the descend by UCB1 formula. History is updated - simulating the descend.
     */
    void uctDescend();

    /** 
     * Finding the resulting move.
     *
     * In the end of the search, finds the best move (sequence of up to 4 steps)
     * for player who owns the root and returns it's string representation 
     * to be outputted (also trap falls are outputed ... e,g. RC1n RC2n RC3x RB1n)
     */
    string findBestMove(Node* bestFirstNode = NULL, const Board* boardGiven = NULL);

    void updateHistory(float);
    void historyReset();

    Node* root();
    Node* actNode();

    /**
     * Gets ply in which node lies.
     */
    int getNodeDepth(Node* node);

    string toString();
    string pathToActToString(bool onlyLastMove = false);
};

class SimplePlayout
{
  private:
    Board*				board_;
    uint					playoutLength_;

	public:
		SimplePlayout();
		SimplePlayout(Board*);

		playoutStatus_e doPlayout();	
    void playOne();	

		uint getPlayoutLength();  
};

class Uct
{
  private:
    Board* board_;
    Tree* tree_;
    Eval* eval_;
    TT* tt_;              //!< Transposition table.
    Logger log_;
    Node* bestMoveNode_;  //pointer to the most visited last step of first move
    int nodesPruned_;
    int nodesExpanded_;
    int nodesInTheTree_;

  public:
    Uct();
    Uct(Board*);
    ~Uct();

    /**
     * Generates uct-search result move.
     *
     * Top level function. Time control wrapper around 
     * loop of doPlayout(s). 
     * @return String representation of move to play.
     */
    string generateMove();

    /**
     * Does one uct-monte carlo playout. 
     *
     * Crucial method of search. Performs UCT descend as deep as possible and 
     * then starts the "monte carlo" playout through object SimplePlayout.
     */
    void doPlayout();

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
    
};

class Engine
{
  private:
	  Logger log_;

  public:
  	string initialSetup(bool); 
  	string doSearch(Board*);		
};
#endif
