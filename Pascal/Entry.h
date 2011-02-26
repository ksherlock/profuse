#ifndef __PASCAL_ENTRY_H__
#define __PASCAL_ENTRY_H__

#include <tr1/memory>

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


    typedef std::tr1::shared_ptr<FileEntry> FileEntryPointer;
    typedef std::tr1::shared_ptr<VolumeEntry> VolumeEntryPointer;
    
    typedef std::tr1::weak_ptr<FileEntry> FileEntryWeakPointer;
    typedef std::tr1::weak_ptr<VolumeEntry> VolumeEntryWeakPointer;

    class Entry : public std::tr1::enable_shared_from_this<Entry> {
        
    public:
        
        virtual ~Entry();

        unsigned blocks() const { return _lastBlock - _firstBlock; }

        unsigned firstBlock() const { return _firstBlock; }
        unsigned lastBlock() const { return _lastBlock; }
        
        unsigned fileKind() const { return _fileKind; }
            
        unsigned inode() const { return _inode; }
        void setInode(unsigned inode) { _inode = inode; }

        VolumeEntryWeakPointer parent() { return _parent; }


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
        
        VolumeEntryWeakPointer _parent;
        unsigned _address;

    };


}

#endif

