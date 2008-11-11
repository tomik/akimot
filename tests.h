#include <cxxtest/TestSuite.h>
#include "board.h"

#define TEST_DIR "./test"
#define INIT_TEST_DIR "./test/init/"
#define INIT_TEST_LIST "./test/init/tests.txt"

class MyTestSuite : public CxxTest::TestSuite 
{
public:
   void testPositionLoad(void)
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

       // TS_ASSERT( 1 + 1 > 1 );
        TS_ASSERT_EQUALS( board1.getSignature(), board2.getSignature() );
      }
   }
};
