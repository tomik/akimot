#ifndef UTILS_H
#define UTILS_H

#include <cassert>

#include <list>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

class Logger
{
	public:
		ostream& operator()(unsigned int = 0);
		//ostream& log(int=0 );
}; 

#endif
