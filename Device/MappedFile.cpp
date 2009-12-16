#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <Device/MappedFile.h>

#include <ProFUSE/Exception.h>
#include <ProFUSE/auto.h>

using namespace Device;
using ProFUSE::POSIXException;
using ProFUSE::Exception;


MappedFile::MappedFile(const char *name, bool readOnly)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::MappedFile"
    
    
    // if unable to open as read/write, open as read-only.
    
    ProFUSE::auto_fd fd;
    
    if (!readOnly)
    {
        fd.reset(::open(name, O_RDWR));
    }
    if (fd < 0)
    {
        fd.reset(::open(name, O_RDONLY));
        readOnly = true;
    }
     
    if (fd < 0)
    {
        throw POSIXException(__METHOD__ ": Unable to open file.", errno);
    }
    // init may throw; auto_fd guarantees the file will be closed if that happens.
    init(fd, readOnly);
    fd.release();
}

// does NOT close fd on failure.
MappedFile::MappedFile(int fd, bool readOnly)
{
    init(fd, readOnly);
}

MappedFile::MappedFile(const char *name, size_t size)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::MappedFile"
    
    _fd = -1;
    _map = MAP_FAILED;
    _size = size;
    _readOnly = false;
    _encoding = ProDOSOrder;
    
    ProFUSE::auto_fd fd(::open(name, O_CREAT | O_TRUNC | O_RDWR, 0644));
    
    if (fd < 0)
        throw POSIXException(__METHOD__ ": Unable to create file.", errno);
    
    // TODO -- is ftruncate portable?
    if (::ftruncate(fd, _size) < 0)
    {
        throw POSIXException(__METHOD__ ": Unable to truncate file.", errno);    
    }
    
    //_map = ::mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, _fd, 0);
    
    ProFUSE::auto_map map(
        NULL,
        _size, 
        PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED,
        fd,
        0
    );
    
    if (map == MAP_FAILED) throw POSIXException(__METHOD__ ": Unable to map file.", errno);

    _fd = fd.release();
    _map = map.release();
}

MappedFile::~MappedFile()
{
    if (_map != MAP_FAILED) ::munmap(_map, _size);
    if (_fd >= 0) ::close(_fd);
}

// does NOT close f on exception.
void MappedFile::init(int f, bool readOnly)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::init"
    
    
    _fd = -1;
    _map = MAP_FAILED;    
    _offset = 0;
    _blocks = 0;
    _size = 0;
    _readOnly = readOnly;
    _encoding = ProDOSOrder;
    _size = ::lseek(f, 0, SEEK_END);
    
    if (_size < 0)
        throw POSIXException(__METHOD__ ": Unable to determine file size.", errno);
        
/*
    _map = ::mmap(NULL, _size, readOnly ? PROT_READ : PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED, fd, 0);
*/
    ::lseek(f, 0, SEEK_SET);
    
    
    ProFUSE::auto_map map(
        NULL,
        _size,
        readOnly ?  PROT_READ  : PROT_READ | PROT_WRITE, 
        MAP_FILE |  MAP_SHARED, //readOnly ? MAP_FILE : MAP_FILE |  MAP_SHARED,
        f,
        0
    );
    
    /*
     _map = ::mmap(NULL, _size, readOnly ? PROT_READ : PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED, f, 0);
    */     
    if (map == MAP_FAILED) throw POSIXException(__METHOD__ ": Unable to map file.", errno);
    
    _fd = f;
    _map = map.release();
}


const unsigned MappedFile::DOSMap[] = {
    0x00, 0x0e, 0x0d, 0x0c, 
    0x0b, 0x0a, 0x09, 0x08, 
    0x07, 0x06, 0x05, 0x04, 
    0x03, 0x02, 0x01, 0x0f
};


void MappedFile::readBlock(unsigned block, void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::readBlock"
    
   
    if (block >= _blocks) throw Exception(__METHOD__ ": Invalid block number.");
    if (bp == 0) throw Exception(__METHOD__ ": Invalid address."); 
    
    
    switch(_encoding)
    {
    case ProDOSOrder:
        {
            size_t address = block * 512;
            std::memcpy(bp, (uint8_t *)_map + _offset + address,  512);
        }
        break;   
        
    case DOSOrder:
        {
            unsigned track = (block & ~0x07) << 9;
            unsigned sector = (block & 0x07) << 1;
            
            for (unsigned i = 0; i < 2; ++i)
            {
                size_t address =  track | (DOSMap[sector+i] << 8); 
                
                std::memcpy(bp, (uint8_t *)_map + _offset + address,  256);
                
                bp = (uint8_t *)bp + 256;
            }     
        }
        break;
    
    default:
        throw Exception(__METHOD__ ": Unsupported Encoding.");        
    }
    
}

void MappedFile::writeBlock(unsigned block, const void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::writeBlock"
        
    if (block > _blocks) throw Exception(__METHOD__ ": Invalid block number.");

    if (_readOnly) throw Exception(__METHOD__ ": File is readonly.");
    
    
    switch(_encoding)
    {
    case ProDOSOrder:
        {
            size_t address = block * 512;
            std::memcpy((uint8_t *)_map + _offset +  address , bp,  512);
        }
        break;
        
    case DOSOrder:
        {
            unsigned track = (block & ~0x07) << 9;
            unsigned sector = (block & 0x07) << 1;
            
            for (unsigned i = 0; i < 2; ++i)
            {
                size_t address =  track | (DOSMap[sector+i] << 8); 
                
                std::memcpy((uint8_t *)_map + _offset + address,  bp, 256);
                bp = (uint8_t *)bp + 256;
            }
        }
        break;

    default:
        throw Exception(__METHOD__ ": Unsupported Encoding.");            
    }
    
}

void MappedFile::sync()
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::sync"
    
    if (_readOnly) return;
    
    if (::msync(_map, _size, MS_SYNC) < 0)
        throw POSIXException(__METHOD__ ": msync error.", errno);
}

void MappedFile::reset()
{
    _offset = 0;
    _blocks = 0;
    _encoding = ProDOSOrder;
}



