#include "Exceptions.h"

BaseException::BaseException(std::string msg)
	: m_message(msg)
{
}

BaseException::~BaseException() throw()
{
}

const char* BaseException::what() const throw()
{
	return m_message.c_str();
}
