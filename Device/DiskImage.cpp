
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>


#include <Device/DiskImage.h>
#include <MappedFile.h>

#include <ProFUSE/Exception.h>


using namespace Device;

using ProFUSE::Exception;
using ProFUSE::POSIXException;


class Reader {
    public:
    virtual ~Reader();
    virtual void readBlock(unsigned block, void *bp) = 0;
    virtual void writeBlock(unsigned block, const void *bp) = 0;
};

class POReader : public Reader {
    public:
    POReader(void *address);
    void readBlock(unsigned block, void *bp);
    void writeBlock(unsigned block, const void *bp);
    private:
    uint8_t *_address;
};

class DOReader : public Reader {
    public
    DOReader(void *address);
    void readBlock(unsigned block, void *bp);
    void writeBlock(unsigned block, const void *bp);
    private:
    uint8_t *_address;
    
    static unsigned Map[];
}

Reader::~Reader()
{
}



unsigned DOReader::Map[] = {
    0x00, 0x0e, 0x0d, 0x0c, 
    0x0b, 0x0a, 0x09, 0x08, 
    0x07, 0x06, 0x05, 0x04, 
    0x03, 0x02, 0x01, 0x0f
};

POReader::POReader(void *address)
{
    _address = (uint8_t *)address;
}

void POReader::readBlock(unsigned block, void *bp)
{
    std::memcpy(bp, _address + block * 512, 512);
}

void POReader::writeBlock(unsigned block, const void *bp)
{
    std::memcpy(_address + block * 512, bp, 512);
}


DOReader::DOReader(void *address)
{
    _address = (uint8_t *)address;
}

void DOReader::readBlock(unsigned block, void *bp)
{

    unsigned track = (block & ~0x07) << 9;
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        size_t offset =  track | (Map[sector+i] << 8); 
        
        std::memcpy(bp, _address + offset,  256);
        
        bp = (uint8_t *)bp + 256;
    }
}

void DOReader::writeBlock(unsigned block, const void *bp)
{
    unsigned track = (block & ~0x07) << 9;
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        size_t offset =  track | (DOSMap[sector+i] << 8); 
        
        std::memcpy(_address + offset,  bp, 256);
        bp = (uint8_t *)bp + 256;
    }
}






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

    if (::strcasecmp(type, "dvx") == 0)
        return 'DVX_';        
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
    _file(name, readOnly)
{
    _offset = 0;
    _blocks = 0;
    _readOnly = readOnly;
    _address = file.address();

}

DiskImage::DiskImage(MappedFile *file, bool readOnly)
{
    _file.adopt(file);

    _offset = 0;
    _blocks = 0;
    _readOnly = readOnly;
    _address = file.address();    
}

DiskImage::~DiskImage()
{ 
    delete _file;
}

bool DiskImage::readOnly()
{
    return _readOnly;
}

unsigned DiskImage::blocks()
{
    return _blocks;
}


void DiskImage::sync()
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::sync"
    
    if (_file) return _file.sync();
    
    throw Exception(__METHOD__ ": File not set."); 
}


void DiskImage::readPO(unsigned block, void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::readPO"
    
    if (block >= _blocks)
        throw Exception(__METHOD__ ": Invalid block.");
    
    std::memcpy(bp, (uint8_t *)_address + block * 512, 512);
}


void DiskImage::writePO(unsigned block, const void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "DiskImage::writePO"
    
    if (block >= _blocks)
        throw Exception(__METHOD__ ": Invalid block.");
    
    std::memcpy((uint8_t *)_address + block * 512, bp, 512);

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
