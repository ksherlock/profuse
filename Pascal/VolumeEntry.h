#ifndef __PASCAL_VOLUMEENTRY_H__
#define __PASCAL_VOLUMEENTRY_H__

#include <Pascal/Entry.h>

#include <vector>

#include <Device/BlockDevice.h>

namespace Pascal {

    class FileEntry;
    
    class VolumeEntry : public Entry {

    public:

        static unsigned ValidName(const char *);

        
        // create new 
        VolumeEntry(const char *name, Device::BlockDevicePointer);
        
        // open existing
        VolumeEntry(Device::BlockDevicePointer);
        virtual ~VolumeEntry();

        const char *name() const { return _fileName; }
        unsigned fileCount() const { return _fileCount; }
        unsigned volumeBlocks() const { return _lastVolumeBlock; }
        
        Pascal::Date lastBoot() const { return _lastBoot; }
        
        unsigned freeBlocks(bool krunched = false) const;
        unsigned maxContiguousBlocks() const;
        bool canKrunch() const;
        
        
        FileEntry *fileAtIndex(unsigned i) const;
        FileEntry *fileByName(const char *name) const;
        

        void *loadBlock(unsigned block);
        void unloadBlock(unsigned block, bool dirty = false);

        void readBlock(unsigned block, void *);
        void writeBlock(unsigned block, void *);

        void sync();
        
        bool readOnly() { return _device->readOnly(); }

        int unlink(const char *name);
        int rename(const char *oldName, const char *newName);
        int copy(const char *oldName, const char *newName);
        FileEntry *create(const char *name, unsigned blocks);
        
        
        int krunch();

        
    protected:
        virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);

    private:
        
        friend class FileEntry;
        
        VolumeEntry();
        
        void init(void *);
        
        
        uint8_t *readDirectoryHeader();
        void writeDirectoryHeader(void *);
        
        uint8_t *readBlocks(unsigned startingBlock, unsigned count);
        void writeBlocks(void *buffer, unsigned startingBlock, unsigned count);
        
        void writeEntry(FileEntry *e);
        void writeEntry();
        
        void calcMaxFileSize();
        
        
        unsigned _fileNameLength;
        char _fileName[8];
        unsigned _lastVolumeBlock;
        unsigned _fileCount;
        unsigned _accessTime;
        Pascal::Date _lastBoot;

        std::vector<FileEntry *> _files;
        unsigned _inodeGenerator;
        
        Device::BlockDevicePointer _device;
        Device::BlockCachePointer _cache;
    };





}

#endif

