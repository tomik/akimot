#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  /*stringstream ss;
  for (int i = 0; i < argc; i++){
    ss << argv[i] << " ";
  }
  cerr << "ARGUMENTS : " << ss.str();
  */

  randomStructuresInit();

  options.parse(argc, (const char **) (argv));
  //options.printAll();
 
  if (options.fnCfg() != ""){
    cfg.loadFromFile(options.fnCfg());
  }else{
    cfg.loadFromFile(string(DEFAULT_CFG));
  }

  if (! cfg.checkConfiguration()){
    logWarning("Incomplete configuration.");
    //exit(1);
  }

  //getMove protocol
  if (options.getMoveMode()){


    Board board;
    Engine* engine = new Engine();

    //last three arguments should be : position game_record gamestate file 
    if (argc - 2 <= 0) {
      cerr << "Expected -g positionfile recordfile gamefile\n";
      return 1;
    }

    string gr = argv[argc - 2];

    //if (! board.initFromPosition(options.fnInput().c_str())){
    if (! board.initFromRecord(gr.c_str(), true)){
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

