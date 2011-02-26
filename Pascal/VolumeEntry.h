#ifndef __PASCAL_VOLUMEENTRY_H__
#define __PASCAL_VOLUMEENTRY_H__

#include <Pascal/Entry.h>

#include <vector>

#include <Device/BlockDevice.h>

namespace Pascal {

    
    class VolumeEntry : public Entry {

    public:
    
        static unsigned ValidName(const char *);
    
        static VolumeEntryPointer Open(Device::BlockDevicePointer);
        static VolumeEntryPointer Create(Device::BlockDevicePointer, const char *name);

        //
        
        virtual ~VolumeEntry();

        const char *name() const { return _fileName; }
        unsigned fileCount() const { return _fileCount; }
        unsigned volumeBlocks() const { return _lastVolumeBlock; }
        
        Pascal::Date lastBoot() const { return _lastBoot; }
        
        unsigned freeBlocks(bool krunched = false) const;
        unsigned maxContiguousBlocks() const;
        bool canKrunch() const;
        
        
        FileEntryPointer fileAtIndex(unsigned i) const;
        FileEntryPointer fileByName(const char *name) const;
        

        void *loadBlock(unsigned block);
        void unloadBlock(unsigned block, bool dirty = false);

        void readBlock(unsigned block, void *);
        void writeBlock(unsigned block, void *);

        void sync();
        
        bool readOnly() { return _device->readOnly(); }

        int unlink(const char *name);
        int rename(const char *oldName, const char *newName);
        int copy(const char *oldName, const char *newName);
        FileEntryPointer create(const char *name, unsigned blocks);
        
        
        int krunch();

        
    protected:
        virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);

    private:
        
        friend class FileEntry;

        
        VolumeEntry();
        VolumeEntry(Device::BlockDevicePointer, const char *name);
        VolumeEntry(Device::BlockDevicePointer);

        VolumeEntryPointer thisPointer() 
        {
            return std::tr1::static_pointer_cast<VolumeEntry>(shared_from_this()); 
        }
                
        void init(void *);
        void setParents();
        

        
        
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

        std::vector<FileEntryPointer> _files;
        unsigned _inodeGenerator;
        
        Device::BlockDevicePointer _device;
        Device::BlockCachePointer _cache;
    };





}

#endif

