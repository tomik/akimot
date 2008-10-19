#ifndef BOARD_H
#define BOARD_H

#include "utils.h"

#define MAX_STEPS  200
#define MAX_PIECES  16      //max number of pieces per player 

#define EMPTY_SQUARE 0x0U
#define EMPTY 0x0U
#define OFF_BOARD_SQUARE 0x9FU
//#define OFF_BOARD 0x18U
#define GOLD 0x10U
#define SILVER 0x8U
#define PIECE_OFF_BOARD 0x7U
#define PIECE_ELEPHANT 0x6U
#define PIECE_CAMEL 0x5U
#define PIECE_HORSE 0x4U
#define PIECE_DOG 0x3U
#define PIECE_CAT 0x2U
#define PIECE_RABBIT 0x1U
#define PIECE_EMPTY 0x0U
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
#define OPP(player) ((16 - player) + 8)
#define PLAYER_TO_INDEX(player)	((16-player)/8)	//0 for GOLD, 1 for SILVER - for indexation
#define INDEX_TO_PLAYER(index)  (uint) (16-8*index)	//0 -> GOLD, 1 -> SILVER - reverse indexation
#define IS_TRAP(index) (index == 33 || index == 36 || index == 63 || index == 66 ) //sets up a boolean expression
#define IS_PLAYER(square) (OWNER(square) == GOLD || OWNER(square) == SILVER )

#define ROW(square) (9-square/10)
#define COL(square) (square%10)

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


class PieceArray
{ 
  private:
    square_t elems[MAX_PIECES];      
    uint len;

  public: 
    PieceArray();
    void add(square_t);
    void del(square_t);
    uint getLen() const;
    square_t operator[](uint) const;
};

class Step
  /** Class representing a step of one player. 
   */ 
{
	private:
    Logger        log_; 

    stepType_t    stepType_;    //! defines what kind of step this is i.e. PASS, SINGLE, PUSH, PULL
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
		Step( stepType_t, player_t );
    Step( stepType_t, player_t, piece_t, square_t, square_t );
    Step( stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t );
    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t );
    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t );

		bool pieceMoved();
		bool operator== ( const Step&);

    player_t getStepPlayer() const;
    const string oneSteptoString(player_t, piece_t, square_t, square_t) const;
    const string toString() const;
    void dump(); 

};

class Board;
class Eval;

#define HASH_ITEMS 78

typedef Step  StepArray[MAX_STEPS];

class Board
		/*This is a crucial class - representing the board. 
		 *
		 * step is either pass or single piece step or push/pull step
		 * move consists of up to 4 steps ( push/pull  counting for 2 )
		 */
{
	private:
    Logger        log_; 

		uint					board_[100];					//actual pieces are stored here 
		bool					frozenBoard_[100];			//keep information on frozen pieces, false == notfrozen, true == frozen

    PieceArray    pieceArray[2];  
		square_t  	  piecesList[MAX_PIECES];					//actual pieces are stored here 
  
    StepArray     stepArray[2];
    uint          stepArrayLen[2];

    uint          rabbitsNum[2];        //kept number of rabbits for each player - for quick check on rabbitsNum != 0 


		// move consists of up to 4 steps ( push/pull  counting for 2 ),
		// thus moveCount_ expresses how far in the game position is 
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;

    player_t toMove_;
    uint     toMoveIndex_;    //0 == GOLD, 1 == SILVER

		player_t winner_;

    uint  generateAllCount;   //how many times generateAll was called :)

    friend class Eval;
  public:
		Board(){stepArrayLen[0] = 0; stepArrayLen[1] = 0;};

    bool init(const char* fn); 

    bool		 isEmpty();
    player_t getPlayerToMove();
    uint     getGenerateAllCount();

		uint			getStepCount();
		player_t	getWinner();

		uint			getAllStepsNum(uint);

		void makeStep(Step&);
		void commitMove();
		void makeStepTryCommit(Step&);

		Step getRandomStep();
		bool createRandomStep(Step&);

		inline bool hasFriend(square_t) const;
		inline bool hasStrongerEnemy(square_t) const;
		inline bool isFrozen(square_t) const;
		
		int generateAllSteps(player_t, StepArray) const;
		void updateAfterStep(square_t from, square_t to);
		void updateAfterKill(square_t square);

    void dump();
		string toString();
		void dumpAllSteps();
		string allStepsToString();

		void testPieceArray();

};

#endif
