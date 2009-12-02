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

    
    virtual ~Exception() throw ();
    
    virtual const char *what();
    
    int error() const { return _error; }

protected:
    Exception(const char *cp, int error);
    Exception(const std::string& string, int error);

private:
    int _error;
    std::string _string;

};

class POSIXException : public Exception {
public:
    POSIXException(const char *cp, int error);
    POSIXException(const std::string& string, int error);
};

class ProDOSException : public Exception {
public:
    ProDOSException(const char *cp, int error);
    ProDOSException(const std::string& string, int error);
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

inline POSIXException::POSIXException(const char *cp, int error) :
    Exception(cp, error)
{
}

inline POSIXException::POSIXException(const std::string& string, int error) :
    Exception(string, error)
{
}

inline ProDOSException::ProDOSException(const char *cp, int error) :
    Exception(cp, error)
{
}

inline ProDOSException::ProDOSException(const std::string& string, int error) :
    Exception(string, error)
{
}



}

#endif