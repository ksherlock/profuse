#include "DavexDiskImage.h"
#include "MappedFile.h"
#include "Buffer.h"
#include "Endian.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

using namespace ProFUSE;
using namespace LittleEndian;

/*
 http://www.umich.edu/~archive/apple2/technotes/ftn/FTN.E0.8004
 */

static const char *IdentityCheck = "\x60VSTORE [Davex]\x00";

DavexDiskImage::DavexDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{
    Validate(file());
}

// private, validation already performed.
DavexDiskImage::DavexDiskImage(MappedFile *f) :
    DiskImage(f)
{
}


DavexDiskImage::~DavexDiskImage()
{
    // scan and update usedBlocks?
}

void DavexDiskImage::Validate(MappedFile *f)
{
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Validate"

    size_t size = f->fileSize();
    const void * data = f->fileData();
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
    
    f->reset();
    f->setBlocks(blocks);
    f->setOffset(512);
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
#undef __METHOD__
#define __METHOD__ "DavexDiskImage::Create"

    uint8_t *data;
    
    MappedFile *file = new MappedFile(name, blocks * 512 + 512);
    
    data = (uint8_t *)file->fileData();

    Buffer header(512);
    
    header.pushBytes(IdentityCheck, 16);
    // file Format
    header.push8(0);
    //version
    header.push8(0);
    // version
    header.push8(0x10);
    
    // reserved.
    header.resize(32);
    
    //deviceNum
    header.push8(1);
    
    // total blocks
    header.push32le(blocks);

    // unused blocks
    header.push32le(0);
    
    // volume Name
    header.pushBytes("\x08Untitled", 9);
    
    // name + reserved.
    header.resize(64);
        
    // file number
    header.push8(1);
    
    //starting block
    header.push32le(0);
    
    // reserved
    header.resize(512);
    
    
    std::memcpy(file->fileData(), header.buffer(), 512);
    file->sync();
    
    file->setOffset(512);
    file->setBlocks(blocks);    
    return new DavexDiskImage(file);
}
