#ifndef __DISKIMAGE_H__
#define __DISKIMAGE_H__

#include <stdint.h>
#include <sys/types.h>

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

    
    static BlockDevicePointer Create(const char *name, size_t blocks);
    static BlockDevicePointer Open(MappedFile *);

    
    virtual BlockCachePointer createBlockCache();

    static bool Validate(MappedFile *, const std::nothrow_t &);
    static bool Validate(MappedFile *);


    ProDOSOrderDiskImage(MappedFile *);
private:
    ProDOSOrderDiskImage();
    
};

class DOSOrderDiskImage : public DiskImage {
public:
    

    static BlockDevicePointer Create(const char *name, size_t blocks);
    static BlockDevicePointer Open(MappedFile *);

    static bool Validate(MappedFile *, const std::nothrow_t &);
    static bool Validate(MappedFile *);


    DOSOrderDiskImage(MappedFile *);    
private:
    DOSOrderDiskImage();

};





}

#endif