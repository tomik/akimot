#ifndef UTILS_H
#define UTILS_H

#include <cassert>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

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
