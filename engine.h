#ifndef ENGINE_H
#define ENGINE_H

#include "utils.h"
#include "board.h"

#include <cmath>


#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 15    //length of playout after which we evaluate
#define UCT_MAX_DEPTH 50


#define MATURE_LEVEL  10
#define GEN_MOVE_PLAYOUTS 15000
#define EXPLORE_RATE 0.2

enum playoutStatus_e { PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL }; 


class Node
{
  float       value_; 
  int         visits_;
  Step        step_;

  Node*       sibling_;
  Node*       firstChild_;

  Logger      log_;

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

    string toString(); 
    string recToString(int);
};

class Tree
{
  Node*      history[UCT_MAX_DEPTH];
  uint       historyTop;
  
  Logger     log_;

  public:
    Tree();
    ~Tree();
    void historyReset();
    void uctDescend();
    void updateHistory(float);
    Node* actNode();
    Node* root();
    string getBestMove();
    string toString();
};


class Uct
{
  Board* board_;
  Tree tree_;
  public:
    Uct();
    Uct(Board*);
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
	Logger logger_;
public:
	string doSearch(Board*);		
	string initialSetup(bool); 
};
#endif
