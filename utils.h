#ifndef UTILS_H
#define UTILS_H

#include <cassert>

#include <list>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <bitset>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <cstring>

//different levels of debug 1 -- lovest, 3 -- highest
#define DEBUG_1				
//#define DEBUG_2
//#define DEBUG_3

using namespace std;

class Logger
{
	public:
		ostream& operator()(unsigned int = 0) const;
		//ostream& log(int=0 );
}; 

#endif
