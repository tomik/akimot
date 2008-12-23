#pragma once

#include "utils.h"
#include "board.h"
#include "uct.h"
#include "eval.h"
#include "timer.h"

#define START_POS_PATH "data/startpos.txt"
#define NEGATIVE_GOAL_CHECK_PATH "data/rabbits/b001.txt"
#define PLAYOUT_DEPTH 10
#define UCT_NODE_MATURE 1
#define SEC_ONE 1
//reflects average number of steps in position
#define UCT_CHILDREN_NUM 25

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
	public: 
		Benchmark();
		Benchmark(Board*,uint);

		void benchmarkEval(); 
		void benchmarkEvalBetter(); 
		void benchmarkCopyBoard(); 
		void benchmarkQuickGoalCheck(); 
		void benchmarkPlayout(); 
		void benchmarkUct(); 
		void benchmarkSearch() const;
    
    void benchmarkAll();

  private:
    Board * board_; 
    uint playoutCount_;
    Timer timer;
};
