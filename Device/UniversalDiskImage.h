#ifndef __UNIVERSALDISKIMAGE_H__
#define __UNIVERSALDISKIMAGE_H__


#include <Device/BlockDevice.h>
#include <Device/DiskImage.h>

#include <stdint.h>

namespace Device {

class UniversalDiskImage : public DiskImage {
public:

    
    
    static UniversalDiskImage *Create(const char *name, size_t blocks);
    static UniversalDiskImage *Open(MappedFile *);

    virtual bool readOnly();

    BlockCache *createBlockCache(unsigned size);

    
private:

    UniversalDiskImage(const char *name, bool readOnly);

    UniversalDiskImage(MappedFile *);
    static void Validate(MappedFile *);
    
    uint32_t _format;
    uint32_t _flags;
    uint32_t _blocks;
    uint32_t _dataOffset;
    uint32_t _dataLength;
};

}

#endif
