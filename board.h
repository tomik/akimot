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
#include <stack>
#include <list>
#include <bitset>

using std::queue;
using std::stack;
using std::list;
using std::bitset;

#include "utils.h"
#include "hash.h"
#include "config.h"

//max number of pieces per player 
#define MAX_PIECES  16      
#define MAX_STEPS  200

#define STEPS_IN_MOVE 4

typedef unsigned long long u64;
//zobrist base table for signature creating 

#define BIS_PLAYER(player) (player == BGOLD || player == BSILVER)

#define BNORTH 8
#define BSOUTH -8
#define BEAST 1
#define BWEST -1

#define BGOLD        0
#define BSILVER      1
#define BNO_PLAYER   2

#define BEMPTY       0
#define BRABBIT      1
#define BCAT         2
#define BDOG         3
#define BHORSE       4
#define BCAMEL       5
#define BELEPHANT    6

#define BOPP(player) (1 - player)				 //opponent
#define BIT_ON(n) (1ULL << (n))          //creates empty board with one bit set on n

#define BIT_LEN 64
#define BSQUARE_NUM 64
#define BSIDE_SIZE 8

#define STEP_PASS     0
#define STEP_SINGLE   1
#define STEP_PUSH     2
#define STEP_PULL     3
//no step is possible ( not even pass ! - position repetition )
#define STEP_NULL     4   
#define PLAYER_NUM    2

// what row is a square in?  1 = bottom, 8 = top
#define ROW(square) (square/10) 
// what column is a square in?  1 = left (a), 8 = right (h)
#define COL(square) (square%10) 

#define SQUARE_TO_INDEX_64(square) (8*(ROW(square)-1) + (COL(square)-1))
#define INDEX_64_TO_SQUARE(index) ((10 * (index/8 + 1)) + ((index % 8) + 1 ))

#define BSQUARE_DISTANCE(s1, s2) (abs(s1/8 - s2/8) + abs(s1%8 - s2%8))

#define OLD_PLAYER_TO_NEW(player) (player == 8 ? BGOLD : BSILVER)

typedef int bplayer_t;
typedef int bpiece_t;
typedef int bcoord_t;

typedef uint stepType_t;


namespace bits{
  #define NOT_A_FILE  0xfefefefefefefefeULL
  #define NOT_H_FILE  0x7f7f7f7f7f7f7f7fULL
  #define NOT_1_RANK  0x00ffffffffffffffULL
  #define NOT_8_RANK  0xffffffffffffff00ULL
  #define MSB         0x8000000000000000ULL
  #define TRAPS       0x0000000000240001ULL
  #define FPARITY     0x5555555555555555ULL
  extern u64 zobrist[2][7][64];     

  /**
   * Init zobrist table.
   *
   * Fills zobrist table with random u64 numbers. 
   * Zobrist algorithm is used for making position signatures.
   */
  void initZobrist();

  extern u64   winRank[2]; 
  extern u64 stepOffset_[2][7][64]; 
  /**
   * Step offset builder. 
   */
  void buildStepOffsets();

  /**
   * String to bits conversion. 
   */
  u64 str2bits(string str);

  /**
   * Left index bit. 
   *
   * Returns highest order bit and shifts the given bitset !  
   */
  int lix(u64& b);

  /**
   * Simpler lix. 
   */
  int lixslow(u64& v);

  /**
   * Bit getter. 
   */
  bool getBit(const u64& b, int n);

  /**
   * Fancy print. 
   */
  ostream& print(ostream& o, const u64& b);

  /**
   * Mask of all neighbors of given bitset.
   * */
  u64 neighbors(u64);

  /**
   * Mask of neighbors for one piece. 
   *
   * @param coord Piece coordinate.
   */
  u64 neighborsOne(bcoord_t coord);

  /**
   * Mask of sphere.
   */
  u64 sphere(int center, int radius);

  /**
   * Mask of circle. 
   */
  u64 circle(int center, int radius);
}


class BBoard;
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
    void add(bcoord_t);

    /**
     * Delte piece from array. 
     *
     * Goes through array, when found deletes item 
     * and replaces empty space with item from the end.
     */
    void del(bcoord_t);

    /**
     * Remove all elements. 
     */
    void clear();

    uint getLen() const;
    bcoord_t operator[](uint) const;

    /**
     * Representation.
     */
    string toString() const;
    
    /**
     * Random square in piece array.
     */
    bcoord_t getRandom() const;

  private:
    bcoord_t elems[MAX_PIECES];      
    uint len;
};


/**
 * Information about a kill in the trap. 
 *
 * Holds information on player, piece, square(trap).
 */
class KillInfo 
{
  public:
    KillInfo();
    KillInfo(bplayer_t player, bpiece_t piece, bcoord_t coord);
    void setValues(bplayer_t player, bpiece_t piece, bcoord_t coord);
    const string toString() const;

  private:
    bool     active_;
    bplayer_t player_;
    bpiece_t  piece_;
    bcoord_t coord_;
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
		Step();
		Step(stepType_t, bplayer_t);
    Step(stepType_t, bplayer_t, bpiece_t, bcoord_t, bcoord_t);
    Step(stepType_t, bplayer_t, bpiece_t, bcoord_t, bcoord_t, bpiece_t, bcoord_t, bcoord_t);

    bplayer_t getPlayer() const;
    bool isPass() const;
    bool isSingleStep() const;
    bool isPushPull() const;

    /**
     * Actual step count.
     *
     * Single == 1, push/pull == 2, NULL/PASS == 0 
     */
    int count() const;

    /**
     * Temporary converter to new steps.
     *
     * For printing the old-like steps (from board). Now steps 
     * conforming to BBoard.
     */
    Step toNew() const;

    /**
     * Checks (pseudo)inversion to given step.
     * 
     * return True if steps have same type, inversed from/to oppfrom/oppto.
     *        false otherwise.
     */
    bool inversed(const Step&) const;

    /**
     * Checks whether step moves any piece. 
     *
     * @return false if step is STEP_PASS/STEP_NULL otherwise true.
     */
		bool pieceMoved() const;
		bool operator== (const Step&) const;
    bool operator<(const Step&) const;

    void setValues( stepType_t, bplayer_t, bpiece_t, bcoord_t, bcoord_t );
    void setValues( stepType_t, bplayer_t, bpiece_t, bcoord_t, bcoord_t, 
                    bpiece_t, bcoord_t, bcoord_t );
    /**
     * Step string representation.
     */
    string toString() const;

	protected:
    stepType_t    stepType_;    
    bplayer_t      player_;      
    bpiece_t       piece_;  
    bcoord_t      from_;     
    bcoord_t      to_;        

    bpiece_t       oppPiece_;  
    bcoord_t      oppFrom_;
    bcoord_t      oppTo_;

    friend class  Board;
    friend class  BBoard;
  
  private: 
    /**
     * Handles print of step of one piece. 
     *
     * Push/pull move calls this method twice.
     */
    const string oneSteptoString(bplayer_t, bpiece_t, bcoord_t, bcoord_t) const;
  
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
    StepWithKills(Step step, const BBoard* board);

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
    void addKills(const BBoard* board);

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

    /**
     * Step count in move getter.
     */
    int getStepCount() const;
    
  private:
    StepList stepList_;
};

/**
 * Record action as parsed from the game record (file).
 *
 * Potential values are: placement in the beginning (e.g. RA1), 
 * normal step (e.g. RA1n), trap fall(e.g. Rc3x)
 */
enum recordAction_e {ACTION_PLACEMENT, ACTION_STEP, ACTION_TRAP_FALL, ACTION_ERROR}; 

typedef pair<bplayer_t, bpiece_t>  PiecePair;

//steps
typedef Step  StepArray[MAX_STEPS];
//heuristics for steps (separated because used VERY LITTLE)
typedef float  HeurArray[MAX_STEPS];

class BBoard
{

  public:

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
		Step findMCstep();


    /**
     * Move generation in Monte Carlo playouts. 
     */
    void findMCmoveAndMake(); 
    
   /**
     * Equality operator.
     *
     * Check signatures and moveCount.
     * Right now doesn't check pieceArrays and other stuff.
     */
		bool operator== (const BBoard& board) const;

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
     * Wrapper around makeMoveNoCommit with commit() added.
     */
		void makeMove(const Move& move);

    /**
     *  Wraper for makeStep with commiting.
     *
     *  Performs makestep on given step. 
     *  If the move is over it updatesWinner and commits.
     *  @param step given step 
     *  @return true if commited false otherwise
     */
		bool makeStepTryCommitMove(const Step&);

    /**
     * Commits the move.
     *
     * Handles switching the sides, updating preMoveSignature.
     */
		void commitMove();

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
     * Forward check. 
     *
     * Checking whether step defined by from, to is causing a kill 
     * i.e. suicide, being pushed/pulled to trap, stops protecting piece on the trap.
     * This function causes no board update and is used in class StepWithKills. 
     */
    bool checkKillForward(bcoord_t from, bcoord_t to, KillInfo* killInfo=NULL) const;

    /**
     * Calculater signature for one step forward. 
     */
    u64 calcAfterStepSignature(const Step& step) const;

    /**
     * Step generation. 
     *
     * Generates all (syntatically) legal steps from the position EXCEPT from Pass.
     * Doesn't check 3 - repetitions rule / virtual pass. 
     */
		int generateAllStepsNoPass(bplayer_t, StepArray&) const;

    /**
     * Knowledge for steps. 
     *
     * Applies knowledge to given stepArray and fills heuristic array 
     *  heurs will have the same size as steps.
     * @param steps - Given step array for heuristics generation.
     * @param stepsNum - Length of steps.
     */
    void getHeuristics(const StepArray& steps, uint stepsNum, HeurArray& heurs) const;

    /**
     * Continue check.
     *
     * @param move to be made from given position.
     * @return True if after move player can still play ( <4 steps ),
     *              otherwise false.
     */
    bool canContinue(const Move& move) const;

    /**
     * Last step getter.
     */
    Step lastStep() const;

    BBoard(const Board& b);

		uint			getStepCount();

    bool makeStepTryCommit(Step& step);

    void commit();

    void updateWinner();

    int genSteps(bplayer_t player, StepArray& steps) const;

    /**
     * Full goal check.
     *
     * Done through limited full width search. 
     */
    bool goalCheck(bplayer_t player, int stepLimit, Move* move=NULL);

    /**
     * String representation.
     */
		string toString() const;

    /**
     * Print of move with kills.
     */
    string MovetoStringWithKills(const Move& m) const;

    /**
     * Signature getter.
     */
    u64 getSignature() const;

    /**
     * Winner getter. 
     */
    bplayer_t getWinner() const;

    /**
     * There is a winner. 
     */
    bplayer_t gameOver() const; 

    /**
     * Actual player getter.
     */
    bplayer_t getPlayerToMove() const;

    /**
     * Next step's player getter.
     */
    bplayer_t getPlayerToMoveAfterStep(const Step& step) const;


  private: 

    BBoard(){};

    /**
     * Reachability check.
     *
     * Used in goalCheck. 
     */
    int reachability(int from, int to, bplayer_t player, 
                    int limit, int used, Move * move);

    /**
     * Step generation for one.
     * //TODO optimize/simplify
     *
     * Wrapper around previous function when not used from generate all. 
     *
     * @return stepsNum
     */
    int genStepsForOne(bcoord_t coord, bplayer_t player,
                        StepArray& steps) const;
    /**
     * Step generation for one piece. 
     *
     * @param coord  Steps are generated for piece at this coord.
     * @param player Player to generate steps for. 
     * @param piece  Piece ( for time save). 
     * @param steps  Steps are stored in this array.
     * @param stepsnum Size of step array.
     */
    void genStepsForOneTuned(bcoord_t coord, bplayer_t player, bpiece_t piece, 
                              StepArray& steps, int& stepsNum, u64 victims) const;

    /**
     * Calculates not frozen mask.
     */
    u64 calcMovable(bplayer_t player) const;

    /**
     * Calculates weaker pieces.
     *
     * @param player Calculation is done for this player. 
     * @param weaker This array is filled. 
     */
    void calcWeaker(bplayer_t player, u64 (&weaker)[7]) const;

    inline void	setSquare(bcoord_t, bplayer_t, bpiece_t);
    inline void	delSquare(bcoord_t, bplayer_t);											
    inline void	delSquare(bcoord_t, bplayer_t, bpiece_t);											

    inline bpiece_t	getPiece(bcoord_t, bplayer_t) const;
    inline bplayer_t getPlayer(bcoord_t) const;

    /**
     * General init - nullifies variables.
     *
     * @param newGame true -> inits static variables for new game
     * e.g. -> zobrist table, thirdRepetition table, etc.
     */
    void  init(bool newGame=false);

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
    bplayer_t sideCharToPlayer(char side) const; 

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
    recordAction_e  parseRecordActionToken(const string& token, bplayer_t& player, 
                                           bpiece_t& piece, bcoord_t& from, bcoord_t& to); 

    /**
    * Parsing piece char (e.g. R,H,c,m, ... ) 
    * 
    * @return pair: (player, piece) belonging to given char.
    * Throws an exception when unknown pieceChar encountered.
    */
    bool parsePieceChar(char pieceChar, bplayer_t& player, bpiece_t& piece); 


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
     * @param step Step to be made (kills are resolved as well).
     * @param update If true - board structure is updated (added
     * steps, frozenBoard update). 
     */
		void makeStep(const Step& step);

    //TODO move evaluation methods to eval ???
    
    /**
     * Knowledge integration into steps. 
     *
     * @param steps Generated steps - 
     *    some is selected from these according to "knowledge".
     * @param stepsNum Size of steps.
     */
    Step chooseStepWithKnowledge(StepArray& steps, uint stepsNum) const;

    /**
     * Evaluates one step.
     *
     * In this play game knowledge is applied.
     */
    float evaluateStep(const Step& step) const;

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
    bool stepIsThirdRepetition(const Step& ) const;

		uint			getStepCount() const;
    u64       getPreMoveSignature() const;

    u64       bitboard_[2][7];

    StepArray     stepArray;
    int          stepArrayLen;
  
    static bool       classInit;
    static ThirdRep*  thirdRep_;

    //signature of position from when the current move started
    u64 signature_;            
    //position signature - for hash tables, corectness checks, etc. 
    u64 preMoveSignature_;     

    /**Last made step.*/
    Step lastStep_;
		// move consists of up to 4 steps ( push/pull  counting for 2 ),
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;

    bplayer_t toMove_;
		bplayer_t winner_;

    friend class Eval;
};
