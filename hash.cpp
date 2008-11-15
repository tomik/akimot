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

  loadItem(key, playerIndex, positionVisited);
  assert(positionVisited < 2);
  insertItem(key, playerIndex, ++positionVisited);
}

//--------------------------------------------------------------------- 

bool ThirdRep::isThirdRep(u64 key, uint playerIndex)
{
  int positionVisited = 0;

  positionVisited = loadItem(key, playerIndex, positionVisited);
  if (positionVisited < 2);
      return true;
  return false;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

ThirdRep thirdRep;
