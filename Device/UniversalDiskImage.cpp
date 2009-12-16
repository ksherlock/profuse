#include <Device/UniversalDiskImage.h>
#include <Device/MappedFile.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <ProFUSE/Exception.h>

using namespace Device;
using namespace LittleEndian;

using ProFUSE::Exception;
using ProFUSE::POSIXException;

UniversalDiskImage::UniversalDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{
    Validate(file());
    const void *data = file()->fileData();
    
    // flags.  bit 31 = locked.
    _flags = Read32(data, 0x10);
}

UniversalDiskImage::UniversalDiskImage(MappedFile *file) :
    DiskImage(file)
{
    _flags = 0;
}

UniversalDiskImage *UniversalDiskImage::Create(const char *name, size_t blocks)
{
    // 64-byte header.
    MappedFile *file = new MappedFile(name, blocks * 512 + 64);

    uint8_t tmp[64];
    
    IOBuffer header(tmp, 64);
    

    // magic + creator
    header.writeBytes("2IMGPRFS", 8); 
    
    // header size.
    header.write16(64);
    
    // version
    header.write16(1);
    
    //image format -- ProDOS order
    header.write32(1); 
    
    // flags
    header.write32(0);
    
    // # blocks. s/b 0 unless prodos-order
    header.write32(blocks);
    
    // offset to disk data
    header.write32(64);
    
    // data length
    header.write32(512 * blocks);
    
    // comment offset, creator, reserved -- 0.
    header.setOffset(64, true);
    
    std::memcpy(file->fileData(), header.buffer(), 64);
    

    file->setOffset(64);
    file->setBlocks(blocks);   
    return new UniversalDiskImage(file);
}

UniversalDiskImage *UniversalDiskImage::Open(MappedFile *file)
{
    Validate(file);
    return new UniversalDiskImage(file);
}


/*
 * TODO -- support dos-order & nibblized
 * TODO -- honor read-only flag.
 *
 */
void UniversalDiskImage::Validate(MappedFile *file)
{
#undef __METHOD__
#define __METHOD__ "UniversalDiskImage::Validate"

    const void *data = file->fileData();
    size_t size = file->fileSize();
    bool ok = false;
    unsigned blocks = 0;
    unsigned offset = 0;
    
    do {
        
        if (size < 64) break;
        
        if (std::memcmp(data, "2IMG", 4)) break;
        
        // only prodos supported, for now...
        if (Read32(data, 0x0c) != 1) break;
        
        offset = Read32(data, 0x20);
        blocks = Read32(data, 0x14);
        
        // file size == blocks * 512
        if (Read32(data, 0x1c) != blocks * 512) break;
        
        if (offset + blocks * 512 > size) break;
        
        ok = true;
    } while (false);
    
    if (!ok)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    file->reset();
    file->setOffset(offset);
    file->setBlocks(blocks);
}


bool UniversalDiskImage::readOnly()
{
    return (_flags & 0x8000000) || DiskImage::readOnly();
}