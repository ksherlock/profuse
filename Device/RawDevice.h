#ifndef __RAWDEVICE_H__
#define __RAWDEVICE_H__

#include <stdint.h>

#include <Device/BlockDevice.h>

#include <File/File.h>

namespace Device {

// /dev/xxx


class RawDevice : public BlockDevice {
public:


    
    static RawDevice *Open(const char *name, File::FileFlags flags);
    
    
    virtual ~RawDevice();
    
    virtual void read(unsigned block, void *bp);
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp);
    virtual void write(TrackSector ts, const void *bp);
    
    virtual bool readOnly();
    virtual bool mapped();
    virtual void sync();
    
    virtual unsigned blocks();
    
private:

    
    RawDevice(const char *name, File::FileFlags flags);
    
    RawDevice(File& file, File::FileFlags flags);
    
    void devSize(int fd);

    File _file;
    bool _readOnly;
    
    uint64_t _size;
    unsigned _blocks;
    
    unsigned _blockSize;
};

}

#endif
