#include "utils.h"

#define INITIAL_SEED 0x38F271A
#define PAIR_SEED 0x49616E42

string logLevelStr[10] = { "debug", "warning", "error", "info", "", "ddebug" };
string logSectionStr[10] = { "uct", "board", "hash", "test", "aei", "eval", "other"};

void logFunction(logLevel_e logLevel, const char* timestamp, const char* file, const char* function, int line, ...)
{

    va_list args;
    va_start(args, line);

    char* msg = va_arg(args, char *);

    char * fullmsg;
    //compiler dependend function - allocates memory to fullmsg
    vasprintf(&fullmsg, msg, args);
    va_end(args);
    if (logLevel == LL_RAW || logLevel == LL_DDEBUG)
      std::cerr << fullmsg << std::endl;
    else
      std::cerr << timestamp << " --- " << "["  << logLevelStr[logLevel]  << "] " << file << "/" << function << "/" << line <<  ":" << fullmsg << std::endl;
    free(fullmsg);

}

//--------------------------------------------------------------------- 
// section FileRead
//--------------------------------------------------------------------- 

FileRead::FileRead(string fn){
  f_.open(fn.c_str(), fstream::in);
  ignoreStart_ = "";
}

//--------------------------------------------------------------------- 

FileRead::FileRead(){
  assert(false);
}

//--------------------------------------------------------------------- 

string FileRead::read() { 
  string line;
  string s = "";
  while (getLine(line)){
    s += line + '\n';
  }
  return s;
}

//--------------------------------------------------------------------- 

bool FileRead::getLine(string & s){
  while (f_.good()){
    getline(f_, s);
    if (s != "" && 
        (ignoreStart_ == "" 
        || trimLeft(s).find(ignoreStart_.c_str(), 0) != 0))
      return true;
  }
  return false;
} 

//--------------------------------------------------------------------- 

bool FileRead::getLineAsPair(string & s1, string & s2, const char * sep){
  string s;
  stringstream ss;
  ss.str("");
  while (f_.good()){
    getline(f_, s);
    if (s != "" &&
        ( ignoreStart_ == ""
          || trimLeft(s).find(ignoreStart_.c_str(), 0) != 0)){
        if (sep == NULL){
        ss.str(s);
        ss >> s1;
        s2 = getStreamRest(ss);
        return true;
      }
      else{ //valid separator
        string::size_type loc = s.find(sep, 0);
        if ( loc == string::npos) { //this should not happen !!!
          logError("Wrong format in intput: %s", s.c_str());
          exit(1);
        }
        s1 = trim(s.substr(0, loc));
        s2 = trim(s.substr(loc + 1));
        return true;
      }
    }
  }
  return false;
} 

//--------------------------------------------------------------------- 

void FileRead::ignoreLines(const char* ignoreStart)
{
  ignoreStart_ = string(ignoreStart);
}

//--------------------------------------------------------------------- 

bool FileRead::good() const
{
  return f_.good();
}

//--------------------------------------------------------------------- 

Grand::Grand() {
  seed(INITIAL_SEED);
}

//--------------------------------------------------------------------- 

Grand::Grand(unsigned int seed) {
  this->seed(seed);
}
//--------------------------------------------------------------------- 

void Grand::seed(unsigned int seed) {
  high_ = seed;
  low_ = high_ ^ PAIR_SEED;
}

//--------------------------------------------------------------------- 

unsigned int Grand::operator()() 
{
  return getOne();
}

//--------------------------------------------------------------------- 

unsigned int Grand::getOne() 
{
  high_ = (high_ << 16) + (high_ >> 16);
  high_ += low_;
  low_ += high_;
  return high_;
}

//--------------------------------------------------------------------- 

float Grand::get01() 
{
  return (double)getOne()/((double)(GRAND_MAX) + (double)(1));
}

//--------------------------------------------------------------------- 

int str2int(const string& str)
{
  stringstream ss(str);
  int n;
  ss >> n;
  return n;
}

//--------------------------------------------------------------------- 

float str2float(const string& str)
{
  stringstream ss(str);
  float f;
  ss >> f;
  return f;
}

//--------------------------------------------------------------------- 

string trimRight(const string& s)
{
  string ret = s;
  ret.erase(ret.find_last_not_of(' ') + 1);
  return ret;
}

//--------------------------------------------------------------------- 

string trimLeft(const string& s)
{
  string ret = s;
  ret.erase(0, ret.find_first_not_of(' '));
  return ret;
}

//--------------------------------------------------------------------- 

string trim(const string& s)
{
  return trimLeft(trimRight(s));
}

//--------------------------------------------------------------------- 

string getStreamRest(istream& is)
{

  stringstream ss ;

  char * buffer;
  int len;
  int pos;
  string s;

  //initial position
  pos = is.tellg();
  
  //save buffer length
  is.seekg (0, ios::end);
  len = is.tellg();

  is.seekg (pos, ios::beg);

  buffer = new char[len - pos + 1];
  is.read(buffer, len - pos);
  //TODO why not bugger[len - pos + 1] = 0 ? 
  buffer[len - pos] = 0; 
  s = string(buffer);
  delete[] buffer;

  //set back g pointer
  is.seekg (pos, ios::beg);

  //cout << endl << s << endl;

  return trimLeft(s);
}

//--------------------------------------------------------------------- 

string replaceAllChars(string s, char c1, char c2)
{
  for (uint i = 0; i < s.length(); i++){
    if (s[i] == c1){
      s[i] = c2;
    }
  }
  return s;
}

//--------------------------------------------------------------------- 
