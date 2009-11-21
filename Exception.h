#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <exception>
#include <string>

namespace ProFUSE {

class Exception : public std::exception
{
public:
    Exception(const char *cp);
    Exception(const std::string &str);
    Exception(const char *cp, int error);
    Exception(const std::string& string, int error);
    
    virtual ~Exception() throw ();
    
    virtual const char *what();
    
    int error() const;
private:
    int _error;
    std::string _string;

};

inline Exception::Exception(const char *cp):
    _error(0),
    _string(cp)
{
}

inline Exception::Exception(const std::string& string):
    _error(0),
    _string(string)
{
}

inline Exception::Exception(const char *cp, int error):
    _error(error),
    _string(cp)
{
}

inline Exception::Exception(const std::string& string, int error):
    _error(error),
    _string(string)
{
}

inline int Exception::error() const
{
    return _error;
}

}

#endif