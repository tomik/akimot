
#include "engine.h"


string Engine::initialSetup(bool isGold)
{
	if (isGold)
		return "Ra1 Rb1 Rc1 Rd1 Re1 Rf1 Rg1 Rh1 Ha2 Db2 Cc2 Md2 Ee2 Cf2 Dg2 Hh2\n";
	else
		return "ra8 rb8 rc8 rd8 re8 rf8 rg8 rh8 ha7 db7 cc7 ed7 me7 cf7 dg7 hh7\n";
}


string Engine::doSearch(Board&) 
{ 
	return "here will be some search soon";
}


SimplePlayout::SimplePlayout(Board* board)
{
	board_ = board;
	playoutLength_ = 0;
}


void SimplePlayout::playOne()
{
	Step step;
	do {
		step = board_->getRandomStep();
		#ifdef DEBUG_2
			board_->dumpAllSteps();
			board_->dump();
		#endif
		board_->makeStep(step);
	}
	while ( board_->getStepCount() < 4 && step.pieceMoved()); 
	board_->commitMove();
	return;
}


playoutStatus_e SimplePlayout::doPlayout()
{
  while (true) {
		playOne();
		playoutLength_++;

		if ( board_->getWinner() != EMPTY ) //somebody won
			return PLAYOUT_OK;

		if ( playoutLength_ > MAX_PLAYOUT_LENGTH ) 
			return PLAYOUT_TOO_LONG;
	}
}

uint SimplePlayout::getPlayoutLength()
{
	return playoutLength_;
}


Benchmark::Benchmark()
{
}


Benchmark::Benchmark(Board* board, uint playoutCount)
{
	board_ = board;	
	playoutCount_ = playoutCount;
}


void Benchmark::doBenchmark()
{
		
  clock_t				clockBegin;
  clock_t 			clockEnd;
  float					timeTotal;
	Board*				playBoard;
  
  playoutStatus_e  playoutStatus;
  uint			 winCount[2] = { 0, 0};
	uint			 playoutTooLong = 0;
	uint			 playoutAvgLen = 0;
  
  clockBegin = clock();
  
  for (uint i = 0 ; i < playoutCount_; i++)  {
		playBoard = board_;
    SimplePlayout simplePlayout(playBoard);
    playoutStatus = simplePlayout.doPlayout ();
    
    switch (playoutStatus) {
		case PLAYOUT_OK:
      winCount [PLAYER_TO_INDEX(playBoard->getWinner())]++;
      break;
    case PLAYOUT_TOO_LONG:
			playoutTooLong++;
      break;
    }
		playoutAvgLen += simplePlayout.getPlayoutLength(); 
  }
  
  clockEnd = clock();
  
//  out << "Initial board:" << endl;
//  out << board_->dump (); todo add to_string method to board
  
	timeTotal = float (clockEnd - clockBegin) / CLOCKS_PER_SEC;
  
  log_()
			<< "Performance: " << endl
      << "  " <<playoutCount_ << " playouts" << endl 
      << "  " << timeTotal << " seconds" << endl
      << "  " << int ( float(playoutCount_) / timeTotal) << " pps" << endl;
  
  log_()
			<< "Gold wins = " << winCount [PLAYER_TO_INDEX(GOLD)] << endl
      << "Silver wins = " << winCount [PLAYER_TO_INDEX(SILVER)] << endl
      << "Playout too long = " << playoutTooLong << endl
      << "P(gold win) = " << float (winCount [PLAYER_TO_INDEX(GOLD)]) / (
														 float (winCount [PLAYER_TO_INDEX(GOLD)] + winCount [PLAYER_TO_INDEX(SILVER)]) ) << endl
			<< "Avererage playout length = " << float (playoutAvgLen) / float (playoutCount_) << endl;
}


