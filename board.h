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
#define NORTH 10
#define SOUTH -10
#define EAST 1
#define WEST -1

#define OWNER(square) (square & OWNER_MASK) 
#define PIECE(square) (square & PIECE_MASK) 
#define OPP(player) ((16 - player) + 8)

//Player to indexation ... 0 for GOLD, 1 for SILVER - for indexation
#define PLAYER_TO_INDEX(player)	((16-player)/8)	
//Index to player 0 -> GOLD, 1 -> SILVER 
#define INDEX_TO_PLAYER(index)  (uint) (16-8*index)	

#define IS_TRAP(index) (index == 33 || index == 36 || index == 63 || index == 66 ) 
#define IS_PLAYER(square) (OWNER(square) == GOLD || OWNER(square) == SILVER )

//#define ROW(square) (9-square/10)
//#define COL(square) (square%10)

#define PLAYER_NUM    2
#define PIECE_NUM     7
#define SQUARE_NUM    100

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3
#define STEP_NO_STEP  4   //no step is possible ( not even pass ! - position repetition )

extern const int direction[4];
extern const int trap[4];

typedef uint player_t;
typedef int  square_t;
typedef uint piece_t;		
typedef uint stepType_t;


/**
 * Array-like structure to hold pieces.
 *
 * This class is used to hold squares where pieces for 
 * one of the players are positioned.
 */
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


/**
 * Information about a kill in the trep. 
 */
class KillInfo 
{
  private:
    player_t player_;
    piece_t  piece_;
    square_t square_;
  public:
    KillInfo();
    KillInfo( player_t player, piece_t piece, square_t square);
    void setValues( player_t player, piece_t piece, square_t square);

    const string toString() const;
};

/**
 * One step of a player.
 *
 * Represents one of the following:
 *   single-step steps - i.e. move of the piece
 *   double-step steps - i.e. push/pulls
 *   pass moves
 *   no step moves     - i.e. resignation 
*/ 
class Step
{

	private:
    stepType_t    stepType_;    //! defines what kind of step this is i.e. PASS, SINGLE, PUSH, PULL
    player_t      player_;      
    piece_t       piece_;  
    square_t      from_;     
    square_t      to_;        

    piece_t       oppPiece_;  //opponent piece/from/to values used for pushing, pulling 
    square_t      oppFrom_;
    square_t      oppTo_;

    Logger        log_; 

    friend class  Board;

  public:
		Step(){};
		Step(stepType_t, player_t);
    Step(stepType_t, player_t, piece_t, square_t, square_t);
    Step(stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t);

    player_t getStepPlayer() const;
    bool isPass() const;
    bool isPushPull() const;
		bool pieceMoved() const;
		bool operator== (const Step&) const;

    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t );
    inline void setValues( stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t );

    const string oneSteptoString(player_t, piece_t, square_t, square_t) const;
    const string toString(bool resultPrint = false) const;
    void dump(); 

};



class Board;
class Eval;

typedef Step  StepArray[MAX_STEPS];

/**
 * Board representation.
 *
 * Crucial building block of the whole program. 
 */
class Board
{

	private:
    static bool   classInit;
    static u64    zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     //zobrist base table for signature creating 

		uint					board_[SQUARE_NUM];					//actual pieces are stored here 
		bool					frozenBoard_[SQUARE_NUM];			//keep information on frozen pieces, false == notfrozen, true == frozen

    PieceArray    pieceArray[2];  
    uint          rabbitsNum[2];        //kept number of rabbits for each player - for quick check on rabbitsNum != 0 
  
    StepArray     stepArray[2];
    uint          stepArrayLen[2];

    u64           signature;            //position signature - for hash tables, corectness checks, etc. 
    u64           preMoveSignature;     //signature of position from when the current move started

		// move consists of up to 4 steps ( push/pull  counting for 2 ),
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;

    player_t toMove_;
    uint     toMoveIndex_;    //0 == GOLD, 1 == SILVER
		player_t winner_;

    Logger        log_; 

    friend class Eval;

  public:
    Board();

    /**
    * Inits position from a method in file.
    *
    * returns true if initialization went right 
    * otherwise returns false
    */
    bool  init(const char* fn); 
    void  initZobrist() const;
    void  makeSignature();

		Step findStepToPlay();
		bool findRandomStep(Step&) const;

		void makeStep(Step&);
		void makeStepTryCommitMove(Step&);
    /**
     * Commits the move.
     *
     * Handles switching the sides, checking the winner, etc.*/
		void commitMove();

    /**
     * Forward check. 
     *
     * No board update.
     */
    bool checkKillForward(square_t from, square_t to, KillInfo* killInfo);

    /**
     * Kill checker.
     *
     * Checks whether kill is happening in the vicinity of given square.
     */
    bool checkKill(square_t square);

    /**
     * Performs kill.
     *
     * Performs operation connected to kill - board update, rabbits num update, etc.
     */
    void performKill(square_t trapPos);

    /**
     * Calculater signature for one step forward. 
     */
    u64 calcAfterStepSignature(Step& step) const;

    /**
     * Generates all (syntatically) legal steps from the position.
     *
     * Doesn't check 3 - repetitions rule / virtual pass. 
     * Pass move is always generated as a last move.
     * */
		int generateAllSteps(player_t, StepArray&) const;

    /**
     * Repetition check.
     *
     * Takes step array and filters out illegal moves considering:
     * 1) virtual pass repetition
     * 2) 3 moves same position repetition
     * */
    int filterRepetitions(StepArray&, int ) const;

    inline bool stepIsVirtualPass( Step& ) const;
    inline bool stepIsThirdRepetition( Step& ) const;

		inline bool hasFriend(square_t) const;
		inline bool hasTwoFriends(square_t, player_t) const;
		inline bool hasStrongerEnemy(square_t) const;
		inline bool isFrozen(square_t) const;

    bool		  isEmpty();
		uint			getAllStepsNum(uint);
		uint			getStepCount();
    int       getPreMoveSignature();
    player_t  getPlayerToMove();
		player_t	getWinner();
		
    void setSquare(square_t, player_t, piece_t);
    void clearSquare(square_t);

		string toString();
		string allStepsToString();
    void dump();
		void dumpAllSteps();

		void testPieceArray();
};

#endif
