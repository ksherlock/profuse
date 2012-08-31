
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <algorithm>

#include <Device/DavexDiskImage.h>
#include <File/MappedFile.h>
#include <Device/Adaptor.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <Cache/MappedBlockCache.h>

using namespace Device;
using namespace LittleEndian;


/*
 http://www.umich.edu/~archive/apple2/technotes/ftn/FTN.E0.8004
 */

static const char *IdentityCheck = "\x60VSTORE [Davex]\x00";


// private, validation already performed.
DavexDiskImage::DavexDiskImage(MappedFile *file) :
    DiskImage(file)
{
    // at this point, file is no longer valid.

    
    // 512-bytes header
    setBlocks((length() / 512) - 1);
    setAdaptor(new POAdaptor(512 + (uint8_t *)address()));
}


DavexDiskImage::~DavexDiskImage()
{
    // scan and update usedBlocks?
}

bool DavexDiskImage::Validate(MappedFile *f, const std::nothrow_t &)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Validate"
    
    size_t size = f->length();
    const void * data = f->address();
    unsigned blocks = (size / 512) - 1;
    

    if (size < 512) return false;
    if (size % 512) return false;
        
    // identity.
    if (std::memcmp(data, IdentityCheck, 16))
        return false;
    
    // file format.
    if (Read8(data, 0x10) != 0)
        return false;
    
    // total blocks
    if (Read32(data, 33) != blocks)
        return false;
    
    // file number -- must be 1
    if (Read8(data, 64) != 1)
        return false;
    
    return true;
}

bool DavexDiskImage::Validate(MappedFile *f)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Validate"


    if (!Validate(f, std::nothrow))
        throw ::Exception(__METHOD__ ": Invalid file format.");
    
    return true;
}

BlockDevicePointer DavexDiskImage::Open(MappedFile *file)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Open"
    Validate(file);
    
    //return BlockDevicePointer(new DavexDiskImage(file));
    
    return MAKE_SHARED(DavexDiskImage, file);
}

BlockDevicePointer DavexDiskImage::Create(const char *name, size_t blocks)
{
    return Create(name, blocks, "Untitled");
}
BlockDevicePointer DavexDiskImage::Create(const char *name, size_t blocks, const char *vname)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Create"

    uint8_t *data;
    uint8_t tmp[512];
    IOBuffer header(tmp,512);

    
    MappedFile *file = MappedFile::Create(name, blocks * 512 + 512);
    
    data = (uint8_t *)file->address();
        
    header.writeBytes(IdentityCheck, 16);
    // file Format
    header.write8(0);
    //version
    header.write8(0);
    // version
    header.write8(0x10);
    
    // reserved.
    header.setOffset(32, true);
    
    //deviceNum
    header.write8(1);
    
    // total blocks
    header.write32(blocks);

    // unused blocks
    header.write32(0);
    
    // volume Name
    if (!vname || !*vname) vname = "Untitled";
    unsigned l = std::strlen(vname);
    header.write8(std::min(l, 15u));
    header.writeBytes(vname, std::min(l, 15u));
    
    // name + reserved.
    header.setOffset(64, true);
        
    // file number
    header.write8(1);
    
    //starting block
    header.write32(0);
    
    // reserved
    header.setOffset(512, true);
    
    
    std::memcpy(file->address(), header.buffer(), 512);
    file->sync();
    
    //return BlockDevicePointer(new DavexDiskImage(file));
    
    return MAKE_SHARED(DavexDiskImage, file);
}


BlockCachePointer DavexDiskImage::createBlockCache()
{
    // need a smart pointer, but only have this....
    return MappedBlockCache::Create(shared_from_this(), 512 + (uint8_t *)address());
    
}
