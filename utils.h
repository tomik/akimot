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


enum logLevel_e { LL_DEBUG, LL_WARNING, LL_ERROR, LL_INFO, LL_RAW};

#define STR_LOAD_FAIL "Fatal error occured while loading position."


void logFunction(logLevel_e logLevel, const char* timestamp, const char* file, const char* function, int line, ...);

#define logInfo(...) logFunction(LL_INFO, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logWarning(...) logFunction(LL_WARNING, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logError(...) logFunction(LL_ERROR, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define logRaw(...) logFunction(LL_RAW, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#ifdef NDEBUG 
  #define logDebug(...) ((void)0)
#else
  #define logDebug(...) logFunction(LL_DEBUG, __TIME__, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif 

/**
 * String to int converter.
 */
int str2int(const string& str);

/**
 * Spaces trim from right.
 */
string trimRight(const string& str);

/**
 * Spaces trim from right.
 */
string trimLeft(const string& str);


/**
 * Get stream rest.
 */
string getStreamRest(istream& is);
