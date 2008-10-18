#include "config.h"

Option::Option(string shortName, string longName, string description):
 shortName_(shortName), longName_(longName), description_(description)
{
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
		} // if 
		if ( consistent ) 
			return true;
		else { 
			log_() << "Unknown option " << token << ", program terminated." << endl; 
			return false;
    }
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
    }else                          //argument is a token's value
      value = argv[i];
  }
  if ( token != "" && ! parseToken(token, value) )    //something failed 
    return false;
  

  return true;
}

void Config::printAll()
{
  ;
  //for (int i = 0; i < optionsNum_; i++)
    //log_() << "shortName: " << options_[i]->shortName_ << " longName: " << options_[i]->longName_ << " value: " << options_[i]->value_ << endl;
}


Config config;
