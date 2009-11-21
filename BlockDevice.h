#ifndef __BLOCKDEVICE_H__
#define __BLOCKDEVICE_H__

#include <stdint.h>
#include <sys/types.h>

#include "Exception.h"

namespace ProFUSE {

class MappedFile;

class BlockDevice {
public:
    virtual ~BlockDevice();
    
    virtual void read(unsigned block, void *bp) = 0;
    virtual void write(unsigned block, const void *bp) = 0;

    virtual bool readOnly() = 0;
    virtual void sync() = 0;
};



class DiskImage : public BlockDevice {
public:

    virtual ~DiskImage();
        
    virtual void read(unsigned block, void *bp);
    virtual void write(unsigned block, const void *bp);
    virtual void sync();
    
    virtual bool readOnly();
    
    
protected:

    DiskImage(MappedFile * = NULL);
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