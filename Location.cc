
#include "Location.h"

#include <cstdlib>
#include <cmath>


std::ostream& operator<<(std::ostream &os, const Location &location) {

	os << "(" << location.row << ", " << location.col << ")";
	return os;
}


int norm1(const Location &location)
{
	return abs(location.row) + abs(location.col);
}

double norm2(const Location &location)
{
	return sqrt(location.row*location.row + location.col*location.col);
}
