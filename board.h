/** 
 *  @file board.h
 *
 *  @brief Board interface.
 *  @full Board representation with all its nuiances is defined here. 
 *  Main pillar is class Board itself representing board and its manipulation 
 *  along with move generation, verification and playing.
 */

#pragma once

#include <queue>
#include <list>

using std::queue;
using std::list;

#include "utils.h"
#include "hash.h"

#define STEPS_IN_MOVE 4
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

#define TOP_ROW 8
#define BOTTOM_ROW 1
#define LEFT_COL 1 
#define RIGHT_COL 8

#define OWNER(square) (square & OWNER_MASK) 
#define PIECE(square) (square & PIECE_MASK) 
#define OPP(player) ((16 - player) + 8)
//#define OPP(player) (player == GOLD ? SILVER : GOLD )

// what row is a square in?  1 = bottom, 8 = top
#define ROW(square) (square/10) 
// what column is a square in?  1 = left (a), 8 = right (h)
#define COL(square) (square%10) 

//GOLD ~ 0, SILVER ~ 1
////((16-player)/8)	
#define PLAYER_TO_INDEX(player)	(player == GOLD ? 0 : 1 )
#define INDEX_TO_PLAYER(index)  (uint) (16-8*index)	

#define SQUARE_DISTANCE(s1, s2) (abs(s1/10 - s2/10) + abs(s1%10 - s2%10))

#define IS_TRAP(index) (index == 33 || index == 36 || index == 63 || index == 66 ) 
#define IS_PLAYER(square) (OWNER(square) == GOLD || OWNER(square) == SILVER )

#define PLAYER_NUM    2
#define PIECE_NUM     7
#define SQUARE_NUM    100

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3
#define STEP_NO_STEP  4   //no step is possible ( not even pass ! - position repetition )

/**empty square in the flag board*/
#define FLAG_BOARD_EMPTY -1

extern const int direction[4];
extern const int trap[4];

typedef uint player_t;
typedef int  square_t;
typedef uint piece_t;		
typedef uint stepType_t;
typedef int FlagBoard[SQUARE_NUM];

class Board;
class Eval;

/**
 * Array-like structure to hold pieces.
 *
 * This class is used to hold squares where pieces for 
 * one of the players are positioned, order of pieces is not fixed, 
 * just their presence is ensured.
 */
class PieceArray
{ 
  public: 
    PieceArray();

    /**
     * Add piece position to array.
     *
     * Adds to the end in constant time. 
     */
    void add(square_t);

    /**
     * Delte piece from array. 
     *
     * Goes through array, when found deletes item 
     * and replaces empty space with item from the end.
     */
    void del(square_t);

    /**
     * Remove all elements. 
     */
    void clear();

    string toString() const;
    uint getLen() const;
    square_t operator[](uint) const;

  private:
    square_t elems[MAX_PIECES];      
    uint len;
};


/**
 * Information about a kill in the trep. 
 *
 * Holds information on player, piece, square(trap).
 */
class KillInfo 
{
  public:
    KillInfo();
    KillInfo( player_t player, piece_t piece, square_t square);
    void setValues( player_t player, piece_t piece, square_t square);
    const string toString() const;

  private:
    bool     active_;
    player_t player_;
    piece_t  piece_;
    square_t square_;
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
  public:
		Step(){};
		Step(stepType_t, player_t);
    Step(stepType_t, player_t, piece_t, square_t, square_t);
    Step(stepType_t, player_t, piece_t, square_t, square_t, piece_t, square_t, square_t);

    player_t getPlayer() const;
    bool isPass() const;
    bool isSingleStep() const;
    bool isPushPull() const;

    /**
     * Checks whether step moves any piece. 
     *
     * @return false if step is STEP_PASS/STEP_NO_STEP otherwise true.
     */
		bool pieceMoved() const;
		bool operator== (const Step&) const;

    //TODO inline
    void setValues( stepType_t, player_t, piece_t, square_t, square_t );
    void setValues( stepType_t, player_t, piece_t, square_t, square_t, 
                    piece_t, square_t, square_t );
    /**
     * Step string representation.
     *
     * Might be overriden in ancestors.
     */
    string toString() const;

	protected:
    stepType_t    stepType_;    //! defines what kind of step this is i.e. PASS, SINGLE, PUSH, PULL
    player_t      player_;      
    piece_t       piece_;  
    square_t      from_;     
    square_t      to_;        

    piece_t       oppPiece_;  //opponent piece/from/to values used for pushing, pulling 
    square_t      oppFrom_;
    square_t      oppTo_;

    friend class  Board;
  
  private: 
    /**
     * Handles print of step of one piece. 
     *
     * Push/pull move calls this method twice.
     */
    const string oneSteptoString(player_t, piece_t, square_t, square_t) const;
    void dump(); 
  
};


/**
 * Step with kills. 
 *
 * Extension of Step with KillInfo ( 1-2 ).
 * Used for printing of step with kill information.
 */
class StepWithKills: public Step
{
  public:
    StepWithKills(Step step, const Board* board);

    /**
     * Print of step.
     *
     * Might override virtual method in predecessor.
     */
    string toString() const;

  private:
    StepWithKills();

    /**
     * Fills KillInfo. 
     *
     * Checks forward kill of the step and 
     * adds kill if neccessary.
     */
    void addKills(const Board* board);

    KillInfo kills[2];
};


typedef list<Step> StepList;
typedef StepList::iterator StepListIter;

/**
 * Move = list of steps (up to STEP_IN_MOVE).
 *
 * Accepts Steps as well as StepWithKills.
 */

class Move
{
  public:
    string toString();
    string toStringWithKills(const Board* board);

    /**
     * Appends step to the move.
     *
     * move: A->B; step : C
     * => move: A->B->C
     */
    void appendStep(Step);
    
    /**
     * Prepends step to the move.
     *
     * move: A->B; step : C
     * => move: C->A->B
     */
    void prependStep(Step);

    /**
     * Append steplist to the move.
     *
     * Takes steps from given stepList 
     * and appends them to the move.
     */
    void appendStepList(StepList);

    /**
     * Steps getter.
     *
     * Function returns list of steps.
     */
    StepList getStepList() const;
    
  private:
    StepList stepList_;
};

/**
 * Record action as parsed from the game record (file).
 *
 * Potential values are: placement in the beginning (e.g. RA1), 
 * normal step (e.g. RA1n), trap fall(e.g. Rc3x)
 */
enum recordAction_e {ACTION_PLACEMENT, ACTION_STEP, ACTION_TRAP_FALL}; 

typedef pair<player_t, piece_t>  PiecePair;

typedef uint board_t[SQUARE_NUM];

typedef Step  StepArray[MAX_STEPS];

/**
 * Board representation.
 *
 * Crucial building block of the whole program. 
 */
class Board
{
  public:
    Board();

    /**
     * Public wrapper around init(newGame=true). 
     */
    void initNewGame();

    /**
    * Inits board from a game record in file.
    *
    * @return true if initialization went right 
    * otherwise false
    */
    bool  initFromRecord(const char* fn); 

    /**
    * Inits board from a position in file.
    * 
    * Wrapper around initFromPositionStream.
    */
    bool  initFromPosition(const char* fn); 

    /**
     * Inits board from compact string.
     * 
     * Compact string is in form PLAYER_CHAR [position in lines]
     * e.g.:w [rrr r rrrdd  e                   ED     RhMH  C   mC   RRRR c RR]
     * @return true if initialization went right 
     * otherwise false
     */
    bool  initFromPositionCompactString(const string& s); 

    /**
     * Step generation for Monte Carlo playouts.
     *
     * Generates (random) step with some restrictions ( i.e. no pass in the first step ).
     * Random step is generated either by calling findRandomStep method or ( if the former 
     * one is unsuccessfull ) by generating all steps and selecting one in random.
     */
		Step findStepToPlay();
    
   /**
     * Equality operator.
     *
     * Check signatures and moveCount.
     * Right now doesn't check pieceArrays and other stuff.
     */
		bool operator== (const Board& board) const;
    
    //TODO from here till private: restructuralize in .cpp
    
    /**
     *  Wraper for makeStep with commiting.
     *
     *  Performs makestep on given step. 
     *  If the move is over it updatesWinner and commits.
     *  @param step given step 
     *  @return true if commited false otherwise
     */
		bool makeStepTryCommitMove(Step&);

     /**
     * Performs whole move. 
     *
     * There is no control whether move is legal.
     * @param move String representation of the move.
     */
		void makeMove(const string& move);

     /**
     * Making whole move.
     *
     * Retrieves the step list from move, 
     * performs them, does no commit.
     */
		void makeMoveNoCommit(const Move& move);

     /**
     * Making whole move.
     *
     * Wrapper around makeMoveNoCommit with commit() added.
     */
		void makeMove(const Move& move);

    /**
     * Commits the move.
     *
     * Handles switching the sides, updating preMoveSignature.
     */
		void commitMove();

    /**
     * Updates winner of the game.
     *
     * Checks winner according to reaching goal, opponent has 0 rabbits.
     */
    void updateWinner();

    /**
     * Quick check for goal.
     *
     * Checking is unreliable ! 
     * (looks only for direct goal score without help of tother pieces).
     * Done by wave algorithm from the goal line for given player. 
     *
     * @return True if knows goal can be reached,   
     *         false otherwise.
     */
    bool quickGoalCheck(player_t player, int stepLimit, Move* move=NULL) const;

    /**
     * Quick check for goal.
     *
     * Wrapper around previous function with 
     * player = player to move in current position
     * stepLimit = steps left for player to move in current position
     */
     bool quickGoalCheck(Move* move=NULL) const;

     /**
      * Traceback on flag board.
      *
      * After successfull goal check, this method determines the 
      * move that scores the goal. 
      */
     Move tracebackFlagBoard(const FlagBoard& flagBoard, int win_square, player_t player) const;

    /**
     * Repetition check.
     *
     * Takes step array and filters out illegal moves considering:
     * 1) virtual pass repetition
     * 2) 3 moves same position repetition
     * */
    int filterRepetitions(StepArray&, int ) const;

    /**
     * Setup pieces phase test.
     *
     * @return True, if it's first move and there are no pieces 
     * for player to move, otherwise false.
     */
    bool  isSetupPhase() const;

    /**
     * Actual player getter.
     */
    player_t  getPlayerToMove() const;

    /**
     * Next step's player getter.
     */
    player_t  getPlayerToMoveAfterStep(const Step& step) const;

    /**
     * String representation of board.
     */
		string toString() const;

    /**
     * Forward check. 
     *
     * Checking whether step defined by from, to is causing a kill 
     * i.e. suicide, being pushed/pulled to trap, stops protecting piece on the trap.
     * This function causes no board update and is used in class StepWithKills. 
     */
    bool checkKillForward(square_t from, square_t to, KillInfo* killInfo) const;

    /**
     * Calculater signature for one step forward. 
     */
    u64 calcAfterStepSignature(const Step& step) const;

    /**
     * Generates all (syntatically) legal steps from the position.
     *
     * Doesn't check 3 - repetitions rule / virtual pass. 
     * Pass move is always generated as a last move.
     * */
		int generateAllSteps(player_t, StepArray&) const;

    u64       getSignature() const;
    player_t	getWinner() const;

  private:
    /**
     * General init - nullifies variables.
     *
     * @param newGame true -> inits static variables for new game
     * e.g. -> zobrist table, thirdRepetition table, etc.
     */
    void  init(bool newGame=false);

    /**
     * Inits board from position stream.
     * 
     * @return true if initialization went right 
     * otherwise false
     */
    bool  initFromPositionStream(istream& ss); 

    /**
     * After load from position actions. 
     *
     * Signature gest created. PieceArray is filled.
     */
    void afterPositionLoad();

    /**
     * Side character to player.
     *
     * Maps 'w','g' -> gold ; 'b', 's' -> silver.
     */
    player_t sideCharToPlayer(char side) const; 

    /**
    * Parsing single token for init from game record.
    *
    * @param token given string token (e.g. Ra1n)
    * @param player player parsed from the token
    * @param piece  piece parsed from the token
    * @param from position parsed from the token
    * @param to (optional) new position parsed from the token (only if it is a step)
    * @return what recordAction was parsed (i.e. placement in the beginning,...)
    */
    recordAction_e  parseRecordActionToken(const string& token, player_t& player, 
                                           piece_t& piece, square_t& from, square_t& to); 

    /**
    * Parsing piece char (e.g. R,H,c,m, ... ) 
    * 
    * @return pair: (player, piece) belonging to given char.
    * Throws an exception when unknown pieceChar encountered.
    */
    PiecePair parsePieceChar(char pieceChar); 

    /**
     * Init zobrist table.
     *
     * Fills zobrist table with random u64 numbers. 
     * Zobrist algorithm is used for making position signatures.
     */
    void  initZobrist() const;

    /**
     * Take (hopefully) unique signature of position - u64 number. 
     *
     * Done by XOR-ing signatures for all pieces on the board.
     */
    void  makeSignature();

    /**
     * Making the step.
     *
     * One of the crucial methods in the boardstructure.
     * Takes given step and performs it. Updates board structure and resolves kills.
     */
		void makeStep(Step& step);

    /**
     * "Random" step generator.
     *
     * Generates random step ( random type, from, to, ... ) and returns it if it's correctness
     * is verified (might try to generate the step more times).
     */
		bool findRandomStep(Step&) const;

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
     * Virtual pass check.
     *
     * @param step - expected to be last step in current move
     * @return true if position after given step is same as in the 
     * beginning of the move, otherwise false. 
     */
    bool stepIsVirtualPass( Step& ) const;

    /**
     * Third repetition check.
     * 
     * @param step - expected to be last step in current move
     * @return true if position after given step leads to a third repetition
     * according to thirdRep object.
     */
    bool stepIsThirdRepetition( Step& ) const;

    /**
     * Has a friend test.
     *
     * Piece on given square has a friend test.
     * Used in trap kill check.
     */
		bool hasFriend(square_t) const;

    /**
     * Has a friend test.
     *
     * Variant for forward tests.
     * Color must be supplied as well.
     */
		inline bool hasFriend(square_t, player_t owner) const;

    /**
     * Two friends test. 
     *
     * This is used for forward tests (without actually moveing pieces).
     * Therefore color of player must be supplied as well.
     */
		bool hasTwoFriends(square_t, player_t) const;

    /**
     * Has stronger enemy test. 
     *
     * Used for checking a trap kill. 
     */
		bool hasStrongerEnemy(square_t) const;

    /**
     * Has stronger enemy test. 
     *
     * Variant for forward tests. 
     * Color and piece must be supplied as well  
     */
		inline bool hasStrongerEnemy(square_t, player_t owner, piece_t piece) const;

    /**
     * Frozen check.
     *
     * Checks whether piece at given square is frozen == !hasFriend and hasStrongerEnemy
     */
		bool isFrozen(square_t) const;

		uint			getAllStepsNum(uint) const;
		uint			getStepCount() const;
    u64       getPreMoveSignature() const;
		
    /**
     * Sets square and updates signature. 
     */
    void setSquare(square_t, player_t, piece_t);

    /**
     * Clears square and update signature.
     */
    void clearSquare(square_t);

		string allStepsToString() const;
		void dumpAllSteps() const;
    void dump() const;


		void testPieceArray();

    //Attributes

    static bool       classInit;
    static u64        zobrist[PLAYER_NUM][PIECE_NUM][SQUARE_NUM];     //zobrist base table for signature creating 
    static ThirdRep*  thirdRep_;

		board_t					board_;					//actual pieces are stored here 
		bool					frozenBoard_[SQUARE_NUM];			//keep information on frozen pieces, false == notfrozen, true == frozen

    PieceArray    pieceArray[2];  
    uint          rabbitsNum[2];        //kept number of rabbits for each player - for quick check on rabbitsNum != 0 
  
    StepArray     stepArray[2];
    uint          stepArrayLen[2];

    u64           signature_;            //position signature - for hash tables, corectness checks, etc. 
    u64           preMoveSignature_;     //signature of position from when the current move started

		// move consists of up to 4 steps ( push/pull  counting for 2 ),
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;

    player_t toMove_;
    uint     toMoveIndex_;    //0 == GOLD, 1 == SILVER
		player_t winner_;

    friend class Eval;

};

