
#include "utils.h"

ostream& Logger::operator()(unsigned int level) const{
	return cerr;
}


template <class T> MyArray<T>::MyArray()
{
  lenValidSpots = 0;
  lenEmptySpots = 0;
       lenSpots = 0;
}


template <class T> void MyArray<T>::add(T elem)
{
  assert(MY_ARRAY_MAX_LEN > lenSpots && lenSpost >= lenValidSpots && lenSpots >= lenEmptySpots);
  if ( lenEmptySpots ) {  //use empty spot in spots array
    validSpots[lenValidSpots++] = emptySpots[lenEmptySpots];
    spots[emptySpots[lenEmptySpots]] = elem;
    lenEmptySpots--;
  }else{    //prolong spots array
    validSpots[lenValidSpots++] = lenSpots;
    spots[lenSpots] = elem;
    lenSpots++;
  }
  return; 
}


template <class T> void MyArray<T>::del(uint index)
{
  emptySpots[lenEmptySpots++] = validSpots[index];   //store for future use
  validSpots[index] = validSpots[lenValidSpots--];   //replace
  return; 
}

template <class T> uint MyArray<T>::getElemCount()
{
  return lenValidSpots;
}

template <class T> T MyArray<T>::pop(uint index)
{
  assert( index < lenValidSpots );

  T elem = spots[validSpots[index]];
  del(index);
  return elem;

}
