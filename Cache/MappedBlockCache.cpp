
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstddef>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <Cache/MappedBlockCache.h>
#include <Device/BlockDevice.h>

#include <Common/Exception.h>
#include <POSIX/Exception.h>


using namespace Device;


BlockCachePointer MappedBlockCache::Create(BlockDevicePointer device, void *data)
{
    //return BlockCachePointer(new MappedBlockCache(device, data));
    return MAKE_SHARED(MappedBlockCache, device, data);
}


MappedBlockCache::MappedBlockCache(BlockDevicePointer device, void *data) :
    BlockCache(device)
{
    _data = (uint8_t *)data;
    _dirty = false;
}

MappedBlockCache::~MappedBlockCache()
{
  if (_dirty) sync();
}


void *MappedBlockCache::acquire(unsigned block)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::load"
    
    if (block >= blocks())
        throw Exception(__METHOD__ ": Invalid block.");
        
    
    return _data + block * 512;
}

void MappedBlockCache::release(unsigned block, int flags)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::unload"

    // kBlockCommitNow implies kBlockDirty. 
    if (flags & kBlockCommitNow)
    {
        sync(block);
        return;
    }

    if (flags & kBlockDirty) _dirty = true;
}



void MappedBlockCache::write(unsigned block, const void *vp)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::write"
    
    if (block >= blocks())
        throw Exception(__METHOD__ ": Invalid block.");
    
    _dirty = true;
    std::memcpy(_data + block * 512, vp, 512);
}


void MappedBlockCache::zeroBlock(unsigned block)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::zeroBlock"
    
    if (block >= blocks())
        throw Exception(__METHOD__ ": Invalid block.");
    
    
    _dirty = true;
    std::memset(_data + block * 512, 0, 512);
}



// sync everything.
void MappedBlockCache::sync()
{
  _device->sync();
  _dirty = false;
}

/*
 *
 * sync an individual page.
 *
 */
void MappedBlockCache::sync(unsigned block)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::sync"


    int pagesize = ::getpagesize();
        
    void *start = _data + block * 512;
    void *end = _data + 512 + block * 512;


    start = (void *)((ptrdiff_t)start / pagesize * pagesize);
    end = (void *)((ptrdiff_t)end / pagesize * pagesize);

    if (::msync(start, pagesize, MS_SYNC) != 0)
        throw POSIX::Exception(__METHOD__ ": msync", errno);
    
    if (start != end)
    {
        if (::msync(end, pagesize, MS_SYNC) != 0)
            throw POSIX::Exception(__METHOD__ ": msync", errno);    
    }
}

void MappedBlockCache::markDirty(unsigned block)
{
    _dirty = true;
}

