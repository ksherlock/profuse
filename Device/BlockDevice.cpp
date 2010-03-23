
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#include <Device/BlockDevice.h>
#include <Device/BlockCache.h>
#include <Device/MappedFile.h>

#include <ProFUSE/Exception.h>


using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;

#pragma mark -
#pragma mark BlockDevice

BlockDevice::BlockDevice()
{
}
BlockDevice::~BlockDevice()
{
}

void BlockDevice::zeroBlock(unsigned block)
{
    uint8_t bp[512];
    std::memset(bp, 0, 512);
    
    write(block, bp);
}

AbstractBlockCache *BlockDevice::blockCache()
{
    if (!_cache)
    {
        _cache = createBlockCache();
    }
    return _cache;
}

AbstractBlockCache *BlockDevice::createBlockCache()
{
    return new BlockCache(this);
}

bool BlockDevice::mapped()
{
    return false;
}

void BlockDevice::sync(unsigned block)
{
    sync();
}

void BlockDevice::sync(TrackSector ts)
{
    sync();
}

void *BlockDevice::read(unsigned block)
{
    return NULL;
}

void *BlockDevice::read(TrackSector ts)
{
    return NULL;
}

