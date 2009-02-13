#ifndef HASHH_DEFINED // Don't include this twice

#define HASHH_DEFINED

#include "board.h"

#define LOWER_BOUND 1
#define UPPER_BOUND 2
#define EXACT_VALUE 3

typedef struct
{
	unsigned long long int hashkey;
	int value;
	int depth;
	unsigned char type; // upper bound, lower bound, or exact
	unsigned char move; // Have we stored a move for this position?
    // Information about the best move:
    unsigned char steps; // How many steps does this move use?
    unsigned char pass; // Are we passing?
    unsigned char piece[2]; // Which piece(s) are we moving?
    unsigned char from[2]; // Square piece(s) are moving from.
    unsigned char to[2]; // Square piece(s) are moving to.
    unsigned char capture_square[2]; // If a piece is captured, where does it happen?
    unsigned char captured_piece[2]; // If a piece is captured, which piece is it?
} hash_entry_t;

extern unsigned long long int hashkey_piece[100][OWNER_MASK|PIECE_MASK]; // hashkey values for each piece, on each square of the board.
extern unsigned long long int hashkey_step[4]; // hashkey values for how many steps have been.
extern unsigned long long int hashkey_at_move[OWNER_MASK]; // hashkey values for who is at move.

void HASH_Init(void);
void HASH_Set_Size(unsigned long long int size);
void HASH_Put_Entry(unsigned long long int hashkey, int value, int depth, unsigned char type, move_t *mp);
hash_entry_t *HASH_Get_Entry(board_t *board);

#endif
