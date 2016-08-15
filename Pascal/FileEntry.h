#ifndef __PASCAL_FILEENTRY_H__
#define __PASCAL_FILEENTRY_H__

#include <Pascal/Entry.h>
#include <Pascal/Date.h>

#include <vector>
#include <string>

namespace Pascal {

    class TextWriter;
    
    class FileEntry : public Entry {

    public:
        

        
        static unsigned ValidName(const char *);
        
        static bool Compress(std::string& text);
        static bool Uncompress(std::string& text);
        
        
        static FileEntryPointer Create(const char *name, unsigned fileKind);
        static FileEntryPointer Open(void *vp);
        

        
        virtual ~FileEntry();

        unsigned fileSize();
        
        unsigned lastByte() const { return _lastByte; }
        
        const char *name() const { return _fileName; }
        Date modification() const { return _modification; }    
        
        
        void setFileKind(unsigned kind);
        
        int read(uint8_t *buffer, unsigned size, unsigned offset);
        int write(const uint8_t *buffer, unsigned size, unsigned offset);
        
        int write(TextWriter& text);
        
        int truncate(unsigned newSize);
        

        FileEntry(const char *name, unsigned fileKind);
        FileEntry(void *vp);
                
    protected:
        
        virtual void writeDirectoryEntry(LittleEndian::IOBuffer *);
        
    private:
        
        
        friend class VolumeEntry;
        

        FileEntryPointer thisPointer() 
        {
            return STATIC_POINTER_CAST(FileEntry, shared_from_this()); 
        }
        
        
        void setName(const char *name);
        
        int truncateCommon(unsigned newSize);
        
        void setFileSize(unsigned size);
        
        unsigned _status;
        
        unsigned _fileNameLength;
        char _fileName[16];
        
        unsigned _lastByte;
        Date _modification;
        
        
        unsigned _maxFileSize; // maximum file size.
        
        
        
        
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

