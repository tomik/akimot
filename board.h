#ifndef __BOARD_H__
#define __BOARD_H__

#include "utils.h"

#define MAX_STEPS 100

#define NORTH -8
#define SOUTH 8
#define EAST 1
#define WEST -1

#define BIT_LEN     64
typedef bitset<BIT_LEN> bit64;
typedef unsigned long long u64;

#define GOLD        0
#define SILVER      1
#define NO_PLAYER   2

#define EMPTY       0
#define RABBIT      1
#define CAT         2
#define DOG         3
#define HORSE       4
#define CAMEL       5
#define ELEPHANT    6

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3
#define STEP_NO_STEP  4   //no step is possible ( not even pass ! - position repetition )

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
							   bit64(string("111111110000000000000000000000000000000000000000000000000000000")) };

  extern bit64					stepOffset_[2][7][64]; //cannot be const - is build by buildStepOffset

 // bit64  zobrist[2][7][64];         /* table of 64 bit psuedo random numbers */
 
	void buildStepOffsets();
	string stepOffsettoString();
	bit64 getNeighbours(bit64);
}

typedef uint player_t;
typedef uint coord_t;
typedef uint piece_t;
typedef uint stepType_t;

class Step
{
    Logger        log_; 

    stepType_t    stepType_;    //values MOVE_SINGLE, MOVE_PUSH, MOVE_PULL, MOVE_PASS
    player_t      player_;      
    piece_t       piece_;  
    coord_t       from_;     
    coord_t       to_;        
    piece_t       oppPiece_;  //opponent piece/from/to values used for pushing, pulling 
    coord_t       oppFrom_;
    coord_t       oppTo_;
    
    friend class  Board;
  public:
		Step(){};
		Step( stepType_t );
    inline void setValues( stepType_t, player_t, piece_t, coord_t, coord_t );
    inline void setValues( stepType_t, player_t, piece_t, coord_t, coord_t, piece_t, coord_t, coord_t );
    inline void setPass(); 
		bool pieceMoved();

    const string oneSteptoString(player_t, piece_t, coord_t, coord_t);
    const string toString();
    void dump(); 

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
		StepList			stepList_;										// for inner step generation ( like generate all and select random )
  
		// move consists of up to 4 steps ( push/pull  counting for 2 ),
		// thus moveCount_ expresses how far in the game position is 
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;
    uint  toMove_;

		player_t winner_;

  public:
    bool init(const char* fn); 

    bool		 isEmpty();
    player_t getPlayerToMove();

    inline void			setSquare(coord_t, player_t, piece_t);
    inline void			delSquare(coord_t);											//deletes piece from square ( traping ) 
    inline piece_t	getSquarePiece(coord_t);
    inline player_t	getSquarePlayer(coord_t);
		uint			getStepCount();
		player_t	getWinner();

		void makeStep(Step&);
		void commitMove();

    int generateSteps(StepList&);
		Step generateRandomStep();


    void dump();
		string toString();
    
};


#endif
