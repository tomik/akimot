#include "utils.h"
#include "board.h"
#include "engine.h"

int main(int argc, char *argv[]) // returns 1 if an error occurs, 0 otherwise
{
	Board board;
	Engine engine;
	Logger logger;
	
	if (argc<2)
	{
		logger.log << "Program requires an argument (name of file containing position).\n";
		return 1;
	}

	srand(0);
	if (board.init(argv[1]))
	{
		logger.log << "Couldn't read position from file.\n";
		return 1;
	} 

	if (board.is_empty()) //first move
	{
		cout << board.initial_setup();
	}else
	{
		cout << engine.do_search(board) << endl;
	}
	
	
	return 0;
}
