#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

#define MAX_MOVES 100

#define NORTH -8
#define SOUTH 8
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

#define TRAPS       0x0000240000240000ULL

#define MOVE_PASS     0
#define MOVE_SINGLE   1
#define MOVE_PUSH     2
#define MOVE_PULL     3

#define OPP(player) (1 - player)				 //opponent
#define BIT_ON(n) (1ULL << (n))          //creates empty board with one bit set on n

namespace bitStuff { 
  const bit64			 one(string("0000000000000000000000000000000000000000000000000000000000000001"));
  const bit64 notAfile(string("1111111011111110111111101111111011111110111111101111111011111110"));
  const bit64 notHfile(string("0111111101111111011111110111111101111111011111110111111101111111"));
  const bit64 not1rank(string("0000000011111111111111111111111111111111111111111111111111111111"));
  const bit64 not8rank(string("1111111111111111111111111111111111111111111111111111111100000000"));
  const bit64 traps		(string("0000000000000000001001000000000000000000001001000000000000000000"));
  const bit64 trapsNeighbours
											(string("0000000000100100010110100010010000100100010110100010010000000000"));
	const bit64	winRank[2] = 
							 { bit64(string("0000000000000000000000000000000000000000000000000000000011111111")),
							   bit64(string("0000000000000000000000000000000000000000000000000000000011111111")) };

 // bit64  zobrist[2][7][64];         /* table of 64 bit psuedo random numbers */
}

typedef unsigned int color_t;
typedef unsigned int coord_t;
typedef unsigned int piece_t;
typedef unsigned int moveType_t;

class Move
{
    Logger        log_; 

    moveType_t    moveType_;    //values MOVE_SINGLE, MOVE_PUSH, MOVE_PULL, MOVE_PASS
    color_t       color_;      
    piece_t       piece_;  
    coord_t       from_;     
    coord_t       to_;        
    piece_t       oppPiece_;  //opponent piece/from/to values used for pushing, pulling 
    coord_t       oppFrom_;
    coord_t       oppTo_;
    
    friend class  Board;
  public:
    void dump(); 
    const string getStepStr(color_t, piece_t, coord_t, coord_t);
    inline void setValues( moveType_t, color_t, piece_t, coord_t, coord_t );
    inline void setValues( moveType_t, color_t, piece_t, coord_t, coord_t, piece_t, coord_t, coord_t );

};

typedef Move   MoveList [MAX_MOVES];			 // fixed array for performance reasons 

class Board
{
    Logger        log_; 

    bit64         bitBoard_[2][7];
    bit64         moveOffset_[2][7][64];     // precomputed move offsets 


    unsigned int  moveCount_;
    unsigned int  stepCount_;
    unsigned int  toMove_;

  public:
    bool init(const char* fn); 

    bool isEmpty();
    bool isGoldMove();

    inline void setSquare(coord_t, color_t, piece_t);
    inline void delSquare(coord_t);											//deletes piece from square ( traping ) 
    inline piece_t getSquarePiece(coord_t);
    inline color_t getSquareColor(coord_t);

		void makeMove(Move&);
		int checkGameEnd();

    int generateMoves(MoveList&);

    void build_move_offsets();

		void test();
    void dump();
    
};


#endif
