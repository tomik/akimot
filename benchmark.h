#pragma once

#include "utils.h"
#include "board.h"
#include "engine.h"
#include "eval.h"

#define START_POS_PATH "test/startpos.txt"
#define PLAYOUT_NUM 10000

/**
 * Benchmarking class.
 * 
 * Benchmarking of various computational stuff -
 * - playouts, evaluations, uct search.
 *
 * Should run ideally always from the same position
 * (like the starting position).
 */
class Benchmark
{
  private:
    Board * board_; 
    uint playoutCount_;

	public: 
		Benchmark();
		Benchmark(Board*,uint);

		void benchmarkEval() const;
		void benchmarkPlayout() const;
		void benchmarkUct() const;
    
    void benchmarkAll() const;
};
