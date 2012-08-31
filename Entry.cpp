#include "Entry.h"
#include "Endian.h"
#include "Buffer.h"
#include "Exception.h"

using namespace ProFUSE;
using namespace LittleEndian;

// do it ourselves since could be different locale or something.
#undef isalpha
#undef isalnumdot
#undef islower
#undef tolower
#undef toupper

static bool isalpha(char c)
{
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z') ;
}

static bool isalnumdot(char c)
{
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <='9')
        || (c == '.') ;

}

static bool islower(char c)
{
    return c >= 'a' && c <= 'z';
}


inline char tolower(char c) { return c | 0x20; }
inline char toupper(char c) { return c & ~0x20; }

/*
 * ProDOS names:
 * 1-15 letters, digits, or '.'
 * first character must be a letter.
 */
unsigned Entry::ValidName(const char *name)
{
    unsigned length = 1;
    
    if (!name) return 0;
    
    if (!isalpha(*name)) return 0;
    
    for (length = 1; length < 17; ++length)
    {
        char c = name[length];
        if (isalnumdot(c)) continue;
        if (c == 0) break;

        return 0;        
    }
    
    if (length > 15) return 0;
    return length;
}


Entry::Entry(unsigned type, const char *name)
{
    _address = 0;
    _index = 0;
    _volume = NULL;
    _nameLength = 0;
    _caseFlag = 0;
    _storageType = type;
    
    // everything else initialized in setName.
    setName(name);
}

Entry::Entry(const void *bp)
{
#undef __METHOD__
#define __METHOD__ "Entry::Entry"

    uint8_t x;
    uint16_t xcase = 0;
    
    _address = 0;
    _index = 0;
    _volume = NULL;
    
    _caseFlag = 0;
    _nameLength = 0;
    
    std::memset(_name, 0, 16);
    std::memset(_namei, 0, 16);
        
    x = Read8(bp, 0x00);
    _storageType = x >> 4;
    _nameLength = x & 0x0f;
    
    for (unsigned i = 0; i < _nameLength; ++i)
    {
        _name[i] = _namei[i] = Read8(bp, i + 1);
        // check legality?
    }
    
    switch (_storageType)
    {
    case SeedlingFile:
    case SaplingFile:
    case TreeFile:
    case PascalFile:
    case ExtendedFile:
    case DirectoryFile:
        xcase = Read16(bp, 0x1c);
        break;
    
    case VolumeHeader:
        xcase = Read16(bp, 0x16);
        break;
    }
    
    if (xcase  & 0x8000)
    {
        _caseFlag = xcase;
        xcase <<= 1;
        for (unsigned i = 0; i < _nameLength; ++i, xcase <<= 1)
        {
            if (xcase & 0x8000)
                _name[i] = tolower(_name[i]);
        }
    }
}

Entry::~Entry()
{
}

/*
 * IIgs Technote #8:
 *
 * bit 15 set
 * bit 14 + i set if name[i] is lowercase.
 */
void Entry::setName(const char *name)
{
#undef __METHOD__
#define __METHOD__ "Entry::setName"

    unsigned caseFlag = 0x8000;
    unsigned length = ValidName(name);
    if (!length) throw Exception("Invalid name.");
    
    std::memset(_name, 0, 16);
    std::memset(_namei, 0, 16);
        
    for (unsigned i = 0; i < length; ++i)
    {
        char c = name[i];
        _name[i] = c;
        _namei[i] = toupper(c);
        if (islower(c))
        {
            caseFlag |= (0x4000 >> i);
        }
    }
    
    _nameLength = length;
    _caseFlag = caseFlag;
}


