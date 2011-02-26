#ifndef __PASCAL_ENTRY_H__
#define __PASCAL_ENTRY_H__

#include <ProFUSE/smart_pointers.h>


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


    typedef SHARED_PTR(FileEntry) FileEntryPointer;
    typedef SHARED_PTR(VolumeEntry) VolumeEntryPointer;
    
    typedef WEAK_PTR(FileEntry) FileEntryWeakPointer;
    typedef WEAK_PTR(VolumeEntry) VolumeEntryWeakPointer;

    class Entry : public ENABLE_SHARED_FROM_THIS(Entry) {
        
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

