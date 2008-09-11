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
