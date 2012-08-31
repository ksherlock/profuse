#ifndef __COMMON_EXCEPTION_H__
#define __COMMON_EXCEPTION_H__

#include <string>
#include <exception>


class Exception : public std::exception
{
public:
    Exception(const char *cp);
    Exception(const std::string &str);

    
    virtual ~Exception() throw ();
    
    virtual const char *what() const throw();
    virtual const char *errorString();
    
    int error() const { return _error; }

protected:
    Exception(const char *cp, int error);
    Exception(const std::string& string, int error);

private:
    int _error;
    std::string _string;

};





#endif