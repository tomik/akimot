#include "utils.h"
#include "board.h"
#include "engine.h"

void test(Board* board)
{

	board->dump();

	Benchmark benchmark(board,10000);
	benchmark.doBenchmark();

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

