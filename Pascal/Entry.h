#ifndef __PASCAL_ENTRY_H__
#define __PASCAL_ENTRY_H__

#include <Pascal/Date.h>


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
        unsigned _address;

    };


}

#endif

