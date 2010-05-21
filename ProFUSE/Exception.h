#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <string>
#include <exception>

namespace ProFUSE {
    

// ProDOS Errors
enum
{
    badSystemCall = 0x01,
    invalidPcount = 0x04,
    gsosActice = 0x07,
    devNotFound = 0x10,
    invalidDevNum = 0x11,
    drvrBadReq = 0x20,
    drvrBadCode = 0x21,
    drvrBadParm = 0x22,
    drvrNotOpen = 0x23,
    drvrPriorOpen = 0x24,
    irqTableFull = 0x25,
    drvrNoResrc = 0x26,
    drvrIOError = 0x27,
    drvrNoDevice = 0x28,
    drvrBusy = 0x29,
    drvrWrtProt = 0x2b,
    drvrBadCount = 0x2c,
    drvrBadBlock = 0x2d,
    drvrDiskSwitch = 0x2e,
    drvrOffLine = 0x2f,
    badPathSyntax = 0x40,
    invalidRefNum = 0x43,
    pathNotFound = 0x44,
    volNotFound = 0x45,
    fileNotFound = 0x46,
    dupPathName = 0x47,
    volumeFull = 0x48,
    volDirFull = 0x49,
    badFileFormat = 0x4a,
    badStoreType = 0x4b,
    eofEncountered = 0x4c,
    outOfRange = 0x4d,
    invalidAccess = 0x4e,
    buffTooSmall = 0x4f,
    fileBusy = 0x50,
    dirError = 0x51,
    unknownVol = 0x52,
    paramRangeError = 0x53,
    outOfMem = 0x54,
    dupVolume = 0x57,
    notBlockDev = 0x58,
    damagedBitMap = 0x5a,
    badPathNames = 0x5b,
    notSystemFile = 0x5c,
    osUnsupported = 0x5d,
    stackOverflow = 0x5f,
    dataUnavail = 0x60,
    endOfDir = 0x61,
    invalidClass = 0x62,
    resForkNotFound = 0x63,
    invalidFSTID = 0x64,
    devNameErr = 0x67,
    resExistsErr = 0x70,
    resAddErr = 0x71
};

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