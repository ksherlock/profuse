#ifndef __DISKIMAGE_H__
#define __DISKIMAGE_H__

#include <stdint.h>
#include <sys/types.h>

#include <ProFUSE/Exception.h>

#include <Device/BlockDevice.h>

#include <Device/Adaptor.h>

#include <File/MappedFile.h>

namespace Device {

    
class DiskImage : public BlockDevice {
public:


    virtual ~DiskImage();
        
    virtual void read(unsigned block, void *bp);
    virtual void write(unsigned block, const void *bp);
    virtual void sync();
    
    virtual bool readOnly();
    virtual unsigned blocks();
    
protected:

    DiskImage();
    
    DiskImage(MappedFile *file = 0);
    
    void setBlocks(unsigned blocks) { _blocks = blocks; }
    
    void setAdaptor(Adaptor *);
    
    void *address() const { return _file.address(); }
    size_t length() const { return _file.length(); }
    
    MappedFile *file() { return &_file; }

private:
    
    MappedFile _file;
    Adaptor *_adaptor;
    
    bool _readOnly;
    unsigned _blocks;
};


class ProDOSOrderDiskImage : public DiskImage {
public:

    
    static ProDOSOrderDiskImage *Create(const char *name, size_t blocks);
    static ProDOSOrderDiskImage *Open(MappedFile *);

    
    virtual BlockCachePointer createBlockCache();
    
private:
    ProDOSOrderDiskImage();

    
    ProDOSOrderDiskImage(MappedFile *);
    static void Validate(MappedFile *);
};

class DOSOrderDiskImage : public DiskImage {
public:
    

    static DOSOrderDiskImage *Create(const char *name, size_t blocks);
    static DOSOrderDiskImage *Open(MappedFile *);

private:
    DOSOrderDiskImage();

    DOSOrderDiskImage(MappedFile *);
    static void Validate(MappedFile *);
};





}

#endif