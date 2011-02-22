
#include <ProFUSE/Exception.h>
#include <cstdio>
#include <cstring>

using namespace ProFUSE;

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


const char *POSIXException::errorString()
{
    return strerror(error());
}

const char *ProDOSException::errorString()
{

    
    switch (error())
    {
        case badSystemCall:
            return "Bad System Call";
        case invalidPcount:
            return "Invalid Parameter Count";
        case gsosActive:
            return "GS/OS Active";
        case devNotFound:
            return "Device Not Found";     
        case invalidDevNum:
            return "Invalid Device Number";
        case drvrBadReq:
            return "Driver Bad Request";
        case drvrBadCode:
            return "Driver Bad Code";
        case drvrBadParm:
            return "Driver Bad Parameter";
        case drvrNotOpen:
            return "Driver Not Open";
        case drvrPriorOpen:
            return "Driver Prior Open";
        case irqTableFull:
            return "IRQ Table Full";
        case drvrNoResrc:
            return "Driver No Resource";
        case drvrIOError:
            return "Driver IO Error";
        case drvrNoDevice:
            return "Driver No Device";
        case drvrBusy:
            return "Driver Busy";
        case drvrWrtProt:
            return "Driver Write Protected";
        case drvrBadCount:
            return "Driver Bad Count";
        case drvrBadBlock:
            return "Driver Bad Block";
        case drvrDiskSwitch:
            return "Driver Disk Switch";
        case drvrOffLine:
            return "Driver Off Line";
        case badPathSyntax:
            return "Bad Path Syntax";
        case invalidRefNum:
            return "Invalid Ref Num";
        case pathNotFound:
            return "Path Not Found";
        case volNotFound:
            return "Volume Not Found";
        case fileNotFound:
            return "File Not Found";
        case dupPathName:
            return "Duplicate Path Name";
        case volumeFull:
            return "Volume Full";
        case volDirFull:
            return "Volume Directory Full";
        case badFileFormat:
            return "Bad File Format";
        case badStoreType:
            return "Bad Storage Type";
        case eofEncountered:
            return "End of File";
        case outOfRange:
            return "Out of Range";
        case invalidAccess:
            return "Invalid Access";
        case buffTooSmall:
            return "Buffer Too Small";
        case fileBusy:
            return "File Busy";
        case dirError:
            return "Directory Error";
        case unknownVol:
            return "Unknown Volume";
        case paramRangeError:
            return "Parameter Range Error";
        case outOfMem:
            return "Out of Memory";
        case dupVolume:
            return "Duplicate Volume";
        case notBlockDev:
            return "Not a Block Device";
        case invalidLevel:
            return "Invalid Level";
        case damagedBitMap:
            return "Damaged Bit Map";
        case badPathNames:
            return "Bad Path Names";
        case notSystemFile:
            return "Not a System File";
        case osUnsupported:
            return "OS Unsupported";
        case stackOverflow:
            return "Stack Overflow";
        case dataUnavail:
            return "Data Unavailable";
        case endOfDir:
            return "End Of Directory";
        case invalidClass:
            return "Invalid Class";
        case resForkNotFound:
            return "Resource Fork Not Found";
        case invalidFSTID:
            return "Invalid FST ID";
        case devNameErr:
            return "Device Name Error";
        case resExistsErr:
            return "Resource Exists Error";
        case resAddErr:
            return "Resource Add Error";
        
        default:
            return "";
    }
    return "";
}
