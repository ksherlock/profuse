
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


using namespace Device;
using namespace LittleEndian;

using ProFUSE::Exception;
using ProFUSE::POSIXException;

/*
 http://www.umich.edu/~archive/apple2/technotes/ftn/FTN.E0.8004
 */

static const char *IdentityCheck = "\x60VSTORE [Davex]\x00";

/*
DavexDiskImage::DavexDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{
    Validate(file());
}
*/

// private, validation already performed.
DavexDiskImage::DavexDiskImage(MappedFile *file) :
    DiskImage(file)
{
    // 512-bytes header
    setBlocks((file->length() / 512) - 1);
    setAdaptor(new POAdaptor(512 + (uint8_t *)file->address()));
}


DavexDiskImage::~DavexDiskImage()
{
    // scan and update usedBlocks?
}

void DavexDiskImage::Validate(MappedFile *f)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Validate"

    size_t size = f->length();
    const void * data = f->address();
    bool ok = false;
    unsigned blocks = (size / 512) - 1;
    
    do  {
        if (size < 512) break;
        if (size % 512) break;
    
        // identity.
        if (std::memcmp(data, IdentityCheck, 16))
            break;
        
        // file format.
        if (Read8(data, 0x10) != 0)
            break;
        
        // total blocks
        if (Read32(data, 33) != blocks)
            break;
        
        // file number -- must be 1
        if (Read8(data, 64) != 1)
            break;
                    
        ok = true;
    } while (false);
    
    
    if (!ok)
        throw Exception(__METHOD__ ": Invalid file format.");
}

DavexDiskImage *DavexDiskImage::Open(MappedFile *file)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Open"
    Validate(file);
    
    return new DavexDiskImage(file);
}

DavexDiskImage *DavexDiskImage::Create(const char *name, size_t blocks)
{
    return Create(name, blocks, "Untitled");
}
DavexDiskImage *DavexDiskImage::Create(const char *name, size_t blocks, const char *vname)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Create"

    uint8_t *data;
    uint8_t tmp[512];
    IOBuffer header(tmp,512);

    
    MappedFile *file = new MappedFile(name, blocks * 512 + 512);
    
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
    
    return new DavexDiskImage(file);
}
