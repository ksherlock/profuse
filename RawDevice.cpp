
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cerrno>


#ifdef __APPLE__
#include <sys/disk.h>
#endif

#ifdef __LINUX__
#include <sys/mount.h> 
#endif

#ifdef __SUN__
#include <sys/dkio.h>
#endif


#include "RawDevice.h"
#include "auto.h"
#include "Exception.h"

using namespace ProFUSE;

#ifdef __SUN__
void RawDevice::devSize(int fd)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::devSize"

    struct dk_minfo minfo;

    if (::ioctl(fd, DKIOCGMEDIAINFO, &minfo) < 0)
        throw POSIXException(__METHOD__ ": Unable to determine device size.", errno);
    
    _size = minfo.dki_lbsize * minfo.dki_capacity;
    _blockSize = 512; // not really, but whatever.
    _blocks = _size / 512;
    
}
#endif

#ifdef __APPLE__
void RawDevice::devSize(int fd)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::devSize"

    uint32_t blockSize; // 32 bit
    uint64_t blockCount; // 64 bit
    
    if (::ioctl(fd, DKIOCGETBLOCKSIZE, &blockSize) < 0)
        throw POSIXException(__METHOD__ ": Unable to determine block size.", errno);
    
    
    if (::ioctl(fd, DKIOCGETBLOCKCOUNT, &blockCount) < 0)
        throw POSIXException(__METHOD__ ": Unable to determine block count.", errno);
        
    _blockSize = blockSize;
    _size = _blockSize * blockCount;
    _blocks = _size / 512;
}

#endif


#ifdef __LINUX__

void RawDevice::devSize(int fd)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::devSize"

    int blocks;

    if (::ioctl(fd, BLKGETSIZE, &blocks) < 0)
        throw POSIXException(__METHOD__ ": Unable to determine device size.", errno);
    
    _size = 512 * blocks;
    _blockSize = 512; // 
    _blocks = blocks;
    
}


#endif

// TODO -- FreeBSD/NetBSD/OpenBSD

RawDevice::RawDevice(const char *name, bool readOnly)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::RawDevice"


    // open read-only, verify if device is readable, and then try to upgrade to read/write?

    auto_fd fd;
    
    if (!readOnly) fd.reset(::open(name, O_RDWR));
    if (fd < 0)
    {
        readOnly = false;
        fd.reset(::open(name, O_RDONLY));
    }
     
    if (fd < 0)
        throw POSIXException(__METHOD__ ": Unable to open device.", errno);

    _fd = -1;

    _readOnly = readOnly;
    _size = 0;
    _blocks = 0;
    _blockSize = 0;
    
    
    devSize(fd);
    
    _fd = fd.release();
}


RawDevice::~RawDevice()
{
    if (_fd >= 0) ::close(_fd);
}


void RawDevice::read(unsigned block, void *bp)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::read"

    if (block >= _blocks) throw Exception(__METHOD__ ": Invalid block number.");
    if (bp == 0) throw Exception(__METHOD__ ": Invalid address."); 

    // sun -- use pread
    // apple - read full native block(s) ?

    size_t ok = ::pread(_fd, bp, 512, block * 512);
    
    // TODO -- EINTR?
    if (ok != 512)
        throw ok < 0
            ? POSIXException(__METHOD__ ": Error reading block.", errno)
            : Exception(__METHOD__ ": Error reading block.");

    
    
}
void RawDevice::write(unsigned block, const void *bp)
{
#undef __METHOD__
#define __METHOD__ "RawDevice::write"

    if (block > _blocks) throw Exception(__METHOD__ ": Invalid block number.");

    if (_readOnly)
        throw Exception(__METHOD__ ": File is readonly.");

    
    size_t ok = ::pwrite(_fd, bp, 512, block * 512);
    
    if (ok != 512)
        throw ok < 0 
            ? POSIXException(__METHOD__ ": Error writing block.", errno)
            : Exception(__METHOD__ ": Error writing block.");
}


bool RawDevice::readOnly()
{
    return _readOnly;
}

void RawDevice::sync()
{
#undef __METHOD__
#define __METHOD__ "RawDevice::sync"

    if (_readOnly) return;
    
    if (::fsync(_fd) < 0)
        throw POSIXException(__METHOD__ ": fsync error.", errno);
}