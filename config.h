#ifndef CONFIG_H
#define CONFIG_H

#include "utils.h"

enum optionType_e { OT_STRING, OT_BOOL_POS, OT_BOOL_NEG, OT_INT };
  
class Config;

class Option 
{
  protected:
    string shortName_;
    string longName_;
    string description_;
    optionType_e  type_;

    friend class Config;
  public:
    Option(){};
    Option(string, string, string);
    virtual void setValue(bool value){};
    virtual void setValue(string value){};
    string toString();
    virtual string valueToString(){ return "";};
};


class OptionInt: public Option 
{
  int value_;
  public: 
    OptionInt(){};
    OptionInt(string, string, string, int);
    int getValue() { return value_; } 
    void setValue(string value) {value_ = atoi(value.c_str()); }      //todo change this
    string valueToString(){ stringstream ss; ss << value_; return ss.str();};
};


class OptionString: public Option 
{
  string value_;
  public: 
    OptionString(){};
    OptionString(string, string, string, string);
    string getValue() { return value_; } 
    void setValue(string value) {value_ = value; } 
    string valueToString(){ stringstream ss; ss << value_; return ss.str();};
};


class OptionBool: public Option 
{
  bool value_;
  public: 
    OptionBool(){};
    OptionBool(string, string, string, bool, bool);
    bool getValue() { return value_; } 
    void setValue(bool value) {value_ = value; } 
    string toString();
    string valueToString(){ stringstream ss; ss << value_; return ss.str();};
};

#define MAX_OPTIONS 30

class Config
{
  Option*     options_[MAX_OPTIONS];
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

