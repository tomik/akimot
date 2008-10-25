#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"

void test(Board* board)
{
	board->dump();

	Benchmark benchmark(board,1);
  benchmark.playoutBenchmark();
}


int main(int argc, char *argv[]) // returns 1 if an error occurs, 0 otherwise
{
	Board board;
	Engine engine;
	Logger logger;

  srand(time(0));

  config.parse(argc, (const char **) (argv));
  config.printAll();
	
	if (argc < 2) {
		logger() << "Program requires an argument (name of file containing position).\n";
		return 1;
	}

	srand((unsigned) time(NULL));

	if (! board.init(config.fnInput())) {
		logger() << "Couldn't read position from file.\n";
		return 1;
	} 

	cout << board.toString();
  
	if (board.isEmpty()) { //first step
		cout << engine.initialSetup(board.getPlayerToMove() == GOLD);
	}else {
		cout << engine.doSearch(&board) << endl;
	}

 //test(&board);
	
	return 0;
}

