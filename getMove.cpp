#include "utils.h"
#include "board.h"
#include "engine.h"

void test(Board board)
{
	Logger logger;

  board.dump();
//  Move move(MOVE_SINGLE, GOLD, ELEPHANT, 52, 44);
//  move.dump();

	//board.test();

  MoveList moveList;
	int listCount;
  board.generateMoves(moveList);
	//	logger() << "Potential one step moves from the position:" << endl;

  time_t now = time (NULL);
	unsigned long long i=0;
	while ( time(NULL) - now < 11 ) {
		i++;
		listCount = board.generateMoves(moveList);
	}
	logger()<< endl << "games per sec: " << i/1600 + i%1600 <<endl;
		

  for (int i = 0; i < listCount; i++) 
    moveList[i].dump();

	logger() << endl;

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

	srand(0);
	if (! board.init(argv[1])) {
		logger() << "Couldn't read position from file.\n";
		return 1;
	} 

	if (board.isEmpty()) { //first move
		cout << engine.initialSetup(board.isGoldMove());
	}else {
		cout << engine.doSearch(board) << endl;
	}

 test(board);
	
	return 0;
}

