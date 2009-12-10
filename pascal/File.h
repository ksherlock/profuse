#ifndef __FILE_H__
#define __FILE_H__

#include "DateRec.h"

#include <vector>

namespace ProFUSE {
    class BlockDevice;
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

class Entry {
    
public:
    
    virtual ~Entry();

    unsigned blocks() const { return _firstBlock - _lastBlock; }

    unsigned firstBlock() const { return _firstBlock; }
    unsigned lastBlock() const { return _lastBlock; }
    
    unsigned fileKind() const { return _fileKind; }
    
    unsigned inode() const { return _inode; }
    void setInode(unsigned inode) { _inode = inode; }

protected:
  
    Entry();
    Entry(void *);
    void init(void *);
  
    unsigned _firstBlock;
    unsigned _lastBlock;
    unsigned _fileKind;
    
    unsigned _inode;
};


class VolumeEntry : public Entry {

public:

    VolumeEntry(ProFUSE::BlockDevice *);
    virtual ~VolumeEntry();

    const char *name() const { return _fileName; }
    unsigned fileCount() const { return _fileCount; }
    
    Pascal::DateRec lastBoot() const { return _lastBoot; }
    
    FileEntry *fileAtIndex(unsigned i) const;

private:
    VolumeEntry();
    
    void init(void *);
    
    unsigned _fileNameLength;
    char _fileName[8];
    unsigned _lastVolumeBlock;
    unsigned _fileCount;
    unsigned _accessTime;
    Pascal::DateRec _lastBoot;

    std::vector<FileEntry *> _files;
    unsigned _inodeGenerator;

};


class FileEntry : public Entry {
    public:
    
    FileEntry(void *vp);
    virtual ~FileEntry();

    unsigned fileSize();
    int  read(uint8_t *buffer, unsigned size, unsigned offset);
    
    const char *name() const { return _fileName; }
    DateRec modification() const { return _modification; }    
    
    protected:
    
    unsigned _status;
    
    unsigned _fileNameLength;
    char _fileName[16];
    
    unsigned _lastByte;
    DateRec _modification;
    
    // non-text files
    unsigned dataFileSize();
    int dataRead(uint8_t *buffer, unsigned size, unsigned offset); 
       
    // for text files.
    void textInit();
    unsigned textFileSize();
    int textRead(uint8_t *buffer, unsigned size, unsigned offset);
    
    unsigned textReadPage(unsigned block, uint8_t *in);
    unsigned textDecodePage(unsigned block, uint8_t *out);    
    
    std::vector<unsigned> *_pageLength;
    unsigned _fileSize;
    
};



}

#endif
