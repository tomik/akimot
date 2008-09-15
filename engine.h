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

#endif

#define MAX_PLAYOUT_LENGTH 60  //these are "moves" ( i.e. 4 steps ) 

enum playoutStatus_e { PLAYOUT_OK, PLAYOUT_TOO_LONG }; 

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

class Benchmark
{
	const Board* board_;
	uint playoutCount_;
	Logger log_;
	public: 
		Benchmark();
		Benchmark(Board*,uint);
		void doBenchmark();
};
