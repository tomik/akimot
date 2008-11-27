#include "benchmark.h"

//---------------------------------------------------------------------
//  section Benchmark
//---------------------------------------------------------------------- 

Benchmark::Benchmark()
{
  board_ = new Board();
  board_->initNewGame(); 
  //TODO REPLACE LOADING FROM FILE WITH FUNCTION  
  board_->initFromPosition(START_POS_PATH);
  playoutCount_ = PLAYOUT_NUM;

}

//--------------------------------------------------------------------- 

Benchmark::Benchmark(Board* board, uint playoutCount)
{
	board_ = board;	
	playoutCount_ = playoutCount;
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkEval() const
{
;
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkPlayout() const
{
		
  clock_t				clockBegin;
  float					timeTotal;

  playoutStatus_e  playoutStatus;
	uint			 playoutAvgLen = 0;

  clockBegin = clock();

  for (uint i = 0 ; i < playoutCount_; i++)  {

    //here we copy the given board
    Board *playBoard = new Board(*board_);

    SimplePlayout simplePlayout(playBoard, 24, 0);
    playoutStatus = simplePlayout.doPlayout ();

		playoutAvgLen += simplePlayout.getPlayoutLength(); 
  }

	rimeTotal = float (clock() - clockBegin) / CLOCKS_PER_SEC;
  

  logRaw("Performance: \n  %d playouts\n  %3.2f seconds\n  %d pps\n  %d average playout length\n", 
            playoutCount_, timeTotal, int ( float(playoutCount_) / timeTotal),int(playoutAvgLen/ float (playoutCount_)));
  
}

//--------------------------------------------------------------------- 

void Benchmark::benchmarkUct() const
{
}
//--------------------------------------------------------------------- 

void Benchmark::benchmarkAll() const
{
  benchmarkEval();
  benchmarkPlayout();
  benchmarkUct();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

