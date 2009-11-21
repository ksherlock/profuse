#ifndef __UNIVERSALDISKIMAGE_H__
#define __UNIVERSALDISKIMAGE_H__


#include "BlockDevice.h"

namespace ProFUSE {

class UniversalDiskImage : public DiskImage {
public:
    UniversalDiskImage(const char *name, bool readOnly);

    static UniversalDiskImage *Create(const char *name, size_t blocks);
    static UniversalDiskImage *Open(MappedFile *);

private:
    UniversalDiskImage(MappedFile *);
    static void Validate(MappedFile *);
};

}

#endif
