#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <exception>
#include <string>

class BaseException : public std::exception
{
protected:
	std::string m_message;

public:
	BaseException(std::string msg);
	virtual ~BaseException() throw();
	
	virtual const char* what() const throw();
};

class HaltUniversalMachine
{
};

#endif // __EXCEPTIONS_H__
