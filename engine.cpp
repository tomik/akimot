
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
	return "pass";
}


SimplePlayout::SimplePlayout(Board* board)
{
	board_ = board;
	playoutLength_ = 0;
}


void SimplePlayout::playOne()
{
	return;
}


playoutStatus_e SimplePlayout::doPlayout()
{
  while (true) {
		playOne();
		playoutLength_++;

		if ( board_->checkGameEnd() ) 
			return PLAYOUT_OK;

		if ( playoutLength_ > MAX_PLAYOUT_LENGTH ) 
			return PLAYOUT_TOO_LONG;
	}
}

