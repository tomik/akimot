#include "hash.h"


u64 getRandomU64() 
{
  return  (((u64) random()) << 40) ^ 
          (((u64) random()) << 20) ^ 
          ((u64) random() );
}

//--------------------------------------------------------------------- 
//  section ThirdRep
//--------------------------------------------------------------------- 

void ThirdRep::update(u64 key, uint playerIndex)
{
  int positionVisited = 0;
  bool found;

  found = loadItem(key, playerIndex, positionVisited);
  assert(!found || positionVisited < 2);
  insertItem(key, playerIndex, ++positionVisited);
}

//--------------------------------------------------------------------- 

bool ThirdRep::isThirdRep(u64 key, uint playerIndex)
{
  int positionVisited = 0;
  bool found = false;

  found = loadItem(key, playerIndex, positionVisited);
  if (! found || positionVisited < 2);
    return false;
  return true;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

ThirdRep thirdRep;
