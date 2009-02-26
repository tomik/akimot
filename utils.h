#pragma once

#include <cassert>


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <limits.h>
#include <math.h>

typedef unsigned long long u64;

typedef unsigned int uint;

using std::string ;

using std::fstream;
using std::ostream;
using std::istream;
using std::stringstream;

using std::cout ;
using std::endl;
using std::cerr;
using std::cin ;
using std::ios;

using std::pair;


enum logLevel_e { LL_DEBUG, LL_WARNING, LL_ERROR, LL_INFO, LL_RAW, LL_DDEBUG };

#define STR_LOAD_FAIL "Fatal error occured while loading position."


void logFunction(logLevel_e logLevel, const char* timestamp, const char* file, const char* function, int line, ...);

#define logInfo(...) logFunction(LL_INFO, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logWarning(...) logFunction(LL_WARNING, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logError(...) logFunction(LL_ERROR, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logRaw(...) logFunction(LL_RAW, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#ifdef NDEBUG 
  #define logDebug(...) ((void)0)
  #define logDDebug(...) ((void)0)
#else
  #define logDebug(...) logFunction(LL_DEBUG, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
  #ifdef DEBUG
    #define logDDebug(...) logFunction(LL_DDEBUG, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
  #else
    #define logDDebug(...) ((void)0)
  #endif 
#endif 

/**
 * Simpler file read.
 */
class FileRead
{
  public: 
    /**
     * Constructor with filename. 
     */
    FileRead(string fn);

    /**
     * Reads whole file as string.
     */
    string read();

    /**
     * Line read.
     *
     * @return True if read was successfull, 
     *         False otherwise. 
     */
    bool getLine(string & s);

    /**
     * Line read. 
     * 
     * Expects line to be a pair of stuff, separated by sep.
     * @param sep Separator - if not specified implicit ' ' is used.
     *
     * @return True if read was successfull,
     *         False otherwise.
     */
    bool getLineAsPair(string & s1, string & s2, const char* sep=NULL);

    /**
     * Set ignore lines flag (for comments, etc.)
     */
    void ignoreLines(const char * start);

    /**
     * Tells whether file is good to read.
     */
    bool good() const;

  private: 
    FileRead();
    fstream f_;
    string ignoreStart_;
};


/**
 * (game) random generator.
 */
class Grand
{
  public:
    Grand();
    void seed(unsigned int seed);
    unsigned int operator()();
  private:
    unsigned int high_;
    unsigned int low_;
};

extern Grand grand;
extern int smallPrimes[];
extern const int smallPrimesNum; 

/**
 * Logistic sigmoid.
 */
inline double log_sig(double param, double value)
{
  const double E = 2.71828182;
  double ret = 1/(1 + pow(E, -1 * param * value));
  assert(ret < 1 && ret > 0);
  return ret;
}

inline int smallRandomPrime()
{
  return smallPrimes[grand() % smallPrimesNum];
}

inline float random01()
{
  return (double)grand()/((double)(RAND_MAX) + (double)(1));
}

int max(int a, int b);

int min(int a, int b);

/**
 * String to int converter.
 */
int str2int(const string& str);

/**
 * String to float converter.
 */
float str2float(const string& str);

/**
 * Spaces trim from right.
 */
string trimRight(const string& str);

/**
 * Spaces trim from right.
 */
string trimLeft(const string& str);

/**
 * Spaces trim from both sides.
 */
string trim(const string& str);

/**
 * Get stream rest.
 */
string getStreamRest(istream& is);

/**
 * Tr analogy.
 *
 * Replaces all ocurences of c1 in s by c2.
 */
string replaceAllChars(string s, char c1, char c2);
