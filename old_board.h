#pragma once 

#include "board.h"

#define OB_EMPTY_SQUARE 0x0U
#define OB_EMPTY 0x0U
#define OB_OFF_BOARD_SQUARE 0x9FU
//#define OFF_BOARD 0x18U
#define OB_GOLD 0x10U
#define OB_SILVER 0x8U
#define OB_PIECE_OFF_BOARD 0x7U
#define OB_PIECE_ELEPHANT 0x6U
#define OB_PIECE_CAMEL 0x5U
#define OB_PIECE_HORSE 0x4U
#define OB_PIECE_DOG 0x3U
#define OB_PIECE_CAT 0x2U
#define OB_PIECE_RABBIT 0x1U
#define OB_PIECE_EMPTY 0x0U
#define OB_PIECE_MASK 0x7U
#define OB_OWNER_MASK 0x18U
#define OB_NORTH 10
#define OB_SOUTH -10
#define OB_EAST 1
#define OB_WEST -1

#define TOP_ROW 8
#define BOTTOM_ROW 1
#define LEFT_COL 1 
#define RIGHT_COL 8

#define OB_OWNER(square) (square & OB_OWNER_MASK) 
#define OB_PIECE(square) (square & OB_PIECE_MASK) 
#define OB_OPP(player) ((16 - player) + 8)
//#define OB_OPP(player) (player == OB_GOLD ? OB_SILVER : OB_GOLD )


//OB_GOLD ~ 0, OB_SILVER ~ 1
////((16-player)/8)	
#define PLAYER_TO_INDEX(player)	(player == OB_GOLD ? 0 : 1 )
#define INDEX_TO_PLAYER(index)  (uint) (16-8*index)	

#define OB_SQUARE_DISTANCE(s1, s2) (abs(s1/10 - s2/10) + abs(s1%10 - s2%10))

#define OB_IS_TRAP(index) (index == 33 || index == 36 || index == 63 || index == 66 ) 
#define OB_IS_PLAYER(square) (OB_OWNER(square) == OB_GOLD || OB_OWNER(square) == OB_SILVER )

#define OB_PIECE_NUM     7
#define OB_SQUARE_NUM    100

extern const int direction[4];
extern const int rabbitForward[2];
extern const int rabbitWinRow[2];
extern const int trap[4];

typedef uint ob_player_t;
typedef int  ob_square_t;
typedef uint ob_piece_t;		
typedef int FlagBoard[OB_SQUARE_NUM];
typedef uint board_t[OB_SQUARE_NUM];

typedef pair<ob_player_t, ob_piece_t> PiecePair;

#define FLAG_BOARD_EMPTY -1

/**
 * Board representation.
 *
 * Crucial building block of the whole program. 
 */
class OB_Board
{
  public:
    OB_Board();

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
		bool operator== (const OB_Board& board) const;
    
    //TODO from here till private: restructuralize in .cpp
    
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
     * (looks only for direct goal score without help of other pieces).
     * Done by wave algorithm from the goal line for given player. 
     *
     * @return True if knows goal can be reached,   
     *         false otherwise.
     */
    bool quickGoalCheck(ob_player_t player, int stepLimit, Move* move=NULL) const;

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
     Move tracebackFlagBoard(const FlagBoard& flagBoard, int win_square, ob_player_t player) const;

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
    ob_player_t  getPlayerToMove() const;

    /**
     * Next step's player getter.
     */
    ob_player_t  getPlayerToMoveAfterStep(const Step& step) const;

    /**
     * String representation of board.
     */
		string toString() const;

    /**
     * Print of move with kills.
     */
    string MovetoStringWithKills(const Move& m) const;

    /**
     * Forward check. 
     *
     * Checking whether step defined by from, to is causing a kill 
     * i.e. suicide, being pushed/pulled to trap, stops protecting piece on the trap.
     * This function causes no board update and is used in class StepWithKills. 
     */
    bool checkKillForward(ob_square_t from, ob_square_t to, KillInfo* killInfo=NULL) const;

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
		int generateAllStepsNoPass(ob_player_t, StepArray&) const;

    /**
     * Step generation. 
     *
     * Wrapper around previous function with added step Pass.
     */
		int generateAllSteps(ob_player_t, StepArray&) const;

    /**
     * Step generation for one piece. 
     *
     * @param square Steps are generated for this piece.
     * @param stepArray Steps are stored in this array.
     * @param stepsnum Size of step array.
     */
    void generateStepsForPiece(
              ob_square_t square, StepArray& steps, uint& stepsNum) const;

    /**
     * Knowledge for steps. 
     *
     * Applies knowledge to given stepArray and fills heuristic array 
     *  heurs will have the same size as steps.
     * @param steps - Given step array for heuristics generation.
     * @param stepsNum - Length of steps.
     */
    void getHeuristics(const StepArray& steps, uint stepsNum, HeurArray& heurs) const;

    u64       getSignature() const;
    ob_player_t	getWinner() const;

    /**
     * There is a winner. 
     */
    ob_player_t gameOver() const; 

    /**
     * Continue check.
     *
     * @param move to be made from given position.
     * @return True if after move player can still play ( <4 steps ),
     *              otherwise false.
     */
    bool canContinue(const Move& move) const;

    /**
     * Checks pass validity.
     *
     * stepsNum must be > 0 and third repetition is not allowed
     */
    bool canPass() const;

    /**
     * Last step getter.
     */
    Step lastStep() const;

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
    ob_player_t sideCharToPlayer(char side) const; 

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
    recordAction_e  parseRecordActionToken(const string& token, ob_player_t& player, 
                                           ob_piece_t& piece, ob_square_t& from, ob_square_t& to); 

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
     * @param step Step to be made (kills are resolved as well).
     * @param update If true - board structure is updated (added
     * steps, frozenBoard update). 
     */
		void makeStep(const Step& step);

    /**
     * "Random" step generator.
     *
     * Generates random step ( random type, from, to, ... ) and returns it if it's correctness
     * is verified (might try to generate the step more times).
     */
		bool findRandomStep(Step&) const;

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
     * Kill checker.
     *
     * Checks whether kill is happening in the vicinity of given square.
     */
    bool checkKill(ob_square_t square);

    /**
     * Performs kill.
     *
     * Performs operation connected to kill - board update, rabbits num update, etc.
     */
    void performKill(ob_square_t trapPos);

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

    /**
     * Has a friend test.
     *
     * Piece on given square has a friend test.
     * Used in trap kill check.
     */
		bool hasFriend(ob_square_t) const;

    /**
     * Has a friend test.
     *
     * Variant for forward tests.
     * Color must be supplied as well.
     */
		inline bool hasFriend(ob_square_t, ob_player_t owner) const;

    /**
     * Two friends test. 
     *
     * This is used for forward tests (without actually moveing pieces).
     * Therefore color of player must be supplied as well.
     */
		bool hasTwoFriends(ob_square_t, ob_player_t) const;

    /**
     * Has stronger enemy test. 
     *
     * Used for checking a trap kill. 
     */
		bool hasStrongerEnemy(ob_square_t) const;

    /**
     * Has stronger enemy test. 
     *
     * Variant for forward tests. 
     * Color and piece must be supplied as well  
     */
		inline bool hasStrongerEnemy(ob_square_t, ob_player_t owner, ob_piece_t piece) const;

    /**
     * Frozen check.
     *
     * Checks whether piece at given square is frozen == !hasFriend and hasStrongerEnemy
     */
		bool isFrozen(ob_square_t) const;

		uint			getStepCount() const;
    u64       getPreMoveSignature() const;
		
    /**
     * Sets square and updates signature. 
     */
    void setSquare(ob_square_t, ob_player_t, ob_piece_t);

    /**
     * Clears square and update signature.
     */
    void clearSquare(ob_square_t);

		string allStepsToString() const;
		void dumpAllSteps() const;
    void dump() const;

    //Attributes

    static bool       classInit;
    static ThirdRep*  thirdRep_;

		board_t					board_;					//actual pieces are stored here 
		bool					frozenBoard_[OB_SQUARE_NUM];			//keep information on frozen pieces, false == notfrozen, true == frozen

    PieceArray    pieceArray[2];  
    uint          rabbitsNum[2];        //kept number of rabbits for each player - for quick check on rabbitsNum != 0 
  
    StepArray     stepArray;
    uint          stepArrayLen;

    u64           signature_;            //position signature - for hash tables, corectness checks, etc. 
    u64           preMoveSignature_;     //signature of position from when the current move started

    /**Last made step.*/
    Step lastStep_;
		// move consists of up to 4 steps ( push/pull  counting for 2 ),
    uint  moveCount_;

		// step is either pass or single piece step or push/pull step,
		// thus stepCount_ takes values 0 - 4 
    uint  stepCount_;

    ob_player_t toMove_;
    uint     toMoveIndex_;    //0 == OB_GOLD, 1 == OB_SILVER
		ob_player_t winner_;

    friend class Eval;

};


/**
 * Simple random playout for old board.
 */
class OB_SimplePlayout
{
	public:
    /**
     * Constructor with board initialization.
     */
		OB_SimplePlayout(OB_Board*, uint maxPlayoutLength, uint evalAfterLength);

    /**
     * Performs whole playout. 
     *
     * Consists of repetitive calls to playOne().
     *
     * @return Final playout status.
     */
		void doPlayout();	

    /**
     * Returns playout length in moves.  
     */
		uint getPlayoutLength();  

  protected:
    /**
     * Performs one move of one player.
     *
     * Implements random step play to get the move.
     */
    void playOne();	

    /**
    * Wrapper around get winner from board.
    */
    bool hasWinner();

    OB_SimplePlayout();

    OB_Board*		board_;
    uint        playoutLength_;
    uint        maxPlayoutLength_;
    uint        evalAfterLength_;

};
