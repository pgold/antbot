#ifndef LOCATION_H_
#define LOCATION_H_

#include <ostream>

/*
    struct for representing locations in the grid.
*/
struct Location
{
    int row, col;

    Location()
    {
        row = col = 0;
    };

    Location(int r, int c)
    {
        row = r;
        col = c;
    };

	bool operator==(const Location &other) const {
		return row == other.row && col == other.col;
	}

	bool operator!=(const Location &other) const {
		return !(*this == other);
	}

	Location operator-(const Location &other) const {
		return Location(row - other.row, col - other.col);
	}

};

std::ostream& operator<<(std::ostream &os, const Location &location);
int norm1(const Location &location);
double norm2(const Location &location);

#endif //LOCATION_H_
