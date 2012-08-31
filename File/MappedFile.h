#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__


#include <new>
#include <sys/mman.h>

#include <File/File.h>

class File;

class MappedFile {
    public:
    
    MappedFile();
    MappedFile(MappedFile&);
    MappedFile(const File &f, File::FileFlags flags, size_t size = -1);
    MappedFile(const char *name, File::FileFlags flags);
    MappedFile(const char *name, File::FileFlags flags, const std::nothrow_t &nothrow);
    
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