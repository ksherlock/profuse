#include "File.h"
#include "../auto.h"
#include "../Endian.h"
#include "../BlockDevice.h"
#include "../BlockCache.h"
#include "../Exception.h"

#include "IOBuffer.h"

#include <algorithm>
#include <cstring>
#include <cctype>
#include <memory>

using namespace LittleEndian;
using namespace Pascal;

/*
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
*/


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


#pragma mark -
#pragma mark VolumeEntry

unsigned VolumeEntry::ValidName(const char *cp)
{
    // 7 chars max.  Legal values: ascii, printable, 
    // no space/tab,
    // no $=?,:
    
    return Entry::ValidName(cp, 7);
}

VolumeEntry::VolumeEntry()
{
    _fileNameLength = 0;
    std::memset(_fileName, 0, 8);
    _lastVolumeBlock = 0;
    _fileCount = 0;
    _accessTime = 0;
    
    setInode(1);
    
    _inodeGenerator = 1; 
    _cache = NULL;
    _device = NULL;
}

VolumeEntry::VolumeEntry(const char *name, ProFUSE::BlockDevice *device)
{
#undef __METHOD__
#define __METHOD__ "VolumeEntry::VolumeEntry"

    unsigned length;
    length = ValidName(name);
    
    if (!length)
        throw ProFUSE::Exception(__METHOD__ ": Invalid volume name.");
    
    _firstBlock = 0;
    _lastBlock = 6;
    _fileKind = kUntypedFile;
    _inode = 1;
    _inodeGenerator = 1;
    
    _fileNameLength = length;
    
    std::memset(_fileName, 0, sizeof(_fileName));
    for (unsigned i = 0; i < _fileNameLength; ++i)
    {
        _fileName[i] = std::toupper(name[i]);
    }
    
    _lastVolumeBlock = device->blocks();
    _fileCount = 0;
    _accessTime = 0;
    _lastBoot = Date::Today(); 
    
    _cache = device->blockCache();
    _device = device;
    
    for (unsigned i = 2; i < 6; ++i)
    {
        device->zeroBlock(i);
    }
    
    void *vp = _cache->load(2);
    IOBuffer b(vp, 0x1a);
    
    writeDirectoryEntry(&b);
        
    _cache->unload(2, true);
    
}


VolumeEntry::VolumeEntry(ProFUSE::BlockDevice *device)
{
    auto_array<uint8_t> buffer(new uint8_t[512]);
    unsigned blockCount;
    
    // read the header block, then load up all the header 
    // blocks.
    
    _device = device;
    _cache = device->blockCache();
    
    device->read(2, buffer.get());

    init(buffer.get());

    // todo -- verify reasonable values.
    
    //printf("%u %u\n", blocks(), _lastBlock - _firstBlock);
    
    // why the fuck didn't this work????
    blockCount = blocks();
    
    if (blockCount > 1)
    {
        buffer.reset(new uint8_t[512 * blockCount]);
        
        for (unsigned i = 0; i < blockCount; ++i)
        {
            device->read(2 + i, buffer.get() + 512 * i);
        }
    }
    
    // now load up all the children.
    // if this throws, memory could be lost...

    
    try
    {    
        
        for (unsigned i = 1; i <= _fileCount; ++i)
        {
            std::auto_ptr<FileEntry> child;
            
            //
            child.reset(new FileEntry(buffer.get() + i * 0x1a));
            
            child->setInode(++_inodeGenerator);
            child->_parent = this;
            _files.push_back(child.release());
        } 
    } 
    catch (...)
    {
        std::vector<FileEntry *>::iterator iter;
        for(iter = _files.begin(); iter != _files.end(); ++iter)
        {
            if (*iter) delete *iter;
        }
       
        throw;
    }


}

VolumeEntry::~VolumeEntry()
{
    std::vector<FileEntry *>::iterator iter;
    for(iter = _files.begin(); iter != _files.end(); ++iter)
    {
        if (*iter) delete *iter;
    }
    
    // _blockCache does not need deleting.
    delete _device;
}


void VolumeEntry::init(void *vp)
{
    Entry::init(vp);
    _fileNameLength = Read8(vp, 6);
    
    // verify filenamelength <= 7
    // verify fileKind == 0
    
    std::memcpy(_fileName, 7 + (uint8_t *)vp, _fileNameLength);
    
    _lastVolumeBlock = Read16(vp, 0x0e);
    _fileCount = Read16(vp, 0x10);
    _accessTime = Read16(vp, 0x12);
    _lastBoot = Date(Read16(vp, 0x14));
    
    setInode(1);
    _inodeGenerator = 1; 
}


FileEntry *VolumeEntry::fileAtIndex(unsigned i) const
{
    return i < _files.size() ? _files[i] : NULL;
}



void *VolumeEntry::loadBlock(unsigned block)
{
    return _cache->load(block);
}
void VolumeEntry::unloadBlock(unsigned block, bool dirty)
{
    return _cache->unload(block, dirty);
}

void VolumeEntry::readBlock(unsigned block, void *buffer)
{
    _device->read(block, buffer);
}
void VolumeEntry::writeBlock(unsigned block, void *buffer)
{
    _device->write(block, buffer);
}



void VolumeEntry::writeDirectoryEntry(IOBuffer *b)
{
    Entry::writeDirectoryEntry(b);
    
    b->write8(0); // reserved
    b->write8(_fileNameLength);
    b->writeBytes(_fileName, 7);
    b->write16(_lastVolumeBlock);
    b->write16(_fileCount);
    b->write16(_accessTime);
    b->write16(_lastBoot);
    
    // rest is reserved.
    b->writeZero(4);
}

#pragma mark -
#pragma mark FileEntry

unsigned FileEntry::ValidName(const char *cp)
{
    return Entry::ValidName(cp, 15);
}

FileEntry::FileEntry(void *vp) :
    Entry(vp)
{
    _status = Read8(vp, 0x05) & 0x01;
    _fileNameLength = Read8(vp, 0x06);
    std::memset(_fileName, 0, 16);
    std::memcpy(_fileName, 0x07 + (uint8_t *)vp, _fileNameLength);
    _lastByte = Read16(vp, 0x16);
    _modification = Date(Read16(vp, 0x18));
    
    _fileSize = 0;
    _pageSize = NULL;
}

FileEntry::FileEntry(const char *name, unsigned fileKind)
{
#undef __METHOD__
#define __METHOD__ "FileEntry::FileEntry"

    unsigned length = ValidName(name);
    
    if (!length)
        throw ProFUSE::Exception(__METHOD__ ": Invalid file name.");
        
    _fileKind = fileKind;
    _status = 0;
    
    _fileNameLength = length;
    std::memset(_fileName, 0, sizeof(_fileName));
    for (unsigned i = 0; i < length; ++i)
        _fileName[i] = std::toupper(name[i]);
    
    _modification = Date::Today();
    _lastByte = 0;
    
    _fileSize = 0;
    _pageSize = NULL;
}

FileEntry::~FileEntry()
{
    delete _pageSize;
}


unsigned FileEntry::fileSize()
{
    switch(fileKind())
    {
    case kTextFile:
        return textFileSize();
        break;
    default:
        return dataFileSize();
        break;
    }
}

void FileEntry::writeDirectoryEntry(IOBuffer *b)
{
    Entry::writeDirectoryEntry(b);
    
    b->write8(_status ? 0x01 : 0x00);
    b->write8(_fileNameLength);
    b->writeBytes(_fileName, 15);
    b->write16(_lastByte);
    b->write16(_modification);
}

int FileEntry::read(uint8_t *buffer, unsigned size, unsigned offset)
{
    unsigned fsize = fileSize();
    
    if (offset + size > fsize) size = fsize - offset;
    if (offset >= fsize) return 0;
    
    switch(fileKind())
    {
    case kTextFile:
        return textRead(buffer, size, offset);
        break;
    default:
        return dataRead(buffer, size, offset);
        break;
    }
}


unsigned FileEntry::dataFileSize()
{
    return blocks() * 512 - 512 + _lastByte;
}

unsigned FileEntry::textFileSize()
{
    if (!_pageSize) textInit();
    return _fileSize;  
}






int FileEntry::dataRead(uint8_t *buffer, unsigned size, unsigned offset)
{
    uint8_t tmp[512];
    
    unsigned count = 0;
    unsigned block = 0;
        
    block = _firstBlock + (offset / 512);
    
    // returned value (count) is equal to size at this point.
    // (no partial reads).
    
    /*
     * 1.  Block align everything
     */
    
    if (offset % 512)
    {
        unsigned bytes = std::min(offset % 512, size);
        
        parent()->readBlock(block++, tmp);
        
        std::memcpy(buffer, tmp + 512 - bytes, bytes);
        
        buffer += bytes;
        count += bytes;
        size -= bytes;
    }
    
    /*
     * 2. read full blocks into the buffer.
     */
     
     while (size >= 512)
     {
        parent()->readBlock(block++, buffer);
        
        buffer += 512;
        count += 512;
        size -= 512;
     }
     
    /*
     * 3. Read any trailing blocks.
     */
    if (size)
    {
        parent()->readBlock(block, tmp);
        std::memcpy(buffer, tmp, size);
        
        count += size; 
    
    }

    
    return count;
}



int FileEntry::textRead(uint8_t *buffer, unsigned size, unsigned offset)
{
    unsigned page = 0;
    unsigned to = 0;
    unsigned block;
    unsigned l;
    unsigned count = 0;
    
    auto_array<uint8_t> tmp;
    unsigned tmpSize = 0;
    
    if (!_pageSize) textInit();
    
    l = _pageSize->size();


    // find the first page.    
    for (page = 1; page < l; ++page)
    {
        unsigned pageSize = (*_pageSize)[page];
        if (to + pageSize > offset)
        {
            break;
        }
        
        to += pageSize;
    }
    
    --page;

    block = _firstBlock + 2 + (page * 2);
    
    
    // offset not needed anymore,
    // convert to offset from *this* page.
    offset -= to;
    
    while (size)
    {
        unsigned pageSize = (*_pageSize)[page];
        unsigned bytes = std::min(size, pageSize - offset);
        
        if (pageSize > tmpSize)
        {
            tmp.reset(new uint8_t[pageSize]);
            tmpSize = pageSize;            
        }
        
        
        // can decode straight to buffer if size >= bytes && offset = 0.
        textDecodePage(block, tmp.get());
        
        
        std::memcpy(buffer, tmp.get() + offset, bytes);
        
        
        block += 2;
        page += 1;
        
        size -= bytes;
        buffer += bytes;
        count += bytes;
        
        offset = 0;
    }

    return count;
}



unsigned FileEntry::textDecodePage(unsigned block, uint8_t *out)
{
    uint8_t buffer[1024];
    unsigned size = 0;
    unsigned bytes = textReadPage(block, buffer);
    
    for (unsigned i = 0; i < bytes; ++i)
    {
        uint8_t c = buffer[i];
        
        if (!c) break;
        if (c == 16 && i != bytes - 1)
        {
            // DLE
            // 16, n -> n-32 spaces.
            unsigned x = buffer[++i] - 32;
            
            if (out) for (unsigned i = 0; i < x; ++i) *out++ = ' ';
            size += x;
        }
        else
        {
            if (out) *out++ = c;
            size += 1;
        }
    }
    
    return size;
}

unsigned FileEntry::textReadPage(unsigned block, uint8_t *in)
{
    // reads up to 2 blocks.
    // assumes block within _startBlock ... _lastBlock - 1

    parent()->readBlock(block, in); 
    if (block + 1 == _lastBlock)
    {
        return _lastByte;
    }
    
    parent()->readBlock(block + 1, in + 512);
    if (block +2 == _lastBlock)
    {
        return 512 + _lastByte;
    }
    
    return 1024;
} 



void FileEntry::textInit()
{
    // calculate the file size and page offsets.
    _pageSize = new std::vector<unsigned>;
    _pageSize->reserve((_lastBlock - _firstBlock + 1 - 2) / 2);

    _fileSize = 0;
    for (unsigned block = _firstBlock + 2; block < _lastBlock; block += 2)
    {
        unsigned size = textDecodePage(block, NULL);
        printf("%u: %u\n", block, size);
        _fileSize += size;
        _pageSize->push_back(size);
    }
}

