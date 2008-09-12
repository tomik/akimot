#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

#define MAX_NUMBER_MOVES 100
#define NORTH -10
#define SOUTH 10
#define EAST 1
#define WEST -1

//dailey 

#define BIT_LEN     64
typedef bitset<BIT_LEN> bit64;
typedef unsigned long long u64;

#define GOLD        0
#define SILVER      1
#define NO_COLOR    2

#define EMPTY       0
#define RABBIT      1
#define CAT         2
#define DOG         3
#define HORSE       4
#define CAMEL       5
#define ELEPHANT    6

#define NOT_A_FILE  0xfefefefefefefefeULL
#define NOT_H_FILE  0x7f7f7f7f7f7f7f7fULL
#define NOT_1_RANK  0x00ffffffffffffffULL
#define NOT_8_RANK  0xffffffffffffff00ULL
#define TRAPS       0x0000240000240000ULL

#define BIT_ON(n) (1ULL << (n))          //creates empty board with one bit set on n
#define FROM_LEFT(n) (BIT_LEN - n - 1)   //for accesing elemt from left in bitsets ( implicitly from right )


namespace BitStuff { 
  const bit64 notAfile(string("1111111011111110111111101111111011111110111111101111111011111110"));
  const bit64 notHfile(string("0111111101111111011111110111111101111111011111110111111101111111"));
  const bit64 not1rank(string("0000000011111111111111111111111111111111111111111111111111111111"));
  const bit64 not8rank(string("1111111111111111111111111111111111111111111111111111111100000000"));

 // bit64  zobrist[2][7][64];         /* table of 64 bit psuedo random numbers */
}


typedef unsigned int color_t;
typedef unsigned int coord_t;
typedef unsigned int piece_t;

class Move
{
};

class Board
{

    bit64         bitBoard_[2][7];
    bit64         move_offset[2][7][64];     /* precomputed move offsets */

    unsigned int  moveCnt_;
    unsigned int  toMove_;
    Logger        log_; 

  public:
    bool init(const char* fn); 

    bool isEmpty();
    bool isGoldMove();

    inline void setSquare(coord_t, color_t, piece_t);
    inline piece_t getSquarePiece(coord_t);
    inline color_t getSquareColor(coord_t);

    int generateOneStepMoves(Move moveList[]);
    int generatePushMoves(Move moveList[]);
    int generatePullMoves(Move moveList[]);
    int generateMoves(Move moveList[]);

    void build_move_offsets();

    void dump();

    
};


#endif
