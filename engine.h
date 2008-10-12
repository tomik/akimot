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

enum playoutStatus_e { PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL }; 


class Node
{
  int         value_; 
  int         visits_;
  Step        step_;

  Node*       sibling_;
  Node*       firstChild_;

  Logger      logger_;

  public:
    Node();
    Node(Step&);
    void addChild(Node*);
    void removeChild(Node*);
    Node* getUctChild();
    Node* getMostExploredChild(); 

    string toString(); 
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

#endif
