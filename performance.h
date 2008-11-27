#include <cxxtest/TestSuite.h>
#include "board.h"
#include "hash.h"
#include "engine.h"

#define RABBIT_TEST_DIR "./test/rabbits/"
#define RABBIT_TEST_LIST "./test/rabbits/list.txt"
#define HASH_TABLE_INSERTS 100
#define UCT_TEST_PLAYOUTS 20000

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
        //config.playoutsPerMove(10000);

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
          thirdRep.clear(); 
          Board* board = new Board();
          board->initFromPosition(fnPosition.c_str());

          Uct* uct = new Uct(board);
          for (int i = 0; i < UCT_TEST_PLAYOUTS; i++) 
            uct->doPlayout();
          cerr << uct->getBestMove() << endl;
          Tree* tree = uct->getTree();

          //cerr << tree->toString();
          cerr << "Best move value: " << uct->getBestMoveValue() << endl;
          TS_ASSERT(uct->getBestMoveValue() > 0.6 );

          delete uct;
          delete board;
        }
     }
};
