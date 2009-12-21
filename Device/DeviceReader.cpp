

#include <Device/DeviceReader.h>

using namespace Device;

DeviceReader::~DeviceReader()
{
}

void *DeviceReader::read(unsigned block)
{
    return NULL;
}

void DeviceReader::read(TrackSector ts)
{
    return NULL;
}

void DeviceReader::mapped()
{
    return false;
}

#pragma mark -
#pragma mark ProDOS Order

void ProDOSOrderDeviceReader::read(unsigned block, void *bp)
{    
    std::memcpy(bp, read(block), 512);
}

void ProDOSOrderDeviceReader::read(TrackSector ts, void *bp)
{    
    std::memcpy(bp, read(ts), 256);
}

void *ProDOSOrderDeviceReader::read(unsigned block)
{
    if (block > _blocks)
    {
        throw ProFUSE::Exception("Invalid block.");
    }
    return block * 512 + (uint8_t *)_data;
}


void *ProDOSOrderDeviceReader::read(TrackSector ts)
{
    unsigned block = (ts.track * 16 + ts.sector) / 2;
    
    if (block > _blocks)
    {
        throw ProFUSE::Exception("Invalid track/sector.");
    }
    
    return (ts.track * 16 + ts.sector) * 256 + (uint8_t *)_data;
}


void ProDOSOrderDeviceReader::write(unsigned block, const void *bp)
{
    std::memcpy(read(block), bp, 512);
}

void ProDOSOrderDeviceReader::write(TrackSector ts, const void *bp)
{
    std::memcpy(read(ts), bp, 256);
} 


#pragma mark -
#pragma mark DOS Order

const unsigned DOSMap[] = {
    0x00, 0x0e, 0x0d, 0x0c, 
    0x0b, 0x0a, 0x09, 0x08, 
    0x07, 0x06, 0x05, 0x04, 
    0x03, 0x02, 0x01, 0x0f
};

void *DOSOrderDeviceReader::read(TrackSector ts)
{    
    if (ts.track > _tracks || ts.sector > 16)
    {
        throw ProFUSE::Exception("Invalid track/sector.");
    }
    
    return (ts.track * 16 + ts.sector) * 256 + (uint8_t *)_data;
}


void DOSOrderDeviceReader::read(unsigned block, void *bp)
{
    TrackSector ts(block >> 3, 0);
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        ts.sector = DOSMap[sector];
        std::memcpy(bp, read(ts), 256);
        bp = 256 + (uint8_t *)bp;
        ++sector;
    }
}


void DOSOrderDeviceReader::read(TrackSector ts, void *bp)
{
    std::memcpy(bp, read(ts), 256);
}


void DOSOrderDeviceReader::write(unsigned block, const void *bp)
{
    TrackSector ts(block >> 3, 0);
    unsigned sector = (block & 0x07) << 1;
    
    for (unsigned i = 0; i < 2; ++i)
    {
        ts.sector = DOSMap[sector];
        std::memcpy(read(ts), bp, 256);
        bp = 256 + (const uint8_t *)bp;
        ++sector;
    }
}

void DOSOrderDeviceReader::write(TrackSector ts, const void *bp)
{
    std::memcpy(read(ts), bp, 256);
} 


#pragma mark -
#pragma mark FileDeviceReader

FileDeviceReader::FileDeviceReader(int fd, unsigned blocks, bool readOnly)
{
    _fd = fd;
    _readOnly = readOnly;
    _blocks = blocks;
}

bool FileDeviceReader::readOnly()
{
    return _readOnly;
}

void FileDeviceReader::write(unsigned block, const void *bp)
{

    off_t offset = block * 512;

    size_t ok = ::pwrite(_fd, bp, 512, offset);
    
    if (ok != 512)
        throw ok < 0 
            ? POSIXException(__METHOD__ ": Error writing block.", errno)
            : Exception(__METHOD__ ": Error writing block.");
}


void FileDeviceReader::write(TrackSector ts, const void *bp)
{
    off_t offset = (ts.track * 16 + ts.sector)  * 256;
    size_t ok = ::pwrite(_fd, bp, 256, offset);
    
    if (ok != 256)
        throw ok < 0 
            ? POSIXException(__METHOD__ ": Error writing block.", errno)
            : Exception(__METHOD__ ": Error writing block.");
}








void FileDeviceReader::read(unsigned block, void *bp)
{

    off_t offset = block * 512;

    size_t ok = ::pread(_fd, bp, 512, offset);
    
    if (ok != 512)
        throw ok < 0 
            ? POSIXException(__METHOD__ ": Error reading block.", errno)
            : Exception(__METHOD__ ": Error reading block.");
}

void FileDeviceReader::read(TrackSector ts, void *bp)
{
    off_t offset = (ts.track * 16 + ts.sector)  * 256;
    size_t ok = ::pread(_fd, bp, 256, offset);
    
    if (ok != 256)
        throw ok < 0 
            ? POSIXException(__METHOD__ ": Error reading block.", errno)
            : Exception(__METHOD__ ": Error reading block.");
}
