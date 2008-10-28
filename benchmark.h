#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "utils.h"
#include "board.h"
#include "engine.h"
#include "eval.h"

class Benchmark
{
  private:
    const Board * board_; 
    uint playoutCount_;

    Logger log_;

	public: 
		Benchmark();
		Benchmark(Board*,uint);

		void playoutBenchmark() const;
};
#endif
