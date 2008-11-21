#include "utils.h"

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

  buffer = new char[len - pos];
  is.read(buffer, len - pos);
  s = buffer;
  delete[] buffer;

  //set back g pointer
  is.seekg (pos, ios::beg);

  return trimLeft(s);
}

//--------------------------------------------------------------------- 

