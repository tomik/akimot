
#include "utils.h"

ostream& Logger::operator()(unsigned int level) const{
	return cerr;
}
