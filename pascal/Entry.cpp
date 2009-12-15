#include <Pascal/File.h>

#include <ProfUSE/auto.h>
#include <ProFUSE/Exception.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <Device/BlockDevice.h>
#include <Device/BlockCache.h>


#include <algorithm>
#include <cstring>
#include <cctype>
#include <memory>

using namespace LittleEndian;
using namespace Pascal;


#pragma mark -
#pragma mark DirEntry

unsigned Entry::ValidName(const char *cp, unsigned maxLength)
{

    // any printable char except:
    // white space
    // $ = ? , (file only)
    // : (volume only)
    if (!cp || !*cp) return 0;
    
    for (unsigned i = 0;  ; ++i)
    {
        unsigned c = cp[i];
        if (c == 0) return i;
        if (i >= maxLength) return 0;
    
        switch(c)
        {

        case ':':
            if (maxLength == 7) return 0;
            break;
        case '$':
        case '=':
        case ',':
        case '?':
            if (maxLength == 15) return 0;
            break;
            
        default:
            if (!::isascii(c)) return 0;
            if (!std::isgraph(c)) return 0;
        }
    }


}


Entry::Entry()
{
    _firstBlock = 0;
    _lastBlock = 0;
    _fileKind = 0;
    _inode = 0;
    _parent = NULL;
}

Entry::Entry(void *vp)
{
    init(vp);
}

Entry::~Entry()
{
}


void Entry::init(void *vp)
{
    _firstBlock = Read16(vp, 0x00);
    _lastBlock = Read16(vp, 0x02);
    _fileKind = Read8(vp, 0x04);
    
    _inode = 0;
}

void Entry::writeDirectoryEntry(IOBuffer *b)
{
    b->write16(_firstBlock);
    b->write16(_lastBlock);
    b->write8(_fileKind);
}


