
#include "Exception.h"
#include <cstdio>
#include <cstring>


Exception::Exception(const char *cp):
    _error(0),
    _string(cp)
{
}

Exception::Exception(const std::string& string):
    _error(0),
    _string(string)
{
}

Exception::Exception(const char *cp, int error):
    _error(error),
    _string(cp)
{
}

Exception::Exception(const std::string& string, int error):
    _error(error),
    _string(string)
{
}


Exception::~Exception() throw()
{
}

const char *Exception::what()
{
    return _string.c_str();
}

const char *Exception::errorString()
{
    return "";
}
