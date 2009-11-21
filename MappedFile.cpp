#include "MappedFile.h"
#include "Exception.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "auto.h"

using namespace ProFUSE;



MappedFile::MappedFile(const char *name, bool readOnly) :
    _fd(-1),
    _map(MAP_FAILED)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::MappedFile"
    
    auto_fd fd(::open(name, readOnly ? O_RDONLY : O_RDWR));
    
    if (fd < 0)
    {
        throw Exception(__METHOD__ ": Unable to open file.", errno);
    }
    //
    init(fd.release(), readOnly);
}

MappedFile::MappedFile(int fd, bool readOnly) :
    _fd(-1),
    _map(MAP_FAILED)
{
    init(fd, readOnly);
}

// todo -- verify throw calls destructor.
MappedFile::MappedFile(const char *name, size_t size) :
    _fd(-1),
    _map(MAP_FAILED)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::MappedFile"
    
    _size = size;
    _readOnly = false;
    auto_fd fd(::open(name, O_CREAT | O_TRUNC | O_RDWR, 0644));
    
    if (fd < 0)
        throw Exception(__METHOD__ ": Unable to create file.", errno);
    
    // TODO -- is ftruncate portable?
    if (::ftruncate(fd, _size) < 0)
    {
        throw Exception(__METHOD__ ": Unable to truncate file.", errno);    
    }
    
    //_map = ::mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, _fd, 0);
    
    auto_map map(
        fd, 
        _size, 
        PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED
    );
    
    if (map == MAP_FAILED) throw Exception(__METHOD__ ": Unable to map file.", errno);

    _fd = fd.release();
    _map = map.release();
}

MappedFile::~MappedFile()
{
    if (_map != MAP_FAILED) ::munmap(_map, _size);
    if (_fd >= 0) ::close(_fd);
}

void MappedFile::init(int f, bool readOnly)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::init"
    
    auto_fd fd(f);
    
    _offset = 0;
    _blocks = 0;
    _size = 0;
    _readOnly = readOnly;
    _dosOrder = false;
    
    _size = ::lseek(_fd, 0, SEEK_END);
    
    if (_size < 0)
        throw Exception(__METHOD__ ": Unable to determine file size.", errno);
        
/*
    _map = ::mmap(NULL, _size, readOnly ? PROT_READ : PROT_READ | PROT_WRITE, 
        MAP_FILE | MAP_SHARED, fd, 0);
*/
    auto_map map(
        fd, 
        _size,
        readOnly ?  PROT_READ  : PROT_READ | PROT_WRITE, 
        readOnly ? MAP_FILE : MAP_FILE |  MAP_SHARED
    );
  
    if (map == MAP_FAILED) throw Exception(__METHOD__ ": Unable to map file.", errno);
    
    _fd = fd.release();
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
    
    
    if (_dosOrder)
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
    else
    {
        size_t address = block * 512;
        std::memcpy(bp, (uint8_t *)_map + _offset + address,  512);
    }
}

void MappedFile::writeBlock(unsigned block, const void *bp)
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::writeBlock"
        
    if (block > _blocks) throw Exception(__METHOD__ ": Invalid block number.");

    if ( (_readOnly) || (_map == MAP_FAILED))
        throw Exception(__METHOD__ ": File is readonly.");
    
    if (_dosOrder)
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
    else
    {
        size_t address = block * 512;
        std::memcpy((uint8_t *)_map + _offset +  address , bp,  512);
    }
}

void MappedFile::sync()
{
    #undef __METHOD__
    #define __METHOD__ "MappedFile::sync"
    
    if ( (_readOnly) || (_map == MAP_FAILED))
        return;
    
    if (::msync(_map, _size, MS_SYNC) < 0)
        throw Exception(__METHOD__ ": msync error.", errno);
}

void MappedFile::reset()
{
    _offset = 0;
    _blocks = 0;
    _dosOrder = false;
}

/*
uint8_t MappedFile::read8(size_t location) const
{
    // check for size?
    uint8_t *map = (uint8_t *)_map;
    
    return map[location];
}
*/

uint16_t MappedFile::read16(size_t location, int byteOrder) const
{
    // check for size?
    uint8_t *map = (uint8_t *)_map;
    
    switch(byteOrder)
    {
    case LittleEndian:
        return (map[location + 1] << 8) 
            | (map[location]);
    case BigEndian:
        return (map[location] << 8) 
            | (map[location+1]);
    default:
        return 0;    
    }    
}

uint32_t MappedFile::read32(size_t location, int byteOrder) const
{
    // check for size?
    uint8_t *map = (uint8_t *)_map;
    
    switch(byteOrder)
    {
    case LittleEndian:
        return (map[location+3] << 24) 
            | (map[location+2] << 16)
            | (map[location+1] << 8)
            | (map[location])
            ;
    case BigEndian:
        return (map[location] << 24) 
            | (map[location+1] << 16)
            | (map[location+2] << 8)
            | (map[location+3])
            ;
            
    default:
        return 0;    
    }    
}


/*
void MappedFile::write8(size_t location, uint8_t data)
{
    uint8_t *map = (uint8_t *)_map;
    
    map[location] = data;
}
*/

void MappedFile::write16(size_t location, uint16_t data, int byteOrder)
{
    uint8_t *map = (uint8_t *)_map;
    
    switch(byteOrder)
    {
    case LittleEndian:
        map[location] = data & 0xff;
        map[location+1] = (data >> 8) & 0xff;
        break;
    case BigEndian:
        map[location] = (data >> 8) & 0xff;
        map[location+1] = data & 0xff;
        break;
    }
}

void MappedFile::write32(size_t location, uint32_t data, int byteOrder)
{
    uint8_t *map = (uint8_t *)_map;
    
    switch(byteOrder)
    {
    case LittleEndian:
        map[location] = data & 0xff;
        map[location+1] = (data >> 8) & 0xff;
        map[location+2] = (data >> 16) & 0xff;
        map[location+3] = (data >> 24) & 0xff;        
        break;
    case BigEndian:
        map[location] = (data >> 24) & 0xff;
        map[location+1] = (data >> 16) & 0xff;
        map[location+2] = (data >> 8) & 0xff;
        map[location+3] = data & 0xff;
        break;
    }    
}
