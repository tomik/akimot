#include "config.h"

Config::Config()
{
  foo_ = OptionBool("f","foo","Foo variable",OT_BOOL_POS, true);
  fnInput_ = OptionString("i","input","Input file", OT_STRING, "");

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
					options_[i]->setValue(atoi(value.c_str())); break;
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
