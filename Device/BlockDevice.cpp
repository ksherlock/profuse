
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#include <Device/BlockDevice.h>
#include <Cache/ConcreteBlockCache.h>

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


BlockCache *BlockDevice::createBlockCache()
{
    unsigned b = blocks();
    unsigned size = std::max(16u, b / 16);
    return new ConcreteBlockCache(this, size);
}
