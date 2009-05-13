/**
 * @file uct.h
 *
 * Implementation of uct algorithm. 
 */


#pragma once 

#include <cmath>
#include <queue>
#include <set>

#include "hash.h"

using std::map;
using std::queue;
using std::sqrt;
using std::set;
using std::make_pair;

#define MAX_PLAYOUT_LENGTH 100  //these are 2 "moves" ( i.e. maximally 2 times 4 steps ) 
#define UCT_MAX_DEPTH 50
#define CHILDREN_CACHE_SIZE 3
#define CCACHE_START_THRESHOLD 50 
#define EVAL_AFTER_LENGTH (cfg.playoutLen())

#define NODE_VICTORY(node_type) (node_type == NODE_MAX ? 2 : -1 )
#define WINNER_TO_VALUE(winner) (winner == GOLD ? 1 : -1 )

enum playoutStatus_e {PLAYOUT_OK, PLAYOUT_TOO_LONG, PLAYOUT_EVAL}; 

class Engine;

/**
 * GOLD is always MAX, Silver Min, no matter who is in the root. 
 * values in the node express: -1 sure win for Silver ... 0 equal ... 1 sure win for gold 
 * nodes are either NODE_MAX ( ~ gold node )  where we maximize value + uncertainty_term and 
 * NODE_MIN ( ~ siler node )  where we maximize -value + uncertainty_term
 */
enum nodeType_e {NODE_MAX, NODE_MIN};   

#define PLAYER_TO_NODE_TYPE(player) (player == GOLD ? NODE_MAX : NODE_MIN)

/**
 * Simple random playout.
 *
 * Performs random playout from position given in constructor.
 * Playout returns playout status.
 */
class SimplePlayout
{
	public:
    /**
     * Constructor with board initialization.
     */
		SimplePlayout(Board*, uint maxPlayoutLength, uint evalAfterLength);

    /**
     * Performs whole playout. 
     *
     * Consists of repetitive calls to playOne().
     *
     * @return Final playout status.
     */
		playoutStatus_e doPlayout();	

    /**
     * Returns playout length in moves.  
     */
		uint getPlayoutLength();  

    virtual ~SimplePlayout(){};

  protected:
    /**
     * Performs one move of one player.
     *
     * Implements random step play to get the move.
     */
    virtual void playOne();	

    /**
    * Wrapper around get winner from board.
    */
    bool hasWinner();

    SimplePlayout();

    Board*		  board_;
    uint        playoutLength_;
    uint        maxPlayoutLength_;
    uint        evalAfterLength_;

};

class AdvisorPlayout : public SimplePlayout
{
  public:
    /**
     * Constructor with board initialization.
     */
		AdvisorPlayout(Board*, uint maxPlayoutLength, uint evalAfterLength, 
                    MoveAdvisor* advisor);

    /**
     * Play one move in simulation with knowledge.
     */
    void playOne();

  private: 
     MoveAdvisor * advisor_;
};

/**Tree wide step.*/
class TWstep
{
  public:
    TWstep(Step, float, int);
    Step step;
    float value;
    int visits;
} ; 

typedef map<Step, TWstep> TWstepsMap;

class TWsteps: public TWstepsMap
{
  public: 
    TWstep& operator[](const Step& step);
  private:
};

//typedef map<Step, TWstep> TWsteps;

typedef set<Node*> NodeSet;

/**
 * Node in uct tree. 
 */
class Node
{
  public:
    Node();

    /**
     * Constructor with step and heuristic 
     */
    Node(TWstep*,  float heur=0);

    /**
     * Finds child with highest UCB1 value.
     */
    Node* findUctChild(Node * realFather);

    /**
     * Finds random child.
     *
     * Mostly for testing.
     */
    Node* findRandomChild() const;

    /**
     * Finds child with most visits. 
     */
    Node* findMostExploredChild() const;

    /**
     * Core of UCT algorithm.
     */
    float exploreFormula(float) const;

    /**
     * Children Cache init. 
     * 
     * Cache array is created and filled with nulls. 
     */
    void cCacheInit();

    /**
     * Updating children Cache. Selects appropriate nodes 
     * and places them into cache.
     */
    void cCacheUpdate(float exploreCoeff);

    /**
     * DRY-purpose method.
     */
    void uctOneChild(Node* act, Node* & best, float & bestUrgency, float exploreCoeff) const;

    /**
     * The UCB1 formula.
     */
    float ucb(float exploreCoeff) const;

    /**
     * The UCB-tuned formula.
     */
    float ucbTuned(float exploreCoeff) const;

    /**
     * Childre addition during node expansion.
     */
    void  addChild(Node* child);

    /**
     * Deletes children recursively. 
     */
    void  delChildrenRec();

    /**
     * Update after playout. 
     *
     * Updates value/visits.
     */
    inline void  update(float);
    
    /**
     * Update after playout. 
     *
     * Updates value/visits of twStep.
     */
    inline void  updateTWstep(float);

    /**
     * Maturity test.
     *
     * Checks whether number of descends through node passed 
     * some given threshold (around number of legal steps from average position) 
     */
    bool  isMature() const;
    bool  isJustMature() const;

    bool  hasChildren() const;
    bool  hasOneChild() const;
    Node* getFather() const;
    void  setFather(Node*);
    Node* getFirstChild() const;
    void  setFirstChild(Node *);
    Node* getSibling() const;
    void  setSibling(Node*);
    NodeList* getTTrep() const;
    void setTTrep(NodeList * node);
    Step  getStep() const;
    TWstep*  getTWstep() const;
    player_t getPlayer() const;
    int   getVisits() const;
    void  setVisits(int visits); 
    float getValue() const;
    void setValue(float value);
    nodeType_e getNodeType() const;

    /**
     * Gets ply in which node lies.
     */
    int getDepth() const;

    /**
     * Depth from last opponent's move.
     */
    int getLocalDepth() const;

    /**
     * Gets level (level ~ +- 4 plys)
     */
    int getLevel() const;

    /**
     * Combination of getLocalDepth and getLevel.
     */
    int getDepthIdentifier() const;

    string toString() const; 

    /**
     * Recursively (children as well) to string. 
     */
    string recToString(int) const;

  private:
    /**Uct value of the node in [-1, 1].*/
    float       value_; 
    /**Heuristic value - purely position dependent.*/
    float       heur_;
    float       squareSum_;
    int         visits_;
    TWstep*     twStep_;

    /**Transposition tables representant*/
    NodeList*       ttRep_;

    Node*       sibling_;
    Node*       firstChild_;  
    Node*       father_;
    int         cCacheLastUpdate_;
    Node**      cCache_; 
};

/**Tree equivalent node.*/
struct EqNode {
  Node* node;
  struct EqNode * next;
};

/**List of Tree equivalent nodes.*/
struct EqNodeBlock {
  EqNode * eqNode;
  struct EqNodeBlock* next;
};


/**Value visit pair.*/
/*typedef pair<float, int> ValueVisitPair;
typedef map<Step, ValueVisitPair> TreeWideSteps;
*/

/**
 * Uct tree. 
 */
class Tree
{

  public:
    /**
     * Constructor with predefined root.
     *
     * Sets the root to be node. Used in tree mockups.
     */
    Tree(Node * root);   

    /**
     * Constructor which creates root node. 
     * 
     * Root node is created(bottom of history stack) with given player.
     */
    Tree(player_t firstPlayer);   

    /**
     * Mocking up constructor. 
     * 
     * Used for mocking up the tree from multiple trees
     * after search in multiple threads.
     */
    Tree(Tree* trees[], int treesNum );

    /**
     * Destructor.
     *
     * Recursively deletes the tree.
     */
    ~Tree();

    /**
     * Node expansion.
     *
     * Creates children for every given step. 
     * @param steps - Given steps (already filtered through repetition tests, 
     *                virtual pass test, transposition tables).
     * @param len - Length of steps array.
     * @param heurArray - Heuristics connected to the steps.
     */
    void  expandNode(Node* node, const StepArray& steps, uint len, const HeurArray* heurs=NULL);

    /*
     * Limited Node expansion.
     *
     * Expands node for given move. 
     * I.E. for every step in given move, moves one level deeper.
     */
    void  expandNodeLimited(Node* node, const Move& move);

    /**
     * UCT-based descend to next level.
     *
     * Crucial method implementing core idea of UCT algorithm - 
     * the descend by UCB1 formula. History is updated - simulating the descend.
     */
    void uctDescend();

    /**
     * Random descend.
     *
     * Mostly for testing purposes.
     */
    void randomDescend();

    /**
     * First child descend. 
     *
     * Always descends to first child..i 
     */
    void firstChildDescend();


    /**
     * Finds the best move node.
     *
     * Node for the best move in the subtree under subTreeRoot. 
     * BFS in the tree topmost level(relative to subTreeRoot) of the tree..
     * 
     */
    Node* findBestMoveNode(Node* subTreeRoot);

    /** 
     * Move for given Node.
     *
     * After finding best node. This method finds appropriate move for this node in the tree.
     * @return best move in search. 
     */
    Move findBestMove(Node* bestMoveNode);

    /**
     * Merging trees. 
     *
     * Not fixed to instance.
     */
    static Node* mergeTrees(EqNode* eqNode, Node* father, 
                            int& nodesNum, int& nodesExpandedNum);

    /**
     * Backpropagation of playout sample.
     */
    void updateHistory(float);

    /**
     * History reset.
     *
     * Performed after updateHistory. Points actual node to the root.
     */
    void historyReset();

    /**
     * Gets tree root(bottom of history stack). 
     */
    Node* root();

    /**
     * Gets the actual node (top of history stack). 
     */
    Node* actNode();

    /**
     * Nodes num getter. 
     */
    int getNodesNum();

    /**
     * Nodes pruned getter. 
     */
    int getNodesPruned();

    /**
     * Nodes expanded getter.
     */
    int getNodesExpandedNum();

    /**
     * String representation of the tree.
     */
    string toString();

    /**
     * Path to actNode() to string.  
     *
     * Returns string with steps leading to actNode().
     * @param onlyLastMove if true then only steps from last move are returned.
     */
    string pathToActToString(bool onlyLastMove = false);

    /**
     * Updates Transposition tables.
     *
     * Every children of given father is checked:
     *    if it's position is unique in TT it's added 
     *    if it's position already exists in TT, it's children are linked
     *
     * @param father it's children will get updated
     */
    void updateTT(Node* father, const Board* board);


  private:
    /**
     * Simple constructor.
     *
     * Sets the root to NULL. 
     */
    Tree();   

    /**
     * Constructor wide init.
     */
    void init();

    /**
     * Level calculation.
     *
     * @param father Node father
     * @param step Step representid in node.
     *
     * @return Level(move num) belonging to node.
     */
    static int calcNodeLevel(Node* father, const Step& step);


    Node*      history[UCT_MAX_DEPTH];
    uint       historyTop;
    /**Expanded nodes.*/
    int   nodesExpandedNum_;
    /**Nodes in the tree.*/
    int   nodesNum_;
    /**Number of pruned nodes in tt.*/
    int nodesPruned_;
    /**Map of tree wide steps. 
     *
     * Every step with its value (average of all values in the tree) 
     * is stored here. This is used to init new node.*/
    TWsteps  twSteps_;
    /**Transposition table.*/
    TT* tt_;              
    
};

/**
 * Uct search. 
 */
class Uct
{
  public:

    /**
     * Preffered constructor to use 
     */
    Uct(const Board* board);

    /**
     * Constructor for results mockup.
     */
    Uct(const Board* board, Uct* ucts[],int uctsNum);

    ~Uct();

    /**
     * Crucial method implementing search.
     *
     * Runs the doPlayout loop.
     */
     void searchTree(const Board*, const Engine*);
    //void searchTree(const Board*, const Engin*, 
     //               bool (Engine::* checkSearchStop) () const);

    /**
     * Results refinement after search.
     */
    void refineResults(const Board* board); 

    /**
     * Does one uct-monte carlo playout. 
     *
     * Crucial method of search. Performs UCT descend as deep as possible and 
     * then starts the "monte carlo" playout through object SimplePlayout.
     */
    void doPlayout(const Board* board);

    /**
     * Get uct search statistics.
     */
    string getStats(float seconds) const;

    /**
     * Get additional info - here it is the string repr. of search tree.
     */
    string getAdditionalInfo() const;

    /**
     * String representation of best move.
     */
    string getBestMoveRepr() const;

    /**
     * Visits to the best move. 
     */
    int getBestMoveVisits() const;

    /**
     * Value of best move.
     */
    float getBestMoveValue() const;

    /**
     * Estimated win ratio.
     *
     * Win ratio(percentage) ~ chance of winning for player to move.
     */
    float getWinRatio() const;

    /**
     * Tree getter. 
     */
    Tree* getTree() const;

    /**
     * Playouts_ getter.
     */
    int getPlayoutsNum() const;

  private:
    /**
     * Simple constructor.
     */
    Uct();

    /**
     * Constructor wide initialization.
     */
    void init(const Board* board);

    /**
     * Decide winner of the game. 
     * @return Returns 1 ( GOLD wins ) or -1 (SILVER wins). 
     *         If winner is not known ( no winning criteria reached ), position
     *         is evaluated and biased coin si flipped to estimate the winner
     */
    double decidePlayoutWinner(const Board*) const;

    /**
     * Fills advisor with moves from tactical search. 
     */
    void fill_advisor(const Board * playBoard);

    /**UCT tree*/
    Tree* tree_;
    /**Evaluation object*/
    Eval* eval_;
    /**Pointer to the most visited last step of first move.*/
    Node* bestMoveNode_;  
    /**Best move calculated from bestMoveNode_.*/
    string bestMoveRepr_;  
    /**Total number of playouts.*/
    int playouts_;
    /**Total number of uct descends through the tree.*/
    int uctDescends_;
    
    /*Move advisor is filled here and used in playouts.*/
    MoveAdvisor * advisor_;
};


