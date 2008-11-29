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

using std::list;
using std::flush;


#define ID_NAME "akimot"
#define ID_AUTHOR "Tomas Kozelek"
#define ID_VERSION "0.1"


enum aeiState_e {AS_ALL, AS_SAME, AS_OPEN, AS_MAIN, AS_GAME, AS_SEARCH};
enum aeiAction_e {AA_OPEN, AA_READY, AA_QUIT, AA_SET_POSITION, 
                  AA_SET_POSITION_FILE, AA_SET_OPTION, AA_NEW_GAME, 
                  AA_SET_VARIABLE, AA_GO, AA_STOP, AA_MAKE_MOVE};
enum  aeiLogLevel_e {AL_ERROR, AL_WARNING, AL_INFO, AL_DEBUG};

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
    AeiRecord(string command, aeiState_e state, aeiState_e nextState, aeiAction_e action);

  private:
    aeiState_e state_; 
    string command_;
    aeiState_e nextState_; 
    aeiAction_e action_;
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
     * Communicate log messages. 
     */
    void aeiLog(const string& msg, const aeiLogLevel_e logLevel) const;

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

    Board* board_;
    Engine* engine_;
    /** Thread handler for running search.*/
    pthread_t engineThread;
};

