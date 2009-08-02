#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"
 
/*! \mainpage Akimot reference manual
 *
 * \section intro_sec Introduction
 *
 * Welcome to Akimot's reference manual. The purpose of this manual is to provide a helper for akimot developers. 
 * If you are completely new to the project, we suggest to start with the files overview giving you the good idea of project's organisation ... 
 * 
 * This manual might be useful in particular cases to quickly understands relations among classes or to get short descriptions of methods/variables. 
 * It is not meant as an exhaustive documentation though. We believe the best way to gain the sound understanding of certain project parts is
 * following: gain basic information about the target from this manual and then dive into the code - we have strived for legibility of code and 
 * documented the more complicated parts.
 *
 * We hope you will find this reference manual useful and that it will help you with your work on the akimot project.
 */


int main(int argc, char *argv[]) 
{

  globalStructuresInit();

  options.parse(argc, (const char **) (argv));
  //options.printAll();

  if (options.help()){
    cout << options.helpToString();
    exit(1);
  }
 
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

      string gr = argv[argc - 2];
    if (options.fnRecord() != "") { 
      logDebug("Loading from record %s.\n", options.fnRecord().c_str());
      if (! board.initFromRecord(options.fnRecord().c_str(), true)){
        logError("Couldn't read record from file %s.\n", options.fnRecord().c_str());
        return 1;
      }
    } 
    else if (options.fnPosition() != "" ){ 
      logDebug("Loading from position %s.\n", options.fnPosition().c_str());
      if (! board.initFromPosition(options.fnPosition().c_str())){
        //logError("Couldn't read position from file %s.\n", options.fnPosition().c_str());
      }

      logDebug("Loading from record %s.\n", options.fnRecord().c_str());
      //try to read as if it's a record
      if (! board.initFromRecord(options.fnPosition().c_str(), true)){
        logError("Couldn't read position or record from file %s.\n", options.fnPosition().c_str());
        return 1;
      }
    } 

    //cerr << "=====" << endl;
    //cerr << board.toString();
    engine->doSearch(&board);
    cout << engine->getBestMove() << endl;
    //cerr << engine->getStats();
    //cerr << engine->getAdditionalInfo();
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

