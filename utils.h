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

//different levels of debug 1 -- low, 2 -- mediocre,  3 -- high

//#define DEBUG_1	    	
//#define DEBUG_2
//#define DEBUG_3

typedef unsigned long long u64;

using namespace std;

enum logLevel_e { LOG_INFO, LOG_DEBUG1, LOG_DEBUG2, LOG_DEBUG3  };

class Logger
{
  string owner_;   //which class is logger part of i.e. board, benchmark, engine 

	public:
    Logger();
    Logger(string);
		ostream& operator()(unsigned int = 0) const;
}; 


#endif
