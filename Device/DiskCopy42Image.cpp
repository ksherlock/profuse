

#include <cstdio>
#include <cstring>
#include <algorithm>

#include <Device/DiskCopy42Image.h>
#include <Device/MappedFile.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

using namespace Device;
using namespace BigEndian;


using ProFUSE::Exception;
using ProFUSE::POSIXException;

DiskCopy42Image::DiskCopy42Image(MappedFile *f) :
    DiskImage(f),
    _changed(false)
{
}

DiskCopy42Image::DiskCopy42Image(const char *name, bool readOnly) :
    DiskImage(name, readOnly),
    _changed(false)
{
    Validate(file());
}

DiskCopy42Image::~DiskCopy42Image()
{
    if (_changed)
    {
        MappedFile *f = file();
        void *data = f->fileData();
        
        if (f)
        {
            uint32_t cs = Checksum(f->offset() + (uint8_t *)data, 
                f->blocks() * 512);
                
            Write32(data, 72, cs);
            f->sync();
        }
        // TODO -- checksum
    }
}

uint32_t DiskCopy42Image::Checksum(void *data, size_t size)
{
    uint32_t rv = 0;
    uint8_t *dp = (uint8_t *)data;   
 
    if (size & 0x01) return rv;
    
    for (size_t i = 0; i < size; i += 2)
    {
        rv += dp[i] << 8;
        rv += dp[i+1];
        
        rv = (rv >> 1) + (rv << 31);
    }
    
    return rv;
}

DiskCopy42Image *DiskCopy42Image::Open(MappedFile *f)
{
    Validate(f);
    return new DiskCopy42Image(f);
}

static uint8_t DiskFormat(size_t blocks)
{
    switch (blocks)
    {
    case 800: return 0;
    case 1600: return 1;
    case 1440: return 2;
    case 2880: return 3;
    default: return 0xff;
    }
}

static uint8_t FormatByte(size_t blocks)
{
    switch(blocks)
    {
    case 800: return 0x12;
    default: return 0x22;
    }
}
DiskCopy42Image *DiskCopy42Image::Create(const char *name, size_t blocks)
{
    return Create(name, blocks, "Untitled");
}

DiskCopy42Image *DiskCopy42Image::Create(const char *name, size_t blocks, const char *vname)
{
    MappedFile *file = new MappedFile(name, blocks * 512 + 84);
    file->setOffset(84);
    file->setBlocks(blocks);
    
    uint8_t tmp[84];
    IOBuffer header(tmp, 84);
    
    // name -- 64byte pstring.
    
    if (vname == NULL) vname = "Untitled"; 
    unsigned l = std::strlen(vname);
    header.write8(std::min(l, 63u));
    header.writeBytes(vname, std::min(l, 63u));

    //header.resize(64);
    header.setOffset(64, true);
    
    // data size -- number of bytes
    header.write32(blocks * 512);
    
    // tag size
    header.write32(0);
    
    // data checksum
    // if data is 0, will be 0.
    //header.push32be(Checksum(file->fileData(), blocks * 512));
    header.write32(0);
    
    // tag checksum
    header.write32(0);
    
    // disk format.
    /*
     * 0 = 400k
     * 1 = 800k
     * 2 = 720k
     * 3 = 1440k
     * 0xff = other
     */
    header.write8(DiskFormat(blocks));
    
    // formatbyte
    /*
     * 0x12 = 400k
     * 0x22 = >400k mac
     * 0x24 = 800k appleII
     */
    header.write8(FormatByte(blocks));
    
    // private
    header.write16(0x100);
    
    std::memcpy(file->fileData(), header.buffer(), 84);
    file->sync();
    
    return new DiskCopy42Image(file);
}

void DiskCopy42Image::Validate(MappedFile *file)
{
#undef __METHOD__
#define __METHOD__ "DiskCopy42Image::Validate"

    size_t bytes = 0;
    size_t size = file->fileSize();
    const void *data = file->fileData();
    bool ok = false;
    uint32_t checksum = 0;
    
    do {
        if (size < 84) break;
        
        // name must be < 64
        if (Read8(data, 0) > 63) break;
        
        if (Read32(data, 82) != 0x100)
            break;
        
        // bytes, not blocks.
        bytes = Read32(data, 64);
        
        if (bytes % 512) break;
        
        if (size < 84 + bytes) break;
        
        // todo -- checksum.
        checksum = Read32(data, 72);
        
        ok = true;
    } while (false);
    
    if (!ok)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    uint32_t cs = Checksum(64 + (uint8_t *)data, bytes);
    
    if (cs != checksum)
    {
        fprintf(stderr, "Warning: checksum invalid.\n");
    }
    file->reset();
    file->setOffset(64);
    file->setBlocks(bytes / 512);
}

void DiskCopy42Image::write(unsigned block, const void *bp)
{
    DiskImage::write(block, bp);
    _changed = true;
}