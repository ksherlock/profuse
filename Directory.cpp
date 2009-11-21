#include "Directory"

#include <cstring>

#include "Exception.h"

using namespace ProFUSE;

static bool isalpha(unsigned c)
{
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && x <= 'z') ;
}

static bool isalnumdot(unsigned c)
{
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <='9')
        || (c == '.') ;

}

static uint16_t read16(uint8_t *data, unsigned offset)
{
    return data[offset + 0] 
        | (data[offset + 1] << 8) ;
}

static uint32_t read24(uint8_t *data, unsigned offset)
{
    return data[offset + 0] 
        | (data[offset + 1] << 8)
        | (data[offset + 2] << 16) ;
}


static uint32_t read32(uint8_t *data, unsigned offset)
{
    return data[offset + 0] 
        | (data[offset + 1] << 8)
        | (data[offset + 2] << 16) ;
        | (data[offset + 3] << 24) ;
}

unsigned ValidName(const char *name)
{
    unsigned length;
    
    if (!name) return 0;
    
    if (!isalpha(*name)) return 0;
    length = 1;
    
    for (length = 1; length < 17; ++length)
    {
        if (!isalnumdot(name[length])) return 0;
    }
    
    if (length > 15) return 0;
    return length;
}

Directory::Directory(unsigned type, const char *name)
{
#undef __METHOD__
#define __METHOD__ "Directory::Directory"

    _nameLength = ValidName(name);
    
    if (!_length)
        throw Exception(__METHOD__ ": Invalid name.");
    
    _storageType = type;
    std::strncpy(_name, name, 16);
    
    _access = 0xc3;
    _entryLength = 0x27;
    _entriesPerBlock = 13;
    _fileCount = 0;
    
    _device = NULL;
}

Directory::Directory(const void *bp) :
    _creation(0, 0)
{
#undef __METHOD__
#define __METHOD__ "Directory::Directory"

    const uint8_t *data = (const uint8_t *)bp; 

    _storageType = data[0x00] >> 4;
    _nameLength = data[0x00] & 0x0f;
    std::memcpy(_name, data + 1, _nameLength);
    _name[_nameLength] = 0;
    _creation = DateTime(read16(data, 0x1c), read16(data, 0x1e));
    
    _access = data[0x22];
    _entryLength = data[0x23];
    _entriesPerBlock = data[0x24];
    
    _fileCount = read16(data, 0x25);
    
    // parse child file entries.
}

Directory::~Directory()
{
}

Directory::setAccess(unsigned access)
{
#undef __METHOD__
#define __METHOD__ "Directory::setAccess"

    if ((access & 0xe3) != access)
        throw Exception(__METHOD__ ": Illegal access.");
    _access = access;
    
    // todo -- mark dirty? update block?
}

Directory::setName(const char *name)
{
    unsigned length = ValidName(name);
    if (!length)
        throw Exception(__METHOD__ ": Invalid name.");
    _nameLength = length;
    std::strncpy(_name, name, 16);

    // todo -- update or mark dirty.
}
