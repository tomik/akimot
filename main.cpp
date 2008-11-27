#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  config.parse(argc, (const char **) (argv));
  //config.logAll();

  Aei aei;

  if (config.benchmark()){
    Benchmark benchmark;
    benchmark.benchmarkAll();
    return 0;
  }
  
  //for debugging
  if (config.debug())
    aei.implicitSessionStart();

  aei.runLoop();
  return 0;

}

