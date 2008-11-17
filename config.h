#ifndef CONFIG_H
#define CONFIG_H

#include "utils.h"

#define PLAYOUTS_PER_MOVE 10000

enum optionType_e { OT_STRING, OT_BOOL_POS, OT_BOOL_NEG, OT_INT };
  
class Config;

class OptionFather
{
  protected: 
    string shortName_;
    string longName_;
    string description_;
    optionType_e  type_;
    bool parsed_;

    friend class Config;

  public:
    OptionFather(){};
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
    Option(){};
    Option(string shortName, string longName, string description, optionType_e type, T defaultValue):
     OptionFather(shortName, longName, description, type), value_(defaultValue) {}

    T getValue() { return value_;}
    void setValueparsed(T value) { if ( ! parsed_) { value_ = value; parsed_ = true;};}
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

#define MAX_OPTIONS 30

class Config
{
  private:
    OptionFather*     options_[MAX_OPTIONS];
    int         optionsNum_;

    OptionBool  useTimeControl_;
    OptionBool  inputIsRecord_;
    OptionInt   secPerMove_;
    OptionInt   playoutsPerMove_;
    OptionString fnInput_;

    Logger      log_;

  public:
    Config();

    bool parse(const int, const char **);
    bool parseToken(string, string);
    bool parseValue(string);

    bool useTimeControl(){ return useTimeControl_.getValue(); } 
    bool inputIsRecord(){ return inputIsRecord_.getValue(); } 
    int secPerMove(){ return secPerMove_.getValue(); } 
    int playoutsPerMove(){ return playoutsPerMove_.getValue(); } 
    const char * fnInput() { return fnInput_.getValue().c_str(); }

    //sometimes it is neccessary to set values in config - e.g. testing 
    void playoutsPerMove(int playouts){ playoutsPerMove_.setValue(playouts); } 
    void useTimeControl(bool use){ useTimeControl_.setValue(use); } 
    void secPerMove(int sec){ secPerMove_.setValue(sec); } 

    void logAll();
    
};

extern Config config;
#endif

