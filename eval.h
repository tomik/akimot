#pragma once

#include "board.h"

#define RING0  0x0000001818000000ULL   /* same as small center */
#define RING1  0x00003c24243c0000ULL   
#define RING2  0x007e424242427e00ULL
#define RING3  0xff818181818181ffULL   

//TODO change ! 
#define TRAP_TO_INDEX(trap) (trap < 32 ? (trap == 18 ? 0 : 1) : trap ==  42 ? 2 : 3)

//game stages num
#define GS_NUM 3 

enum gameStage_e {GS_BEGIN, GS_MIDDLE, GS_LATE};

extern u64 adv[8][2];

enum trapType_e { TT_UNSAFE, TT_HALF_SAFE, TT_SAFE, TT_ACTIVE};

class Eval;
class Values;

class ValueItem { 
  public: 
    ValueItem() {};
    ValueItem(string name, itemType_e type, void* item, int num);

    bool isSingleValue() const;

  private:
    string name_;
    itemType_e type_;
    void* item_;
    int num_;
    friend class Values;
};

typedef list <ValueItem> ValueList;

class Values 
{
  public:
    /**
     * Loads values from given string. 
     *
     * Values are loaded according to valuesList.
     */
    Values(string config);

    /**
     * Init from implicit values.
     */
    Values();

    /**
     * Actual init called from constructors;
     */
    void init();

    /**
     * Dump to string.
     */
    string toString() const;
    
  private: 
    /**
     * Load from file.
     */
    bool loadFromString(string);

    /**
     * Mirroring. 
     *
     * Horizontally for second (symetrical) half of the board.
     * Vertically for second player. 
     */
    void mirrorPiecePositions();

    static void baseMirrorIndexes(int & player, int & coord);

    /**
     * Token -> item mapping. 
     *
     * Goes through tokens and tries to find given one.
     * @return True if found, false otherwise.
     */
    bool getItemForToken(string token, ValueItem& valueItem) const;


    ValueList values;
    friend class Eval;

    //static piece values  
    int pieceValue[PIECE_NUM + 1];

    //penalties for few rabbits 0 .. 8 
    int rabbitPenalty[RABBITS_NUM + 1]; 

    //penalty for being frozen per piece percentage from value of piece
    float frozenPenaltyRatio;

    //traps
    int trapSoleVal;  
    int trapMoreThanOneVal;
    int trapSafeVal;  
    int trapActiveVal;
    int trapPotVal;

    int activeTrapBlockedPenalty;

    //ratio substracted from piece value if framed 
    float framePenaltyRatio;  

    int camelHostagePenalty;

    int elephantBlockadePenalty;

    //ratio substracted from piece value if supports framed piece (not mobile)
    float pinnedPenaltyRatio;  

    //int piecePos[2][PIECE_NUM + 1][BIT_LEN];
    int piecePos[GS_NUM][2][PIECE_NUM + 1][BIT_LEN];

};

/**
 * Board evaluation class.
 * 
 * It is declared as a friend in the board class - thus it can access it's private items.
 * Always returns evaluation from the point of view of GOLD player.
 */
class Eval
{
	public:
    Eval();

    /**
     * Inits base evaluation as well.
     */
    Eval(const Board* board);
    
    /**
     * Common init
     */
    void init();

    /**
     * Evaluation.
     */
    int evaluate(const Board*) const;

    /**
     * Evaluation by ddailey.
     */
    int evaluateDailey(const Board*) const;

    /**
     * Transfers int/float evaluation to percent.
     */
    float evaluateInPercent(const Board*) const;

    /**
     * Value getter for piece.
     */
    float getPieceValue(piece_t piece) const;

    /**
     * Evaluates one step.
     *
     * In this play game knowledge is applied.
     */
    float evaluateStep(const Board*, const Step& step) const;

  private: 

    gameStage_e determineGameStage(const Bitboard& bitboard) const;
    
    bool blocked(player_t player, piece_t piece, coord_t coord, const Board* b) const;
    
    static string trapTypeToStr(trapType_e trapType);

    EvalTT * evalTT_;

    Values * vals_;

    /**Base evaluation ... for relative evaluation.*/
    double base_eval_;
    double eval_max_;

};
