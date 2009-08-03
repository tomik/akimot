/**
 * @file eval.h
 *
 * @brief Knowledge management and evaluation.
 * @full Static evaluation function is defined here together with evaluation mapping to winning probability. 
 * Also contains step evaluation by knowledge.
 */


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

/**
 * One evaluation element.
 *
 * Loaded from configuration file.
 */
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

/**
 * Base class for values holding. 
 */
class Values 
{
  public:

    virtual ~Values(){};

    /**
     * Actual init called from constructors;
     */
    virtual void init() = 0;

    //virtual ~Values(); 

    /**
     * Dump to string.
     */
    string toString() const;
    
  protected: 
    ValueList values;

    /**
     * Load from file.
     */
    bool loadFromString(string);

};


/**
 * Values for "constants" in evaluation.
 */
class EvaluationValues:public Values
{
  public: 
    /**
     * Loads values from given string. 
     *
     * Values are loaded according to valuesList.
     */
    EvaluationValues(string config);

    /**
     * Init from implicit values.
     */
    EvaluationValues();

    void init();

  private:
    friend class Eval;

    /**
     * Mirroring. 
     *
     * Horizontally for second (symetrical) half of the board.
     * Vertically for second player. 
     */
    void mirrorPiecePositions();

    /**
     * For mirroring.
     */
    static void baseMirrorIndexes(int & player, int & coord);

    /**Static piece values.*/
    int pieceValue[PIECE_NUM + 1];

    /**Penalties for few rabbits 0 .. 8.*/
    int rabbitPenalty[RABBITS_NUM + 1]; 

    /**Penalty for being frozen in piece value percentage.*/
    float frozenPenaltyRatio;

    int trapSoleVal;  
    int trapMoreThanOneVal;
    int trapSafeVal;  
    int trapActiveVal;
    int trapPotVal;

    int activeTrapBlockedPenalty;
    float framePenaltyRatio;  
    int camelHostagePenalty;
    int elephantBlockadePenalty;

    float pinnedPenaltyRatio;  

    /**Piece positioning evaluation.*/
    int piecePos[GS_NUM][2][PIECE_NUM + 1][BIT_LEN];
};

/**
 * Values for "constants" in step knowledge.
 */
class StepKnowledgeValues: public Values{
  public:
    StepKnowledgeValues(string config);

    StepKnowledgeValues();

    void init();

  private:
    float passPenalty;
    float inverseStepPenalty;
    float elephantStepVal;
    float camelStepVal;
    float horseStepVal;
    float pushPullVal;
    float leaveBuddyInTrapPenalty;
    float suicidePenalty;
    float stepInDangerousTrapPenalty;
    float pushPullToTrapVal;
    float killVal;
    float rabbitStepBeginVal;
    float rabbitStepMiddleVal;
    float rabbitStepLateVal;
    float localityVal;
    int localityReach;
    
    friend class Eval;
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
     * Used in the playout and for bias in the tree.
     */
    float evaluateStep(const Board*, const Step& step) const;

  private: 

    /**
     * Calculates gamestage based on number of pieces.
     */
    gameStage_e determineGameStage(const Bitboard& bitboard) const;
    
    bool blocked(player_t player, piece_t piece, coord_t coord, const Board* b) const;
    
    static string trapTypeToStr(trapType_e trapType);

    EvalTT * evalTT_;

    EvaluationValues * vals_;
    StepKnowledgeValues * skvals_;

    /**Maximal evaluation given as constant depending on used evaluation method.*/
    double eval_max_;

};
