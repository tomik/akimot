#pragma once

#include "utils.h"
#include <map>

using std::map;

typedef pair<u64, int>  PositionPair;
typedef map<u64, int>   PositionMap; 

//random initialization for hash-like structures
u64 getRandomU64();

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
  protected:
    u64 playerSignature_[2];
    Logger log_;

  public:
    HashTableBoard(): HashTable<T>()
    {
      playerSignature_[0] = getRandomU64();
      playerSignature_[1] = getRandomU64(); 
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::hasItem.
     */
    bool hasItem(u64 key, uint playerIndex)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];
      return HashTable<T>::hasItem(key);
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::insertItem.
     */
    void insertItem(u64 key, uint playerIndex, T item)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];
      HashTable<T>::insertItem(key, item);
    }

    //--------------------------------------------------------------------- 

    /**
     * Wrapper around HashTable::loadItem.
     */
    bool loadItem(u64 key, uint playerIndex, T& item)
    {
      assert(playerIndex == 0 || playerIndex == 1);
      key ^= playerSignature_[playerIndex];

      return HashTable<T>::loadItem(key, item);
    }

};

/**
 * Checking third repetitions.
 * 
 * Stores key(position, playerIndex) ---> number of position repetitions so far 
 */
class ThirdRep: public HashTableBoard<int>
{
	public:
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
