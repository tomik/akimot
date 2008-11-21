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

#define ID_NAME "akimot"
#define ID_AUTHOR "Tomas Kozelek"
#define ID_VERSION "0.1"

//strings used in communication
#define STR_AEI "aei"
#define STR_AEI_OK "aeiok"
#define STR_READY "isready"
#define STR_READY_OK "readyok"
#define STR_NEW_GAME "newgame"
#define STR_SET_POSITION "setposition"
#define STR_SET_POSITION_FILE "setpositionfile"
#define STR_SET_OPTION "setoption"
#define STR_QUIT "quit"
#define STR_GO "go"
#define STR_PONDER "ponder"
#define STR_INFINITE "infinite"
#define STR_STOP "stop"
#define STR_BEST_MOVE "bestmove"
#define STR_BYE "bye"

#define STR_TC_MOVE "tcmove"
#define STR_TC_RESERVE "tcreserve"
#define STR_TC_PERCENT "tcpercent"
#define STR_TC_MAX "tcmax"
#define STR_TC_TOTAL "tctotal"
#define STR_TC_TURNS "tcturns"
#define STR_TC_TURN_TIME "tcturntime"
#define STR_TC_W_RESERVE "wreserve"
#define STR_TC_B_RESERVE "breserve"
#define STR_TC_W_USED "wused"
#define STR_TC_B_USED "bused"
#define STR_TC_LAST_MOVE_USED "lastmoveused"
#define STR_TC_MOVE_USED "tcmoveused"


#define STR_INVALID_COMMAND "Invalid command"

#define STR_NAME "name"
#define STR_AUTHOR "author"
#define STR_VERSION "version"

enum aeiState_e { AS_ALL, AS_SAME, AS_OPEN, AS_MAIN, AS_GAME, AS_SEARCH};
enum aeiAction_e { AA_OPEN, AA_READY, AA_QUIT, AA_SET_POSITION, AA_SET_POSITION_FILE, 
                  AA_SET_OPTION, AA_NEW_GAME, AA_SET_VARIABLE, AA_GO, AA_STOP};

class Engine;
class Aei;


/**
 * One record in aei finite automata.
 *
 * Defines state + command -> newstate + action
 */
class AeiRecord 
{
  private:
    aeiState_e state_; 
    string command_;
    aeiState_e nextState_; 
    aeiAction_e action_;
    friend class Aei;

  public:
    AeiRecord(string command, aeiState_e state, aeiState_e nextState, aeiAction_e action);
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
  private:
    AeiRecordList records_;
    TimeControlList timeControls_;
    aeiState_e state_;
    string response_;

    Board* board_;
    Engine* engine_;
    pthread_t engineThread;

  public:
    Aei();
    ~Aei();

    /**
     * Waits for commands in loop. 
     */
    void runLoop();

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
     * Hardcoded beginning of session.
     *
     * For debugging purposes.  
     */
    void implicitSessionStart();

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
};

