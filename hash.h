#ifndef HASH_H
#define HASH_H

#include "utils.h"
#include <map>

typedef pair<u64, int>  PositionPair;
typedef map<u64, int>   PositionMap; 


//random initialization for hash-like structures
u64 getRandomU64();

/**
 * Checking third repetitions.
 * 
 * Stores signatures of given positions in hash structure ( so far ordinary Map )  
 */
class ThirdRep
{
  private:
    PositionMap table;
    u64 playerSignature_[2];
    Logger log_;

	public:
    ThirdRep();
    void  update(u64 key, uint playerIndex );
    bool  check(u64 key, uint playerIndex ); 
}; 

//forward declaration
class Node; 

typedef map<u64, Node*> TTpair;
typedef map<u64, Node*> TT;

/**
 * Search transposition table.
 *
 * Implements mapping: 
 * "key(position signature, player, move - tree depth ) ---> 
 *    pointer to the node in the tree"
 */
class SearchTT
{
  private:
    TT  table;

  public: 
    SearchTT();
    u64 playerSignature_[2];

    /**
     * Store key in the table. 
     */
    void saveItem(u64 key, uint playerIndex, Node* node);

    /**
     * Loads item from table.
     *
     * @return pointer to the corresponding node in the tree OR null
     */
    Node* loadItem(u64 key, uint playerIndex);

};

#endif

extern ThirdRep thirdRep;
