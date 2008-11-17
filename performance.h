#include <cxxtest/TestSuite.h>
#include "board.h"
#include "hash.h"
#include "engine.h"

#define RABBIT_TEST_DIR "./test/rabbits/"
#define RABBIT_TEST_LIST "./test/rabbits/list.txt"
#define HASH_TABLE_INSERTS 100

class PerformanceTestSuite : public CxxTest::TestSuite 
{
  public:

    /**
     * Test on rabbits reaching the goal. 
     */
     void testRabbitReachGoal(void)
     {
        fstream f;
        string fnPosition;
        stringstream ss;
        string line;

        f.open(RABBIT_TEST_LIST, fstream::in);

        //TODO remove magic values
        //config.useTimeControl(true);
        //config.secPerMove(3);
        config.playoutsPerMove(100000);

        while (f.good()){
          getline(f, line);
          if (line == "")
            continue;
          ss.clear();
          ss.str(line);
          fnPosition = "";
          ss >> fnPosition;
          fnPosition = RABBIT_TEST_DIR + fnPosition;
          cerr << endl << "Processing " << fnPosition << endl;

          //TODO clear thirdRep table ! 
          Board* board = new Board();

          board->initFromPosition(fnPosition.c_str());

          Uct* uct = new Uct(board);
          cerr << uct->generateMove() << endl;
          Tree* tree = uct->getTree();

          //cerr << tree->toString();
          cerr << "Best move value: " << uct->getBestMoveValue() << endl;
          TS_ASSERT(uct->getBestMoveValue() > 0.6 );

          delete uct;
          delete board;
        }
     }
};
