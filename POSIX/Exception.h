#ifndef __POSIX_EXCEPTION_H__
#define __POSIX_EXCEPTION_H__

#include <Common/Exception.h>

namespace POSIX {
    

class Exception : public ::Exception {
public:
    Exception(const char *cp, int error);
    Exception(const std::string& string, int error);
    
    virtual const char *errorString();    
    
private:
    typedef ::Exception super;
};


inline Exception::Exception(const char *cp, int error) :
    super(cp, error)
{
}

inline Exception::Exception(const std::string& string, int error) :
    super(string, error)
{
}


}

#endif