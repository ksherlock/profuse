#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__

#include <stdint.h>
#include <sys/types.h>

#include <Device/Device.h>
#include <Device/TrackSector.h>

#include <ProFUSE/Exception.h>


#include <File/File.h>

namespace Device {

class BlockDevice {
public:


    // static methods.
    static unsigned ImageType(const char *type, unsigned defv = 0);
    
    static BlockDevice *Open(const char *name, File::FileFlags flags, unsigned imageType = 0);
    static BlockDevice *Create(const char *fname, const char *vname, unsigned blocks, unsigned imageType = 0);
    
    
    
    
    
    virtual ~BlockDevice();
    
    virtual BlockCachePointer createBlockCache(BlockDevicePointer device);
    
    
    virtual void read(unsigned block, void *bp) = 0;
    //virtual void read(TrackSector ts, void *bp) = 0
    
    virtual void write(unsigned block, const void *bp) = 0;
    //virtual void write(TrackSector ts, const void *bp) = 0;


    virtual unsigned blocks() = 0;
    
    virtual bool mapped();
    
    virtual bool readOnly() = 0;
    
    virtual void sync() = 0;
    virtual void sync(unsigned block);
    //virtual void sync(TrackSector ts);
    

    void zeroBlock(unsigned block);
    
protected:
    BlockDevice();
    
};



}

#endif
