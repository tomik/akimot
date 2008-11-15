#include <cxxtest/TestSuite.h>
#include "board.h"
#include "hash.h"

#define TEST_DIR "./test"
#define INIT_TEST_DIR "./test/init/"
#define INIT_TEST_LIST "./test/init/tests.txt"
#define HASH_TABLE_INSERTS 100

class MyTestSuite : public CxxTest::TestSuite 
{
  public:
    /**
     * Board init test.
     *
     * Inits two boards from game record file and position file
     * and tests their equality.
     */
     void testBoardInit(void)
     {
        Board board1;
        Board board2;
        fstream f;
        string fnPosition;
        string fnRecord;
        stringstream ss;
        string line;

        f.open(INIT_TEST_LIST, fstream::in);

        while (f.good()){
          getline(f, line);
          ss.clear();
          ss.str(line);
          ss >> fnPosition;
          ss >> fnRecord;
          fnPosition = INIT_TEST_DIR + fnPosition;
          fnRecord = INIT_TEST_DIR + fnRecord;

          board1.initFromPosition(fnPosition.c_str());
          board2.initFromRecord(fnRecord.c_str());

          TS_ASSERT_EQUALS( board1.getSignature(), board2.getSignature());
        }
     }

    /**
     * Hash table insert/is member/load test.
     */
    void testHashTable(void)
    {
      HashTable<int> hashTable; 
      u64 keys[HASH_TABLE_INSERTS];

      //store some random values into hash table
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        keys[i] = getRandomU64();
        hashTable.insertItem(keys[i], i);
      }

      //positive-lookup test
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        int item;
        TS_ASSERT_EQUALS(hashTable.isMember(keys[i]), true); 
        if (! hashTable.loadItem(keys[i], item))
          TS_ASSERT_EQUALS(true, false); 
      }
       
      //negative-lookup test
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        int key = getRandomU64();
        int item;
        TS_ASSERT_EQUALS(hashTable.isMember(key), false); 
        if (hashTable.loadItem(key, item))
          TS_ASSERT_EQUALS(true, false); 
      }
    }
};
