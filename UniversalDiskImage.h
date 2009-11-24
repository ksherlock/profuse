#ifndef __UNIVERSALDISKIMAGE_H__
#define __UNIVERSALDISKIMAGE_H__


#include "BlockDevice.h"
#include <stdint.h>

namespace ProFUSE {

class UniversalDiskImage : public DiskImage {
public:
    UniversalDiskImage(const char *name, bool readOnly);

    static UniversalDiskImage *Create(const char *name, size_t blocks);
    static UniversalDiskImage *Open(MappedFile *);

    virtual bool readOnly();

private:
    UniversalDiskImage(MappedFile *);
    static void Validate(MappedFile *);
    
    uint32_t _flags;
};

}

#endif
