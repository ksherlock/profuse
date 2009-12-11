#include "File.h"
#include "../auto.h"
#include "../Endian.h"
#include "../BlockDevice.h"
#include "../BlockCache.h"

#include <algorithm>
#include <cstring>
#include <memory>

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
    _cache = NULL;
    _device = NULL;
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
    _lastBoot = DateRec(Read16(vp, 0x14));
    
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

    _pageSize->reserve((_lastBlock - _firstBlock + 1 - 2) / 2);

    _fileSize = 0;
    for (unsigned block = _firstBlock + 2; block < _lastBlock; block += 2)
    {
        unsigned size = textDecodePage(block, NULL);
        _fileSize += size;
        _pageSize->push_back(size);
    }
}

