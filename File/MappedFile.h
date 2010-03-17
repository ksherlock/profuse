#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__

#include <sys/mman.h>

#include <File/File.h>


class MappedFile {
    public:
    
    MappedFile();
    MappedFile(File f, bool readOnly);
    MappedFile(MappedFile&);
    
    ~MappedFile();
    
    void sync();
    void close();
    
    void *address() const { return _address; }
    size_t length() const { return _length; }
    
    void swap(MappedFile &);
    void adopt(MappedFile &);
    
    private:
    
    MappedFile& operator=(MappedFile &);
    
    void *_address;
    size_t _length;
};


#endif