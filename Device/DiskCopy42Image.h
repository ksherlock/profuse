#ifndef __DISKCOPY42IMAGE_H__
#define __DISKCOPY42IMAGE_H__

#include <Device/BlockDevice.h>
#include <Device/DiskImage.h>

#include <stdint.h>

namespace Device {

class DiskCopy42Image : public DiskImage {
public:
    virtual ~DiskCopy42Image();

    static BlockDevicePointer Create(const char *name, size_t blocks);
    static BlockDevicePointer Create(const char *name, size_t blocks, const char *vname);
    
    static BlockDevicePointer Open(MappedFile *);

    static uint32_t Checksum(void *data, size_t size);

    static bool Validate(MappedFile *, const std::nothrow_t &);
    static bool Validate(MappedFile *);
    
    
    
    virtual void write(unsigned block, const void *bp);
    

    virtual BlockCachePointer createBlockCache();    


    DiskCopy42Image();
    DiskCopy42Image(MappedFile *);    

private:

    bool _changed;
};

}

#endif