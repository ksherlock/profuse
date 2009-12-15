
#include <ProFUSE/Exception.h>

using namespace ProFUSE;

Exception::~Exception() throw()
{
}

const char *Exception::what()
{
    return _string.c_str();
}