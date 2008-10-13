#ifndef ENGINE_H
#define ENGINE_H

#include "utils.h"
#include "board.h"

class Engine
{
	Logger logger_;
public:
	string doSearch(Board&);		
	string initialSetup(bool); 
};

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define EVAL_AFTER_LENGTH 15    //length of playout after which we evaluate
#define UCT_MAX_DEPTH 30

enum playoutStatus_e { PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL }; 


class Node
{
  int         value_; 
  int         visits_;
  Step        step_;

  Node*       sibling_;
  Node*       firstChild_;

  Logger      log_;

  public:
    Node();
    Node(Step&);
    void  addChild(Node*);
    void  removeChild(Node*);
    Node* getUctChild();
    Node* getMostExploredChild(); 
    void  freeChildren();
    void  update(float);

    string toString(); 
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
    string toString();
};


/*
class Uct
{
  Tree tree;
  public:
    void treePlayout();
}
*/


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

#endif
