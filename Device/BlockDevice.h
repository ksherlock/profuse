#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__

#include <stdint.h>
#include <sys/types.h>

#include <ProFUSE/Exception.h>

#include <Device/TrackSector.h>

namespace Device {

class BlockDevice {
public:

    virtual ~BlockDevice();
    
    virtual void read(unsigned block, void *bp) = 0;
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp) = 0;
    virtual void write(TrackSector ts, const void *bp);


    virtual unsigned blocks() = 0;
    
    virtual bool mapped();
    
    virtual bool readOnly() = 0;
    
    virtual void sync() = 0;
    virtual void sync(unsigned block);
    virtual void sync(TrackSector ts);
    

    void zeroBlock(unsigned block);
    
protected:
    BlockDevice();
    
};



}

#endif
