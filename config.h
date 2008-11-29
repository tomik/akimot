/**
 * @file config.h
 *
 * Command line configuration.
 */

#pragma once

#include "utils.h"
#include <list>

using std::list;

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
 * After introduction of aei rather obsolete.
 */
class Config
{
  private:
    OptionList  options_;

    OptionString fnAeiInit_;
    OptionBool benchmark_; 

  public:
    Config();

    bool parse(const int, const char **);
    bool parseToken(string, string);
    bool parseValue(string);

    bool benchmark() { return benchmark_.getValue(); }
    string fnAeiInit() { return fnAeiInit_.getValue(); }

    void printAll();
    
};

extern Config config;
