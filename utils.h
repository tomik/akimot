#ifndef UTILS_H
#define UTILS_H

//#define NDEBUG //switchis off assert ! 

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

//#define DEBUG_1				
//#define DEBUG_2
//#define DEBUG_3

using namespace std;

class Logger
{
	public:
		ostream& operator()(unsigned int = 0) const;
		//ostream& log(int=0 );
}; 

#define MY_ARRAY_MAX_LEN    150     //DIRTY - add as a constructor parameter

template <class T> 
class MyArray
/*clever  array with fixed size, const time of indexation, deleting */
{ 
  private:
    uint  validSpots[MY_ARRAY_MAX_LEN];       //indexes to spots
    uint  emptySpots[MY_ARRAY_MAX_LEN];       //indexes to spots
    T          spots[MY_ARRAY_MAX_LEN];       //data

    uint lenValidSpots;
    uint lenEmptySpots;
    uint lenSpots;

  public: 

    MyArray()
    {
      lenValidSpots = 0;
      lenEmptySpots = 0;
      lenSpots = 0;
    }

    void add(T elem)
    {
      assert(MY_ARRAY_MAX_LEN > lenSpots && lenSpots >= lenValidSpots && lenSpots >= lenEmptySpots);
      if ( lenEmptySpots ) {  //use empty spot in spots array
        validSpots[lenValidSpots++] = emptySpots[--lenEmptySpots];
        spots[emptySpots[lenEmptySpots]] = elem;
      }else{    //prolong spots array
        validSpots[lenValidSpots++] = lenSpots;
        spots[lenSpots++] = elem;
      }
      return; 
    }

    void del(uint index)
    {
      assert(index >= 0 && index < lenValidSpots && lenValidSpots > 0);

      emptySpots[lenEmptySpots++] = validSpots[index];   //store for future use
      validSpots[index] = validSpots[--lenValidSpots];   //replace
      return; 
    }

     
    uint getElemCount()
    {
      return lenValidSpots;
    }

     
    T getElem(uint index)
    {
      assert( index >= 0 && index < lenValidSpots );
      return spots[validSpots[index]];
    }

 };

#endif
