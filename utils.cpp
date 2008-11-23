#include "utils.h"

string logLevelStr[10] = { "debug", "warning", "error", "info"};
string logSectionStr[10] = { "uct", "board", "hash", "test", "aei", "eval", "other"};

Logger::Logger()
{
}


Logger::Logger(string owner="owner")
{
  owner_ = owner;
}


ostream& Logger::operator()(unsigned int messageLevel) const{
  /*

  int logLevel = 0

  #ifdef DEBUG1
   logLevel = 1 
  #endif
  #ifdef DEBUG2
   logLevel = 2 
  #endif
  #ifdef DEBUG3
   logLevel = 3 
  #endif

  */
  return cerr;
}

//--------------------------------------------------------------------- 

void logFunction(logLevel_e logLevel, const char* timestamp, const char* file, const char* function, int line, ...)
{

    va_list args;
    va_start(args, line);

    char* msg = va_arg(args, char *);

    char * fullmsg;
    //compiler dependend function - allocates memory to fullmsg
    vasprintf(&fullmsg, msg, args);
    va_end(args);
    std::cerr << timestamp << " --- " << "["  << logLevelStr[logLevel]  << "] " << file << "/" << function << "/" << line <<  ":" << fullmsg << std::endl;
    free(fullmsg);

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

