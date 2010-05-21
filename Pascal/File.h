#ifndef __PASCAL_FILE_H__
#define __PASCAL_FILE_H__

#include <Pascal/Date.h>

#include <vector>
#include <string>

namespace Device {
    class BlockDevice;
    class BlockCache;
}

namespace LittleEndian {
    class IOBuffer;
}

namespace Pascal {


enum {
    kUntypedFile,
    kBadBlockFile,
    kCodeFile,
    kTextFile,
    kInfoFile,
    kDataFile,
    kGrafFile,
    kFotoFile,
    kSecureDir
};

class FileEntry;
class VolumeEntry;

class Entry {
    
public:
    
    virtual ~Entry();

    unsigned blocks() const { return _lastBlock - _firstBlock; }

    unsigned firstBlock() const { return _firstBlock; }
    unsigned lastBlock() const { return _lastBlock; }
    
    unsigned fileKind() const { return _fileKind; }
    
    unsigned inode() const { return _inode; }
    void setInode(unsigned inode) { _inode = inode; }

    VolumeEntry *parent() { return _parent; }


    static bool Compress(std::string& text);
    static bool Uncompress(std::string &text);

protected:
    
    static unsigned ValidName(const char *name, unsigned maxSize);
  
    virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);
  
    Entry();
    Entry(void *);
    void init(void *);
  
    unsigned _firstBlock;
    unsigned _lastBlock;
    unsigned _fileKind;
    
    unsigned _inode;

private:
    friend class VolumeEntry;
    VolumeEntry *_parent;

};


class VolumeEntry : public Entry {

public:

    static unsigned ValidName(const char *);

    
    // create new 
    VolumeEntry(const char *name, Device::BlockDevice *);
    
    // open existing
    VolumeEntry(Device::BlockDevice *);
    virtual ~VolumeEntry();

    const char *name() const { return _fileName; }
    unsigned fileCount() const { return _fileCount; }
    unsigned volumeBlocks() const { return _lastVolumeBlock; }
    
    Pascal::Date lastBoot() const { return _lastBoot; }
    
    FileEntry *fileAtIndex(unsigned i) const;
    FileEntry *fileByName(const char *name) const;
    
    void addChild(FileEntry *child, unsigned blocks);



    void *loadBlock(unsigned block);
    void unloadBlock(unsigned block, bool dirty = false);

    void readBlock(unsigned block, void *);
    void writeBlock(unsigned block, void *);

    void sync();

    unsigned unlink(const char *name);
    unsigned rename(const char *oldName, const char *newName);
    
    unsigned krunch();

    
protected:
    virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);

private:
    VolumeEntry();
    
    void init(void *);
    
    
    uint8_t *readDirectoryHeader();
    void writeDirectoryHeader(uint8_t *);
    
    
    unsigned _fileNameLength;
    char _fileName[8];
    unsigned _lastVolumeBlock;
    unsigned _fileCount;
    unsigned _accessTime;
    Pascal::Date _lastBoot;

    std::vector<FileEntry *> _files;
    unsigned _inodeGenerator;
    
    Device::BlockDevice *_device;
    Device::BlockCache *_cache;
};


class FileEntry : public Entry {
    public:

    
    static unsigned ValidName(const char *);
    
    static bool Compress(std::string& text);
    static bool Uncompress(std::string& text);
    
    
    FileEntry(const char *name, unsigned fileKind);
    FileEntry(void *vp);
    virtual ~FileEntry();

    unsigned fileSize();
    
    unsigned lastByte() const { return _lastByte; }
    
    int read(uint8_t *buffer, unsigned size, unsigned offset);
    int write(uint8_t *buffer, unsigned size, unsigned offset);
    
    const char *name() const { return _fileName; }
    Date modification() const { return _modification; }    
        
    
    protected:
    
    virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);
    
    private:    
    friend class VolumeEntry;
    
    unsigned _status;
    
    unsigned _fileNameLength;
    char _fileName[16];
    
    unsigned _lastByte;
    Date _modification;
    
    // non-text files
    unsigned dataFileSize();
    int dataRead(uint8_t *buffer, unsigned size, unsigned offset); 
       
    // for text files.
    void textInit();
    unsigned textFileSize();
    int textRead(uint8_t *buffer, unsigned size, unsigned offset);
    
    unsigned textReadPage(unsigned block, uint8_t *in);
    unsigned textDecodePage(unsigned block, uint8_t *out);    
    
    std::vector<unsigned> *_pageSize;
    unsigned _fileSize;
    
};



}

#endif

