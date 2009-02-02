#pragma once

#include "board.h"

#define RING0  0x0000001818000000ULL   /* same as small center */
#define RING1  0x00003c24243c0000ULL   
#define RING2  0x007e424242427e00ULL
#define RING3  0xff818181818181ffULL   

//TODO change ! 
#define TRAP_TO_INDEX(trap) (trap < 32 ? (trap == 18 ? 0 : 1) : trap ==  42 ? 2 : 3)

extern u64 adv[8][2];

enum trapType_e { TT_UNSAFE, TT_HALF_SAFE, TT_SAFE, TT_DOMINANT};

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
     * Evaluation by ddailey.
     */
    int evaluateDailey(const Board*) const;

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

//  private: 

   // bool isBlockaded(player_t player, coord_t coord);
   // bool isDominant(player_t player, coord_t); 
};
