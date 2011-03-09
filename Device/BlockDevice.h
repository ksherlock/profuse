#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__

#include <stdint.h>
#include <sys/types.h>

#include <Device/Device.h>
#include <Device/TrackSector.h>

#include <ProFUSE/Exception.h>
#include <ProFUSE/smart_pointers.h>

#include <File/File.h>

class MappedFile;

namespace Device {

    class BlockDevice : public ENABLE_SHARED_FROM_THIS(BlockDevice) {
public:


    // static methods.
    static unsigned ImageType(const char *type, unsigned defv = 0);
    static unsigned ImageType(MappedFile *, unsigned defv = 0);
        
        
    static BlockDevicePointer Open(const char *name, File::FileFlags flags, unsigned imageType = 0);
    static BlockDevicePointer Create(const char *fname, const char *vname, unsigned blocks, unsigned imageType = 0);
    
    
    
    
    
    virtual ~BlockDevice();
    
    virtual BlockCachePointer createBlockCache();
    
    
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
