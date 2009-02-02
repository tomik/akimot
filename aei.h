/** 
 * @file aei.h
 * Arimaa engine interface.
 *
 * Some details.
 */

#pragma once

#include <list>
#include <pthread.h>

#include "utils.h"
#include "config.h"
#include "board.h"
#include "engine.h"
#include "uct.h"

using std::list;
using std::flush;


#define ID_NAME "akimot"
#define ID_AUTHOR "Tomas Kozelek"
#define ID_VERSION "0.1"


enum aeiState_e {AS_ALL, AS_SAME, AS_OPEN, AS_MAIN, AS_GAME, AS_SEARCH, AS_PONDER};
enum aeiAction_e {AA_OPEN, AA_READY, AA_QUIT, AA_SET_POSITION, 
                  AA_SET_POSITION_FILE, AA_SET_OPTION, AA_NEW_GAME, AA_SET_VARIABLE, 
                  AA_GO, AA_GO_NO_THREAD, AA_STOP, AA_MAKE_MOVE, AA_MAKE_MOVE_REC, 
                  AA_BOARD_DUMP, AA_TREE_DUMP, AA_GOAL_CHECK, AA_EVAL};
enum aeiLogLevel_e {AL_ERROR, AL_WARNING, AL_INFO, AL_DEBUG};

/**
 * Command Set definition
 * AC_STD : standard AEI
 * AC_EXT : extended AC_STD with debug/shortcut stuff. 
 * More info in states definition.
 */
enum aeiCommandSet_e {AC_STD, AC_EXT};

class Engine;
class Aei;


/**
 * One record in aei finite automata.
 *
 * Defines state + command -> newstate + action.
 * Action is then handled in Aei::handleInput. 
 */
class AeiRecord 
{
  public:
    AeiRecord(string command, aeiState_e state, aeiState_e nextState, 
                aeiAction_e action);
    AeiRecord(string command, aeiState_e state, aeiState_e nextState, 
                aeiAction_e action, aeiCommandSet_e commandSet_);

  private:
    aeiState_e state_; 
    string command_;
    aeiState_e nextState_; 
    aeiAction_e action_;
    aeiCommandSet_e commandSet_;
    friend class Aei;
};


typedef list<AeiRecord> AeiRecordList;
typedef pair<string, timeControl_e> timeControlPair; 
typedef list<timeControlPair> TimeControlList;


/**
 * Arimaa Engine Interface Controller.
 *
 * This class operates communication with external interface(gameroom, user)
 * and controls the searching engine.
 */
class Aei 
{
  public:
    Aei();
    ~Aei();

    /**
     * Constructor with specified command set. 
     */
    Aei(aeiCommandSet_e commandSet);

    /**
     * Waits for commands in loop. 
     */
    void runLoop();

    /**
     * Hardcoded beginning of session.
     *
     * For debugging purposes.  
     */
    void initFromFile(string fn);

  private:
    /**
     * Initialization.
     *
     * Inits the parsing automata states and variables.
     * Reused in both constructors.
     */
    void init();

    /**
     * Handles single command; 
     */
    void handleInput(const string& input);

    /**
     * Parse option from setoption command. 
     */
    void handleOption(const string& commandRest);

    /**
     * Initiates thread creation and search start. 
     *
     * @param arg Search arguments - e.g. ponder, infinite, etc.
     */
    void startSearch(const string& arg);

    /**
     * Performs static goal check on the current position. 
     */
    void goalCheck();

    /**
     * Evaluates actual position. 
     */
    void evalActPos();

    /**
     * Threaded wrapper around engine.doSearch(). 
     */
    void searchInThread();

    /**
     * Wrapper around Aei::searchinThread().
     *
     * Declared as static so it can be threaded.
     */
    static void* SearchInThreadWrapper(void *instance);


    /**
     * Stop search.
     *
     * Gives request to stop the search. 
     * Waits for the search thread to join. 
     *
     * @param fromThread      If true, method is called from thread 
     *                          -> doesn't need to join the thread 
     *                        otherwise does nothing ( e.g. when makemove stopped go ponder)
     */
    void stopSearch(bool fromThread=false);

    /**
     * Sends bestmove/winration/timeinfo. 
     */
    void sendSearchInfo();

    /**
     * Communicate log messages. 
     */
    void aeiLog(const string& msg, const aeiLogLevel_e logLevel) const;

    /**
     * Sending additional information.
     */
    void sendInfo(const string& type, const string& value) const;

    /**
     * Quit the program. 
     */
    void quit() const;

    /**
     * Send id information.
     *
     * Uses information from ID_ macros - ID_AUTHOR, IT_NAME, ID_VERSION.
     */
    void sendId() const;

    /**
     * Wrapper around sending.
     */
    void send(const string& s) const;

    /** Records of aei finite automata.*/
    AeiRecordList records_;
    /** List of pairs <option name, time control for time manager>*/
    TimeControlList timeControls_;
    /** State of the automata.*/
    aeiState_e state_;
    /** Response to be send through communication channel.*/
    string response_;

    aeiCommandSet_e commandSet_;
    //bool sendMoveAfterSearch_;

    Board* board_;
    Engine* engine_;
    /** Thread handler for running search.*/
    pthread_t engineThread_;
};

