#include <algorithm>
#include "Array.h"

using namespace std;

Array::Array(unsigned int s)
	: data(NULL)
	, size(0)
{
	if (s != 0) {
		size = s;
		data = new Platter[size];
		fill(data, data + size, 0);
	}
}

Array::~Array()
{
	delete[] data;
}
