#include "File.h"
#include "../auto.h"
#include "../Endian.h"
#include "../BlockDevice.h"

#include <cstring>
#include <memory.h>

using namespace LittleEndian;
using namespace Pascal;

#pragma mark -
#pragma mark DirEntry

Entry::Entry()
{
    _firstBlock = 0;
    _lastBlock = 0;
    _fileKind = 0;
    _inode = 0;
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


#pragma mark -
#pragma mark VolumeEntry

VolumeEntry::VolumeEntry()
{
    _fileNameLength = 0;
    std::memset(_fileName, 0, 8);
    _lastVolumeBlock = 0;
    _fileCount = 0;
    _accessTime = 0;
    
    setInode(1);
    _inodeGenerator = 1; 
}

VolumeEntry::VolumeEntry(ProFUSE::BlockDevice *device)
{
    auto_array<uint8_t> buffer(new uint8_t[512]);
    unsigned blockCount;
    
    // read the header block, then load up all the header 
    // blocks.
    
    device->read(2, buffer.get());

    init(buffer.get());

    // todo -- verify reasonable values.
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
        for (unsigned i = 1; i < _fileCount; ++i)
        {
            std::auto_ptr<FileEntry> child;
            
            child.reset(new FileEntry(buffer.get() + i * 0x1a));
            
            child->setInode(++_inodeGenerator);
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
    _lastBoot = DateRec(Read16(vp, 0x14));
    
    setInode(1);
    _inodeGenerator = 1; 
}


FileEntry *VolumeEntry::fileAtIndex(unsigned i) const
{
    return i < _files.size() ? _files[i] : NULL;
}



#pragma mark -
#pragma mark FileEntry

FileEntry::FileEntry(void *vp) :
    Entry(vp)
{
    _status = Read8(vp, 0x05) & 0x01;
    _fileNameLength = Read8(vp, 0x06);
    std::memset(_fileName, 0, 16);
    std::memcpy(_fileName, 0x07 + (uint8_t *)vp, _fileNameLength);
    _lastByte = Read16(vp, 0x16);
    _modification = DateRec(Read16(vp, 0x18));
}

FileEntry::~FileEntry()
{
    delete _pageLength;
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


int FileEntry::read(uint8_t *buffer, unsigned size, unsigned offset)
{
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

#if 0

unsigned File::fileSize() {
    return (_lastBlock - _firstBlock - 1) * 512 + _lastByte;
}

int File::read(uint8_t *buffer, unsigned size, unsigned offset)
{
    uint8_t tmp[512];
    
    unsigned fileSize = fileSize();
    unsigned count = 0;
    unsigned block = 0;
    
    if (offset >= fileSize) return 0;
    
    if (offset + size > fileSize) size = fileSize - offset;
    
    block = _startBlock + (offset / 512);
    
    // returned value (count) is equal to size at this point.
    // (no partial reads).
    
    /*
     * 1.  Block align everything
     */
    
    if (offset % 512)
    {
        unsigned bytes = std::min(offset % 512, size);
        
        _device->read(block++, tmp);
        
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
        _device->read(block++, buffer);
        
        buffer += 512;
        count += 512;
        size -= 512;
     }
     
    /*
     * 3. Read any trailing blocks.
     */
    if (size)
    {
        _device->read(block, tmp);
        std::memcpy(buffer, tmp, size);
        
        count += size; 
    
    }

    
    return count;
}



int TextFile::read(uint8_t *buffer, unsigned size, unsigned offset)
{
    unsigned page = 0;
    unsigned to = 0;
    unsigned l = _pageSize.length();
    unsigned block;
    unsigned count = 0;
    
    auto_array<uint8_t> tmp;
    unsigned tmpSize = 0;
    
    if (offset >= _fileSize) return 0;
    if (offset + size > _fileSize) size = _fileSize - offset;
    

    // find the first page.    
    for (page = 1; page < l; ++page)
    {
        if (to + _pageSize[page] > offset)
        {
            break;
        }
        
        to += _pageSize[i];
    }
    
    --page;

    block = _startBlock + 2 + (page * 2);
    
    
    // offset not needed anymore,
    // convert to offset from *this* page.
    offset -= to;
    
    while (size)
    {
        unsigned pageSize = _pageSize[page];
        if (pageSize > tmp)
        {
            tmp.reset(new uint8_t[pageSize]);
            tmpSize = pageSize;            
        }
        
        bytes = std::min(size, pageSize - offset);
        
        decodePage(block, tmp.get());
        
        
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



unsigned TextFile::decodePage(unsigned block, uint8_t *out)
{
    uint8_t buffer[1024];
    unsigned size = 0;
    unsigned bytes = readPage(block, buffer);
    
    for (unsigned i = 0; i < bytes; ++i)
    {
        uint8_t c = buffer[i];
        
        if (!c) break;
        if (c == 16 && i != bytes - 1)
        {
            // DLE
            // 16, n -> n-32 spaces.
            unsigned x = buffer[++i] - 32;
            
            if (out) for(unsigned i = 0; i < x; ++i) *out++ = ' ';
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

unsigned TextFile::readPage(unsigned block, uint8_t *in)
{
    // reads up to 2 blocks.
    // assumes block within _startBlock ... _endBlock - 1
    unsigned size = 0;

    _device->read(block, in); 
    if (block + 1 == _endBlock)
    {
        return _lastByte;
    }
    
    _device->read(block + 1, in + 512);
    if (block +2 == _endBlock)
    {
        return 512 + _lastByte;
    }
    
    return 1024;
} 



void TextFile::init()
{
    // calculate the file size and page offsets.

    _pageSize.reserve((_endBlock - _startBlock - 2) / 2);

    _fileSize = 0;
    for (unsigned block = _startBlock + 2; block < _endBlock; block += 2)
    {
        unsigned size = decodePage(block, NULL);
        _fileSize += size;
        _pageSize.push_back(size);
    }
}

#endif
