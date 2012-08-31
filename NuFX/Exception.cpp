#include "Exception.h"

namespace NuFX {

const char *NuFX::Exception::errorString()
{
    return ::NuStrError((NuError)error());
}

}