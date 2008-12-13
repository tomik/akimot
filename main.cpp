#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  srand(time(0));

  config.parse(argc, (const char **) (argv));
  //config.printAll();
 
  //getMove protocol
  if (config.getMoveMode()){

    Board board;
    Engine* engine = new Uct();

    srand((unsigned) time(NULL));

    //initSuccess = board.initFromRecord(config.fnInput());

    if (! board.initFromPosition(config.fnInput().c_str())){
      cout << "Couldn't read position from file.\n";
      return 1;
    } 
    cerr << board.toString();
    engine->doSearch(&board);
    cout << engine->getBestMove() << endl;
    return 0;
  } 

  //aei protocol;

  Aei* aei;

  if (config.localMode()){
    //use extended aei command set
    aei = new Aei(AC_EXT);
  } else
  {
    aei = new Aei();
  }

  if (config.benchmarkMode()){
    Benchmark benchmark;
    benchmark.benchmarkAll();
    return 0;
  }
  
  if (config.fnAeiInit() != "")
    aei->initFromFile(config.fnAeiInit());

  aei->runLoop();
  return 0;

}

