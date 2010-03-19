
#include <algorithm>
#include <cerrno>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <Cache/MappedBlockCache.h>
#include <ProFUSE/Exception.h>
#include <ProFUSE/auto.h>



using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


MappedBlockCache::MappedBlockCache(Device *device, void *data) :
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

void MappedBlockCache::unload(unsigned block, int flags)
{
#undef __METHOD__
#define __METHOD__ "MappedBlockCache::unload"

    if (flags & kBlockCommitNow)
    {
        _dirty = true;
        sync();
        return;
    }

    if (flags & kBlockDirty) _dirty = true;
}

void MappedBlockCache::sync()
{
  _device->sync();
  _dirty = false;
}
