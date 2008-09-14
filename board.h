#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

#define MAX_STEPS 100

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

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3

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

typedef uint color_t;
typedef uint coord_t;
typedef uint piece_t;
typedef uint stepType_t;

class Step
{
    Logger        log_; 

    stepType_t    stepType_;    //values MOVE_SINGLE, MOVE_PUSH, MOVE_PULL, MOVE_PASS
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
    inline void setValues( stepType_t, color_t, piece_t, coord_t, coord_t );
    inline void setValues( stepType_t, color_t, piece_t, coord_t, coord_t, piece_t, coord_t, coord_t );
    inline void setPass(); 
		bool isPass();

};

typedef Step	StepList [MAX_STEPS];			 // fixed array for performance reasons 

class Board
		/*This is a crucial class - representing the board. 
		 *
		 * step is either pass or single piece step or push/pull step
		 * move consists of up to 4 steps ( push/pull  counting for 2 )
		 */
{
    Logger        log_; 

    bit64         bitBoard_[2][7];
    bit64					stepOffset_[2][7][64];				// precomputed step offsets TODO: move outside the class
		StepList			stepList_;										// for inner step generation ( like generate all and select random )
  
		// move consists of up to 4 steps ( push/pull  counting for 2 ),
		// thus moveCount_ expresses how far in the game position is 
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;
    uint  toMove_;

  public:
    bool init(const char* fn); 

    bool isEmpty();
    bool isGoldMove();

    inline void setSquare(coord_t, color_t, piece_t);
    inline void delSquare(coord_t);											//deletes piece from square ( traping ) 
    inline piece_t getSquarePiece(coord_t);
    inline color_t getSquareColor(coord_t);
		uint	getStepCount();

		void makeStep(Step&);
		int checkGameEnd();

    int generateSteps(StepList&);
		Step generateRandomStep();

    void buildStepOffsets();

    void dump();
    
};


#endif
