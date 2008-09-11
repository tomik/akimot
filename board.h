#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

class Board
{
	public:
		bool init(char*);
		string initial_setup();
		bool is_empty();
};

#endif
