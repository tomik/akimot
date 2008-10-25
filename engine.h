#ifndef ENGINE_H
#define ENGINE_H

#include "utils.h"
#include "config.h"
#include "board.h"
#include "eval.h"


#include <cmath>


#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 15    //length of playout after which we evaluate
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
  float       value_; 
  int         visits_;
  Step        step_;

  Node*       sibling_;
  Node*       firstChild_;  
  nodeType_e  nodeType_;
  Node*       father_;

  public:
    Node();
    Node(const Step&);
    void  addChild(Node*);
    void  removeChild(Node*);
    void  expand(const StepArray& stepArray, uint len);
    float ucb(float);
    Node* getUctChild();
    Node* getMostExploredChild(); 
    Step  getStep();
    void  freeChildren();
    void  update(float);
    bool  isMature();
    bool  hasChildren();
    Node* getFather();

    int   getVisits();
    nodeType_e getNodeType();

    string toString(); 
    string recToString(int);
};


class Tree
{
  Node*      history[UCT_MAX_DEPTH];
  uint       historyTop;
  
  Logger     log_;

  public:
    Tree(){};
    Tree(player_t);   
    ~Tree();
    void historyReset();
    void uctDescend();
    void updateHistory(float);
    Node* actNode();
    Node* root();
    string getBestMove(Node* bestFirstNode = NULL);
    string toString();
    string pathToActToString(bool onlyLastMove = false);
};


class Uct
{
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
    int decidePlayoutWinner(Board*, playoutStatus_e);
    void doPlayout();
    string generateMove();
};


class SimplePlayout
{
	Board*				board_;
	uint					playoutLength_;
	void playOne();	
	public:
		SimplePlayout(Board*);
		playoutStatus_e doPlayout();	
		uint getPlayoutLength();  
};


class Engine
{
	Logger log_;
public:
	string doSearch(Board*);		
	string initialSetup(bool); 
};
#endif
