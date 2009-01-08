/**
 * @file config.h
 *
 * Command line configuration.
 */

#pragma once

#include "utils.h"
#include <list>

using std::list;

#define CFG_SEP "="
#define CFG_COMMENT "#"
#define DEFAULT_CFG "default.cfg"

class Cfg;

enum cfgItemType_e { IT_STRING, IT_BOOL, IT_INT, IT_FLOAT };

/**
 * Configuration item.
 */
class CfgItem
{
  public: 
    CfgItem(const char* name, cfgItemType_e type, void* item, const char * defaultValue_);
  private:
    string name_;
    cfgItemType_e type_;
    void * item_;
    bool set_;
    string defaultValue_;

    friend class Cfg;
};

typedef list<CfgItem > CfgItemList;
typedef CfgItemList::iterator CfgItemListIter;

/**
 * Program configuration. 
 *
 * Mostly influences the search/board/time management.
 */
class Cfg
{
  public: 
    /**
     * Binds items to variables.
     */
    Cfg();

    /**
     * Loads items from file.
     */
    void loadFromFile(string fn);

    /**
     * Checks whether every item has been set.
     */
    bool checkConfiguration();
    
    inline bool localPlayout() { return localPlayout_; }
    inline int randomStepTries() { return randomStepTries_; }
    inline float fpu() { return fpu_; }
    inline bool knowledgeInTree() { return knowledgeInTree_;}
    inline float exploreRate() { return exploreRate_; }
    inline int playoutLen() { return playoutLen_; }
    inline int matureLevel() { return matureLevel_; }
    inline float tcMoveDefault() { return tcMoveDefault_; }
    inline bool exactPlayoutValue() { return exactPlayoutValue_; }
    inline bool knowledgeInPlayout() { return knowledgeInPlayout_;}
    inline uint knowledgeTournamentSize() { return knowledgeTournamentSize_; }
    inline int searchThreadsNum() { return searchThreadsNum_; }

  private:
    CfgItemList items_;

    /**Locality in playout switch.*/
    bool localPlayout_;
    /**How many times random step is tried.*/
    int  randomStepTries_;
    /**FPU value*/
    float fpu_;
    /**Use knowledge in uct tree.*/
    bool knowledgeInTree_;
    /**Uct explore rate.*/
    float exploreRate_;
    /**Lenght of playout before evaluation.*/
    int playoutLen_;
    /**Number of traverses through node before expansion.*/
    int matureLevel_;
    /**Default time per move.*/
    float tcMoveDefault_;
    /**Use exact evaluation value [-1, 1] or approximation {1, -1}.*/
    bool exactPlayoutValue_;
    /**Use knowledge in playout.*/
    bool knowledgeInPlayout_;
    /**How many steps go to step tournament in playout (must be > 0).*/
    uint knowledgeTournamentSize_;
    /**Number of threads for search.*/
    int searchThreadsNum_;
};

enum optionType_e { OT_STRING, OT_BOOL_POS, OT_BOOL_NEG, OT_INT };
  
class Options;

class OptionFather
{
  protected: 
    string shortName_;
    string longName_;
    string description_;
    optionType_e  type_;
    bool parsed_;

    friend class Options;

  public:
    OptionFather(){};
    virtual ~OptionFather(){};
    OptionFather(string shortName, string longName, string description, optionType_e type):
      shortName_(shortName), longName_(longName), description_(description), type_(type) { parsed_ = false;};
    virtual void setValue(bool){};
    virtual void setValue(string){};
    virtual void setValue(int){};
    virtual void setValueParsed(bool){};
    virtual void setValueParsed(string){};
    virtual void setValueParsed(int){};
    virtual string toString(){ return "";};
};

template <typename T> class Option: public OptionFather
{
    T   value_;

  public:
    Option(){;};
    Option(string shortName, string longName, string description, optionType_e type, T defaultValue):
     OptionFather(shortName, longName, description, type), value_(defaultValue) {;};
    ~Option(){;};

    T getValue() { return value_;}
    void setValueParsed(T value) { if ( ! parsed_) { value_ = value; parsed_ = true;};}
    void setValue(T value) { value_ = value;}
     
    string toString() {
      stringstream ss;
      ss << "[" << shortName_ << ", " << longName_ << ", " << description_ << ", " << value_ << "]" << endl;
      return ss.str();
    }

};

typedef Option<int> OptionInt;
typedef Option<bool> OptionBool;
typedef Option<string> OptionString;

typedef list<OptionFather*> OptionList;

/**
 * Management of command line configuration. 
 * 
 * For cases when asi is not used.
 */
class Options
{
  private:
    OptionList  options_;

    /**AEI init file - for debugging.*/
    OptionString fnAeiInit_;
    /**Position input file - for getMoveMode_.*/
    OptionString fnInput_;
    /**Configuration file.*/
    OptionString fnCfg_;
    OptionBool benchmarkMode_; 
    OptionBool getMoveMode_; 
    OptionBool localMode_;

  public:
    Options();

    bool parse(const int, const char **);
    bool parseToken(string, string);
    bool parseValue(string);

    bool benchmarkMode() { return benchmarkMode_.getValue(); }
    bool localMode() { return localMode_.getValue(); }
    bool getMoveMode() { return getMoveMode_.getValue(); }
    string fnAeiInit() { return fnAeiInit_.getValue(); }
    string fnInput() { return fnInput_.getValue(); }
    string fnCfg() { return fnCfg_.getValue(); }

    void printAll();
    
};

extern Cfg cfg;
extern Options options;
