
#include <algorithm>
#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <Cache/MappedBlockCache.h>
#include <Device/BlockDevice.h>

#include <ProFUSE/Exception.h>



using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


MappedBlockCache::MappedBlockCache(BlockDevice *device, void *data) :
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
    _dirty = true;
    std::memcpy(_data + block * 512, vp, 512);
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
        throw POSIXException(__METHOD__ ": msync", errno);
    
    if (start != end)
    {
        if (::msync(end, pagesize, MS_SYNC) != 0)
            throw POSIXException(__METHOD__ ": msync", errno);    
    }
}

void MappedBlockCache::markDirty(unsigned block)
{
    _dirty = true;
}

