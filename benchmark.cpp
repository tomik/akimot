#include "benchmark.h"

//---------------------------------------------------------------------
//  section Benchmark
//---------------------------------------------------------------------- 

Benchmark::Benchmark()
{
}

//--------------------------------------------------------------------- 

Benchmark::Benchmark(Board* board, uint playoutCount)
{
	board_ = board;	
	playoutCount_ = playoutCount;
}

//--------------------------------------------------------------------- 

void Benchmark::playoutBenchmark() const
{
		
  clock_t				clockBegin;
  clock_t 			clockEnd;
  float					timeTotal;

  int           evalGold = 0;
  int           evalCount = 0;
  int           initEval = 0;
  Eval          eval;
  
  playoutStatus_e  playoutStatus;
  uint			 winCount[2] = { 0, 0};
	uint			 playoutTooLong = 0;
	uint			 playoutAvgLen = 0;

  clockBegin = clock();
  
  for (uint i = 0 ; i < playoutCount_; i++)  {

    //here we copy the given board
    Board *playBoard = new Board(*board_);

    initEval = eval.evaluate(playBoard);

    SimplePlayout simplePlayout(playBoard);
    playoutStatus = simplePlayout.doPlayout ();
    
    switch (playoutStatus) {
		case PLAYOUT_OK:
      winCount [PLAYER_TO_INDEX(playBoard->getWinner())]++;
      break;
    case PLAYOUT_TOO_LONG:
			playoutTooLong++;
      break;
    case PLAYOUT_EVAL:
      evalGold += eval.evaluate(playBoard);
      evalCount++;
      break;
    }
    
		playoutAvgLen += simplePlayout.getPlayoutLength(); 
  }

  if (evalCount == 0)
    evalCount = 1;
  
  clockEnd = clock();
	timeTotal = float (clockEnd - clockBegin) / CLOCKS_PER_SEC;
  
  log_()
			<< "Performance: " << endl
      << "  " <<playoutCount_ << " playouts" << endl 
      << "  " << timeTotal << " seconds" << endl
      << "  " << int ( float(playoutCount_) / timeTotal) << " pps" << endl;
  
  log_()
			<< "Gold init eval = " << initEval << endl 
			<< "Gold avg eval = " << evalGold/float(evalCount) << endl 
			<< "Playout Eval = " << evalCount << endl
      << "Playout too long = " << playoutTooLong << endl
			<< "Gold wins = " << winCount [PLAYER_TO_INDEX(GOLD)] << endl
      << "Silver wins = " << winCount [PLAYER_TO_INDEX(SILVER)] << endl
      << "P(gold win) = " << float (winCount [PLAYER_TO_INDEX(GOLD)]) / (
														 float (winCount [PLAYER_TO_INDEX(GOLD)] + winCount [PLAYER_TO_INDEX(SILVER)]) ) << endl
			<< "Average playout length = " << float (playoutAvgLen) / float (playoutCount_) << endl;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

