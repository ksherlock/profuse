#ifndef __RAWDEVICE_H__
#define __RAWDEVICE_H__

#include <stdint.h>

#include "BlockDevice.h"

namespace ProFUSE {

// /dev/xxx


class RawDevice : BlockDevice {
public:

    RawDevice(const char *name, bool readOnly);
    
    virtual ~RawDevice();
    
    virtual void read(unsigned block, void *bp);
    virtual void write(unsigned block, const void *bp);

    virtual bool readOnly();
    virtual void sync();
    
private:

    void devSize(int fd);

    int _fd;
    bool _readOnly;
    
    uint64_t _size;
    unsigned _blocks;
    
    unsigned _blockSize;
}


#endif
