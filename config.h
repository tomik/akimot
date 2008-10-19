#ifndef CONFIG_H
#define CONFIG_H

#include "utils.h"

enum optionType_e { OT_STRING, OT_BOOL_POS, OT_BOOL_NEG, OT_INT };
  
class Config;

class OptionFather
{
  protected: 
    string shortName_;
    string longName_;
    string description_;
    optionType_e  type_;
    friend class Config;
  public:
    OptionFather(){};
    OptionFather(string shortName, string longName, string description, optionType_e type):
      shortName_(shortName), longName_(longName), description_(description), type_(type) {}
    virtual void setValue(bool){};
    virtual void setValue(string){};
    virtual void setValue(int){};
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
    void setValue(T value) { value_ = value;}
    string toString() {
      stringstream ss;
      ss << "short: " << shortName_ << " long: " << longName_ << " value: " << value_ << endl;
      return ss.str();
    }

};

typedef Option<int> OptionInt;
typedef Option<bool> OptionBool;
typedef Option<string> OptionString;

#define MAX_OPTIONS 30

class Config
{
  OptionFather*     options_[MAX_OPTIONS];
  int         optionsNum_;
  OptionBool  foo_;
  OptionString fnInput_;
 //
  Logger      log_;

  public:
    Config();
    bool parseToken(string, string);
    bool parseValue(string);
    bool parse(const int, const char **);
    void printAll();

    bool foo(){ return foo_.getValue(); } 
    const char * fnInput() { return fnInput_.getValue().c_str(); }
    
};

extern Config config;
#endif

