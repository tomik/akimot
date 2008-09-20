
#include "utils.h"

ostream& Logger::operator()(unsigned int level) {
	return cerr;
}
