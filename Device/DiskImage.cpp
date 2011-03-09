
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>

#include <Device/DiskImage.h>



#include <File/MappedFile.h>

#include <Cache/MappedBlockCache.h>

#include <ProFUSE/Exception.h>


using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


/*
DiskImage::DiskImage(const char *name, bool readOnly)
{
    File fd(name, readOnly ? O_RDONLY : O_RDWR);
    MappedFile mf(fd, readOnly);
    _file.adopt(mf);
    
    _blocks = 0;
    _readOnly = readOnly;
    _adaptor = NULL;
}
*/

DiskImage::DiskImage(MappedFile *file)
{
    _file.adopt(*file);

    _blocks = 0;
    _readOnly = _file.readOnly();
    _adaptor = NULL;
}

DiskImage::~DiskImage()
{ 
    delete _adaptor;
}

bool DiskImage::readOnly()
{
    return _readOnly;
}

unsigned DiskImage::blocks()
{
    return _blocks;
}


void DiskImage::setAdaptor(Adaptor *adaptor)
{
    delete _adaptor;
    _adaptor = adaptor;
}


void DiskImage::read(unsigned block, void *bp)
{
#undef __METHOD__
#define __METHOD__ "DiskImage::read"

    if (block >= _blocks)
        throw Exception(__METHOD__ ": Invalid block.");
    
    _adaptor->readBlock(block, bp);    
}

void DiskImage::write(unsigned block, const void *bp)
{
#undef __METHOD__
#define __METHOD__ "DiskImage::write"
    
    if (block >= _blocks)
        throw Exception(__METHOD__ ": Invalid block.");
    
    _adaptor->writeBlock(block, bp);
}

void DiskImage::sync()
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::sync"
    
    if (_file.isValid()) return _file.sync();
    
    throw Exception(__METHOD__ ": File not set."); 
}



 
ProDOSOrderDiskImage::ProDOSOrderDiskImage(MappedFile *file) :
    DiskImage(file)
{
    // at this point, file is no longer valid.
    
    setBlocks(length() / 512);
    setAdaptor(new POAdaptor(address()));
}

BlockDevicePointer ProDOSOrderDiskImage::Create(const char *name, size_t blocks)
{
    MappedFile *file = MappedFile::Create(name, blocks * 512);
    //return BlockDevicePointer(new ProDOSOrderDiskImage(file));
    return MAKE_SHARED(ProDOSOrderDiskImage, file);
}

BlockDevicePointer ProDOSOrderDiskImage::Open(MappedFile *file)
{
    Validate(file);
    //return BlockDevicePointer(new ProDOSOrderDiskImage(file));
    return MAKE_SHARED(ProDOSOrderDiskImage, file);
}


bool ProDOSOrderDiskImage::Validate(MappedFile *f, const std::nothrow_t &)
{
#undef __METHOD__
#define __METHOD__ "ProDOSOrderDiskImage::Validate"
    
    
    size_t size = f->length();
    
    if (size % 512)
        return false;
    
    return true;
    
}

bool ProDOSOrderDiskImage::Validate(MappedFile *f)
{
    #undef __METHOD__
    #define __METHOD__ "ProDOSOrderDiskImage::Validate"
    
    if (!f || !f->isValid()) throw Exception(__METHOD__ ": File not set.");

    if (!Validate(f, std::nothrow))
        throw Exception(__METHOD__ ": Invalid file format.");
    
    return true;
}

BlockCachePointer ProDOSOrderDiskImage::createBlockCache()
{
    return MappedBlockCache::Create(shared_from_this(), address());
}

#pragma mark -
#pragma mark DOS Order Disk Image


/*
DOSOrderDiskImage::DOSOrderDiskImage(const char *name, bool readOnly) :
    DiskImage(name, readOnly)
{    
    Validate(file());
}
*/



DOSOrderDiskImage::DOSOrderDiskImage(MappedFile *file) :
    DiskImage(file)
{
    // at this point, file is no longer valid.
    
    setBlocks(length() / 512);
    setAdaptor(new DOAdaptor(address()));
}


BlockDevicePointer DOSOrderDiskImage::Create(const char *name, size_t blocks)
{
    MappedFile *file = MappedFile::Create(name, blocks * 512);
    //return BlockDevicePointer(new DOSOrderDiskImage(file));
    return MAKE_SHARED(DOSOrderDiskImage, file);
}

BlockDevicePointer DOSOrderDiskImage::Open(MappedFile *file)
{
    Validate(file);
    //return BlockDevicePointer(new DOSOrderDiskImage(file));
    return MAKE_SHARED(DOSOrderDiskImage, file);
    
}

bool DOSOrderDiskImage::Validate(MappedFile *f, const std::nothrow_t &)
{
#undef __METHOD__
#define __METHOD__ "DOSOrderDiskImage::Validate"
        
    size_t size = f->length();
    
    if (size % 512)
        return false;
    
    return true;
    
}

bool DOSOrderDiskImage::Validate(MappedFile *f)
{
    #undef __METHOD__
    #define __METHOD__ "DOSOrderDiskImage::Validate"

    if (!f || !f->isValid()) throw Exception(__METHOD__ ": File not set.");
    
    
    if (!Validate(f, std::nothrow))
        throw Exception(__METHOD__ ": Invalid file format.");
    
    return true;
}
