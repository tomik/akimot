#ifndef BOARD_H
#define BOARD_H

#include "utils.h"

#define MAX_STEPS 200

#define EMPTY_SQUARE 0x0U
#define EMPTY 0x0U
#define OFF_BOARD_SQUARE 0x9FU
//#define OFF_BOARD 0x18U
#define GOLD 0x10U
#define SILVER 0x8U
#define OFF_BOARD_PIECE 0x7U
#define ELEPHANT_PIECE 0x6U
#define CAMEL_PIECE 0x5U
#define HORSE_PIECE 0x4U
#define DOG_PIECE 0x3U
#define CAT_PIECE 0x2U
#define RABBIT_PIECE 0x1U
#define EMPTY_PIECE 0x0U
#define PIECE_MASK 0x7U
#define OWNER_MASK 0x18U
#define FLIP_SIDE GOLD^SILVER
#define TRUE 1
#define FALSE 0
#define NORTH -10
#define SOUTH 10
#define EAST 1
#define WEST -1

#define OWNER(square) (square & OWNER_MASK) 
#define PIECE(square) (square & PIECE_MASK) 
#define ROW(square) (9-square/10) // what row is a square in?  1 = bottom, 8 = top
#define COL(square) (square%10) // what column is a square in?  1 = left (a), 8 = right (h)
#define OPP(player) ( (16 - player) + 8 )
#define PLAYER_TO_INDEX(player)	((16-player)/8)	//0 for GOLD, 1 for SILVER - for indexation
#define INDEX_TO_PLAYER(index)  (uint) (16-8*index)	//0 -> GOLD, 1 -> SILVER - reverse indexation
#define IS_TRAP(index) (index == 33 || index == 36 || index == 63 || index == 66 ) //sets up a boolean expression
#define IS_PLAYER(square) (OWNER(square) == GOLD || OWNER(square) == SILVER )

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3
#define STEP_NO_STEP  4   //no step is possible ( not even pass ! - position repetition )

extern const int direction[4];
extern const int trap[4];

typedef uint player_t;
typedef int square_t;
typedef uint piece_t;		
typedef uint stepType_t;

class Step
{
	private:
    Logger        log_; 

    stepType_t    stepType_;    //values MOVE_SINGLE, MOVE_PUSH, MOVE_PULL, MOVE_PASS
    player_t      player_;      
    piece_t       piece_;  
    square_t      from_;     
    square_t      to_;        
    piece_t       oppPiece_;  //opponent piece/from/to values used for pushing, pulling 
    square_t      oppFrom_;
    square_t      oppTo_;
    
    friend class  Board;
  public:
		Step(){};
		Step( stepType_t );
    Step( stepType_t, player_t, piece_t, square_t, square_t );
    Step( stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t );
    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t );
    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t );
		bool pieceMoved();

    const string oneSteptoString(player_t, piece_t, square_t, square_t) const;
    const string toString() const;
    void dump(); 
		bool operator== ( const Step&);

};

class Board;

#define HASH_ITEMS 78


//typedef MyArray<Step> StepArray;

typedef Step SimpleStepArray[MAX_STEPS]; 

class Board
		/*This is a crucial class - representing the board. 
		 *
		 * step is either pass or single piece step or push/pull step
		 * move consists of up to 4 steps ( push/pull  counting for 2 )
		 */
{
	private:
    Logger        log_; 

		uint						board_[100];					//actual pieces are stored here 
		bool					frozenBoard_[100];			//keep information on frozen pieces, false == notfrozen, true == frozen

		bool 					stepHashSingle[HASH_ITEMS][4];			  
		bool 					stepHashPush[HASH_ITEMS][4][4];			
		bool 					stepHashPull[HASH_ITEMS][4][4];			

  // MyArray<Step>   stepArray[2];        //first index == player, 0 == gold, 1 == SILVER 
  
    Step          stepArray[2][MAX_STEPS];   
    uint          stepArrayLen[2];

		// move consists of up to 4 steps ( push/pull  counting for 2 ),
		// thus moveCount_ expresses how far in the game position is 
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;
    player_t toMove_;

		player_t winner_;

  public:
		Board(){stepArrayLen[0] = 0; stepArrayLen[1] = 0;};

    bool init(const char* fn); 

    bool		 isEmpty();
    player_t getPlayerToMove();

		uint			getStepCount();
		player_t	getWinner();

		uint			getAllStepsNum(uint);


		void makeStep(Step&);
		void commitMove();

    void generateAllSteps(player_t);
		Step getRandomStep();
    bool checkStepValidity(const Step&);
    inline void removeStepFromStepHash(const Step&);
    void clearStepArray(player_t);

		inline bool hasFriend(square_t) const;
		inline bool hasStrongerEnemy(square_t) const;
		inline bool isFrozen(square_t) const;
		
		void updateAfterStep(square_t from, square_t to);
		void updateAfterKill(square_t square);
		void updateStepsForNeighbours(square_t, square_t newPosition = -1);

		void generateSingleStepsFromSquare(square_t);
		void generatePushPullsFromSquare(square_t, square_t = -1);
		void generatePushesFromSquareThrough(square_t, square_t, square_t = -1);
		void generatePullsFromSquareVictim(square_t, square_t, square_t = -1);
		void generatePushesToSquare(square_t);
		void generatePullsToSquareFrom(square_t,square_t);

		inline void generatePull(square_t, square_t, square_t);
		inline void generatePush(square_t, square_t, square_t);
		inline void generateSingleStep(square_t, square_t);

    void dump();
		string toString();
		void dumpAllSteps();
		string allStepsToString();

		void testStepsStructure();
		int generateAllStepsOld(player_t, SimpleStepArray, bool) const;

    void setStepHash(const Step&, bool);
    bool checkStepHash(const Step&); 
    inline uint directionToIndex(uint direction);

};


#endif
