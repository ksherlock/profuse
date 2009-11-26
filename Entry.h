#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include <vector>
#include <stdint.h>

#include "DateTime.h"


namespace ProFUSE {

class BlockDevice;
class Bitmap;
class FileEntry;
class Volume;
class Buffer;


enum Access {
    DestroyEnabled = 0x80,
    RenameEnabled = 0x40,
    BackupNeeded = 0x20,
    Invisible = 0x04,
    WriteEnabled = 0x02,
    ReadEnabled = 0x01
};

enum StorageType {
    DeletedFile = 0x00,
    SeedlingFile = 0x01,
    SaplingFile = 0x02,
    TreeFile = 0x03,
    PascalFile = 0x04,
    ExtendedFile = 0x05,

    DirectoryFile = 0x0d,
    DirectoryHeader = 0x0e,
    VolumeHeader = 0x0f
};


class Entry {
public:
    virtual ~Entry();

    virtual void write(Buffer *) = 0;


    unsigned storageType() const { return _storageType; }
    
    unsigned nameLength() const { return _nameLength; }
    const char *name() const { return _name; }
    const char *namei() const { return _namei; }
    
    unsigned caseFlag() const { return _caseFlag; }
    
    
    void setName(const char *name);
    
    
    
    // returns strlen() on success, 0 on failure.
    static unsigned ValidName(const char *);


    unsigned block() const { return _address / 512; }
    unsigned offset() const { return _address % 512; }

    unsigned address() const { return _address; }

    unsigned index() const { return _index; }
    
    Volume *volume() { return _volume; }

protected:
    Entry(unsigned storageType, const char *name);
    Entry(const void *bp);

    
    void setStorageType(unsigned type)
    { _storageType = type; }
    
    void setAddress(unsigned address)
    { _address = address; }

    void setIndex(unsigned index)
    { _index = index; }
    
    void setVolume(Volume *v)
    { _volume = v; }
        

    
private:

    unsigned _address;
    unsigned _index;
    
    Volume *_volume;
    
    unsigned _storageType;
    unsigned _nameLength;
    char _namei[15+1];  // insensitive, ie, uppercase.
    char _name[15+1];
    
    unsigned _caseFlag;
};

class Directory : public Entry {
public:
    virtual ~Directory();
    

    DateTime creation() const { return _creation; }
    
    unsigned access() const { return _access; }
    unsigned entryLength() const { return _entryLength; }
    unsigned entriesPerBlock() const { return _entriesPerBlock; }
    
    unsigned fileCount() const { return _fileCount; }
    
    unsigned version() const { return _version; }
    unsigned minVersion() const { return _minVersion; }
    
    void setAccess(unsigned access);
    
    
protected:
    Directory(unsigned type, const char *name);
    Directory(const void *bp);

    std::vector<FileEntry *> _children;
    std::vector<unsigned> _entryBlocks;
        
private:

    DateTime _creation;
    unsigned _version;
    unsigned _minVersion;
    unsigned _access;
    unsigned _entryLength;      // always 0x27
    unsigned _entriesPerBlock;  //always 0x0d

    unsigned _fileCount;
};



class VolumeDirectory: public Directory {
public:

    VolumeDirectory(const char *name, BlockDevice *device);
    virtual ~VolumeDirectory();

    unsigned bitmapPointer() const { return _bitmapPointer; }
    unsigned totalBlocks() const { return _totalBlocks; }
    
    // bitmap stuff...
    int allocBlock();
    void freeBlock(unsigned block);

    virtual void write(Buffer *);

    BlockDevice *device() const { return _device; }
private:
    Bitmap *_bitmap;
    BlockDevice *_device;
    
    DateTime _modification;
    unsigned _totalBlocks;
    unsigned _bitmapPointer;
    
    // inode / free inode list?

};


class SubDirectory : public Directory {
public:
    SubDirectory(FileEntry *);
private:
    unsigned _parentPointer;
    unsigned _parentEntryNumber;
    unsigned _parentEntryLength;
};


class FileEntry : public Entry {
public:

    unsigned fileType() const { return _fileType; }
    unsigned auxType() const { return _auxType; }
    unsigned blocksUsed() const { return _blocksUsed; }
    unsigned eof() const { return _eof; }
    
    unsigned access() const { return _access; }
    
    
    DateTime creation() const { return _creation; }
    DateTime modification() const { return _modification; }

private:
    unsigned _fileType;
    unsigned _keyPointer;
    unsigned _blocksUsed;
    unsigned _eof;
    DateTime _creation;
    //version
    //min version
    unsigned _access;
    unsigned _auxType;
    DateTime _modification;
    unsigned _headerPointer;
    
};

}
#endif
