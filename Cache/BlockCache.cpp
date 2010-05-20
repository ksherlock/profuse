
#include <algorithm>
#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>


#include <Cache/BlockCache.h>
#include <Device/BlockDevice.h>

#include <ProFUSE/Exception.h>
#include <ProFUSE/auto.h>




using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;

#pragma mark -
#pragma mark BlockCache

BlockCache::BlockCache(BlockDevice *device)
{
    _device = device;
    _blocks = device->blocks();
    _readOnly = device->readOnly();
}

BlockCache::~BlockCache()
{
    delete _device;
}

void BlockCache::write(unsigned block, const void *bp)
{
    void *address = acquire(block);
    std::memcpy(address, bp, 512);
    release(block, true);
}

void BlockCache::read(unsigned block, void *bp)
{
    void *address = acquire(block);
    std::memcpy(bp, address, 512);
    release(block, false);
}


BlockCache *BlockCache::Create(BlockDevice *device)
{
    if (!device) return NULL;
    
    return device->createBlockCache();
}


void BlockCache::zeroBlock(unsigned block)
{
    /*
    void *address = acquire(block);
    std::memset(address, 0, 512);
    release(block, true);
    */

    uint8_t buffer[512];
    
    std::memset(buffer, 0, 512);
    write(block, buffer);    
}
