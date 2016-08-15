#ifndef __UNIVERSALDISKIMAGE_H__
#define __UNIVERSALDISKIMAGE_H__


#include <Device/BlockDevice.h>
#include <Device/DiskImage.h>

#include <stdint.h>

namespace Device {

class UniversalDiskImage : public DiskImage {
public:

    
    
    static BlockDevicePointer Create(const char *name, size_t blocks);
    static BlockDevicePointer Open(MappedFile *);

    virtual bool readOnly();

    virtual BlockCachePointer createBlockCache();


    static bool Validate(MappedFile *, const std::nothrow_t &);
    static bool Validate(MappedFile *);

    
    UniversalDiskImage(MappedFile *);
private:

    UniversalDiskImage();
    
    uint32_t _format;
    uint32_t _flags;
    uint32_t _blocks;
    uint32_t _dataOffset;
    uint32_t _dataLength;
};

}

#endif
