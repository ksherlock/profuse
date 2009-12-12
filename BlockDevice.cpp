#include "BlockDevice.h"
#include "BlockCache.h"
#include "Exception.h"
#include "MappedFile.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace ProFUSE;

#pragma mark -
#pragma mark BlockDevice

BlockDevice::~BlockDevice()
{
    delete _cache;
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



#pragma mark -
#pragma mark DiskImage

unsigned DiskImage::ImageType(const char *type, unsigned defv)
{
    const char *tmp;
    
    if (type == 0 || *type == 0) return defv;
    
    // type could be a filename, in which case we check the extension.
    tmp = std::strrchr(type, '.');
    if (tmp) type = tmp + 1;
    if (*type == 0) return defv;
    
    
    if (::strcasecmp(type, "2mg") == 0)
        return '2IMG';
    if (::strcasecmp(type, "2img") == 0)
        return '2IMG';
        
    if (::strcasecmp(type, "dc42") == 0)
        return 'DC42';
        
    if (::strcasecmp(type, "po") == 0)
        return 'PO__';
    if (::strcasecmp(type, "dmg") == 0)
        return 'PO__';
        
    if (::strcasecmp(type, "dsk") == 0)
        return 'DO__';
    if (::strcasecmp(type, "do") == 0)
        return 'DO__';
        
    if (::strcasecmp(type, "davex") == 0)
        return 'DVX_';
        
    /*
    // not supported yet.        
    if (::strcasecmp(tmp, "sdk") == 0)
        return 'SDK_';
    */      
    return defv;
}



DiskImage::DiskImage(const char *name, bool readOnly) :
    _file(NULL)
{
    _file = new MappedFile(name, readOnly);
}

DiskImage::DiskImage(MappedFile *file) :
    _file(file)
{
}

DiskImage::~DiskImage()
{ 
    delete _file;
}

bool DiskImage::readOnly()
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::readOnly"
    
    if (_file) return _file->readOnly();
    
    throw Exception(__METHOD__ ": File not set."); 
}

unsigned DiskImage::blocks()
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::blocks"
    
    if (_file) return _file->blocks();
    
    throw Exception(__METHOD__ ": File not set."); 
}

void DiskImage::read(unsigned block, void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::read"
    
    
    if (_file) return _file->readBlock(block, bp);
    
    throw Exception(__METHOD__ ": File not set."); 
}

void DiskImage::write(unsigned block, const void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::write"
    
    if (_file) return _file->writeBlock(block, bp);
    
    throw Exception(__METHOD__ ": File not set."); 
}



void DiskImage::sync()
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::sync"
    
    if (_file) return _file->sync();
    
    throw Exception(__METHOD__ ": File not set."); 
}



AbstractBlockCache *DiskImage::createBlockCache()
{
    if (_file->encoding() == MappedFile::ProDOSOrder)
        return new MappedBlockCache(_file->imageData(), _file->blocks());
        
    return BlockDevice::createBlockCache();
}


ProDOSOrderDiskImage::ProDOSOrderDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{
    Validate(file());
}

ProDOSOrderDiskImage::ProDOSOrderDiskImage(MappedFile *file) :
    DiskImage(file)
{
}

ProDOSOrderDiskImage *ProDOSOrderDiskImage::Create(const char *name, size_t blocks)
{
    MappedFile *file = new MappedFile(name, blocks * 512);
    file->setBlocks(blocks);
    return new ProDOSOrderDiskImage(file);
}

ProDOSOrderDiskImage *ProDOSOrderDiskImage::Open(MappedFile *file)
{
    Validate(file);
    return new ProDOSOrderDiskImage(file);
}

void ProDOSOrderDiskImage::Validate(MappedFile *f)
{
    #undef __METHOD__
    #define __METHOD__ "ProDOSOrderDiskImage::Validate"
    
    if (!f) throw Exception(__METHOD__ ": File not set.");
    
    size_t size = f->fileSize();
    
    if (size % 512)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    f->reset();
    f->setBlocks(size / 512);
}

DOSOrderDiskImage::DOSOrderDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{    
    Validate(file());
}

DOSOrderDiskImage::DOSOrderDiskImage(MappedFile *file) :
    DiskImage(file)
{
}

DOSOrderDiskImage *DOSOrderDiskImage::Create(const char *name, size_t blocks)
{
    MappedFile *file = new MappedFile(name, blocks * 512);
    file->setEncoding(MappedFile::DOSOrder);
    file->setBlocks(blocks);
    return new DOSOrderDiskImage(file);
}

DOSOrderDiskImage *DOSOrderDiskImage::Open(MappedFile *file)
{
    Validate(file);
    return new DOSOrderDiskImage(file);
}

void DOSOrderDiskImage::Validate(MappedFile *f)
{
    #undef __METHOD__
    #define __METHOD__ "DOSOrderDiskImage::Validate"

    if (!f) throw Exception(__METHOD__ ": File not set.");
    
    size_t size = f->fileSize();
    
    if (size % 512)
        throw Exception(__METHOD__ ": Invalid file format.");
    
    f->reset();
    f->setEncoding(MappedFile::DOSOrder);
    f->setBlocks(size / 512);
}
