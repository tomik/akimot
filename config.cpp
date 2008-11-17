
/** 
 *  @file config.cpp
 *  @brief Configuration managemenent.
 *  @full Parsing arguments from command-line and storing them into 
 *    run-wide variables in class Config.
 */

#include "config.h"

//---------------------------------------------------------------------
//  section Config 
//--------------------------------------------------------------------- 

Config::Config()
{
  useTimeControl_ = OptionBool("t","use_time_tontrol","use time control",OT_BOOL_POS, false);
  fnInput_ = OptionString("i","input","Input file", OT_STRING, "");
  secPerMove_ = OptionInt("s","sec_per_move","Seconds per move - trivial time control", OT_INT, 3);
  playoutsPerMove_ = OptionInt("p","playouts_per_move","Playouts per move - mostly for debugging", OT_INT, PLAYOUTS_PER_MOVE);
  inputIsRecord_ = OptionBool("r","input_record","input is a record of the game",OT_BOOL_POS, false);

  options_[0] =  &useTimeControl_;
  options_[1] =  &fnInput_;
  options_[2] =  &secPerMove_;
  options_[3] =  &playoutsPerMove_;
  options_[4] =  &inputIsRecord_;
  optionsNum_ = 5;

}

//--------------------------------------------------------------------- 

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
      value = argv[i];
      if ( token != "" ){   //there already was a token and it's value => parse them first 
        if (parseToken(token, value) ){
          token.clear();
          value.clear();
        }
        else 
          return false;
      }
      else {                      //token is =="" or value ==""
        parseValue(value);
      }
    }
  }
  if ( token != "" && ! parseToken(token, value) )    //something failed 
    return false;
  

  return true;
}

//--------------------------------------------------------------------- 

bool Config::parseToken(string token, string value) {
  bool consistent = false;
  for ( int i = 0 ; i < optionsNum_; i++ ) 
		if ( "-" + options_[i]->shortName_ == token || "--" + options_[i]->longName_ == token) {
			consistent = true;
			switch ( options_[i]->type_ ) {
				case OT_STRING :
					options_[i]->setValueParsed(value); break;
				case OT_BOOL_POS :
					options_[i]->setValueParsed(true); break;
				case OT_BOOL_NEG :
					options_[i]->setValueParsed(false); break;
				case OT_INT :
					options_[i]->setValueParsed(atoi(value.c_str())); break;
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

//--------------------------------------------------------------------- 

bool Config::parseValue(string value)
{
  //todo ... quite dummy - just sets value for file input - undummyfy 
  //for instance - mark options without name ( and go through them and parse values to them ) 
  fnInput_.setValueParsed(value);
  return true;
}

//--------------------------------------------------------------------- 

void Config::logAll()
{
  log_() << "Program configuration: " << endl;
  for (int i = 0; i < optionsNum_; i++)
   log_() <<  options_[i]->toString();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

Config config;
