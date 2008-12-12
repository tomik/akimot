#include <cxxtest/TestSuite.h>
#include "utils.h"
#include "board.h"
#include "hash.h"
#include "uct.h"

#define TEST_DIR "./test"
#define INIT_TEST_DIR "./test/init/"
#define INIT_TEST_LIST "./test/init/list.txt"
#define RABBITS_TEST_DIR "./test/rabbits/"
#define RABBITS_TEST_LIST "./test/rabbits/list.txt"
#define HASH_TABLE_INSERTS 100

//mature level is defined in uct.h
#define UCT_TREE_TEST_MATURE 1 
#define UCT_TREE_NODES 100000
#define UCT_TREE_NODES_DELETE 100
#define UCT_TREE_MAX_CHILDREN 30

class FileRead;


class DebugTestSuite : public CxxTest::TestSuite 
{
  public:

    /**
     * Utilities test - mostly string functions. 
     */
    void testUtils(void)
    {
      string s;

      s = " x ";
      TS_ASSERT_EQUALS(trimRight(trimLeft(s)), "x");

      s = "    x  ";
      TS_ASSERT_EQUALS(trimRight(trimLeft(s)), "x");

      stringstream ss("ab cd");
      ss >> s;
      TS_ASSERT_EQUALS(getStreamRest(ss), "cd");

      ss.clear();
      ss.str("makemove Ra1");
      ss >> s;
      TS_ASSERT_EQUALS(getStreamRest(ss), "Ra1");
    }

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
        string fnPosition;
        string fnRecord;
        string s1, s2;
        FileRead* f = new FileRead(string(INIT_TEST_LIST));

        while (f->getLineAsPair(s1, s2)){
          fnPosition = INIT_TEST_DIR + s1;
          fnRecord = INIT_TEST_DIR + s2;

          if (! board1.initFromPosition(fnPosition.c_str()))
            TS_FAIL((string) "Init from " + fnPosition + (string) " failed");
          
          if (! board2.initFromRecord(fnRecord.c_str()))
            TS_FAIL((string) "Init from " + fnRecord + (string) " failed");

          TS_ASSERT_EQUALS( board1, board2);
        }
     }

    /**
     * Hash table insert/hasItem/load test.
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
        TS_ASSERT_EQUALS(hashTable.hasItem(keys[i]), true); 
        if (! hashTable.loadItem(keys[i], item))
          TS_FAIL("positive-lookup failed");
      }
       
      //negative-lookup test
      for (int i = 0; i < HASH_TABLE_INSERTS; i++){
        int key = getRandomU64();
        int item;
        TS_ASSERT_EQUALS(hashTable.hasItem(key), false); 
        if (hashTable.loadItem(key, item))
          TS_FAIL("negative-lookup suceeded");
      }
    }

    /**
     * Uct test.
     *
     * Dummy version of Uct::doPlayout. Tests whether the uct tree is consistent 
     * after large number of inserts(UCT_TREE_NODES). Position in the bottom of the 
     * tree is evaluated randomly and randomDescend() is applied instead of uctDescend().  
     */
    void testUct(void)
    {
      //tree with random player in the root
      Tree* tree = new Tree(INDEX_TO_PLAYER(random() % 2));
      int winValue[2] = {1, -1};
      StepArray steps;
      int stepsNum;
      int i = 0;

      //build the tree in UCT manner
      for (int i = 0; i < UCT_TREE_MAX_CHILDREN; i++)
        steps[i] = Step(STEP_PASS, GOLD);

      while (true) {
        tree->historyReset();     //point tree's actNode to the root 
        while (true) { 
          if (! tree->actNode()->hasChildren()) { 
            //if (tree->actNode()->getVisits() > UCT_TREE_TEST_MATURE) {
              stepsNum = (rand() % UCT_TREE_MAX_CHILDREN) + 1;
              tree->expandNode(tree->actNode(), steps, stepsNum);
              i++;
              break;
            //}
            //tree->updateHistory(winValue[(random() % 2)]);
            //break;
          }
          tree->randomDescend(); 
          //tree->uctDescend(); 
        } 
        if (i >= UCT_TREE_NODES)
          break;
      }
      //remove given number of leafs
      for (int i = 0; i < UCT_TREE_NODES_DELETE; i++){
        tree->historyReset();
        while (tree->actNode()->hasChildren())
          tree->randomDescend(); 
        tree->removeNodeCascade(tree->actNode());
      }

      //TODO DFS through tree and check child.father == father, etc. ? 
    } //testUct


  void testThirdRepetition(void)
  {
    Board* board = new Board();
    board->initNewGame();
    thirdRep.update(board->getSignature(), 1 - PLAYER_TO_INDEX(board->getPlayerToMove()) );
    thirdRep.update(board->getSignature(), 1 - PLAYER_TO_INDEX(board->getPlayerToMove()) );
    TS_ASSERT_EQUALS(thirdRep.isThirdRep(board->getSignature(), 1 - PLAYER_TO_INDEX(board->getPlayerToMove())), true);
  }

  /**
   * Search evaluation test. 
   */
  void testSearchEval(void)
  {
    Board* board = new Board();
    SearchExt* searchExt_ = new SearchExt();
    string s1, s2;
    bool expected;

    FileRead* f = new FileRead(string(RABBITS_TEST_LIST));
    while (f->getLineAsPair(s1,s2)){
      expected = bool(str2int(s2));

      string fn = string(RABBITS_TEST_DIR) + s1;
      board->initNewGame();
      board->initFromPosition(fn.c_str());
      cerr << board->toString();
      Move move;
      if (board->quickGoalCheck(GOLD, STEPS_IN_MOVE, &move))
        cerr << "play: " << move.toString() << " buddy ! " << endl;
      else
        cerr << "Dunno how to score a goal." << endl;
          
      TS_ASSERT_EQUALS(board->quickGoalCheck(GOLD, STEPS_IN_MOVE), expected);
    }

  }
};

