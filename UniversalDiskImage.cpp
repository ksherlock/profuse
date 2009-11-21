#include "UniversalDiskImage.h"
#include "MappedFile.h"
#include "Buffer.h"

using namespace ProFUSE;

UniversalDiskImage::UniversalDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{
    Validate(file());
}

UniversalDiskImage::UniversalDiskImage(MappedFile *file) :
    DiskImage(file)
{
}

UniversalDiskImage *UniversalDiskImage::Create(const char *name, size_t blocks)
{
    // 64-byte header.
    MappedFile *file = new MappedFile(name, blocks * 512 + 64);

    Buffer header(64);
    

    // magic + creator
    header.pushBytes("2IMGPRFS", 8); 
    
    // header size.
    header.push16le(64);
    
    // version
    header.push16le(1);
    
    //image format -- ProDOS order
    header.push32le(1); 
    
    // flags
    header.push32le(0);
    
    // # blocks. s/b 0 unless prodos-order
    header.push32le(blocks);
    
    // offset to disk data
    header.push32le(64);
    
    // data length
    header.push32le(512 * blocks);
    
    // comment offset, creator, reserved -- 0.
    header.resize(64);
    
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
#define __METHOD__ "DavexDiskImage::Validate"

    uint8_t *data = (uint8_t *)file->fileData();
    size_t size = file->fileSize();
    bool ok = false;
    unsigned blocks = 0;
    unsigned offset = 0;
    
    do {
        
        if (size < 64) break;
        
        if (std::memcmp(data, "2IMG", 4)) break;
        
        // only prodos supported, for now...
        if (file->read32(0x0c, LittleEndian) != 1) break;
        
        offset = file->read32(0x20, LittleEndian);
        blocks = file->read32(0x14, LittleEndian);
        
        // file size == blocks * 512
        if (file->read32(0x1c, LittleEndian) != blocks * 512) break;
        
        if (offset + blocks * 512 > size) break;
        
        ok = true;
    } while (false);
    
    if (!ok)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    file->reset();
    file->setOffset(offset);
    file->setBlocks(blocks);
}