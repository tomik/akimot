#ifndef SEARCHH_DEFINED // Don't include this twice

#define SEARCHH_DEFINED

#include "board.h"

typedef struct
{
    int depth_limit;
    double time;
    unsigned long long int nodes;
    unsigned long long int evals;
} test_data_t;

int SEARCH_Start_Search(board_t *bp, move_t ml[4], test_data_t *tp);

#endif
