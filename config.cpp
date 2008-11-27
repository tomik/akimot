
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
  debug_ = OptionBool("d","debug","loads aei session implicitly",OT_BOOL_POS, false);
  benchmark_ = OptionBool("b","benchmark","runs in benchmarking mode",OT_BOOL_POS, false);

  options_.clear();
  options_.push_back(&useTimeControl_);
  options_.push_back(&fnInput_);
  options_.push_back(&secPerMove_);
  options_.push_back(&playoutsPerMove_);
  options_.push_back(&inputIsRecord_);
  options_.push_back(&debug_);
  options_.push_back(&benchmark_);

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
  OptionList::iterator it;
  for ( it = options_.begin(); it != options_.end(); it++ ) 
		if ( "-" + (*it)->shortName_ == token || "--" + (*it)->longName_ == token) {
			consistent = true;
			switch ( (*it)->type_ ) {
				case OT_STRING :
					(*it)->setValueParsed(value); break;
				case OT_BOOL_POS :
					(*it)->setValueParsed(true); break;
				case OT_BOOL_NEG :
					(*it)->setValueParsed(false); break;
				case OT_INT :
					(*it)->setValueParsed(atoi(value.c_str())); break;
				default : break;
			}
      if ((value != "") && ((*it)->type_ == OT_BOOL_POS || (*it)->type_ == OT_BOOL_NEG))
        parseValue(value);
		} // if 
		if ( consistent ) 
			return true;
		else { 
			//log_() << "Unknown option " << token << ", program terminated." << endl; 
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
  OptionList::iterator it;
  for ( it = options_.begin(); it != options_.end(); it++ ) 
    log_() <<  (*it)->toString();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

Config config;
