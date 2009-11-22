
#include <cstring>
#include <memory>

#include "Directory"
#include "Buffer.h"
#include "Endian.h"

#include "Exception.h"


using namespace ProFUSE;
using namespace LittleEndian;

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

static bool islower(unsigned c)
{
    return c >= 'a' && c <= 'z';
}


/*
 * ProDOS names:
 * 1-15 letters, digits, or '.'
 * first character must be a letter.
 */
unsigned Entry::ValidName(const char *name)
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

Entry::Entry(unsigned type, const char *name)
{
    _storageType = type;
    _address = 0;
    _volume = NULL;
    
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
    _volume = NULL;
    _caseFlag = 0;
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
                _name[i] = _name[i] | 0x20;
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
        _namei[i] = c & ~0x20; // upper
        if (islower(c))
        {
            caseFlag |= (0x4000 >> i);
        }
    }
    
    _length = length;
    _caseFlag = caseFlag;
}







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
    void *dp = 4 + (const uint8_t *)bp;

    _creation = DateTime(Read16(dp, 0x18), Read16(dp, 0x1a));
    
    _version = Read8(dp, 0x1c);
    _minVersion = Read8(dp, 0x1d);
    
    _access = Read8(dp, 0x1e);
    _entryLength = Read8(dp, 0x1f);
    _entriesPerBlock = Read8(dp, 0x20);
    
    _fileCount = read16(dp, 0x21);
    
    // parse child file entries ... requires ability to read other blocks.
}

Directory::~Directory()
{
}

Directory::setAccess(unsigned access)
{
#undef __METHOD__
#define __METHOD__ "Directory::setAccess"

    if ((access & 0xe7) != access)
        throw Exception(__METHOD__ ": Illegal access.");
    _access = access;
    
    // todo -- mark dirty? update block?
}


#pragma mark VolumeDirectory

VolumeDirectory(const char *name, BlockDevice *device) :
    Directory(VolumeHeader, name)
{    
    _totalBlocks = device->blocks();
    _bitmapPointer = 6;
    
    _entryBlocks.push_back(2);
    _entryBlocks.push_back(3);
    _entryBlocks.push_back(4)
    _entryBlocks.push_back(5);
    
    std::auto_ptr<Bitmap> bitmap(new Bitmap(_totalBlocks));
    
    
    // 2 bootcode blocks
    for (unsigned i = 0; i < 2; ++i)
    {
        bitmap->allocBlock(i);
        device->zeroBlock(i);
    }
    
    //4 volume header blocks.
    for (unsigned i = 2; i < 6; ++i)
    {
        bitmap->allocBlock(i);
        
        buffer.clear();
        // prev block, next block
        buffer.push16le(i == 2 ? 0 : i - 1);
        buffer.push16le(i == 5 ? 0 : i + 1);
        
        if (i == 2)
        {
            // create the volume header.
            // all ivars must be set.
            write(&out);
        }
        
        buffer.rezize(512);
        device->write(i, buffer.buffer());
    }

    // TODO -- create/write the volume entry....

    // allocate blocks for the bitmap itself
    unsigned bb = bitmap->bitmapBlocks();    
    for (unsigned i = 0; i < bb; ++i)
        bitmap->allocBlock(i);
    
    // now write the bitmap...
    const uint8_t *bm = (const uint8_t *)bitmap->bitmap();
    for (unsigned i = 0; i < bb; ++i)
    {
        device->writeBlock(_bitmapPointer + i, 512 * i + bm);
    }
    
    setDevice(device);
    _bitmap = bitmap.release();
}

VolumeDirectory::~VolumeDirectory()
{
}

void VolumeDirectory::write(Buffer *out)
{
    out->push8((VolumeHeader << 4 ) | (nameLength());
    out->pushBytes(namei(), 15);
    
    // reserved.  SOS uses 0x75 for the first byte [?] 
    out->push8(0);
    out->push8(0);
    
    // last mod
    out->push16le(_modification.date());
    out->push16le(_modification.time());
    
    // filename case bits
    out->push16le(caseFlag());
    
    // creation
    out->push16le(creation().date());
    out->push16le(creation().time());    

    out->push8(version());
    out->push8(minVersion());
    

    out->push8(access());
    out->push8(entryLength());
    out->push8(entriesPerBlock());
    out->push16le(fileCount());
    
    out->push16le(_bitmapPointer());
    out->push16le(_totalBlocks);
}

#pragma mark SubDirectory

SubDirectory::SubDirectory(unsigned type, const char *name) :
    Directory(type, name)
{
    _parentPointer = 0;
    _parentEntryNumber = 0;
    _parentEntryLength = 0x27;
}

SubDirectory::SubDirectory(const void *bp) :
    Directory(bp)
{
    const void *dp = 4 + (const uint8_t *)bp;
    
    _parentPointer = Read16(dp, 0x23);
    _parentEntryNumber = Read8(dp, 0x25);
    _parentEntryLength = Read8(dp, 0x26);
}

SubDirectory::~SubDirectory
{
}

void SubDirectory::write(Buffer *out)
{
    out->push8((VolumeHeader << 4 ) | (nameLength());
    out->pushBytes(namei(), 15);
    
    // reserved.  0x76 is stored in the first byte.
    // no one knows why.
    out->push8(0x76);
    out->push8(0);
    out->push8(0);
    out->push8(0);
    out->push8(0);
    out->push8(0);
    out->push8(0);
    out->push8(0);

    // creation
    out->push16le(creation().date());
    out->push16le(creation().time());    
    
    out->push8(version());
    out->push8(minVersion());
    
    out->push8(access());
    out->push8(entryLength());
    out->push8(entriesPerBlock());
    out->push16le(fileCount());
    out->push16le(_parentPointer);
    out->push8(_parentEntryNumber);
    out->push8(_parentEntryLength);
}




FileEntry::FileEntry(unsigned type, const char *name) :
    Entry(type, name)
{
    _fileType = type == DirectoryFile ? 0x0f : 0x00;
    _keyPointer = 0;
    _blocksUsed = 0;
    _eof = 0;
    _access = 0xe3;
    _auxType = 0;
    _headerPointer = 0;
}


FileEntry::FileEntry(const void *vp) :
    Entry(vp),
    _creation(0,0),
    _modification(0,0)
{
#undef __METHOD__
#define __METHOD__ "FileEntry::FileEntry"

    switch(storageType())
    {
    case SeedlingFile:
    case SaplingFile:
    case TreeFile:
    case PascalFile:
    case ExtendedFile:
    case DirectoryFile:
        break;
    default:
        throw Exception(__METHOD__ ": Invalid storage type.");
    }


    _fileType = Read8(vp, 0x10);
    _keyPointer = Read16(vp, 0x11);
    _blocksUsed = Read16(vp, 0x13);
    _eof = Read24(vp, 0x15);
    _creation = DateTime(Read16(vp, 0x18), Read16(vp, 0x1a));
    _access = Read8(vp, 0x1e);
    _auxType = Read16(vp, 0x1f);
    _modification = DateTime(Read16(vp, 0x21), Read16(vp, 0x23));
    _headerPointer = Read16(vp, 0x25);


}

FileEntry::~FileEntry()
{
}


void FileEntry::write(Buffer *out)
{
    out->push8((VolumeHeader << 4 ) | (nameLength());
    out->pushBytes(namei(), 15);
    
    out->push8(_fileType);
    out->push16le(_keyPointer);
    out->push16le(_blocksUsed);
    out-push24le(_eof);

    // creation
    out->push16le(_creation.date());
    out->push16le(_creation.time());    
    
    // case bits
    out->push16le(caseFlag());
    
    out->push8(_access);
    put->push16le(_auxType);
        
    // modification
    out->push16le(_modification.date());
    out->push16le(_modification.time());       

    out->push16le(_headerPointer);
}