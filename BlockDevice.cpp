#include "BlockDevice.h"
#include "Exception.h"
#include "MappedFile.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace ProFUSE;



BlockDevice::~BlockDevice()
{
}

void BlockDevice::zeroBlock(unsigned block)
{
    uint8_t bp[512];
    std::memset(bp, 0, 512);
    
    write(block, bp);
}


#pragma mark DiskImage

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
