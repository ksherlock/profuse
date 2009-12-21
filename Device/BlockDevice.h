#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__

#include <stdint.h>
#include <sys/types.h>

#include <ProFUSE/Exception.h>

#include <Device/TrackSector.h>

namespace Device {

class MappedFile;
class AbstractBlockCache;



class BlockDevice {
public:

    virtual ~BlockDevice();
    
    virtual void read(unsigned block, void *bp) = 0;
    virtual void read(TrackSector ts, void *bp);
    
    virtual void write(unsigned block, const void *bp) = 0;
    virtual void write(TrackSector ts, const void *bp);

    // direct access to mapped memory (not always possible).
    virtual void *read(unsigned block);
    virtual void *read(TrackSector ts);

    virtual unsigned blocks() = 0;
    
    virtual bool mapped();
    
    virtual bool readOnly() = 0;
    
    virtual void sync() = 0;
    virtual void sync(unsigned block);
    virtual void sync(TrackSector ts);
    

    void zeroBlock(unsigned block);
    
    AbstractBlockCache *blockCache();

protected:
    BlockDevice();
    virtual AbstractBlockCache *createBlockCache();

private:
    AbstractBlockCache *_cache;
};



class DiskImage : public BlockDevice {
public:

    static unsigned ImageType(const char *type, unsigned defv = 0);

    virtual ~DiskImage();
        
    virtual void read(unsigned block, void *bp);
    virtual void write(unsigned block, const void *bp);
    virtual void sync();
    
    virtual bool readOnly();
    virtual unsigned blocks();
    
protected:

    virtual AbstractBlockCache *createBlockCache();

    DiskImage(MappedFile * = 0);
    DiskImage(const char *name, bool readOnly);

    MappedFile *file() { return _file; }
    void setFile(MappedFile *);
    
private:
    MappedFile *_file;
};


class ProDOSOrderDiskImage : public DiskImage {
public:

    ProDOSOrderDiskImage(const char *name, bool readOnly);
    
    static ProDOSOrderDiskImage *Create(const char *name, size_t blocks);
    static ProDOSOrderDiskImage *Open(MappedFile *);
    
private:
    ProDOSOrderDiskImage(MappedFile *);
    static void Validate(MappedFile *);
};

class DOSOrderDiskImage : public DiskImage {
public:
    
    DOSOrderDiskImage(const char *name, bool readOnly);

    static DOSOrderDiskImage *Create(const char *name, size_t blocks);
    static DOSOrderDiskImage *Open(MappedFile *);

private:
    DOSOrderDiskImage(MappedFile *);
    static void Validate(MappedFile *);
};





}

#endif