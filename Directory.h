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


enum {
    DestroyEnabled = 0x80,
    RenameEnabled = 0x40,
    BackupNeeded = 0x20,
    WriteEnabled = 0x02,
    ReadEnabled = 0x01
};


class Entry {
public:
    virtual ~Entry();



    unsigned storageType() const { return _storageType; }
    
    unsigned nameLength() const { return _nameLength; }
    const char *name() const { return _name; }
    void setName(const char *name);
    
    
    
    // returns strlen() on success, 0 on failure.
    static unsigned ValidName(const char *);

protected:
    Entry(int storageType, const char *name);
    
private:

    unsigned _address; // absolute address on disk.
    Volume *_volume;

    unsigned _storageType;
    unsigned _nameLength;
    char _name[15+1];

};

class Directory public Entry {
public:
    virtual ~Directory();
    

    DateTime creation() const { return _creation; }
    
    unsigned access() const { return _access; }
    unsigned entryLength() const { return _entryLength; }
    unsigned entriesPerBlock() const { return _entriesPerBlock; }
    
    unsigned fileCount() const { return _fileCount; }
    
    void setAccess(unsigned access);
    
    
protected:
    Directory(unsigned type, const char *name);
    Directory(BlockDevice *, unsigned block);

    std::vector<FileEntry *> _children;
    std::vector<unsigned> _entryBlocks;
    
    BlockDevice *_device;
        
    
private:

    DateTime _creation;
    // version
    // min version
    unsigned _access;
    usnigned _entryLength;      // always 0x27
    unsigned _entriesPerBlock;  //always 0x0d

    unsigned _fileCount;
};



class VolumeDirectory: public Directory {
public:

    virtual ~VolumeDirectory();

    unsigned bitmapPointer() const { return _bitmapPointer; }
    unsigned totalBlocks() const { return _totalBlocks; }
    
    // bitmap stuff...
    int allocBlock();
    void freeBlock(unsigned block);

private:
    Bitmap *_bitmap;
    
    unsigned _totalBlocks;
    unsigned _bitmapPointer;

    // inode / free inode list?

};


class SubDirectory : public Directory {
public:

private:
    unsigned _parentPointer;
    unsigned _parentEntryNumber;
    unsigned _parentEntryLength;
};


class FileEntry : public Entry {
public:


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
