#ifndef __DISKCOPY42IMAGE_H__
#define __DISKCOPY42IMAGE_H__

#include "BlockDevice.h"

#include <stdint.h>

namespace ProFUSE {

class DiskCopy42Image : public DiskImage {
public:
    DiskCopy42Image(const char *name, bool readOnly);
    virtual ~DiskCopy42Image();

    static DiskCopy42Image *Create(const char *name, size_t blocks);
    static DiskCopy42Image *Open(MappedFile *);

    static uint32_t Checksum(void *data, size_t size);

    virtual void write(unsigned block, const void *bp);
    
     
private:

    DiskCopy42Image(MappedFile *);
    static void Validate(MappedFile *);
    bool _changed;
};

}

#endif