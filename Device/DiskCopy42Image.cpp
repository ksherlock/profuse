

#include <cstdio>
#include <cstring>
#include <algorithm>

#include <Device/DiskCopy42Image.h>

#include <Endian/Endian.h>
#include <Endian/IOBuffer.h>

#include <Cache/MappedBlockCache.h>


using namespace Device;
using namespace BigEndian;


using ProFUSE::Exception;
using ProFUSE::POSIXException;



enum {
    oDataSize = 64,
    oDataChecksum = 72,
    oPrivate = 82,
    oUserData = 84
};

DiskCopy42Image::DiskCopy42Image(MappedFile *f) :
    DiskImage(f),
    _changed(false)
{
    setAdaptor(new POAdaptor(oUserData + (uint8_t *)f->address()));
    setBlocks(Read32(f->address(), oDataSize) / 512);
}



DiskCopy42Image::~DiskCopy42Image()
{
    if (_changed)
    {
        MappedFile *f = file();
        
        if (f)
        {
            void *data = f->address();
            
            uint32_t cs = Checksum(oUserData + (uint8_t *)data, Read32(data, oDataSize));
                
            Write32(data, oDataChecksum, cs);
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
    MappedFile *file = MappedFile::Create(name, blocks * 512 + oUserData);

    
    
    uint8_t tmp[oUserData];
    IOBuffer header(tmp, oUserData);
    
    // name -- 64byte pstring.
    
    if (vname == NULL) vname = "Untitled"; 
    unsigned l = std::strlen(vname);
    header.write8(std::min(l, 63u));
    header.writeBytes(vname, std::min(l, 63u));

    //header.resize(64);
    header.setOffset(oDataSize, true);
    
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
    
    std::memcpy(file->address(), header.buffer(), oUserData);
    file->sync();
    
    return new DiskCopy42Image(file);
}

void DiskCopy42Image::Validate(MappedFile *file)
{
#undef __METHOD__
#define __METHOD__ "DiskCopy42Image::Validate"

    size_t bytes = 0;
    size_t size = file->length();
    const void *data = file->address();
    bool ok = false;
    uint32_t checksum = 0;
    
    do {
        if (size < oUserData) break;
        
        // name must be < 64
        if (Read8(data, 0) > 63) break;
        
        if (Read32(data, oPrivate) != 0x100)
            break;
        
        // bytes, not blocks.
        bytes = Read32(data, oDataSize);
        
        if (bytes % 512) break;
        
        if (size < oUserData + bytes) break;
        
        // todo -- checksum.
        checksum = Read32(data, oDataChecksum);
        
        ok = true;
    } while (false);
    
    if (!ok)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    uint32_t cs = Checksum(oUserData + (uint8_t *)data, bytes);
    
    if (cs != checksum)
    {
        fprintf(stderr, __METHOD__ ": Warning: checksum invalid.\n");
    }

}

void DiskCopy42Image::write(unsigned block, const void *bp)
{
    DiskImage::write(block, bp);
    _changed = true;
}


BlockCache *DiskCopy42Image::createBlockCache()
{
    // if not readonly, mark changed so crc will be updated at close.
    
    if (!readOnly()) _changed = true;
    
    return new MappedBlockCache(this, address());
}