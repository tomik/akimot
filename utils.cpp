#include "utils.h"

string logLevelStr[10] = { "debug", "warning", "error", "info"};
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
    if (logLevel == LL_RAW)
      std::cerr << fullmsg << std::endl;
    else
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

