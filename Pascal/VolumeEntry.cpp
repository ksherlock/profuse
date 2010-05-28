
#include <memory>
#include <algorithm>
#include <cerrno>

#include <Pascal/Pascal.h>

#include <ProFUSE/auto.h>
#include <ProFUSE/Exception.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <Device/BlockDevice.h>
#include <Cache/BlockCache.h>



using namespace LittleEndian;
using namespace Pascal;

using namespace Device;

using ProFUSE::Exception;
using ProFUSE::ProDOSException;
using ProFUSE::POSIXException;

enum {
    kMaxFiles = 77
};

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

VolumeEntry::VolumeEntry(const char *name, Device::BlockDevice *device)
{
#undef __METHOD__
#define __METHOD__ "VolumeEntry::VolumeEntry"

    unsigned length;
    unsigned deviceBlocks = device->blocks();
    
    
    deviceBlocks = std::min(0xffffu, deviceBlocks);
    if (deviceBlocks < 6)
        throw Exception(__METHOD__ ": device too small.");
    
    
    length = ValidName(name);
    
    if (!length)
        throw ProDOSException(__METHOD__ ": Invalid volume name.", ProFUSE::badPathSyntax);
    
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
    
    _lastVolumeBlock = deviceBlocks;
    _fileCount = 0;
    _accessTime = 0;
    _lastBoot = Date::Today(); 
    
    _cache = BlockCache::Create(device);
    _device = device;
    
    _address = 512 * 2;


    for (unsigned i = 2; i < 6; ++i)
    {
        _cache->zeroBlock(i);
    }
    
    void *vp = _cache->acquire(2);
    IOBuffer b(vp, 512);
    
    writeDirectoryEntry(&b);
        
    _cache->release(2, true);
    
    _cache->sync();
}


VolumeEntry::VolumeEntry(Device::BlockDevice *device)
{
    unsigned blockCount;
    unsigned deviceBlocks = device->blocks();
    ProFUSE::auto_array<uint8_t> buffer(new uint8_t[512]);
    
    
    // read the header block, then load up all the header 
    // blocks.
    
    _device = device;
    _cache = BlockCache::Create(device);
    
    _address = 512 * 2;
    
    _cache->read(2, buffer.get());

    init(buffer.get());

    // todo -- verify reasonable values.
    
    //printf("%u %u\n", blocks(), _lastBlock - _firstBlock);
    
    blockCount = blocks();
    
    if (blockCount > 1)
    {
        buffer.reset(readDirectoryHeader());
    }
    
    // now load up all the children.
    // if this throws, memory could be lost...

    
    try
    {    
        
        std::vector<FileEntry *>::iterator iter;

        unsigned block;
        
        for (unsigned i = 1; i <= _fileCount; ++i)
        {
            std::auto_ptr<FileEntry> child;
            
            //
            child.reset(new FileEntry(buffer.get() + i * 0x1a));
            
            child->setInode(++_inodeGenerator);
            child->_parent = this;
            child->_address = 512 * 2 + i * 0x1a;
                        
            _files.push_back(child.release());
        }
        
        // sanity check _firstBlock, _lastBlock?
        
        block = _lastBlock;
        
        for (iter = _files.begin(); iter != _files.end(); ++iter)
        {
            FileEntry *e = *iter;
            bool error = false;
            if (e->_firstBlock > e->_lastBlock)
                error = true;

            if (e->_firstBlock >= _lastVolumeBlock)
                error = true;

            if (e->_lastBlock > _lastVolumeBlock)
                error = true;
            
            if (e->_firstBlock < block)
                error = true;
            
            if (error)
                throw ProDOSException(__METHOD__ ": Invalid file entry.", ProFUSE::dirError);
            
            block = e->_lastBlock;
            
        }
        
        
        calcMaxFileSize();
        
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
    
    delete _cache;
    // _device is deleted by _cache.
    //delete _device;
}


void VolumeEntry::init(void *vp)
{
#undef __METHOD__
#define __METHOD__ "VolumeEntry::init"
    
    Entry::init(vp);
    _fileNameLength = Read8(vp, 6);
    
    // verify filenamelength <= 7
    if (_fileNameLength > 7)
        throw new  ProDOSException(__METHOD__ ": invalid name length", ProFUSE::badPathSyntax);
    
    // verify fileKind == 0
    // verify _fileCount reasonable
    // verify _lastVolumeBlock reasonable
    // verify _blocks reasonable.
    
    
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

FileEntry *VolumeEntry::fileByName(const char *name) const
{
    std::vector<FileEntry *>::const_iterator iter;
    for(iter = _files.begin(); iter != _files.end(); ++iter)
    {
        FileEntry *e = *iter;
        if (::strcasecmp(name, e->name()) == 0) return e;
    }
    return NULL;
}

unsigned VolumeEntry::unlink(const char *name)
{
    unsigned index;

    std::vector<FileEntry *>::iterator iter;
    
    // TODO -- update _maxFileSize.
    
    if (_device->readOnly()) return ProFUSE::drvrWrtProt; // WRITE-PROTECTED DISK
    
    for (index = 0, iter = _files.begin(); iter != _files.end(); ++iter, ++index)
    {
        FileEntry *e = *iter;
        if (::strcasecmp(name, e->name()) == 0)
        {
            
            // if not the first entry, update the previous entry's 
            // _maxFileSize.
            if (iter != _files.begin())
            {
                FileEntry *prev = iter[-1];
                prev->_maxFileSize += e->_maxFileSize;
            }
            
            delete e;
            *iter = NULL;
            break;
        }
    }
    if (iter == _files.end()) return ProFUSE::fileNotFound; // FILE NOT FOUND

    
    _files.erase(iter);
    _fileCount--;
    
    // reset addresses.
    for ( ; iter != _files.end(); ++iter)
    {
        FileEntry *e = *iter;
        e->_address -= 0x1a;
    }

    // need to update the header blocks.
    ProFUSE::auto_array<uint8_t> buffer(readDirectoryHeader());


    // update the filecount.
    IOBuffer b(buffer.get(), 512 * blocks());
    writeDirectoryEntry(&b);
 
    // move up all the entries.
    uint8_t *address = buffer.get() + 0x1a + 0x1a * index;
    std::memmove(address, address + 0x1a, 0x1a * (_fileCount - index));
    // zero out the memory on the previous entry.
    std::memset(buffer.get() + 0x1a + _fileCount * 0x1a, 0, 0x1a);

    // now write to disk.
    writeDirectoryHeader(buffer.get());
    
    _cache->sync();
    return 0;
}


unsigned VolumeEntry::rename(const char *oldName, const char *newName)
{
    FileEntry *e;

    
    // 1. verify old name exists.
    e = fileByName(oldName);
    if (!e)
        return ProFUSE::fileNotFound;
    
    
    // 2. verify new name is valid
    if (!FileEntry::ValidName(newName)) 
        return ProFUSE::badPathSyntax;


    // 3. verify new name does not exist.
    
    if (fileByName(newName))
        return ProFUSE::dupPathName;
    
        
    // 4. set the name (throws on error, which won't happen since
    // we already verified the name is valid.
    e->setName(newName);
    
    
    // 5. write to disk.
    
    writeEntry(e);
    _cache->sync();
    
    return 0;
}


/*
 * create a file.  if blocks is defined, verifies the file could
 * expand to fit.
 *
 */
FileEntry *VolumeEntry::create(const char *name, unsigned blocks)
{
    // 0. check read only access.
    // 1. verify < 77 file names.
    // 2. verify space at end of disk.
    // 3. make sure it's a legal file name.
    // 4. verify it's not a duplicate file name.
    // 6. create the file entry.
    // 7. insert into _files, write to disk, update _maxFileSize
    
    unsigned lastBlock;
    unsigned maxBlocks;

    std::auto_ptr<FileEntry>entry;
    FileEntry *prev = NULL;
    FileEntry *curr = NULL;
    
    if (readOnly())
    {
        errno = EACCES;
        return NULL;
    }
    
    if (_fileCount == kMaxFiles)
    {
        errno = ENOSPC;
        return NULL;
    }
    
    if (_fileCount)
    {
        prev = _files.back();
        lastBlock = prev->_lastBlock;
    }
    else {
        lastBlock = _lastBlock;
    }

    maxBlocks = _lastVolumeBlock - lastBlock;
    
    if (maxBlocks < blocks || maxBlocks == 0)
    {
        errno = ENOSPC;
        return NULL;
    }
    
    if (!FileEntry::ValidName(name))
    {
        errno = ENAMETOOLONG; // or invalid.
        return NULL;
    }
    
    if (fileByName(name))
    {
        errno = EEXIST;
        return NULL;
    }
    
    
    entry.reset(new FileEntry(name, kUntypedFile));
    
    _files.push_back(entry.get());

    curr = entry.release();
    
    curr->_firstBlock = lastBlock;
    curr->_lastBlock = lastBlock + 1;
    curr->_lastByte = 0;
    curr->_maxFileSize = maxBlocks * 512;
    
    if (prev)
    {    
        prev->_maxFileSize = prev->blocks() * 512;
    }
    
    writeEntry(curr);
    
    return curr;
    
}




unsigned VolumeEntry::krunch()
{
    unsigned prevBlock;
    
    std::vector<FileEntry *>::const_iterator iter;
    
    bool gap = false;
    
    // sanity check to make sure no weird overlap issues.

    prevBlock = lastBlock();
    
    for (iter = _files.begin(); iter != _files.end(); ++iter)
    {
        FileEntry *e = *iter;
        
        unsigned first = e->firstBlock();
        unsigned last = e->lastBlock();
        
        if (first != prevBlock) gap = true;
        
        if (first < prevBlock) 
            return ProFUSE::damagedBitMap;
        
        if (last < first) 
            return ProFUSE::damagedBitMap;
        
        if (first < volumeBlocks()) 
            return ProFUSE::damagedBitMap;
        
        
        prevBlock = last;
        
    }

    
    if (!gap) return 0;
    
    
    // need to update the header blocks.
    ProFUSE::auto_array<uint8_t> buffer(readDirectoryHeader());
    IOBuffer b(buffer.get(), 512 * blocks());
    

    prevBlock = lastBlock();
    
    unsigned offset = 0;
    for (iter = _files.begin(); iter != _files.end(); ++iter, ++offset)
    {
        FileEntry *e = *iter;
        
        b.setOffset(0x1a + 0x1a * offset);
        
        unsigned first = e->firstBlock();
        unsigned last = e->lastBlock();
        
        unsigned blocks = last - first;
        unsigned offset = first - prevBlock;
        
        if (offset == 0)
        {
            prevBlock = last;
            continue;
        }
        
        e->_firstBlock = first - offset;
        e->_lastBlock = last - offset;
        
        e->writeDirectoryEntry(&b);
        
        for (unsigned i = 0; i < blocks; ++i)
        {
            uint8_t buffer[512];
            
            _cache->read(first + i, buffer);
            _cache->write(prevBlock +i, buffer);
            _cache->zeroBlock(first + i);
        }
    }
    
    // now save the directory entries.

    // load up the directory blocks.
    writeDirectoryHeader(buffer.get());
    
    
    _cache->sync();
    

    calcMaxFileSize();
    
    return 0;
}






void *VolumeEntry::loadBlock(unsigned block)
{
    return _cache->acquire(block);
}
void VolumeEntry::unloadBlock(unsigned block, bool dirty)
{
    return _cache->release(block, dirty);
}

void VolumeEntry::readBlock(unsigned block, void *buffer)
{
    _cache->read(block, buffer);
}
void VolumeEntry::writeBlock(unsigned block, void *buffer)
{
    _cache->write(block, buffer);
}


void VolumeEntry::sync()
{
    _cache->sync();
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



uint8_t *VolumeEntry::readDirectoryHeader()
{
    return readBlocks(2, blocks());
}

void VolumeEntry::writeDirectoryHeader(void *buffer)
{
    writeBlocks(buffer, 2, blocks());
}


uint8_t *VolumeEntry::readBlocks(unsigned startingBlock, unsigned count)
{
    ProFUSE::auto_array<uint8_t> buffer(new uint8_t[512 * count]);
        
    for (unsigned i = 0; i < count; ++i)
        _cache->read(startingBlock + i, buffer.get() + 512 * i);
    
    return buffer.release();
}


void VolumeEntry::writeBlocks(void *buffer, unsigned startingBlock, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
        _cache->write(startingBlock + i, (uint8_t *)buffer + 512 * i);
}


// does not sync.
void VolumeEntry::writeEntry(FileEntry *e)
{
    unsigned address = e->_address;
    unsigned startBlock = address / 512;
    unsigned endBlock = (address + 0x1a - 1) / 512;
    unsigned offset = address % 512;
    
    if (startBlock == endBlock)
    {
        void *buffer = _cache->acquire(startBlock);
        
        IOBuffer b((uint8_t *)buffer + offset, 0x1a);
        
        e->writeDirectoryEntry(&b);
        
        _cache->release(startBlock, true);
    }
    else
    {
        // crosses page boundaries.
        ProFUSE::auto_array<uint8_t> buffer(readBlocks(startBlock, 2));
        
        IOBuffer b(buffer.get() + offset, 0x1a);
        
        e->writeDirectoryEntry(&b);        
        
        writeBlocks(buffer.get(), startBlock, 2);
    }
}


// set _maxFileSize for all entries.
void VolumeEntry::calcMaxFileSize()
{
    std::vector<FileEntry *>::reverse_iterator riter;
    unsigned block = _lastVolumeBlock;
    for (riter = _files.rbegin(); riter != _files.rend(); ++riter)
    {
        FileEntry *e = *riter;
        e->_maxFileSize = (block - e->_firstBlock) * 512;
        block = e->_firstBlock;
    }
    
}
