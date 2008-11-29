#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "benchmark.h"
#include "aei.h"


int main(int argc, char *argv[]) 
{

  config.parse(argc, (const char **) (argv));
  //config.printAll();

  Aei aei;

  if (config.benchmark()){
    Benchmark benchmark;
    benchmark.benchmarkAll();
    return 0;
  }
  
  if (config.fnAeiInit() != "")
    aei.initFromFile(config.fnAeiInit());

  aei.runLoop();
  return 0;

}

