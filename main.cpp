#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  randomStructuresInit();

  options.parse(argc, (const char **) (argv));
  //options.printAll();
 
  cfg.loadFromFile(string(DEFAULT_CFG));
  if (options.fnCfg() != ""){
    cfg.loadFromFile(options.fnCfg());
  }

  if (! cfg.checkConfiguration()){
    logWarning("Incomplete configuration.");
    //exit(1);
  }

  //getMove protocol
  if (options.getMoveMode()){

    Board board;
    Engine* engine = new Engine();


    //initSuccess = board.initFromRecord(options.fnInput());

    if (! board.initFromPosition(options.fnInput().c_str())){
      cout << "Couldn't read position from file.\n";
      return 1;
    } 
    cerr << "=====" << endl;
    cerr << board.toString();
    engine->doSearch(&board);
    cout << engine->getBestMove() << endl;
    cerr << engine->getStats();
    cerr << engine->getAdditionalInfo();
    return 0;
  } 

  //aei protocol;

  Aei* aei;

  if (options.localMode()){
    //use extended aei command set
    aei = new Aei(AC_EXT);
  } else
  {
    aei = new Aei();
  }

  if (options.benchmarkMode()){
    Benchmark benchmark;
    benchmark.benchmarkAll();
    return 0;
  }
  
  if (options.fnAeiInit() != "")
    aei->initFromFile(options.fnAeiInit());

  aei->runLoop();
  return 0;

}

