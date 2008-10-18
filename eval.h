#ifndef EVAL_H
#define EVAL_H

#include "board.h"

#define ELEPHANT_VALUE 20000
#define CAMEL_VALUE 5000
#define HORSE_VALUE 3000
#define DOG_VALUE 1800
#define CAT_VALUE 1500
#define RABBIT_VALUE 1000

#define EVAL_MAX 
#define EVAL_MIN

class Eval
/*board evaluation class 
 *it is declared as a friend in the board class - thus it can access it's private items
 *always returns evaluation from the point of view of GOLD player
 */
{
  Logger log_;
	public:
    int evaluate(const Board*);
    float evaluateInPercent(const Board*); 
}; 

#endif
