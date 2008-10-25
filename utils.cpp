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
