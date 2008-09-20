#include "utils.h"
#include "board.h"
#include "engine.h"

void test(Board* board)
{

	board->dump();

	Benchmark benchmark(board,1);
	benchmark.doBenchmark();

/*
	Logger logger;
  Step step(STEP_SINGLE, GOLD, ELEPHANT, 52, 44);
  step.dump();

	//board.test();
  StepList stepList;
	int listCount;
  board.generateSteps(stepList);

  time_t now = time (NULL);
	unsigned long long i=0;
	while ( time(NULL) - now < 11 ) {
		i++;
		listCount = board.generateSteps(stepList);
	}
	logger()<< endl << "games per sec: " << i/1600 + i%1600 <<endl;
		

	logger() << "Potential single steps from the position:" << endl;
  for (int i = 0; i < listCount; i++) 
    stepList[i].dump();

	logger() << endl;
*/

}

int main(int argc, char *argv[]) // returns 1 if an error occurs, 0 otherwise
{
	Board board;
	Engine engine;
	Logger logger;
	
	if (argc < 2) {
		logger() << "Program requires an argument (name of file containing position).\n";
		return 1;
	}

	srand((unsigned) time(NULL));

	if (! board.init(argv[1])) {
		logger() << "Couldn't read position from file.\n";
		return 1;
	} 

	if (board.isEmpty()) { //first step
		cout << engine.initialSetup(board.getPlayerToMove() == GOLD);
	}else {
		cout << engine.doSearch(board) << endl;
	}

 test(&board);
	
	return 0;
}

