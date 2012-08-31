
#include "Exception.h"
#include <cstdio>
#include <cstring>

namespace POSIX {

const char *Exception::errorString()
{
    return strerror(error());
}

}