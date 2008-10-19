#include "config.h"

Option::Option(string shortName, string longName, string description):
 shortName_(shortName), longName_(longName), description_(description)
{
}

string Option::toString()
{
  stringstream ss;
  ss << "short: " << shortName_ << " long: " << longName_ << " value: " << valueToString() << endl;
  return ss.str();
}


OptionInt::OptionInt(string shortName, string longName, string description, int defaultValue): 
  Option(shortName, longName, description), value_(defaultValue)
{
  type_ = OT_INT;
}


OptionString::OptionString(string shortName, string longName, string description, string defaultValue): 
  Option(shortName, longName, description), value_(defaultValue)
{
  type_ = OT_STRING;
}


OptionBool::OptionBool(string shortName, string longName, string description, bool defaultValue, bool positive): 
  Option(shortName, longName, description), value_(defaultValue)
{
  if (positive)
    type_ = OT_BOOL_POS;
  else
    type_ = OT_BOOL_NEG;
}


Config::Config()
{
  foo_ = OptionBool("f","foo","Foo variable",true, true);
  fnInput_ = OptionString("i","input","Input file","");

  options_[0] =  &foo_;
  options_[1] =  &fnInput_;
  optionsNum_ = 2;

}


bool Config::parseToken(string token, string value) {
  bool consistent = false;
  for ( int i = 0 ; i < optionsNum_; i++ ) 
		if ( "-" + options_[i]->shortName_ == token || "--" + options_[i]->longName_ == token) {
			consistent = true;
			switch ( options_[i]->type_ ) {
				case OT_STRING :
					options_[i]->setValue(value); break;
				case OT_BOOL_POS :
					options_[i]->setValue(true); break;
				case OT_BOOL_NEG :
					options_[i]->setValue(false); break;
				case OT_INT :
					options_[i]->setValue(value); break;
				default : break;
			}
      if ((value != "") && (options_[i]->type_ == OT_BOOL_POS || options_[i]->type_ == OT_BOOL_NEG))
        parseValue(value);
		} // if 
		if ( consistent ) 
			return true;
		else { 
			log_() << "Unknown option " << token << ", program terminated." << endl; 
			return false;
    }
}

bool Config::parseValue(string value)
{
  //todo ... quite dummy - just sets value for file input - undummyfy 
  //for instance - mark options without name ( and go through them and parse values to them ) 
  fnInput_.setValue(value);
  return true;
}


bool Config::parse(const int argc, const char ** argv ) 
{
  string token = "";
  string value = "";

  for ( int i = 1; i  < argc ; i++ ) {
    if ( argv[i][0] == '-' ) { 			// argument is token 
      if ( token != "" && ! parseToken(token, value) )    //something failed 
        return false;
      token = argv[i]; // set new actual token 
      value.clear();
    }else  {                        //argument is a token's value
      if ( token != "" && value != "" )   //there already was a token and it's value => parse them first 
        if (parseToken(token, value) ){
          token.clear();
          value.clear();
        }
        else {
          return false;
        }
      else {                      //token is =="" or value ==""
        value = argv[i];
        if ( token == "" ) 
          parseValue(value);
      }
    }
  }
  if ( token != "" && ! parseToken(token, value) )    //something failed 
    return false;
  

  return true;
}

void Config::printAll()
{
  log_() << "Program configuration: " << endl;
  for (int i = 0; i < optionsNum_; i++)
   log_() <<  options_[i]->toString();
}


Config config;
