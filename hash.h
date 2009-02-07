#pragma once

#include "utils.h"
#include <map>

using std::map;

typedef pair<u64, int>  PositionPair;
typedef map<u64, int>   PositionMap; 

//random initialization for hash-like structures
u64 getRandomU64();

//corressponds to maximum depth of UCT tree (in levels ~ moves) 
#define MAX_LEVELS 50

template<typename T> class HashTable
{
  protected:
    map <u64, T> table; 

  public:
    
    HashTable()
    {
      table.clear();
    }

    //--------------------------------------------------------------------- 
    
    void clear()
    {
      table.clear();
    }

    //--------------------------------------------------------------------- 
    
    bool isEmpty()
    {
      return table.empty();
    }

    //--------------------------------------------------------------------- 
    
    /**
     * Membership test.
     *
     * @return true if item is in the table, otherwise returns false
     */
    bool hasItem(u64 key)
    {
      if (table.find(key) != table.end()) 
        return true;
      return false;
    }

    /**
     * Inserts key->item into table.
     */
    void insertItem(u64 key, T item)
    {
      table[key] = item;
    }

    /**
     * Loads key->item from table.
     *
     * @param item Reference to variable into which item will be loaded.
     * @return true if there is such a key, false otherwise 
     */
    bool loadItem(u64 key, T& item)
    {
      if (hasItem(key)){
        item = HashTable<T>::table[key];
        return true;
      }
      return false;
    }
};

template<typename T> class HashTableBoard : public HashTable<T>
{
  public:
    HashTableBoard(): HashTable<T>()
    {
      playerSignature_[0] = getRandomU64();
      playerSignature_[1] = getRandomU64(); 
      for (int i = 0; i < MAX_LEVELS; i++){
        levelSignature_[i] = getRandomU64();
      }
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::hasItem.
     */
    bool hasItem(u64 key, uint playerIndex, uint level=0)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];
      key ^= levelSignature_[level % MAX_LEVELS]; 
      return HashTable<T>::hasItem(key);
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::insertItem.
     */
    void insertItem(u64 key, uint playerIndex, T item, uint level=0)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];
      key ^= levelSignature_[level % MAX_LEVELS]; 
      HashTable<T>::insertItem(key, item);
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::loadItem.
                    node->getLevel(),
     */
    bool loadItem(u64 key, uint playerIndex, T& item, uint level=0)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];
      key ^= levelSignature_[level % MAX_LEVELS]; 

      return HashTable<T>::loadItem(key, item);
    }

  protected:
    u64 playerSignature_[2];
    u64 levelSignature_[MAX_LEVELS];

};

/**
 * Checking third repetitions.
 * 
 * Stores key(position, playerIndex) ---> number of position repetitions so far 
 */
class ThirdRep: public HashTableBoard<int>
{
	public:
    ThirdRep(): HashTableBoard<int>()
    {
      playerSignature_[0] = getRandomU64();
      playerSignature_[1] = getRandomU64(); 
    }

    void print() { 
      cerr << playerSignature_[0] << " | " << playerSignature_[1] << "|" << isEmpty() << endl; 
    }
    /**
     * Updates number of repetitions (+1).
     *
     * Number of repetitions for giben key,playerIndex must be < 2 (asserts)!
     */
    void  update(u64 key, uint playerIndex );

    /**
     * Checks whether position is third repetition.
     */
    bool  isThirdRep(u64 key, uint playerIndex ); 
  protected:
    u64 playerSignature_[2];
}; 

extern ThirdRep thirdRep;

//forward declaration
class Node; 

//typedef map<u64, Node*> TTpair;
//typedef map<u64, Node*> TTmap;

/**
 * transposition table.
 *
 * Implements mapping: 
 * "key(position signature, player, move - tree depth ) ---> 
 *    pointer to the node in the tree"
 */
typedef HashTableBoard<Node *> TT;

typedef HashTable<float> EvalTT;
