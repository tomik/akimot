#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  logInfo( "This is a test message %s"," and this is the end of the test message");
//for (int i = 0; i < argc; i ++)
//  cout << argv[i] << endl;

  config.parse(argc, (const char **) (argv));
  //config.logAll();
  //
  Aei aei;
  
  //for debugging
  if (config.debug())
    aei.implicitSessionStart();

  aei.runLoop();
  return 0;

/*
	Board board;
	Engine engine;
	Logger logger;
  bool initSuccess = true;

  srand(time(0));
  if (config.inputIsRecord()) 
	  initSuccess = board.initFromRecord(config.fnInput());
  else
	  initSuccess = board.initFromPosition(config.fnInput());

  if (! initSuccess){
		logger() << "Couldn't read position from file.\n";
		return 1;
	} 

	if (board.isEmpty()) { //first step
		cout << engine.initialSetup(board.getPlayerToMove() == GOLD);
	}else {
    engine.doSearch(&board);
		cout << engine.getBestMove() << endl;
	}

	return 0;
*/

}

