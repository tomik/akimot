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
    Node* findUctChild();
    Node* findMostExploredChild(); 

    float ucb(float);

    void  addChild(Node*);
    void  removeChild(Node*);
    void  freeChildren();
    void  update(float);

    bool  isMature();
    bool  hasChildren();
    Node* getFather();
    Step  getStep();
    int   getVisits();
    nodeType_e getNodeType();

    string toString(); 
    string recToString(int);
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
    Logger log_;
    Node* bestMoveNode_;  //pointer to the most visited last step of first move
    int nodesExpanded_;
    int nodesInTheTree_;

  public:
    Uct();
    Uct(Board*);
    ~Uct();

    string generateMove();
    void doPlayout();

    /**
     * Decide winner of the game. 
     * 
     * Always returns 1 ( GOLD wins ) or -1 (SILVER wins). If winner is not known ( no winning criteria reached )
     * position is evaluated and biased coin si flipped to estimate the winner 
     */
    int decidePlayoutWinner(Board*, playoutStatus_e);
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
