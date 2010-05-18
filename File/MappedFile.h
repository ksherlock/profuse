#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__

#include <sys/mman.h>

#include <File/File.h>


class MappedFile {
    public:
    
    MappedFile();
    MappedFile(MappedFile&);
    MappedFile(const File &f, bool readOnly, size_t size = -1);
    MappedFile(const char *name, bool readOnly);
    
    ~MappedFile();
    
    
    static MappedFile *Create(const char *name, size_t size);
    
    bool isValid() const
    {
        return _address != MAP_FAILED;
    }
    
    void sync();
    void close();
    
    void *address() const { return _address; }
    size_t length() const { return _length; }
    bool readOnly() const { return _readOnly; }
    
    void swap(MappedFile &);
    void adopt(MappedFile &);
    
    private:
    
    MappedFile& operator=(MappedFile &);
    
    void init(const File &f, bool readOnly, size_t size);
    
    void *_address;
    size_t _length;
    bool _readOnly;
};


#endif