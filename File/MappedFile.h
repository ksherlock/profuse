#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__

#include <sys/mman.h>

#include "File.h"

namespace File {

class MappedFile {
    public:
    
    MappedFile();
    MappedFile(File f, int flags);
    MappedFile(MappedFile&);
    
    ~MappedFile();
    
    void sync();
    void close();
    
    void *address() const { return _address; }
    size_t length() const { return _length; }
    
    private:
    
    void *_address;
    size_t _length;
};

}

#endif