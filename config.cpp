
/** 
 *  @file config.cpp
 *  @brief Configuration managemenent.
 *  @full Parsing arguments from command-line and storing them into 
 *    run-wide variables in class Config.
 */

#include "config.h"

//---------------------------------------------------------------------
//  section CfgItem 
//--------------------------------------------------------------------- 

CfgItem::CfgItem(const char* name, cfgItemType_e type, void* item, const char* defaultValue)
  : name_(string(name)), type_(type), item_(item), defaultValue_(defaultValue)
{
  set_ = false;
}

//---------------------------------------------------------------------
//  section Cfg 
//--------------------------------------------------------------------- 

Cfg::Cfg()
{
  items_.push_back(CfgItem("local_playout", IT_BOOL, (void*)&localPlayout_,"1"));
  items_.push_back(CfgItem("random_step_tries", IT_INT, (void*)&randomStepTries_,"5"));
  items_.push_back(CfgItem("fpu", IT_FLOAT, (void*)&fpu_,"1.1"));
  items_.push_back(CfgItem("knowledge_in_tree", IT_BOOL, (void*)&knowledgeInTree_,"1"));
  items_.push_back(CfgItem("explore_rate", IT_FLOAT, (void*)&exploreRate_,"0.2"));
  items_.push_back(CfgItem("playout_len", IT_INT, (void*)&playoutLen_,"3"));
  items_.push_back(CfgItem("mature_level", IT_INT, (void*)&matureLevel_,"20"));
  items_.push_back(CfgItem("tc_move_default", IT_FLOAT, (void*)&tcMoveDefault_,"1"));
  items_.push_back(CfgItem("exact_playout_value", IT_BOOL, (void*)&exactPlayoutValue_,"0"));
  items_.push_back(CfgItem("knowledge_in_playout", IT_BOOL, (void*)&knowledgeInPlayout_,"1"));
  items_.push_back(CfgItem("knowledge_tournament_size", IT_INT, (void*)&knowledgeTournamentSize_,"3"));
  items_.push_back(CfgItem("search_threads_num", IT_INT, (void*)&searchThreadsNum_,"1"));
}

//--------------------------------------------------------------------- 

void Cfg::loadFromFile(string fn)
{
  FileRead* f = new FileRead(fn);
  string name, value;
  f->ignoreLines(CFG_COMMENT);
  while (f->getLineAsPair(name, value, CFG_SEP)){
    //cerr << name << " : " << value << endl; 
    bool found = false;
    for (CfgItemListIter it = items_.begin(); it != items_.end(); it++){
      if (it->name_ == name) {
        it->set_ = true;
        switch (it->type_){
          case IT_BOOL : 
            *(bool*)(it->item_) = bool(str2int(value));
            break;
          case IT_INT : 
            *(int *)(it->item_) = str2int(value);
            break;
          case IT_FLOAT : 
            *(float *)(it->item_) = str2float(value);
            break;
          default:
            assert(false);
            logError("Unknown option type.");
            exit(1);
            break;
        }
        found = true; 
      } 
    }
    if (! found){
      logError("Unknown option %s.", name.c_str());
      exit(1);
    }
  }
}

//--------------------------------------------------------------------- 

bool Cfg::checkConfiguration()
{
  for (CfgItemListIter it = items_.begin(); it != items_.end(); it++){
    if (! it->set_)
      return false;
  }
  return true;
}

//---------------------------------------------------------------------
//  section Options 
//--------------------------------------------------------------------- 

Options::Options()
{
  fnInput_ = OptionString("i","input","input file - in combination with -g", OT_STRING, "");
  fnCfg_ = OptionString("c","cfg","Configuration file.", OT_STRING, "");
  fnAeiInit_ = OptionString("a","aeiinit","Aei init file", OT_STRING, "");
  benchmarkMode_ = OptionBool("b","benchmark","runs in benchmarkModeing mode",OT_BOOL_POS, false);
  localMode_ = OptionBool("l","local","runs in localMode - for debugging",OT_BOOL_POS, false);
  getMoveMode_ = OptionBool("g","getmove","runs in getMove mode",OT_BOOL_POS, false);

  options_.clear();
  options_.push_back(&fnAeiInit_);
  options_.push_back(&fnInput_);
  options_.push_back(&fnCfg_);
  options_.push_back(&benchmarkMode_);
  options_.push_back(&localMode_);
  options_.push_back(&getMoveMode_);
}

//--------------------------------------------------------------------- 

bool Options::parse(const int argc, const char ** argv ) 
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

bool Options::parseToken(string token, string value) {
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
			logError("Unknown option %s, program terminated.", token.c_str()); 
      exit(1);
    }
}

//--------------------------------------------------------------------- 

bool Options::parseValue(string value)
{
  //todo ... quite dummy - just sets value for file input - undummyfy 
  //for instance - mark options without name ( and go through them and parse values to them ) 
  fnInput_.setValueParsed(value);
  return true;
}

//--------------------------------------------------------------------- 

void Options::printAll()
{
  logRaw("Program configuration: ");
  OptionList::iterator it;
  for ( it = options_.begin(); it != options_.end(); it++ ) 
    logRaw(((*it)->toString()).c_str());
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

Options options;
Cfg cfg;
