
#include <cstring>
#include <memory>

#include "Entry.h"
#include "Buffer.h"
#include "Endian.h"

#include "Exception.h"


using namespace ProFUSE;
using namespace LittleEndian;



Directory::Directory(unsigned type, const char *name) :
    Entry(type, name)
{
#undef __METHOD__
#define __METHOD__ "Directory::Directory"

    
    _access = 0xe3;
    _entryLength = 0x27;
    _entriesPerBlock = 13;
    _fileCount = 0;
    _version = 5; // ProDOS FST uses 5, ProDOS uses 0.
    _minVersion = 0;
}

Directory::Directory(const void *bp) :
    Entry(4 + (const uint8_t *)bp),
    _creation(0, 0)
{
#undef __METHOD__
#define __METHOD__ "Directory::Directory"

    // input is a block pointer.  To simplify,
    // create a new pointer past the 4-byte linked list part.
    const void *dp = 4 + (const uint8_t *)bp;

    _creation = DateTime(Read16(dp, 0x18), Read16(dp, 0x1a));
    
    _version = Read8(dp, 0x1c);
    _minVersion = Read8(dp, 0x1d);
    
    _access = Read8(dp, 0x1e);
    _entryLength = Read8(dp, 0x1f);
    _entriesPerBlock = Read8(dp, 0x20);
    
    _fileCount = Read16(dp, 0x21);
    
    // parse child file entries ... requires ability to read other blocks.
}

Directory::~Directory()
{
}

void Directory::setAccess(unsigned access)
{
#undef __METHOD__
#define __METHOD__ "Directory::setAccess"


    _access = access;
    
    // todo -- mark dirty? update block?
}

