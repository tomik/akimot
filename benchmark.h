/** 
 * @file benchmark.h
 * Benchmarking tool.
 *
 * Performs following benchmarks: 
 * \li board copying 
 * \li evaluation 
 * \li playout speed 
 * \li old board playout speed
 * \li uct traversing 
 * \li complete uct search
 */

#pragma once

#include "utils.h"
#include "board.h"
#include "engine.h"
#include "uct.h"
#include "eval.h"
#include "timer.h"

#define START_POS_PATH "data/startpos.txt"
//#define START_POS_PATH "data/rabbits/t009.txt"
#define NEGATIVE_GOAL_CHECK_PATH "data/rabbits/b001.txt"
#define PLAYOUT_DEPTH 3 
#define UCT_NODE_MATURE 5
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
		void benchmarkCopyBoard(); 
		void benchmarkPlayout(); 
		void benchmarkOldPlayout(); 
		void benchmarkUct(); 
		void benchmarkSearch() const;
    
    void benchmarkAll();

  private:
    Board * board_; 
    uint playoutCount_;
    Timer timer;
};
