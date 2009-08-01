
/** 
 *  @file config.cpp
 *  @brief Configuration managemenent.
 *  @full Parsing arguments from command-line and storing them into 
 *    run-wide variables in class Config.
 */

#include "config.h"

//for values
#include "eval.h"

string getItemAsString(void* item, itemType_e type, int index)
{ 
  stringstream ss;

  switch (type){
    case IT_STR : 
      ss << *(string*)(item);
      break;
    case IT_BOOL : 
      ss << *(bool*)(item); 
      break;
    case IT_INT : 
      ss << *(int *)(item);
      break;
    case IT_FLOAT : 
      ss << *(float *)(item);
      break;
    case IT_FLOAT_AR: 
      assert(index >= 0);
      ss << (*(float **)(item))[index]; 
      break;
    case IT_INT_AR:   
      assert(index >= 0);
      ss <<  ((int *)(item))[index];
      break;
    default:
      assert(false);
      break;
  }

  return ss.str();

}

bool fillItemFromString(void* item, itemType_e type, string value, int index){ 
  //TODO ... exception to check types ? 
  switch (type){
    case IT_STR : 
      *(string*)(item) = value;
      break;
    case IT_BOOL : 
      *(bool*)(item) = bool(str2int(value));
      break;
    case IT_INT : 
      *(int *)(item) = str2int(value);
      break;
    case IT_FLOAT : 
      *(float *)(item) = str2float(value);
      break;
    case IT_FLOAT_AR: 
      assert(index >= 0);
      (*(float **)(item))[index] = str2float(value);
      break;
    case IT_INT_AR:   
      assert(index >= 0);
      ((int *)(item))[index] = str2int(value);
      break;
    default:
      assert(false);
      return false;
      break;
  }
  return true;
  
}

//---------------------------------------------------------------------
//  section CfgItem 
//--------------------------------------------------------------------- 

CfgItem::CfgItem(const char* name, itemType_e type, void* item, const char* defaultValue)
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
  items_.push_back(CfgItem("use_best_eval", IT_BOOL, (void*)&useBestEval_,"1"));
  items_.push_back(CfgItem("extensions_in_eval", IT_BOOL, (void*)&extensionsInEval_,"0"));
  items_.push_back(CfgItem("uct_transposition_tables", IT_BOOL, (void*)&uct_tt_,"1"));
  items_.push_back(CfgItem("fpu", IT_FLOAT, (void*)&fpu_,"1.1"));
  items_.push_back(CfgItem("ucb_tuned", IT_BOOL, (void*)&ucbTuned_,"0"));
  items_.push_back(CfgItem("dynamic_exploration", IT_BOOL, (void*)&dynamicExploration_,"0"));
  items_.push_back(CfgItem("children_cache", IT_BOOL, (void*)&childrenCache_,"0"));
  items_.push_back(CfgItem("knowledge_in_tree", IT_BOOL, (void*)&knowledgeInTree_,"0"));
  items_.push_back(CfgItem("uct_relative_update", IT_BOOL, (void*)&uctRelativeUpdate_,"1"));
  items_.push_back(CfgItem("history_heuristic", IT_BOOL, (void*)&historyHeuristic_,"0"));
  items_.push_back(CfgItem("explore_rate", IT_FLOAT, (void*)&exploreRate_,"0.2"));
  items_.push_back(CfgItem("playout_len", IT_INT, (void*)&playoutLen_,"3"));
  items_.push_back(CfgItem("mature_level", IT_INT, (void*)&matureLevel_,"15"));
  items_.push_back(CfgItem("tc_move_default", IT_FLOAT, (void*)&tcMoveDefault_,"1"));
  items_.push_back(CfgItem("exact_playout_value", IT_BOOL, (void*)&exactPlayoutValue_,"1"));
  items_.push_back(CfgItem("knowledge_in_playout", IT_BOOL, (void*)&knowledgeInPlayout_,"1"));
  items_.push_back(CfgItem("playout_by_moves", IT_BOOL, (void*)&playoutByMoves_,"0"));
  items_.push_back(CfgItem("move_advisor", IT_FLOAT, (void*)&moveAdvisor_,"0"));
  items_.push_back(CfgItem("active_trapping", IT_FLOAT, (void*)&activeTrapping_,"0"));
  items_.push_back(CfgItem("knowledge_tournament_size", IT_INT, (void*)&knowledgeTournamentSize_,"3"));
  items_.push_back(CfgItem("search_threads_num", IT_INT, (void*)&searchThreadsNum_,"1"));
  //items_.push_back(CfgItem("evaluation_config", IT_STR, (void*)&evalCfg_,""));
  vals_ = NULL; 
}

//--------------------------------------------------------------------- 

void Cfg::loadFromFile(string fn)
{
  FileRead* f = new FileRead(fn);
  string name;
  f->ignoreLines(CFG_COMMENT);
  stringstream ss; 
  string sectionContent;

  if (! f->good()){
    logError("Cannot open configuration file.");
    exit(1);
  } 

  ss.str(replaceAllChars(f->read(), CFG_SEP, ' '));
  while (ss.good()){
    ss >> name;
    if (name[0] == '[' && name[name.length() - 1] == ']'){
      //it is a section
      string rest = getStreamRest(ss);
      uint index = rest.length();
      for (uint i = 0; i < rest.length(); i++){
        if (rest[i] == '['){
          index = i;
          break;
        }
      }
      sectionContent = rest.substr(0, index);
      //handle empty section
      if (! trim(sectionContent).length()){
        continue;
      }

      //dispatch the section content
      if (name == "[evaluation_values]"){
        vals_ = new Values(sectionContent);
      }else if (name == "[step_knowledge]"){
      }else { 
        loadFromSection(sectionContent);
      }

      ss.str("");
      if (index < rest.length()){
        ss.str(rest.substr(index));
      }
    }

  }

  if (! vals_) {
    logWarning("Wrong evaluation configuration ... falling to implicit evaluation.");
    vals_ = new Values();
  }
}

//--------------------------------------------------------------------- 

void Cfg::loadFromSection(string content)
{
  string value, name;
  stringstream ss(content);

  while (ss.good()){
    ss >> name;
    ss >> value; 
    bool found = false;
    for (CfgItemListIter it = items_.begin(); it != items_.end(); it++){
      if (it->name_ == name) {
        it->set_ = true;
        if (! fillItemFromString(it->item_, it->type_, value)){
            logError("Unknown option type.");
            exit(1);
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
  bool res = true;
  for (CfgItemListIter it = items_.begin(); it != items_.end(); it++){
    if (! it->set_){
      logWarning("Missing configuration item %s.", it->name_.c_str());
      res = false;
    }
  }
  return res;
}

//---------------------------------------------------------------------
//  section Options 
//--------------------------------------------------------------------- 

Options::Options()
{
  fnPosition_ = OptionString("","Position","Position file - in combination with -g.", OT_STRING, "");
  fnRecord_ = OptionString("","Record","Record file - in combination with -g.", OT_STRING, "");
  fnGameState_ = OptionString("","GameState","GameState file - in combination with -g.", OT_STRING, "");
  fnCfg_ = OptionString("c","cfg","Configuration file (substitute for default.cfg).", OT_STRING, "");
  fnAeiInit_ = OptionString("a","aeiinit","Aei init file.", OT_STRING, "");
  benchmarkMode_ = OptionBool("b","benchmark","Toggle benchmark mode.",OT_BOOL_POS, false);
  localMode_ = OptionBool("l","local","Use AEI extended set (mostly for debugging).",OT_BOOL_POS, false);
  getMoveMode_ = OptionBool("g","getmove","Toggle getMove mode.",OT_BOOL_POS, false);
  help_ = OptionBool("h", "help", "Print this help.", OT_BOOL_POS, false);

  options_.clear();
  options_.push_back(&fnAeiInit_);
  options_.push_back(&fnCfg_);
  options_.push_back(&benchmarkMode_);
  options_.push_back(&localMode_);
  options_.push_back(&getMoveMode_);
  options_.push_back(&help_);

  values_.clear();
  //order in which options are expected is important
  values_.push_back(&fnPosition_);
  values_.push_back(&fnRecord_);
  values_.push_back(&fnGameState_);
}

//--------------------------------------------------------------------- 

bool Options::parse(const int argc, const char ** argv ) 
{
  string token = "";
  string value = "";

  for ( int i = 1; i  < argc ; i++ ) {
    // argument is token
    if ( argv[i][0] == '-' ) {
      //something failed
      if ( token != "" && ! parseToken(token, value))
        return false;
      // set new actual token
      token = argv[i];
      value.clear();
    //argument is a token's value
    }else {
      value = argv[i];
      //there already was a token and it's value => parse them first 
      if ( token != "" ){
        if (parseToken(token, value) ){
          token.clear();
          value.clear();
        }
        else 
          return false;
      }
      //token is =="" or value ==""
      else {
        parseValue(value);
      }
    }
  }
  //something failed 
  if ( token != "" && ! parseToken(token, value))
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
        if (! parseValue(value)) {
			    return false;
        }
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
  if (values_.empty()) {
    logError("Wrong command line configuration ... no value to parse.");  
    return false;
  }
  values_.front()->setValueParsed(value);
  values_.pop_front();
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

string Options::helpToString()
{
  stringstream ss;
  OptionList::iterator it;
  ss << "Akimot command line help" << endl << "syntax: akimot [options] [files]" << endl;
  for (it = options_.begin(); it != options_.end(); it++ ) {
    ss << (*it)->help();
  }
  return ss.str();
}

//--------------------------------------------------------------------- 
//--------------------------------------------------------------------- 

Options options;
Cfg cfg;
