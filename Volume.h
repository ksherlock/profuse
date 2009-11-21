#ifndef __VOLUME_H__
#define __VOLUME_H__

#include <vector>
#include <stdint.h>

namespace ProFUSE {

class Bitmap;
class BlockDevice;

class Volume;
class Directory;
class FileEntry;


class Entry {
public:
    Entry() : _address(0), _volume(NULL) { }
    virtual ~Entry();
    
    unsigned address() { return _address; }
    void setAddress(unsigned address) { _address = address; }
    
    Volume *volume() const { return _volume; }
    void setVolume(Volume *v) { _volume = v; }
    
private:
    // physical location on disk (block * 512 + offset)
    unsigned _address; 
    Volume *_volume;   
};

class Directory {

    FileEntry *childAtIndex(unsigned index);
    
private:
    unsigned _childCount;
    
    std::vector<FileEntry *>_children;
};

class FileEntry {
    virtual ~FileEntry();
    
    virtual unsigned inode();
    Directory *directory();
    Directory *parent();
    
private:
    Directory *_parent;    
    Volume *_volume;
}

class Volume {
public:

    ~Volume();
    
    Volume *Create(BlockDevice *);
    Volume *Open(BlockDevice *);


    int allocBlock();

private:

    Volume(BlockDevice *, int);
    
    
    Bitmap *_bitmap;
    BlockDevice *_device;
    
  
};


class FileEntry : public Entry {
public:
    virtual ~FileEntry();

    unsigned inode();
    
private:
    _unsigned _inode;
    _unsigned _lookupCount;
    _unsigned _openCount;
};
}

#endif