#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "Defs.h"

class Array
{
public:
	Platter* data;
	unsigned int size;

public:
	Array(unsigned int s = 0);
	virtual ~Array();	
};

#endif // __ARRAY_H__
