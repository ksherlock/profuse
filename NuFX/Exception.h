#ifndef __NUFX_EXCEPTION_H__
#define __NUFX_EXCEPTION_H__

#include <Common/Exception.h>
#include <NufxLib.h>

namespace NuFX {


class Exception : public ::Exception
{
public:    
    Exception(const char *cp, NuError error);
    Exception(const std::string& string, NuError error);
    
    virtual const char *errorString();

private:
    typedef ::Exception super;
};   


inline Exception::Exception(const char *cp, NuError error) :
    super(cp, error)
{
}

inline Exception::Exception(const std::string& string, NuError error) :
    super(string, error)
{
}

}

#endif