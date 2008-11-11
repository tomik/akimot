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

ThirdRep::ThirdRep()
{
  table.clear();
  playerSignature_[0] = getRandomU64();
  playerSignature_[0] = getRandomU64(); 
}

//--------------------------------------------------------------------- 

void ThirdRep::update(u64 key, uint playerIndex)
{
  assert(playerIndex == 0 || playerIndex == 1);
  key ^= playerSignature_[playerIndex];

  PositionMap::iterator iter;
  iter = table.find(key);
  if (iter != table.end() ) {
    assert(iter->second < 2 );
    iter->second++;
  }
  else 
    table.insert(PositionPair(key,0));
}

//--------------------------------------------------------------------- 

bool ThirdRep::check(u64 key, uint playerIndex)
{
  assert(playerIndex == 0 || playerIndex == 1);
  key ^= playerSignature_[playerIndex];

  PositionMap::iterator iter;
  iter = table.find(key);
  if (iter != table.end()) 
    if (iter->second >= 2) 
      return true;
  return false;
}


ThirdRep thirdRep;

//--------------------------------------------------------------------- 
// section SearchTT 
//--------------------------------------------------------------------- 

SearchTT::SearchTT()
{
  table.clear();
  playerSignature_[0] = getRandomU64();
  playerSignature_[0] = getRandomU64(); 
}

//--------------------------------------------------------------------- 

void SearchTT::saveItem(u64 key, uint playerIndex, Node* node)
{
  assert(playerIndex == 0 || playerIndex == 1);
  assert(node != NULL);
  key ^= playerSignature_[playerIndex];

  TT::iterator iter;
  table[key] = node;
}

//--------------------------------------------------------------------- 

Node* SearchTT::loadItem(u64 key, uint playerIndex)
{
  assert(playerIndex == 0 || playerIndex == 1);
  key ^= playerSignature_[playerIndex];

  TT::iterator iter;
  iter = table.find(key);
  if (iter != table.end()) 
    return iter->second;
  return NULL;
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 





