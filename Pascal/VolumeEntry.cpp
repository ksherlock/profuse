
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



// djb hash, case insensitive.
static unsigned NameHash(const char *cp)
{
    unsigned hash = 5381;
    unsigned c;
    while ((c = *cp++))
    {
        c = std::toupper(c);
        hash = ((hash << 5) + hash) + c;
    }

    return hash & 0x7FFFFFFF;
}

static bool NameEqual(const char *a, const char *b)
{
    return ::strcasecmp(a, b) == 0;
}



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
        throw ProDOSException(__METHOD__ ": invalid name length", ProFUSE::badPathSyntax);
    
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

/*
 * Remove a file.
 * Returns 0 on succes, -1 (and errno) on failure.
 * May also throw a device error.
 */
int VolumeEntry::unlink(const char *name)
{
    unsigned index;

    std::vector<FileEntry *>::iterator iter;
        
    if (_device->readOnly())
    {
        errno = EROFS;
        return -1;
    }
    
    // TODO -- consider having an unordered_map of file names.
    
    for (iter = _files.begin(); iter != _files.end(); ++iter)
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
    if (iter == _files.end())
    {
        errno = ENOENT;
        return -1;
    }

    
    _files.erase(iter);
    _fileCount--;
    
    index = distance(_files.begin(), iter);
    
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


/*
 * Renames a file, removing any other file with newName (if present)
 * Returns 0 on success, -1 on error (and sets errno)
 * May also throw an error of some sort.
 */
int VolumeEntry::rename(const char *oldName, const char *newName)
{
    FileEntry *e;

    
    // check if read only.
    if (_device->readOnly())
    {
        errno = EROFS;
        return -1;
    }
    
    // verify file exists.
    e = fileByName(oldName);
    if (!e)
    {
        errno = ENOENT;
        return -1;
    }
    
    
    // verify new name is valid.
    if (!FileEntry::ValidName(newName)) 
    {
        errno = EINVAL; // invalid name, at least.
        return -1;
    }

    // delete the new file
    if (unlink(newName) != 0)
    {
        if (errno != ENOENT) return -1;
    }

    // update with the new name.
    e->setName(newName);
    
    // and commit to disk.
    
    writeEntry(e);
    _cache->sync();
    
    return 0;
}


/*
 * copy a file (here to simplify copying text files).
 * if newName exists, delete it.
 * 
 */
int VolumeEntry::copy(const char *oldName, const char *newName)
{
    FileEntry *oldEntry;
    FileEntry *newEntry;
    // check if read only.
    if (_device->readOnly())
    {
        errno = EROFS;
        return -1;
    }
    
    // verify file exists.
    oldEntry = fileByName(oldName);
    if (!oldEntry)
    {
        errno = ENOENT;
        return -1;
    }
    
    
    // verify new name is valid.
    if (!FileEntry::ValidName(newName)) 
    {
        errno = EINVAL; // invalid name, at least.
        return -1;
    }    
    
    // if newName does not exist, call create w/ block size and write everything.
    // if newName does exist, overwrite it if it will fit.  Otherwise, remove it, create a file, etc.
    
    unsigned blocks = oldEntry->blocks();
    newEntry = fileByName(newName);
    
    if (newEntry)
    {
        // if newEntry is large enough, overwrite it.
        if (newEntry->_maxFileSize >= blocks * 512)
        {
            if (newEntry->_pageSize)
            {
                delete newEntry->_pageSize;
                newEntry->_pageSize = NULL;
                newEntry->_fileSize = 0;
            }
        }
        else
        {
            if (maxContiguousBlocks() < blocks)
            {
                errno = ENOSPC;
                return -1;                
            }
            
            newEntry = NULL;
            
            if (unlink(newName) != 0) return -1;
        }        
    }
        
    
    if (newEntry == NULL)
    {
        // newName does not exist (or was just deleted), so create a new file (if possible) and write to it.
        if (maxContiguousBlocks() < blocks)
        {
            errno = ENOSPC;
            return -1;
        }
        
        newEntry = create(newName, blocks);
        
        if (!newEntry) return -1; // create() sets errno.
        
    }

    newEntry->_fileKind = oldEntry->fileKind();
    newEntry->_lastByte = oldEntry->lastByte();
    newEntry->_lastBlock = newEntry->firstBlock() + blocks;
    newEntry->_modification = Date::Today();
   
    for (unsigned i = 0; i < blocks; i++)
    {
        void *src = loadBlock(oldEntry->firstBlock() + i);
        _cache->write(newEntry->firstBlock() + i, src);
        unloadBlock(oldEntry->firstBlock() + i, false);
    }
    
    _cache->sync();
    
    
    return 0;
}

/*
 * create a file.  if blocks is defined, verifies the file could
 * expand to fit.
 * returns FileEntry on success, NULL (and errno) on failure.
 *
 */
/*
 * TODO -- if blocks is defined, try to fit in a gap rather than putting at the end.
 *
 *
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
    
    unsigned lastBlock = _lastBlock;

    
    std::auto_ptr<FileEntry>entry;
    FileEntry *prev = NULL;
    FileEntry *curr = NULL;
    std::vector<FileEntry *>::iterator iter;
    
    
    if (readOnly())
    {
        errno = EROFS;
        return NULL;
    }
    
    if (_fileCount == kMaxFiles)
    {
        errno = ENOSPC;
        return NULL;
    }
    
    
    if (!FileEntry::ValidName(name))
    {
        errno = EINVAL;
        return NULL;
    }
    
    if (fileByName(name))
    {
        errno = EEXIST;
        return NULL;
    }
    
    
    entry.reset(new FileEntry(name, kUntypedFile));
    
    
    std::auto_ptr<uint8_t> buffer(readDirectoryHeader());
        
    for (iter = _files.begin(); iter != _files.end(); ++iter)
    {
        FileEntry *e = *iter;
        
        unsigned freeSpace = e->_firstBlock - _lastBlock;
        // this could do something stupid like selecting a slot with only 1 free block but too bad.
        
        if (freeSpace < blocks)
        {
            lastBlock = e->_lastBlock;
            prev = e;
            continue;
        }
        
        // update previous entry max file size.
        if (prev)
        {
            prev->_maxFileSize = prev->blocks() * 512;
        }
        
        // insert() inserts an item *before* the current item and returns an iterator to the 
        // element inserted.
        // afterwards, iter will point to curr+1
        
        // keep track of the index *before* the insert.
        unsigned index = distance(_files.begin(), iter); // current index.
        
        iter = _files.insert(iter, entry.get());
        ++_fileCount;
        
        curr = entry.release();
        
        curr->_parent = this;
        curr->_firstBlock = lastBlock;
        curr->_lastBlock = lastBlock + 1;
        curr->_lastByte = 0;
        curr->_maxFileSize = freeSpace * 512;
        //curr->_address = 2 * 512 + 0x1a + 0x1a * index;
        
        // move all entries after this one up by 0x1a bytes, then write this header.
        unsigned count = _fileCount - index - 1;
        unsigned offset = index * 0x1a + 0x1a;
        std::memmove(buffer.get() + offset + 0x1a, buffer.get() + offset, count * 0x1a);
        break;
    }
    
    if (iter == _files.end())
    {

        // check if we can append
        unsigned freeSpace = _lastVolumeBlock - lastBlock;
        if (freeSpace < blocks)
        {
            errno = ENOSPC;
            return NULL;        
        }
                
        _files.push_back(entry.get());
        _fileCount++;
        
        curr = entry.release();
        
        curr->_parent = this;
        curr->_firstBlock = lastBlock;
        curr->_lastBlock = lastBlock + 1;
        curr->_lastByte = 0;
        curr->_maxFileSize = freeSpace * 512;
        curr->_address = 2 * 512 + 0x1a * _fileCount; // s/b +1 since entry 0 is the header.
        
        if (prev)
        {    
            prev->_maxFileSize = prev->blocks() * 512;
        }
        
        writeEntry(curr);
        writeEntry(); // header.        
        
    }
    else
    {


        // update all addresses to make life easier.
        unsigned address = 2 * 512 + 0x1a;
        for (iter = _files.begin(); iter != _files.end(); ++iter, address += 0x1a)
        {
            FileEntry *e = *iter;
            e->_address = address;
        }
             
        IOBuffer b(buffer.get(), 4 * 512);
        writeDirectoryEntry(&b);
        
        b.setOffset(curr->_address - 2 * 512);
        curr->writeDirectoryEntry(&b);
        
        writeDirectoryHeader(buffer.get());

    }
        
    _cache->sync();
    return curr;
}



/*
 * TODO -- consider trying to move files from the end to fill gaps
 * if it would reduce the number of blocks that need to be re-arranged.
 *
 * build a list of gaps, reverse iterate and move last file to first gap
 * only 77 files, so can consider all options.
 *
 *
 *
 *
 *
 */
int VolumeEntry::krunch()
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




/*
 * return the number of free blocks.
 * if krunched is true, returns sum of all free blocks
 * if krunched is false, returns free blocks at end.
 *
 */
unsigned VolumeEntry::freeBlocks(bool krunched) const
{
    unsigned freeBlocks = 0;
    unsigned lastBlock = 0;
    
    if (krunched)
    {
        std::vector<FileEntry *>::const_iterator iter;
        
        lastBlock = _lastBlock;
        
        for (iter = _files.begin(); iter != _files.end(); ++iter)
        {
            const FileEntry *e = *iter;
            freeBlocks += e->_firstBlock - lastBlock;
            lastBlock = e->_lastBlock;
        }
    }
    else
    {
        lastBlock = _fileCount ? _files.back()->_lastBlock : _lastBlock;
    }

    
    freeBlocks += _lastVolumeBlock - lastBlock;
    return freeBlocks;
}


/*
 * return true if there are any gaps, false otherwise.
 *
 *
 */
bool VolumeEntry::canKrunch() const
{
    
    std::vector<FileEntry *>::const_iterator iter;
    
    unsigned lastBlock = _lastBlock;

    for (iter = _files.begin(); iter != _files.end(); ++iter)
    {
        const FileEntry *e = *iter;
        if (e->_lastBlock != lastBlock) return true;
        lastBlock = e->_lastBlock;
    }
    
    return false;
}

/*
 * returns the largest free block.
 *
 */
unsigned VolumeEntry::maxContiguousBlocks() const
{
    unsigned max = 0;
    
    std::vector<FileEntry *>::const_iterator iter;
    
    unsigned lastBlock = _lastBlock;
    
    for (iter = _files.begin(); iter != _files.end(); ++iter)
    {
        const FileEntry *e = *iter;
        unsigned free = e->_firstBlock - lastBlock;
        max = std::max(max, free);
        
        lastBlock = e->_lastBlock;
    }
    
    max = std::max(max, _lastVolumeBlock - lastBlock);
    
    return max;    
    
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


// write directory entry, does not sync.
void VolumeEntry::writeEntry()
{
    IOBuffer iob(loadBlock(2),512);
    writeDirectoryEntry(&iob);
    unloadBlock(2, true);
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
