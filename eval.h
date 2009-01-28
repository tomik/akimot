#pragma once

#include "board.h"

#define ELEPHANT_VALUE 20000
#define CAMEL_VALUE 5000
#define HORSE_VALUE 3000
#define DOG_VALUE 1800
#define CAT_VALUE 1500
#define RABBIT_VALUE 1000

#define RING0  0x0000001818000000ULL   /* same as small center */
#define RING1  0x00003c24243c0000ULL   
#define RING2  0x007e424242427e00ULL
#define RING3  0xff818181818181ffULL   

extern u64 adv[8][2];

/**
 * Board evaluation class.
 * 
 * It is declared as a friend in the board class - thus it can access it's private items.
 * Always returns evaluation from the point of view of GOLD player.
 */
class Eval
{
	public:
    /**
     * Evaluation.
     */
    int evaluate(const Board*) const;

    /**
     * Transfers int/float evaluation to percent.
     */
    float evaluateInPercent(const Board*) const;

    /**
     * Evaluates one step.
     *
     * In this play game knowledge is applied.
     */
    float evaluateStep(const Board*, const Step& step) const;
}; 

