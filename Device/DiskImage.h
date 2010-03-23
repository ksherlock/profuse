#ifndef __DISKIMAGE_H__
#define __DISKIMAGE_H__

#include <stdint.h>
#include <sys/types.h>

#include <ProFUSE/Exception.h>

#include <Device/BlockDevice.h>

#include <MappedFile.h>

namespace Device {


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

    
    DiskImage(MappedFile * = 0);
    DiskImage(const char *name, bool readOnly);

    void setOffset(uint32_t offset);
    void *address() const { return _address; }
    void *base() const { return _file.address(); }

    void readPO(unsigned block, void *bp);
    void writePO(unsigned block, const void *bp);
    
    void readDO(unsigned block, void *bp);
    void writeDO(unsigned block, const void *bp);

    // readNib
    // writeNib

private:
    
    MappedFile _file;
    void *_address;
    uint32_t _offset;
    
    bool _readOnly;
    unsigned _blocks;
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